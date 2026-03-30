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
 * Define Event Maker Events IDs
 */

#ifndef EVENT_MAKER_EVENTS_H
#define EVENT_MAKER_EVENTS_H

#define EVENT_MAKER_RESERVED_EID      0
#define EVENT_MAKER_INIT_INF_EID      1
#define EVENT_MAKER_CC_ERR_EID        2
#define EVENT_MAKER_NOOP_INF_EID      3
#define EVENT_MAKER_RESET_INF_EID     4
#define EVENT_MAKER_MID_ERR_EID       5
#define EVENT_MAKER_CMD_LEN_ERR_EID   6
#define EVENT_MAKER_PIPE_ERR_EID      7
#define EVENT_MAKER_VALUE_INF_EID     8
#define EVENT_MAKER_CR_PIPE_ERR_EID   9
#define EVENT_MAKER_SUB_HK_ERR_EID    10
#define EVENT_MAKER_SUB_1HZ_ERR_EID    11
#define EVENT_MAKER_SUB_CMD_ERR_EID   12
#define EVENT_MAKER_TABLE_REG_ERR_EID 13
#define EVENT_MAKER_SEM_ERR_EID       14
#define EVENT_MAKER_TIMER_ERR_EID     15
#define EVENT_MAKER_TIMER_CREATED_INF_EID 16
#define EVENT_MAKER_TABLE_ACCESS_ERR_EID 17
#define EVENT_MAKER_CHILD_CREATE_ERR_EID 18

#endif /* EVENT_MAKER_EVENTS_H */
