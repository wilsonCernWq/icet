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

#include <string.h>

static void gl3_destroy(void);

static const char gl_identifier[] = "OGL3";

void icetGL3Initialize(void)
{
    if (icetGL3IsInitialized()) {
        icetRaiseWarning(ICET_INVALID_OPERATION,
                         "icetGLInitialize called multiple times.");
    } else {
        if (icetStateGetType(ICET_RENDER_LAYER_ID) != ICET_NULL) {
            icetRaiseError(ICET_INVALID_OPERATION,
                           "Attempted to initialize two different rendering "
                           "layers.");
        }
    }

    icetStateSetPointer(ICET_RENDER_LAYER_ID, gl_identifier);
    icetStateSetBoolean(ICET_GL3_INITIALIZED, ICET_TRUE);

    icetStateSetPointer(ICET_GL3_DRAW_FUNCTION, NULL);
    icetStateSetInteger(ICET_GL3_COLOR_TEXTURE, 0);
    icetStateSetInteger(ICET_GL3_DEPTH_TEXTURE, 0);

    {
        GLuint framebuffer_id;
        glGenFramebuffers(1, &framebuffer_id);
        icetStateSetInteger(ICET_GL3_FRAMEBUFFER, framebuffer_id);
    }

    icetStateSetPointer(ICET_RENDER_LAYER_DESTRUCTOR, gl3_destroy);
}

IceTBoolean icetGL3IsInitialized(void)
{
    if (icetStateGetType(ICET_RENDER_LAYER_ID) != ICET_NULL) {
        IceTVoid* render_layer_id;
        icetGetPointerv(ICET_RENDER_LAYER_ID, &render_layer_id);
        if (strcmp(gl_identifier, (const char*)render_layer_id) != 0) {
            return ICET_FALSE;
        }
    } else {
        return ICET_FALSE;
    }

    if (icetStateGetType(ICET_GL3_INITIALIZED) != ICET_NULL) {
        IceTBoolean initialized;
        icetGetBooleanv(ICET_GL3_INITIALIZED, &initialized);
        if (initialized) {
            return ICET_TRUE;
        }
    }
    return ICET_FALSE;
}

void gl3_destroy(void)
{
    icetRaiseDebug("In OpenGL layer destructor.");

    {
        IceTInt icet_texture;
        GLuint gl_texture;
        icetGetIntegerv(ICET_GL3_COLOR_TEXTURE, &icet_texture);
        gl_texture = icet_texture;
        if (gl_texture != 0)
        {
            glDeleteTextures(1, &gl_texture);
        }
    }
    {
        IceTInt icet_texture;
        GLuint gl_texture;
        icetGetIntegerv(ICET_GL3_DEPTH_TEXTURE, &icet_texture);
        gl_texture = icet_texture;
        if (gl_texture != 0)
        {
            glDeleteTextures(1, &gl_texture);
        }
    }

    {
        IceTInt icet_framebuffer;
        GLuint gl_framebuffer;
        icetGetIntegerv(ICET_GL3_FRAMEBUFFER, &icet_framebuffer);
        gl_framebuffer = icet_framebuffer;
        if (gl_framebuffer != 0)
        {
            glDeleteTextures(1, &gl_framebuffer);
        }
    }
}
