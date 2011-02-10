/* -*- c -*- *******************************************************/
/*
 * Copyright (C) 2011 Sandia Corporation
 * Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 * the U.S. Government retains certain rights in this software.
 *
 * This source code is released under the New BSD License.
 */

#include <IceTDevTiming.h>

#include <IceTDevDiagnostics.h>
#include <IceTDevState.h>

#include <stdlib.h>
#include <stdio.h>

void icetStateResetTiming(void)
{
    icetStateSetDouble(ICET_RENDER_TIME, 0.0);
    icetStateSetDouble(ICET_BUFFER_READ_TIME, 0.0);
    icetStateSetDouble(ICET_BUFFER_WRITE_TIME, 0.0);
    icetStateSetDouble(ICET_COMPRESS_TIME, 0.0);
    icetStateSetDouble(ICET_BLEND_TIME, 0.0);
    icetStateSetDouble(ICET_COMPOSITE_TIME, 0.0);
    icetStateSetDouble(ICET_TOTAL_DRAW_TIME, 0.0);

    icetStateSetInteger(ICET_DRAW_TIME_ID, 0);
    icetStateSetInteger(ICET_SUBFUNC_TIME_ID, 0);

    icetStateSetInteger(ICET_BYTES_SENT, 0);
}

static void icetTimingBegin(IceTEnum start_pname,
                            IceTEnum id_pname,
                            IceTEnum result_pname,
                            const char *name)
{
    icetRaiseDebug1("Beginning %s", name);

    {
        IceTInt current_id;
        icetGetIntegerv(id_pname, &current_id);
        if (current_id != 0) {
            char message[256];
            sprintf(message,
                    "Called start for timer 0x%x,"
                    " but end never called for timer 0x%x",
                    result_pname,
                    current_id);
            icetRaiseError(message, ICET_SANITY_CHECK_FAIL);
        }
    }

    icetStateSetInteger(id_pname, result_pname);
    icetStateSetDouble(start_pname, icetWallTime());
}

static void icetTimingEnd(IceTEnum start_pname,
                          IceTEnum id_pname,
                          IceTEnum result_pname,
                          const char *name)
{
    icetRaiseDebug1("Ending %s", name);

    {
        IceTInt current_id;
        icetGetIntegerv(id_pname, &current_id);
        if ((IceTEnum)current_id != result_pname) {
            char message[256];
            sprintf(message,
                    "Started timer 0x%x, but ended timer 0x%x",
                    result_pname,
                    current_id);
            icetRaiseError(message, ICET_SANITY_CHECK_FAIL);
        }
    }

    icetStateSetInteger(id_pname, 0);

    {
        IceTDouble start_time;
        IceTDouble old_time;
        icetGetDoublev(start_pname, &start_time);
        icetGetDoublev(result_pname, &old_time);
        icetStateSetDouble(result_pname, icetWallTime() - start_time);
    }
}

void icetTimingRenderBegin(void)
{
    icetTimingBegin(ICET_SUBFUNC_START_TIME,
                    ICET_SUBFUNC_TIME_ID,
                    ICET_RENDER_TIME,
                    "render");
}
void icetTimingRenderEnd(void)
{
    icetTimingEnd(ICET_SUBFUNC_START_TIME,
                  ICET_SUBFUNC_TIME_ID,
                  ICET_RENDER_TIME,
                  "render");
}

void icetTimingBufferReadBegin(void)
{
    icetTimingBegin(ICET_SUBFUNC_START_TIME,
                    ICET_SUBFUNC_TIME_ID,
                    ICET_BUFFER_READ_TIME,
                    "buffer read");
}
void icetTimingBufferReadEnd(void)
{
    icetTimingEnd(ICET_SUBFUNC_START_TIME,
                  ICET_SUBFUNC_TIME_ID,
                  ICET_BUFFER_READ_TIME,
                  "buffer read");
}

void icetTimingBufferWriteBegin(void)
{
    icetTimingBegin(ICET_SUBFUNC_START_TIME,
                    ICET_SUBFUNC_TIME_ID,
                    ICET_BUFFER_WRITE_TIME,
                    "buffer write");
}
void icetTimingBufferWriteEnd(void)
{
    icetTimingEnd(ICET_SUBFUNC_START_TIME,
                  ICET_SUBFUNC_TIME_ID,
                  ICET_BUFFER_WRITE_TIME,
                  "buffer write");
}

void icetTimingCompressBegin(void)
{
    icetTimingBegin(ICET_SUBFUNC_START_TIME,
                    ICET_SUBFUNC_TIME_ID,
                    ICET_COMPRESS_TIME,
                    "compress");
}
void icetTimingCompressEnd(void)
{
    icetTimingEnd(ICET_SUBFUNC_START_TIME,
                  ICET_SUBFUNC_TIME_ID,
                  ICET_COMPRESS_TIME,
                  "compress");
}

void icetTimingBlendBegin(void)
{
    icetTimingBegin(ICET_SUBFUNC_START_TIME,
                    ICET_SUBFUNC_TIME_ID,
                    ICET_BLEND_TIME,
                    "blend");
}
void icetTimingBlendEnd(void)
{
    icetTimingEnd(ICET_SUBFUNC_START_TIME,
                  ICET_SUBFUNC_TIME_ID,
                  ICET_BLEND_TIME,
                  "blend");
}

void icetTimingDrawFrameBegin(void)
{
    icetTimingBegin(ICET_DRAW_START_TIME,
                    ICET_DRAW_TIME_ID,
                    ICET_TOTAL_DRAW_TIME,
                    "draw frame");
}
void icetTimingDrawFrameEnd(void)
{
    icetTimingEnd(ICET_DRAW_START_TIME,
                  ICET_DRAW_TIME_ID,
                  ICET_TOTAL_DRAW_TIME,
                  "draw frame");
}
