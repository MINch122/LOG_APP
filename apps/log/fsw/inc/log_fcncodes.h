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
 *   Specification for the LOG command function codes
 *
 * @note
 *   This file should be strictly limited to the command/function code (CC)
 *   macro definitions.  Other definitions such as enums, typedefs, or other
 *   macros should be placed in the msgdefs.h or msg.h files.
 */
#ifndef LOG_FCNCODES_H
#define LOG_FCNCODES_H

#include "log_fcncode_values.h"

/************************************************************************
 * Macro Definitions
 ************************************************************************/

/*
** Log command codes
*/
#define LOG_NOOP_CC                LOG_CCVAL(NOOP)
#define LOG_RESET_COUNTERS_CC      LOG_CCVAL(RESET_COUNTERS)
#define LOG_PROCESS_CC             LOG_CCVAL(PROCESS)
#define LOG_DISPLAY_PARAM_CC       LOG_CCVAL(DISPLAY_PARAM)
#define LOG_GET_CURRENT_HEADER_CC  LOG_CCVAL(GET_CURRENT_HEADER)
#define LOG_GET_LOG_COUNT_CC       LOG_CCVAL(GET_LOG_COUNT)

#endif
