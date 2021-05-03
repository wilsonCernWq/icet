/* -*- c -*- *******************************************************/
/*
 * Copyright (C) 2021 Sandia Corporation
 * Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 * the U.S. Government retains certain rights in this software.
 *
 * This source code is released under the New BSD License.
 */

#include <glad/glad.h>

#include <IceTGL3.h>

#include <IceTDevDiagnostics.h>
#include <IceTDevState.h>
#include <IceTDevTiming.h>

#ifndef MIN
#define MIN(x, y)       ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x, y)       ((x) < (y) ? (y) : (x))
#endif

static void setupOpenGL3Render(GLfloat *background_color,
                               IceTDrawCallbackType *original_callback,
                               IceTBoolean *ok_to_proceed);
static GLuint setupColorTexture(IceTBoolean *dirty);
static GLuint setupDepthTexture(IceTBoolean *dirty);
static void setupFramebuffer();
static void finalizeOpenGL3Render(const GLfloat *background_color,
                                  IceTDrawCallbackType original_callback);
static void openGL3DrawCallbackFunction(const IceTDouble *projection_matrix,
                                        const IceTDouble *modelview_matrix,
                                        const IceTFloat *background_color,
                                        const IceTInt *readback_viewport,
                                        IceTImage result);
static void correctOpenGL3RenderTimes(IceTDouble total_time);

void icetGL3DrawCallbackTexture(IceTGL3DrawCallbackTextureType func)
{
    if (!icetGL3IsInitialized()) {
        icetRaiseError(ICET_INVALID_OPERATION,
                       "IceT OpenGL 3 layer not initialized."
                       " Call icetGL3Initialize.");
        return;
    }

    icetStateSetPointer(ICET_GL3_DRAW_FUNCTION, (IceTVoid *)func);
}

IceTImage icetGL3DrawFrame(const IceTDouble *projection_matrix,
                           const IceTDouble *modelview_matrix)
{
    IceTImage image;
    GLfloat background_color[4];
    IceTDrawCallbackType original_callback;
    IceTDouble start_time;
    IceTBoolean ok_to_proceed;

    start_time = icetWallTime();

    setupOpenGL3Render(background_color,
                       &original_callback,
                       &ok_to_proceed);
    if (!ok_to_proceed) {
        return icetImageNull();
    }

    /* Hand control to the core layer to render and composite. */
    image = icetDrawFrame(projection_matrix,
                          modelview_matrix,
                          background_color);

    finalizeOpenGL3Render(background_color,
                          original_callback);

    correctOpenGL3RenderTimes(icetWallTime() - start_time);

    return image;
}

static GLuint setupColorTexture(IceTBoolean *dirty)
{
    IceTInt width;
    IceTInt height;
    GLuint color_texture_id =
        *icetUnsafeStateGetInteger(ICET_GL3_COLOR_TEXTURE);

    icetGetIntegerv(ICET_PHYSICAL_RENDER_WIDTH, &width);
    icetGetIntegerv(ICET_PHYSICAL_RENDER_HEIGHT, &height);

    if (color_texture_id != 0)
    {
        GLint actual_width;
        GLint actual_height;

        glBindTexture(GL_TEXTURE_2D, color_texture_id);
        glGetTexLevelParameteriv(
                    GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &actual_width);
        glGetTexLevelParameteriv(
                    GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &actual_height);
        glBindTexture(GL_TEXTURE_2D, 0);

        if ((width == actual_width) && (height == actual_height)) {
            /* Texture should be fine. Just leave as is and return not dirty. */
            return color_texture_id;
        } else {
            /* Texture is wrong size. Delete it and create a new one. */
            glDeleteTextures(1, &color_texture_id);
            icetStateSetInteger(ICET_GL3_COLOR_TEXTURE, 0);
        }
    }

    /* Return texture is "dirty" and framebuffer needs to be rebuilt. */
    *dirty = ICET_TRUE;

    glGenTextures(1, &color_texture_id);
    glBindTexture(GL_TEXTURE_2D, color_texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    /* Do the format and type parameters matter if the data type is NULL? */
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 width,
                 height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    icetStateSetInteger(ICET_GL3_COLOR_TEXTURE, color_texture_id);
    return color_texture_id;
}

static GLuint setupDepthTexture(IceTBoolean *dirty)
{
    IceTInt width;
    IceTInt height;
    GLuint depth_texture_id =
        *icetUnsafeStateGetInteger(ICET_GL3_DEPTH_TEXTURE);

    icetGetIntegerv(ICET_PHYSICAL_RENDER_WIDTH, &width);
    icetGetIntegerv(ICET_PHYSICAL_RENDER_HEIGHT, &height);

    if (depth_texture_id != 0)
    {
        GLint actual_width;
        GLint actual_height;

        glBindTexture(GL_TEXTURE_2D, depth_texture_id);
        glGetTexLevelParameteriv(
                    GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &actual_width);
        glGetTexLevelParameteriv(
                    GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &actual_height);
        glBindTexture(GL_TEXTURE_2D, 0);

        if ((width == actual_width) && (height == actual_height)) {
            /* Texture should be fine. Just leave as is and return not dirty. */
            return depth_texture_id;
        } else {
            /* Texture is wrong size. Delete it and create a new one. */
            glDeleteTextures(1, &depth_texture_id);
            icetStateSetInteger(ICET_GL3_DEPTH_TEXTURE, 0);
        }
    }

    /* Return texture is "dirty" and framebuffer needs to be rebuilt. */
    *dirty = ICET_TRUE;

    glGenTextures(1, &depth_texture_id);
    glBindTexture(GL_TEXTURE_2D, depth_texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    /* Do the format and type parameters matter if the data type is NULL? */
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_DEPTH_COMPONENT,
                 width,
                 height,
                 0,
                 GL_DEPTH_COMPONENT,
                 GL_FLOAT,
                 NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    icetStateSetInteger(ICET_GL3_DEPTH_TEXTURE, depth_texture_id);
    return depth_texture_id;
}

static void setupFramebuffer()
{
    GLuint color_buffer_id;
    GLuint depth_buffer_id;
    GLuint framebuffer_id = *icetUnsafeStateGetInteger(ICET_GL3_FRAMEBUFFER);
    GLenum draw_buffer = GL_COLOR_ATTACHMENT0;
    IceTInt global_viewport[4];
    IceTSizeType physical_width;
    IceTSizeType physical_height;
    GLint max_size;
    IceTBoolean buffer_dirty = ICET_FALSE;

    /* Determine what size buffer to use. */
    icetGetIntegerv(ICET_GLOBAL_VIEWPORT, global_viewport);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
    physical_width = MIN(global_viewport[2], max_size);
    physical_height = MIN(global_viewport[3], max_size);
    icetPhysicalRenderSize(physical_width, physical_height);

    /* Create any necessary OpenGL objects for buffer. */
    color_buffer_id = setupColorTexture(&buffer_dirty);
    depth_buffer_id = setupDepthTexture(&buffer_dirty);

    /* Enable framebuffer and update textures if necessary. */
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);

    if (buffer_dirty) {
        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D,
                               color_buffer_id,
                               0);
        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D,
                               depth_buffer_id,
                               0);
    }

    glDrawBuffers(1, &draw_buffer);
    glViewport(0, 0, physical_width, physical_height);
}

static void setupOpenGL3Render(GLfloat *background_color,
                               IceTDrawCallbackType *original_callback,
                               IceTBoolean *ok_to_proceed)

{
    IceTVoid *value;

    *ok_to_proceed = ICET_FALSE;

    if (!icetGL3IsInitialized()) {
        icetRaiseError(ICET_INVALID_OPERATION,
                       "IceT OpenGL 3 layer not initialized."
                       " Call icetGLInitialize.");
        return;
    }

    /* Check the GL callback. */
    icetGetPointerv(ICET_GL3_DRAW_FUNCTION, &value);
    if (value == NULL) {
        icetRaiseError(
                    ICET_INVALID_OPERATION,
                    "GL3 Drawing function not set. Call icetGL3DrawCallback.");
        return;
    }

    /* Set the background color to OpenGL's background color. */
    glGetFloatv(GL_COLOR_CLEAR_VALUE, background_color);

    /* Set up the framebuffer textures. */
    setupFramebuffer();

    /* Set up core callback to call the GL layer. */
    icetGetPointerv(ICET_DRAW_FUNCTION, &value);
    *original_callback = (IceTDrawCallbackType)value;
    icetDrawCallback(openGL3DrawCallbackFunction);

    *ok_to_proceed = ICET_TRUE;
}

static void finalizeOpenGL3Render(const GLfloat *background_color,
                                  IceTDrawCallbackType original_callback)
{
    /* Restore core IceT callback. */
    icetDrawCallback(original_callback);

    /* Fix background color. */
    glClearColor(background_color[0], background_color[1],
                 background_color[2], background_color[3]);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void openGL3DrawCallbackFunction(const IceTDouble *projection_matrix,
                                        const IceTDouble *modelview_matrix,
                                        const IceTFloat *background_color,
                                        const IceTInt *readback_viewport,
                                        IceTImage result)

{
    GLuint framebuffer_id = *icetUnsafeStateGetInteger(ICET_GL3_FRAMEBUFFER);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);

    (void)result;

    /* Set up OpenGL. */
    {
        /* Set the clear color as the background IceT currently wants. */
        glClearColor(background_color[0], background_color[1],
                     background_color[2], background_color[3]);
    }

    /* Call the rendering callback. */
    {
        IceTVoid *value;
        IceTGL3DrawCallbackTextureType callback;
        icetRaiseDebug("Calling OpenGL draw function.");
        icetGetPointerv(ICET_GL3_DRAW_FUNCTION, &value);
        callback = (IceTGL3DrawCallbackTextureType)value;
        (*callback)(projection_matrix,
                    modelview_matrix,
                    readback_viewport,
                    (GLuint)*icetUnsafeStateGetInteger(ICET_GL3_FRAMEBUFFER));
    }
}

static void correctOpenGL3RenderTimes(IceTDouble total_time)
{
    IceTDouble render_time;
    IceTDouble buf_read_time;
    IceTDouble buf_write_time;
    IceTDouble composite_time;

    /* The OpenGL layer added some time to the rendering.  Update the total
       time and make sure the rest are consistent. */
    icetStateSetDouble(ICET_TOTAL_DRAW_TIME, total_time);

    icetGetDoublev(ICET_RENDER_TIME, &render_time);
    icetGetDoublev(ICET_BUFFER_READ_TIME, &buf_read_time);
    icetGetDoublev(ICET_BUFFER_WRITE_TIME, &buf_write_time);

    composite_time = total_time - render_time - buf_read_time - buf_write_time;
    icetStateSetDouble(ICET_COMPOSITE_TIME, composite_time);
}
