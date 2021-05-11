/* -*- c -*- *******************************************************/
/*
 * Copyright (C) 2021 Sandia Corporation
 * Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 * the U.S. Government retains certain rights in this software.
 *
 * This source code is released under the New BSD License.
 */

#include <glad/glad.h>

#include <IceTDevGL3Image.h>

#include <IceTDevDiagnostics.h>
#include <IceTDevTiming.h>

#include <stdio.h>

static void readPixels(IceTImage target_image,
                       IceTInt *rendered_viewport,
                       IceTInt *target_viewport)
{
    IceTEnum color_format = icetImageGetColorFormat(target_image);
    IceTEnum depth_format = icetImageGetDepthFormat(target_image);
    IceTSizeType x_offset = rendered_viewport[0];
    IceTSizeType y_offset = rendered_viewport[1];
    IceTSizeType width = icetImageGetWidth(target_image);
    /*IceTSizeType height = icetImageGetHeight(target_image);*/
    GLuint framebuffer_id = *icetUnsafeStateGetInteger(ICET_GL3_FRAMEBUFFER);
    GLuint depth_framebuffer_id = *icetUnsafeStateGetInteger(ICET_GL3_DEPTH_FRAMEBUFFER);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);

    /* Read the OpenGL buffers. */
    glPixelStorei(GL_PACK_ROW_LENGTH, (GLint)icetImageGetWidth(target_image));

    /* These pixel store parameters are not working on one of the platforms
     * I am testing on (thank you Mac).  Instead of using these, just offset
     * the buffers we read in from. */
    /* glPixelStorei(GL_PACK_SKIP_PIXELS, target_viewport[0]); */
    /* glPixelStorei(GL_PACK_SKIP_ROWS, target_viewport[1]); */

    if (color_format == ICET_IMAGE_COLOR_RGBA_UBYTE) {
        IceTUInt *colorBuffer = icetImageGetColorui(target_image);
        glReadPixels((GLint)x_offset,
                     (GLint)y_offset,
                     (GLsizei)target_viewport[2],
                     (GLsizei)target_viewport[3],
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     colorBuffer + (  target_viewport[0]
                                    + width*target_viewport[1]));
    } else if (color_format == ICET_IMAGE_COLOR_RGBA_FLOAT) {
        IceTFloat *colorBuffer = icetImageGetColorf(target_image);
        glReadPixels((GLint)x_offset,
                     (GLint)y_offset,
                     (GLsizei)target_viewport[2],
                     (GLsizei)target_viewport[3],
                     GL_RGBA,
                     GL_FLOAT,
                     colorBuffer + 4*(  target_viewport[0]
                                      + width*target_viewport[1]));
    } else if (color_format == ICET_IMAGE_COLOR_RGB_FLOAT) {
        IceTFloat *colorBuffer = icetImageGetColorf(target_image);
        glReadPixels((GLint)x_offset,
                     (GLint)y_offset,
                     (GLsizei)target_viewport[2],
                     (GLsizei)target_viewport[3],
                     GL_RGB,
                     GL_FLOAT,
                     colorBuffer + 3*(  target_viewport[0]
                                      + width*target_viewport[1]));
    } else if (color_format != ICET_IMAGE_COLOR_NONE) {
        icetRaiseError(ICET_SANITY_CHECK_FAIL,
                       "Invalid color format 0x%X.", color_format);
    }

    if (depth_format == ICET_IMAGE_DEPTH_FLOAT) {
        IceTFloat *depthBuffer = icetImageGetDepthf(target_image);
        glBindFramebuffer(GL_FRAMEBUFFER, depth_framebuffer_id);
        glReadPixels((GLint)x_offset,
                     (GLint)y_offset,
                     (GLsizei)target_viewport[2],
                     (GLsizei)target_viewport[3],
                     GL_RED,
                     GL_FLOAT,
                     depthBuffer + (  target_viewport[0]
                                    + width*target_viewport[1]));
    } else if (depth_format != ICET_IMAGE_DEPTH_NONE) {
        icetRaiseError(ICET_SANITY_CHECK_FAIL,
                       "Invalid depth format 0x%X.", depth_format);
    }

    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    /* glPixelStorei(GL_PACK_SKIP_PIXELS, 0); */
    /* glPixelStorei(GL_PACK_SKIP_ROWS, 0); */

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void icetGL3GetRenderedBufferImage(IceTImage target_image,
                                   IceTInt *rendered_viewport,
                                   IceTInt *target_viewport)
{
    icetTimingBufferReadBegin();

    readPixels(target_image, rendered_viewport, target_viewport);

    icetImageClearAroundRegion(target_image, target_viewport);

    icetTimingBufferReadEnd();
}

void icetGL3GetCompressedRenderedBufferImage(IceTSparseImage target_image,
                                             IceTInt *rendered_viewport,
                                             IceTInt *target_viewport)
{
    /* TODO: This is inefficient. It would be better to compress the image
     * on the graphics card and transfer the sparse pixels back. */

    IceTSizeType width, height;
    IceTImage image_buffer;

    width = icetSparseImageGetWidth(target_image);
    height = icetSparseImageGetHeight(target_image);

    image_buffer =  icetGetStateBufferImage(ICET_RENDER_BUFFER, width, height);
    readPixels(image_buffer, rendered_viewport, target_viewport);

    icetCompressImageRegion(image_buffer,
                            target_viewport,
                            target_viewport,
                            width,
                            height,
                            target_image);
}
