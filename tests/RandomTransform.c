/* -*- c -*- *****************************************************************
** Copyright (C) 2003 Sandia Corporation
** Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
** the U.S. Government retains certain rights in this software.
**
** This source code is released under the New BSD License.
**
** Test that has each processor draw a randomly placed quadrilateral.
** Makes sure that all compositions are equivalent.
*****************************************************************************/

#include <IceTGL.h>
#include <IceTDevCommunication.h>
#include <IceTDevState.h>
#include <IceTDevImage.h>
#include "test_codes.h"
#include "test-util.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>

static int g_tile_dim;
static float g_color[3];
static IceTFloat g_modelview[16];
static IceTImage g_refimage_opaque;
static IceTImage g_refimage_transparent;

static void draw(void)
{
    printf("In draw\n");
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBegin(GL_QUADS);
      glVertex3d(-1.0, -1.0, 0.0);
      glVertex3d(1.0, -1.0, 0.0);
      glVertex3d(1.0, 1.0, 0.0);
      glVertex3d(-1.0, 1.0, 0.0);
    glEnd();
    printf("Leaving draw\n");
}

#define DIFF(x, y)      ((x) < (y) ? (y) - (x) : (x) - (y))

static int compare_color_buffers(IceTSizeType local_width,
                                 IceTSizeType local_height,
                                 IceTImage refimage,
                                 IceTImage testimage)
{
    IceTSizeType ref_off_x, ref_off_y;
    IceTSizeType bad_pixel_count;
    IceTSizeType x, y;
    char filename[FILENAME_MAX];
    IceTUByte *refcbuf, *cb;
    IceTInt rank;

    printf("Checking returned image.\n");

    icetGetIntegerv(ICET_RANK, &rank);

    if (    (local_width != icetImageGetWidth(testimage))
         || (local_height != icetImageGetHeight(testimage)) ) {
        printf("Image dimensions not what is expected!!!!!\n");
        printf("Expected %dx%d, received %dx%d\n",
               (int)local_width, (int)local_height,
               (int)icetImageGetWidth(testimage),
               (int)icetImageGetHeight(testimage));
        return 0;
    }

    refcbuf = malloc(icetImageGetNumPixels(refimage)*4);
    icetImageCopyColorub(refimage, refcbuf, ICET_IMAGE_COLOR_RGBA_UBYTE);

    cb = malloc(icetImageGetNumPixels(testimage)*4);
    icetImageCopyColorub(testimage, cb, ICET_IMAGE_COLOR_RGBA_UBYTE);

    ref_off_x = (rank%g_tile_dim) * local_width;
    ref_off_y = (rank/g_tile_dim) * local_height;
    bad_pixel_count = 0;
#define CBR(x, y) (cb[(y)*local_width*4 + (x)*4 + 0])
#define CBG(x, y) (cb[(y)*local_width*4 + (x)*4 + 1])
#define CBB(x, y) (cb[(y)*local_width*4 + (x)*4 + 2])
#define CBA(x, y) (cb[(y)*local_width*4 + (x)*4 + 3])
#define REFCBUFR(x, y) (refcbuf[(y)*SCREEN_WIDTH*4 + (x)*4 + 0])
#define REFCBUFG(x, y) (refcbuf[(y)*SCREEN_WIDTH*4 + (x)*4 + 1])
#define REFCBUFB(x, y) (refcbuf[(y)*SCREEN_WIDTH*4 + (x)*4 + 2])
#define REFCBUFA(x, y) (refcbuf[(y)*SCREEN_WIDTH*4 + (x)*4 + 3])
/* #define CB_EQUALS_REF(x, y)                  \ */
/*     (   (CBR((x), (y)) == REFCBUFR((x) + ref_off_x, (y) + ref_off_y) )\ */
/*      && (CBG((x), (y)) == REFCBUFG((x) + ref_off_x, (y) + ref_off_y) )\ */
/*      && (CBB((x), (y)) == REFCBUFB((x) + ref_off_x, (y) + ref_off_y) )\ */
/*      && (   CBA((x), (y)) == REFCBUFA((x) + ref_off_x, (y) + ref_off_y)\ */
/*       || CBA((x), (y)) == 0 ) ) */
#define CB_EQUALS_REF(x, y)                     \
    (   (DIFF(CBR((x), (y)), REFCBUFR((x) + ref_off_x, (y) + ref_off_y)) < 5) \
     && (DIFF(CBG((x), (y)), REFCBUFG((x) + ref_off_x, (y) + ref_off_y)) < 5) \
     && (DIFF(CBB((x), (y)), REFCBUFB((x) + ref_off_x, (y) + ref_off_y)) < 5) \
     && (DIFF(CBA((x), (y)), REFCBUFA((x) + ref_off_x, (y) + ref_off_y)) < 5) )

    for (y = 0; y < local_height; y++) {
        for (x = 0; x < local_width; x++) {
            if (!CB_EQUALS_REF(x, y)) {
              /* Uh, oh.  Pixels don't match.  This could be a genuine
               * error or it could be a floating point offset when
               * projecting edge boundries to pixels.  If the latter is the
               * case, there will be very few errors.  Count the errors,
               * and make sure there are not too many.  */
                bad_pixel_count++;
            }
        }
    }

  /* Check to make sure there were not too many errors. */
    if (   (bad_pixel_count > 0.001*local_width*local_height)
        && (bad_pixel_count > local_width)
        && (bad_pixel_count > local_height) )
    {
      /* Too many errors.  Call it bad. */
        printf("Too many bad pixels!!!!!!\n");
      /* Write current images. */
        sprintf(filename, "ref%03d.ppm", rank);
        write_ppm(filename, refcbuf, (int)SCREEN_WIDTH, (int)SCREEN_HEIGHT);
        sprintf(filename, "bad%03d.ppm", rank);
        write_ppm(filename, cb, (int)local_width, (int)local_height);
      /* Write difference image. */
        for (y = 0; y < local_height; y++) {
            for (x = 0; x < local_width; x++) {
                IceTSizeType off_x = x + ref_off_x;
                IceTSizeType off_y = y + ref_off_y;
                if (CBR(x, y) < REFCBUFR(off_x, off_y)){
                    CBR(x,y) = REFCBUFR(off_x,off_y) - CBR(x,y);
                } else {
                    CBR(x,y) = CBR(x,y) - REFCBUFR(off_x,off_y);
                }
                if (CBG(x, y) < REFCBUFG(off_x, off_y)){
                    CBG(x,y) = REFCBUFG(off_x,off_y) - CBG(x,y);
                } else {
                    CBG(x,y) = CBG(x,y) - REFCBUFG(off_x,off_y);
                }
                if (CBB(x, y) < REFCBUFB(off_x, off_y)){
                    CBB(x,y) = REFCBUFB(off_x,off_y) - CBB(x,y);
                } else {
                    CBB(x,y) = CBB(x,y) - REFCBUFB(off_x,off_y);
                }
            }
        }
        sprintf(filename, "diff%03d.ppm", rank);
        write_ppm(filename, cb, (int)local_width, (int)local_height);
        return 0;
    }
#undef CBR
#undef CBG
#undef CBB
#undef CBA
#undef REFCBUFR
#undef REFCBUFG
#undef REFCBUFB
#undef REFCBUFA
#undef CB_EQUALS_REF

    free(refcbuf);
    free(cb);

    return 1;
        
}

static int compare_depth_buffers(IceTSizeType local_width,
                                 IceTSizeType local_height,
                                 IceTImage refimage,
                                 IceTImage testimage)
{
    IceTSizeType ref_off_x, ref_off_y;
    IceTSizeType bad_pixel_count;
    IceTSizeType x, y;
    char filename[FILENAME_MAX];
    IceTFloat *refdbuf;
    IceTFloat *db;
    IceTInt rank;

    printf("Checking returned image.\n");

    icetGetIntegerv(ICET_RANK, &rank);

    refdbuf = icetImageGetDepthf(refimage);
    db = icetImageGetDepthf(testimage);
    ref_off_x = (rank%g_tile_dim) * local_width;
    ref_off_y = (rank/g_tile_dim) * local_height;
    bad_pixel_count = 0;

    for (y = 0; y < local_height; y++) {
        for (x = 0; x < local_width; x++) {
            if (DIFF(db[y*local_width + x],
                     refdbuf[(y+ref_off_y)*SCREEN_WIDTH
                            +x + ref_off_x]) > 0.00001) {
              /* Uh, oh.  Pixels don't match.  This could be a genuine
               * error or it could be a floating point offset when
               * projecting edge boundries to pixels.  If the latter is the
               * case, there will be very few errors.  Count the errors,
               * and make sure there are not too many.  */
                bad_pixel_count++;
            }
        }
    }

  /* Check to make sure there were not too many errors. */
    if (   (bad_pixel_count > 0.001*local_width*local_height)
        && (bad_pixel_count > local_width)
        && (bad_pixel_count > local_height) )
    {
        IceTUByte *errbuf;

      /* Too many errors.  Call it bad. */
        printf("Too many bad pixels!!!!!!\n");

        errbuf = malloc(4*local_width*local_height);

      /* Write encoded image. */
        for (y = 0; y < local_height; y++) {
            for (x = 0; x < local_width; x++) {
                IceTFloat ref = refdbuf[(y+ref_off_y)*SCREEN_WIDTH
                                        +x + ref_off_x];
                IceTFloat rendered = db[y*local_width + x];
                IceTUByte *encoded = &errbuf[4*(y*local_width+x)];
                IceTFloat error = rendered - ref;
                if (error < 0) {
                    encoded[0] = 0;
                    encoded[1] = 0;
                    encoded[2] = (IceTUByte)(-error*255);
                } else {
                    encoded[0] = (IceTUByte)(error*255);
                    encoded[1] = 0;
                    encoded[2] = 0;
                }
                encoded[3] = 255;
            }
        }
        sprintf(filename, "depth_error%03d.ppm", rank);
        write_ppm(filename, (IceTUByte *)errbuf,
                  (int)local_width, (int)local_height);

        free(errbuf);

        return 0;
    }
    return 1;
}

static void check_results(int result)
{
    int rank, num_proc;
    int fail = (result != TEST_PASSED);

    icetGetIntegerv(ICET_RANK, &rank);
    icetGetIntegerv(ICET_NUM_PROCESSES, &num_proc);

    if (rank+1 < num_proc) {
        int in_fail;
        icetCommRecv(&in_fail, 1, ICET_INT, rank+1, 1111);
        fail |= in_fail;
    }
    if (rank-1 >= 0) {
        icetCommSend(&fail, 1, ICET_INT, rank-1, 1111);
        icetCommRecv(&fail, 1, ICET_INT, rank-1, 2222);
    }
    if (rank+1 < num_proc) {
        icetCommSend(&fail, 1, ICET_INT, rank+1, 2222);
    }

    if (fail) {
        finalize_test(TEST_FAILED);
        exit(TEST_FAILED);
    }
}

static void RandomTransformDoRender(IceTBoolean transparent,
                                    IceTSizeType local_width,
                                    IceTSizeType local_height)
{
    IceTImage image;
    IceTInt rank;
    int result = TEST_PASSED;

    icetGetIntegerv(ICET_RANK, &rank);

    printf("Rendering frame.\n");
    if (transparent) {
        glColor4f(0.5f*g_color[0], 0.5f*g_color[1], 0.5f*g_color[2], 0.5f);
    } else {
        glColor4f(g_color[0], g_color[1], g_color[2], 1.0f);
    }
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0f,
            (GLfloat)((2.0*local_width*g_tile_dim)/SCREEN_WIDTH-1.0),
            -1.0f,
            (GLfloat)((2.0*local_height*g_tile_dim)/SCREEN_HEIGHT-1.0),
            -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(g_modelview);
    image = icetGLDrawFrame();
    swap_buffers();

    if (rank < g_tile_dim*g_tile_dim) {
        if (transparent) {
            if (!compare_color_buffers(local_width, local_height,
                                       g_refimage_transparent, image)) {
                result = TEST_FAILED;
            }
        } else {
            if (icetImageGetColorFormat(image) != ICET_IMAGE_COLOR_NONE) {
                if (!compare_color_buffers(local_width, local_height,
                                           g_refimage_opaque, image)) {
                    result = TEST_FAILED;
                }
            }
            if (icetImageGetDepthFormat(image) != ICET_IMAGE_DEPTH_NONE) {
                if (!compare_depth_buffers(local_width, local_height,
                                           g_refimage_opaque, image)) {
                    result = TEST_FAILED;
                }
            }
        }
    } else {
        printf("Not a display node.  Not testing image.\n");
    }
    check_results(result);
}

static void RandomTransformTryInterlace(IceTBoolean transparent,
                                        IceTSizeType local_width,
                                        IceTSizeType local_height)
{
    printf("\nTurning image interlace on.\n");
    icetEnable(ICET_INTERLACE_IMAGES);
    RandomTransformDoRender(transparent, local_width, local_height);

    printf("\nTurning image interlace off.\n");
    icetDisable(ICET_INTERLACE_IMAGES);
    RandomTransformDoRender(transparent, local_width, local_height);
}

static void RandomTransformTryStrategy()
{
    IceTSizeType local_width = SCREEN_WIDTH/g_tile_dim;
    IceTSizeType local_height = SCREEN_HEIGHT/g_tile_dim;
    IceTSizeType viewport_width = SCREEN_WIDTH;
    IceTSizeType viewport_height = SCREEN_HEIGHT;
    IceTSizeType viewport_offset_x = 0;
    IceTSizeType viewport_offset_y = 0;
    IceTBoolean test_ordering;
    int x, y;

    icetGetBooleanv(ICET_STRATEGY_SUPPORTS_ORDERING, &test_ordering);

    printf("\nRunning on a %d x %d display.\n", g_tile_dim, g_tile_dim);
    icetResetTiles();
    for (y = 0; y < g_tile_dim; y++) {
        for (x = 0; x < g_tile_dim; x++) {
            icetAddTile((IceTInt)(x*local_width),
                        (IceTInt)(y*local_height),
                        local_width,
                        local_height,
                        y*g_tile_dim + x);
        }
    }

    if (g_tile_dim > 1) {
        viewport_width
            = rand()%(SCREEN_WIDTH-local_width) + local_width;
        viewport_height
            = rand()%(SCREEN_HEIGHT-local_height) + local_height;
    }
    if (viewport_width < SCREEN_WIDTH) {
        viewport_offset_x = rand()%(SCREEN_WIDTH-viewport_width);
    }
    if (viewport_width < SCREEN_HEIGHT) {
        viewport_offset_y = rand()%(SCREEN_HEIGHT-viewport_height);
    }
            
    glViewport((GLint)viewport_offset_x, (GLint)viewport_offset_y,
               (GLsizei)viewport_width, (GLsizei)viewport_height);
    /* glViewport(0, 0, local_width, local_height); */

    printf("\nDoing color buffer.\n");
    icetSetColorFormat(ICET_IMAGE_COLOR_RGBA_UBYTE);
    icetSetDepthFormat(ICET_IMAGE_DEPTH_FLOAT);
    icetCompositeMode(ICET_COMPOSITE_MODE_Z_BUFFER);
    icetDisable(ICET_COMPOSITE_ONE_BUFFER);
    icetDisable(ICET_ORDERED_COMPOSITE);

    RandomTransformTryInterlace(ICET_FALSE, local_width, local_height);

    printf("\nDoing float color buffer.\n");
    icetSetColorFormat(ICET_IMAGE_COLOR_RGBA_FLOAT);
    icetSetDepthFormat(ICET_IMAGE_DEPTH_FLOAT);
    icetCompositeMode(ICET_COMPOSITE_MODE_Z_BUFFER);
    icetEnable(ICET_COMPOSITE_ONE_BUFFER);
    icetDisable(ICET_ORDERED_COMPOSITE);

    RandomTransformTryInterlace(ICET_FALSE, local_width, local_height);

    printf("\nDoing depth buffer.\n");
    icetSetColorFormat(ICET_IMAGE_COLOR_NONE);
    icetSetDepthFormat(ICET_IMAGE_DEPTH_FLOAT);
    icetCompositeMode(ICET_COMPOSITE_MODE_Z_BUFFER);
    icetDisable(ICET_ORDERED_COMPOSITE);

    RandomTransformTryInterlace(ICET_FALSE, local_width, local_height);

    if (test_ordering) {
        printf("\nDoing blended color buffer.\n");
        icetSetColorFormat(ICET_IMAGE_COLOR_RGBA_UBYTE);
        icetSetDepthFormat(ICET_IMAGE_DEPTH_NONE);
        icetCompositeMode(ICET_COMPOSITE_MODE_BLEND);
        icetEnable(ICET_ORDERED_COMPOSITE);

        RandomTransformTryInterlace(ICET_TRUE, local_width, local_height);

        printf("\nDoing blended float color buffer.\n");
        icetSetColorFormat(ICET_IMAGE_COLOR_RGBA_FLOAT);
        icetSetDepthFormat(ICET_IMAGE_DEPTH_NONE);
        icetCompositeMode(ICET_COMPOSITE_MODE_BLEND);
        icetEnable(ICET_ORDERED_COMPOSITE);

        RandomTransformTryInterlace(ICET_TRUE, local_width, local_height);
    } else {
        printf("\nStrategy does not support ordering, skipping.\n");
    }
}

static int RandomTransformRun()
{
    int i;
    IceTImage image;
    IceTVoid *refbuf;
    IceTVoid *refbuf2;
    int result = TEST_PASSED;
    int rank, num_proc;
    IceTInt *image_order;
    IceTInt rep_group_size;
    const IceTInt *rep_group;
    IceTFloat background_color[3];
    unsigned int seed;
    int strategy_idx;

    icetGetIntegerv(ICET_RANK, &rank);
    icetGetIntegerv(ICET_NUM_PROCESSES, &num_proc);

  /* Decide on an image order and data replication group size. */
    image_order = malloc(num_proc * sizeof(IceTInt));
    if (rank == 0) {
        seed = (int)time(NULL);
        printf("Base seed = %u\n", seed);
        srand(seed);
        for (i = 0; i < num_proc; i++) image_order[i] = i;
        printf("Image order:\n");
        for (i = 0; i < num_proc; i++) {
            int swap_idx = rand()%(num_proc-i) + i;
            int swap = image_order[swap_idx];
            image_order[swap_idx] = image_order[i];
            image_order[i] = swap;
            printf("%4d", image_order[i]);
        }
        printf("\n");
        if (rand()%2) {
          /* No data replication. */
            rep_group_size = 1;
        } else {
            rep_group_size = rand()%num_proc + 1;
        }
        printf("Data replication group sizes: %d\n", rep_group_size);
        for (i = 1; i < num_proc; i++) {
            icetCommSend(&seed, 1, ICET_INT, i, 29);
            icetCommSend(image_order, num_proc, ICET_INT, i, 30);
            icetCommSend(&rep_group_size, 1, ICET_INT, i, 31);
        }
    } else {
        icetCommRecv(&seed, 1, ICET_INT, 0, 29);
        icetCommRecv(image_order, num_proc, ICET_INT, 0, 30);
        icetCommRecv(&rep_group_size, 1, ICET_INT, 0, 31);
        srand(seed + rank);
    }
    icetCompositeOrder(image_order);

  /* Agree on background color. */
    if (rank == 0) {
        background_color[0] = (float)rand()/(float)RAND_MAX;
        background_color[1] = (float)rand()/(float)RAND_MAX;
        background_color[2] = (float)rand()/(float)RAND_MAX;
        for (i = 1; i < num_proc; i++) {
            icetCommSend(background_color, 3, ICET_FLOAT, i, 32);
        }
    } else {
        icetCommRecv(background_color, 3, ICET_FLOAT, 0, 32);
    }

  /* Set up IceT. */
    icetGLDrawCallback(draw);
    icetBoundingBoxd(-1.0, 1.0, -1.0, 1.0, -0.125, 0.125);
    icetEnable(ICET_CORRECT_COLORED_BACKGROUND);

  /* Set up OpenGL. */
    glClearColor(background_color[0], background_color[1],
                 background_color[2], 1.0f);
    glDisable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);

  /* Get random transformation matrix. */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(2.0f*rand()/RAND_MAX - 1.0f,
                 2.0f*rand()/RAND_MAX - 1.0f,
                 2.0f*rand()/RAND_MAX - 1.0f);
    glRotatef(360.0f*rand()/RAND_MAX, 0.0f, 0.0f,
              (float)rand()/RAND_MAX);
    glScalef((float)(1.0/sqrt(num_proc) - 1.0)*(float)rand()/RAND_MAX + 1.0f,
             (float)(1.0/sqrt(num_proc) - 1.0)*(float)rand()/RAND_MAX + 1.0f,
             1.0f);
    glGetFloatv(GL_MODELVIEW_MATRIX, g_modelview);

  /* Set up data replication groups and ensure that they all share the same
   * transformation. */
  /* Pick a color based on my index into the object ordering. */
    for (i = 0; image_order[i] != rank; i++);
    i /= rep_group_size;
    printf("My data replication group: %d\n", i);
    icetDataReplicationGroupColor(i);
    if ((i&0x07) == 0) {
        g_color[0] = 0.5f;  g_color[1] = 0.5f;  g_color[2] = 0.5f;
    } else {
        g_color[0] = 1.0f*((i&0x01) == 0x01);
        g_color[1] = 1.0f*((i&0x02) == 0x02);
        g_color[2] = 1.0f*((i&0x04) == 0x04);
    }
  /* Get the true group size. */
    icetGetIntegerv(ICET_DATA_REPLICATION_GROUP_SIZE, &rep_group_size);
    rep_group = icetUnsafeStateGetInteger(ICET_DATA_REPLICATION_GROUP);
    if (rep_group[0] == rank) {
        for (i = 1; i < rep_group_size; i++) {
            icetCommSend(g_modelview, 16, ICET_FLOAT, rep_group[i], 40);
        }
    } else {
        icetCommRecv(g_modelview, 16, ICET_FLOAT, rep_group[0], 40);
    }

    printf("Transformation:\n");
    printf("    %f %f %f %f\n",
           g_modelview[0], g_modelview[4], g_modelview[8], g_modelview[12]);
    printf("    %f %f %f %f\n",
           g_modelview[1], g_modelview[5], g_modelview[9], g_modelview[13]);
    printf("    %f %f %f %f\n",
           g_modelview[2], g_modelview[6], g_modelview[10], g_modelview[14]);
    printf("    %f %f %f %f\n",
           g_modelview[3], g_modelview[7], g_modelview[11], g_modelview[15]);

  /* Let everyone get a base image for comparison. */
    glViewport(0, 0, (GLsizei)SCREEN_WIDTH, (GLsizei)SCREEN_HEIGHT);
    icetStrategy(ICET_STRATEGY_SEQUENTIAL);
    icetResetTiles();
    for (i = 0; i < num_proc; i++) {
        icetAddTile(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, i);
    }

    printf("\nGetting base images for z compare.\n");
    icetSetColorFormat(ICET_IMAGE_COLOR_RGBA_UBYTE);
    icetSetDepthFormat(ICET_IMAGE_DEPTH_FLOAT);
    icetCompositeMode(ICET_COMPOSITE_MODE_Z_BUFFER);
    icetDisable(ICET_COMPOSITE_ONE_BUFFER);
    glColor4f(g_color[0], g_color[1], g_color[2], 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(g_modelview);
    image = icetGLDrawFrame();
    swap_buffers();

    refbuf = malloc(icetImageBufferSize(icetImageGetWidth(image),
                                        icetImageGetHeight(image)));
    g_refimage_opaque = icetImageAssignBuffer(refbuf, icetImageGetWidth(image),
                                              icetImageGetHeight(image));
    icetImageCopyPixels(image,
                        0,
                        g_refimage_opaque,
                        0,
                        icetImageGetNumPixels(image));

    printf("Getting base image for color blend.\n");
    icetSetColorFormat(ICET_IMAGE_COLOR_RGBA_UBYTE);
    icetSetDepthFormat(ICET_IMAGE_DEPTH_NONE);
    icetCompositeMode(ICET_COMPOSITE_MODE_BLEND);
    icetEnable(ICET_ORDERED_COMPOSITE);
    glColor4f(0.5f*g_color[0], 0.5f*g_color[1], 0.5f*g_color[2], 0.5f);
    image = icetGLDrawFrame();
    swap_buffers();

    refbuf2 = malloc(icetImageBufferSize(icetImageGetWidth(image),
                                         icetImageGetHeight(image)));
    g_refimage_transparent = icetImageAssignBuffer(refbuf2,
                                                   icetImageGetWidth(image),
                                                   icetImageGetHeight(image));
    icetImageCopyPixels(image,
                        0,
                        g_refimage_transparent,
                        0,
                        icetImageGetNumPixels(image));

    glViewport(0, 0, (GLsizei)SCREEN_WIDTH, (GLsizei)SCREEN_HEIGHT);

    for (strategy_idx = 0; strategy_idx < STRATEGY_LIST_SIZE; strategy_idx++) {
        IceTEnum strategy = strategy_list[strategy_idx];
        int si_strategy_idx;
        int num_single_image_strategy;

        icetStrategy(strategy);
        printf("\n\nUsing %s strategy.\n", icetGetStrategyName());

        if (strategy_uses_single_image_strategy(strategy)) {
            num_single_image_strategy = SINGLE_IMAGE_STRATEGY_LIST_SIZE;
        } else {
          /* Set to one since single image strategy does not matter. */
            num_single_image_strategy = 1;
        }

        for (si_strategy_idx = 0;
             si_strategy_idx < num_single_image_strategy;
             si_strategy_idx++) {
            IceTEnum single_image_strategy
                = single_image_strategy_list[si_strategy_idx];

            icetSingleImageStrategy(single_image_strategy);
            printf("\nUsing %s single image sub-strategy.\n",
                   icetGetSingleImageStrategyName());

            for (g_tile_dim = 1;
                 g_tile_dim*g_tile_dim <= num_proc;
                 g_tile_dim++) {
                RandomTransformTryStrategy();
            }
        }
    }

    printf("Cleaning up.\n");
    free(image_order);
    free(refbuf);
    free(refbuf2);
    glViewport(0, 0, (GLsizei)SCREEN_WIDTH, (GLsizei)SCREEN_HEIGHT);

    return result;
}

int RandomTransform(int argc, char *argv[])
{
    /* To remove warning */
    (void)argc;
    (void)argv;

    return run_test(RandomTransformRun);
}
