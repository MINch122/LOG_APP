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
 * \file
 *   This file contains the source code for the Event Maker Ground Command-handling functions
 */

/*
** Include Files:
*/
#include "event_maker.h"
#include "event_maker_cmds.h"
#include "event_maker_msgids.h"
#include "event_maker_eventids.h"
#include "event_maker_version.h"
#include "event_maker_tbl.h"
#include "event_maker_utils.h"
#include "event_maker_msg.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the Apps     */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
CFE_Status_t EVENT_MAKER_SendHkCmd(const EVENT_MAKER_SendHkCmd_t *Msg)
{
    /*
    ** Get command execution counters...
    */
    EVENT_MAKER_Data.HkTlm.Payload.CommandErrorCounter = EVENT_MAKER_Data.ErrCounter;
    EVENT_MAKER_Data.HkTlm.Payload.CommandCounter      = EVENT_MAKER_Data.CmdCounter;

    /*
    ** Send housekeeping telemetry packet...
    */
    CFE_SB_TimeStampMsg(CFE_MSG_PTR(EVENT_MAKER_Data.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(EVENT_MAKER_Data.HkTlm.TelemetryHeader), true);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* EVENT_MAKER NOOP commands                                                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t EVENT_MAKER_NoopCmd(const EVENT_MAKER_NoopCmd_t *Msg)
{
    EVENT_MAKER_Data.CmdCounter++;

    CFE_EVS_SendEvent(EVENT_MAKER_NOOP_INF_EID, CFE_EVS_EventType_INFORMATION, "EVENT_MAKER: NOOP command %s",
                      EVENT_MAKER_VERSION);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
CFE_Status_t EVENT_MAKER_ResetCountersCmd(const EVENT_MAKER_ResetCountersCmd_t *Msg)
{
    EVENT_MAKER_Data.CmdCounter = 0;
    EVENT_MAKER_Data.ErrCounter = 0;

    CFE_EVS_SendEvent(EVENT_MAKER_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "EVENT_MAKER: RESET command");

    return CFE_SUCCESS;
}

CFE_Status_t EVENT_MAKER_DisabledCmd(const EVENT_MAKER_DisabledCmd_t *Msg)
{
    EVENT_MAKER_Data.CmdCounter++;

    EVENT_MAKER_Data.event_enable = 0;

    CFE_EVS_SendEvent(EVENT_MAKER_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "EVENT_MAKER: DISABLED command");

    return CFE_SUCCESS;
}