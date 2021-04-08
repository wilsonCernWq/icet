/* -*- c -*- *******************************************************/
/*
 * Copyright (C) 2021 Sandia Corporation
 * Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 * the U.S. Government retains certain rights in this software.
 *
 * This source code is released under the New BSD License.
 */

#include <IceTGL3.h>

#include <IceTDevDiagnostics.h>
#include <IceTDevState.h>
#include <IceTDevTiming.h>

static void setupOpenGL3Render(GLfloat *background_color,
                               IceTDrawCallbackType *original_callback,
                               IceTBoolean *ok_to_proceed);
static GLuint setupColorTexture();
static GLuint setupDepthTexture();
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

static GLuint setupColorTexture()
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
            /* Texture should be fine. Just leave as is. */
            return color_texture_id;
        } else {
            /* Texture is wrong size. Delete it and create a new one. */
            glDeleteTextures(1, &color_texture_id);
            icetStateSetInteger(ICET_GL3_COLOR_TEXTURE, 0);
        }
    }

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

static GLuint setupDepthTexture()
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
            /* Texture should be fine. Just leave as is. */
            return depth_texture_id;
        } else {
            /* Texture is wrong size. Delete it and create a new one. */
            glDeleteTextures(1, &depth_texture_id);
            icetStateSetInteger(ICET_GL3_DEPTH_TEXTURE, 0);
        }
    }

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

static void setupOpenGL3Render(GLfloat *background_color,
                               IceTDrawCallbackType *original_callback,
                               IceTBoolean *ok_to_proceed)

{
    IceTVoid *value;
    GLuint color_buffer_id;
    GLuint depth_buffer_id;
    GLuint framebuffer_id;
    GLenum draw_buffer = GL_COLOR_ATTACHMENT0;

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
    color_buffer_id = setupColorTexture();
    depth_buffer_id = setupDepthTexture();
    framebuffer_id = *icetUnsafeStateGetInteger(ICET_GL3_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);
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
    glDrawBuffers(1, &draw_buffer);

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

    /* Temporarily stop render time while reading back buffer. */
    icetTimingRenderEnd();

    icetTimingBufferReadBegin();

    /* Read the OpenGL buffers. */
    /* TODO: This is an inefficient way to get data back from the */
    /* graphics card. What we really want to do is compress the data */
    /* on the card. */
    {
        IceTEnum color_format = icetImageGetColorFormat(result);
        IceTEnum depth_format = icetImageGetDepthFormat(result);
        IceTSizeType x_offset = readback_viewport[0];
        IceTSizeType y_offset = readback_viewport[1];
        IceTSizeType width = icetImageGetWidth(result);
        /* IceTSizeType height = icetImageGetHeight(result); */

        glPixelStorei(GL_PACK_ROW_LENGTH, (GLint)icetImageGetWidth(result));

      /* These pixel store parameters are not working on one of the platforms
       * I am testing on (thank you Mac).  Instead of using these, just offset
       * the buffers we read in from. */
        /* glPixelStorei(GL_PACK_SKIP_PIXELS, readback_viewport[0]); */
        /* glPixelStorei(GL_PACK_SKIP_ROWS, readback_viewport[1]); */

        if (color_format == ICET_IMAGE_COLOR_RGBA_UBYTE) {
            IceTUInt *colorBuffer = icetImageGetColorui(result);
            glReadPixels((GLint)x_offset,
                         (GLint)y_offset,
                         (GLsizei)readback_viewport[2],
                         (GLsizei)readback_viewport[3],
                         GL_RGBA,
                         GL_UNSIGNED_BYTE,
                         colorBuffer + (  readback_viewport[0]
                                        + width*readback_viewport[1]));
        } else if (color_format == ICET_IMAGE_COLOR_RGBA_FLOAT) {
            IceTFloat *colorBuffer = icetImageGetColorf(result);
            glReadPixels((GLint)x_offset,
                         (GLint)y_offset,
                         (GLsizei)readback_viewport[2],
                         (GLsizei)readback_viewport[3],
                         GL_RGBA,
                         GL_FLOAT,
                         colorBuffer + 4*(  readback_viewport[0]
                                          + width*readback_viewport[1]));
        } else if (color_format == ICET_IMAGE_COLOR_RGB_FLOAT) {
            IceTFloat *colorBuffer = icetImageGetColorf(result);
            glReadPixels((GLint)x_offset,
                         (GLint)y_offset,
                         (GLsizei)readback_viewport[2],
                         (GLsizei)readback_viewport[3],
                         GL_RGB,
                         GL_FLOAT,
                         colorBuffer + 3*(  readback_viewport[0]
                                          + width*readback_viewport[1]));
        } else if (color_format != ICET_IMAGE_COLOR_NONE) {
            icetRaiseError(ICET_SANITY_CHECK_FAIL,
                           "Invalid color format 0x%X.", color_format);
        }

        if (depth_format == ICET_IMAGE_DEPTH_FLOAT) {
            IceTFloat *depthBuffer = icetImageGetDepthf(result);;
            glReadPixels((GLint)x_offset,
                         (GLint)y_offset,
                         (GLsizei)readback_viewport[2],
                         (GLsizei)readback_viewport[3],
                         GL_DEPTH_COMPONENT,
                         GL_FLOAT,
                         depthBuffer + (  readback_viewport[0]
                                        + width*readback_viewport[1]));
        } else if (depth_format != ICET_IMAGE_DEPTH_NONE) {
            icetRaiseError(ICET_SANITY_CHECK_FAIL,
                           "Invalid depth format 0x%X.", depth_format);
        }

        glPixelStorei(GL_PACK_ROW_LENGTH, 0);
        /* glPixelStorei(GL_PACK_SKIP_PIXELS, 0); */
        /* glPixelStorei(GL_PACK_SKIP_ROWS, 0); */
    }

    icetTimingBufferReadEnd();

    /* Start render timer again.  It's going to be shut off immediately on
       return anyway, but the calling function expects it to be running. */
    icetTimingRenderBegin();
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
