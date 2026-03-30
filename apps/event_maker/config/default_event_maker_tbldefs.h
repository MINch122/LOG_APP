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
 *   Specification for the EVENT_MAKER table related
 *   constant definitions.
 */
#ifndef EVENT_MAKER_TBLDEFS_H
#define EVENT_MAKER_TBLDEFS_H

#include "common_types.h"
#include "event_maker_mission_cfg.h"

/*
** Example Table structure
*/
typedef struct
{
    uint32 Period;            // milliseconds between events (fires only once if zero)
    uint32 PeriodDeviationMs; // random deviation to add to the period in milliseconds
    uint32 EveryNth;          // send an event every Nth time through the loop
    uint32 Offset;            // offset in milliseconds of the first event
    
    uint16 EventID;
    uint16 EventType;
    char   Message[CFE_MISSION_EVS_MAX_MESSAGE_LENGTH]; // Event message. Null str ("") to generate randomly (TBA).
} EVENT_MAKER_SimulatedEventEntry_t;

#endif
