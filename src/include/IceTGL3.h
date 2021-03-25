/* -*- c -*- *******************************************************/
/*
 * Copyright (C) 2021 Sandia Corporation
 * Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 * the U.S. Government retains certain rights in this software.
 *
 * This source code is released under the New BSD License.
 */

#ifndef __IceTGL3_h
#define __IceTGL3_h

#include <IceT.h>

#ifdef __APPLE__
#  define GL_SILENCE_DEPRECATION
#  include <OpenGL/gl.h>
#else
#  include <GL/gl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

ICET_GL3_EXPORT void icetGL3Initialize(void);

ICET_GL3_EXPORT IceTBoolean icetGL3IsInitialized(void);

ICET_GL3_EXPORT void icetGL3SetReadBuffer(GLenum mode);

ICET_GL3_EXPORT IceTImage icetGL3DrawFrame(const IceTDouble *projection_matrix,
                                           const IceTDouble *modelview_matrix);

typedef void (*IceTGL3DrawCallbackTextureType)(
        const GLdouble *projection_matrix,
        const GLdouble *modelview_matrix,
        const GLint *readback_viewport,
        GLuint color_texture,
        GLuint depth_texture);

ICET_GL3_EXPORT void icetGL3DrawCallbackTexture(
        IceTGL3DrawCallbackTextureType callback);

#define ICET_GL3_STATE_START ICET_RENDER_LAYER_STATE_START
#define ICET_GL3_STATE_END   ICET_RENDER_LAYER_STATE_END

#define ICET_GL3_INITIALIZED     (ICET_GL3_STATE_START | (IceTEnum)0x0001)

#define ICET_GL3_DRAW_FUNCTION   (ICET_GL3_STATE_START | (IceTEnum)0x0020)
#define ICET_GL3_COLOR_TEXTURE   (ICET_GL3_STATE_START | (IceTEnum)0x0021)
#define ICET_GL3_DEPTH_TEXTURE   (ICET_GL3_STATE_START | (IceTEnum)0x0022)

#define ICET_GL3_STATE_ENABLE_START ICET_RENDER_LAYER_ENABLE_START
#define ICET_GL3_STATE_ENABLE_END   ICET_RENDER_LAYER_ENABLE_END

#define ICET_GL3_BUFFER_START    ICET_RENDER_LAYER_BUFFER_START
#define ICET_GL3_BUFFER_END      ICET_RENDER_LAYER_BUFFER_END


#ifdef __cplusplus
}
#endif

#endif /* __IceTGL3_h */
