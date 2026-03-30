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

/**
 * @file
 *
 * EVENT_MAKER Application Platform Configuration Header File
 *
 * This is a compatibility header for the "platform_cfg.h" file that has
 * traditionally provided both public and private config definitions
 * for each CFS app.
 *
 * These definitions are now provided in two separate files, one for
 * the public/mission scope and one for internal scope.
 *
 * @note This file may be overridden/superceded by mission-provided definitions
 * either by overriding this header or by generating definitions from a command/data
 * dictionary tool.
 */
#ifndef EVENT_MAKER_INTERNAL_CFG_H
#define EVENT_MAKER_INTERNAL_CFG_H

#include "event_maker_internal_cfg_values.h"

/***********************************************************************/
#define EVENT_MAKER_PLATFORM_PIPE_DEPTH         EVENT_MAKER_PLATFORM_CFGVAL(PIPE_DEPTH)
#define DEFAULT_EVENT_MAKER_PLATFORM_PIPE_DEPTH 32 /* Depth of the Command Pipe for Application */

#define EVENT_MAKER_PLATFORM_PIPE_NAME         EVENT_MAKER_PLATFORM_CFGVAL(PIPE_NAME)
#define DEFAULT_EVENT_MAKER_PLATFORM_PIPE_NAME "EVT_MKR_CMD_PIPE"

#define EVENT_MAKER_PLATFORM_SIMULATED_EVENT_ENTRIES    EVENT_MAKER_PLATFORM_CFGVAL(SIMULATED_EVENT_ENTRIES)
#define DEFAULT_EVENT_MAKER_PLATFORM_SIMULATED_EVENT_ENTRIES   20

#define EVENT_MAKER_PLATFORM_SIMULATED_EVENT_TICK_PER_SEC    EVENT_MAKER_PLATFORM_CFGVAL(SIMULATED_EVENT_TICK_PER_SEC)
#define DEFAULT_EVENT_MAKER_PLATFORM_SIMULATED_EVENT_TICK_PER_SEC   100 /* 10 ms per tick */

#define EVENT_MAKER_PLATFORM_TABLE_FILE         EVENT_MAKER_PLATFORM_CFGVAL(TABLE_FILE)
#define DEFAULT_EVENT_MAKER_PLATFORM_TABLE_FILE "/cf/event_maker_tbl.tbl"

#endif
