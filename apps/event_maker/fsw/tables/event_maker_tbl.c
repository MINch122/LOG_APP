/************************************************************************
 * NASA Docket No. GSC-19,200-1, and identified as "cFS Draco"
 *
 * Copyright (c) 2023 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

#include "cfe_tbl_filedef.h" /* Required to obtain the CFE_TBL_FILEDEF macro definition */
#include "event_maker_tbl.h"
#include "cfe_evs_api_typedefs.h"
#include "event_maker_platform_cfg.h"

/*
** Configure this table for simulated event message experiments.
** You can add entries if needed (max EVENT_MAKER_PLATFORM_SIMULATED_EVENT_ENTRIES).
*/
EVENT_MAKER_SimulatedEventEntry_t SimulatedEvents[EVENT_MAKER_PLATFORM_SIMULATED_EVENT_ENTRIES] = {
    {
        .Period            = 1000,      /* period in millisec */
        .PeriodDeviationMs = 0,         /* standard deviation of the random noise in the period */
        .EveryNth          = 0,         /* fires every N period (ignored if zero) */
        .Offset            = 0,         /* first message starts after this millisec */
        .EventID           = 10,                            /* event body */
        .EventType         = CFE_EVS_EventType_ERROR,       /* event body */
        .Message           = "1 sec periodic (exactly)"     /* event body */
    },

    {
        .Period            = 500,
        .PeriodDeviationMs = 100,
        .EveryNth          = 6,
        .Offset            = 5000,
        .EventID           = 1,
        .EventType         = CFE_EVS_EventType_ERROR,
        .Message           = "3 sec periodic, after 5 sec offset"
    },

    {
        .Period            = 2000,
        .PeriodDeviationMs = 0,
        .EveryNth          = 2,
        .Offset            = 10000,
        .EventID           = 2,
        .EventType         = CFE_EVS_EventType_CRITICAL,
        .Message           = "4 sec periodic (exactly), after 10 sec offset"
    },

    {
        .Period            = 0,
        .PeriodDeviationMs = 0,
        .EveryNth          = 0,
        .Offset            = 3000,
        .EventID           = 3,
        .EventType         = CFE_EVS_EventType_INFORMATION,
        .Message           = "Fire once after 3 sec"
    },

    {
        .Period            = 5000,
        .PeriodDeviationMs = 500,
        .EveryNth          = 1,
        .Offset            = 500,
        .EventID           = 4,
        .EventType         = CFE_EVS_EventType_DEBUG,
        .Message           = "Debug purpose event message"
    }
};

/*
** The macro below identifies:
**    1) the data structure type to use as the table image format
**    2) the name of the table to be placed into the cFE Example Table File Header
**    3) a brief description of the contents of the file image
**    4) the desired name of the table image binary file that is cFE compatible
*/
CFE_TBL_FILEDEF(SimulatedEvents, EVENT_MAKER.SimulatedEvents, Table Utility Test Table, event_maker_tbl.tbl)
