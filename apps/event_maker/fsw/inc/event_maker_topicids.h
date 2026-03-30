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
 *   EVENT_MAKER Application Topic IDs
 */
#ifndef EVENT_MAKER_TOPICIDS_H
#define EVENT_MAKER_TOPICIDS_H

#include "event_maker_topicid_values.h"

#define EVENT_MAKER_MISSION_CMD_TOPICID             EVENT_MAKER_MISSION_TIDVAL(CMD)
#define DEFAULT_EVENT_MAKER_MISSION_CMD_TOPICID     0x82
#define EVENT_MAKER_MISSION_SEND_HK_TOPICID         EVENT_MAKER_MISSION_TIDVAL(SEND_HK)
#define DEFAULT_EVENT_MAKER_MISSION_SEND_HK_TOPICID 0x83
#define EVENT_MAKER_MISSION_HK_TLM_TOPICID          EVENT_MAKER_MISSION_TIDVAL(HK_TLM)
#define DEFAULT_EVENT_MAKER_MISSION_HK_TLM_TOPICID  0x83

#endif
