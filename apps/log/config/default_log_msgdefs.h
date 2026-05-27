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
 *   Specification for the LOG command and telemetry
 *   message payload and constant definitions.
 */
#ifndef DEFAULT_LOG_MSGDEFS_H
#define DEFAULT_LOG_MSGDEFS_H

#include "common_types.h"
#include "log_fcncodes.h"

#pragma pack(push,1)

typedef struct LOG_DisplayParam_Payload
{
    uint32 ValU32;                                    /**< 32 bit unsigned integer value */
    int16  ValI16;                                    /**< 16 bit signed integer value */
    char   ValStr[LOG_MISSION_STRING_VAL_LEN]; /**< An example string */
} LOG_DisplayParam_Payload_t;

typedef struct 
{
    uint32 start_time;
    uint32 end_time;

    uint8 query_level;    // bit mask: LOG_MASK_xxx 참고
} LOG_QueryTime_Payload_t;

typedef struct
{
    uint16 start_idx;
    uint16 log_number;  // start_idx부터 로그 몇 개를 가져올 건지

    uint8 query_level;
} LOG_QueryNumber_Payload_t;


/*************************************************************************/
/*
** Type definition (Log housekeeping)
*/

typedef struct LOG_HkTlm_Payload
{
    uint8 CommandCounter;
    uint8 CommandErrorCounter;
    uint8 spare[2];
} LOG_HkTlm_Payload_t;

#pragma pack(pop)

#endif
