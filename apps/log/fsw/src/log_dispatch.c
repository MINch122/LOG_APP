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
 *   This file contains the source code for the Log.
 */

/*
** Include Files:
*/
#include "log.h"
#include "log_dispatch.h"
#include "log_cmds.h"
#include "log_eventids.h"
#include "log_msgids.h"
#include "log_msg.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* Verify command packet length                                               */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
bool LOG_VerifyCmdLength(const CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength)
{
    bool              result       = true;
    size_t            ActualLength = 0;
    CFE_SB_MsgId_t    MsgId        = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_FcnCode_t FcnCode      = 0;

    CFE_MSG_GetSize(MsgPtr, &ActualLength);

    /*
    ** Verify the command packet length.
    */
    if (ExpectedLength != ActualLength)
    {
        CFE_MSG_GetMsgId(MsgPtr, &MsgId);
        CFE_MSG_GetFcnCode(MsgPtr, &FcnCode);

        CFE_EVS_SendEvent(LOG_CMD_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Invalid Msg length: ID = 0x%X,  CC = %u, Len = %u, Expected = %u",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId), (unsigned int)FcnCode, (unsigned int)ActualLength,
                          (unsigned int)ExpectedLength);

        result = false;

        LOG_Data.ErrCounter++;
    }

    return result;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* LOG ground commands                                                     */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void LOG_ProcessGroundCommand(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_MSG_FcnCode_t CommandCode = 0;

    CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &CommandCode);

    /*
    ** Process LOG app ground commands
    */
    switch (CommandCode)
    {
        case LOG_NOOP_CC:
            if (LOG_VerifyCmdLength(&SBBufPtr->Msg, sizeof(LOG_NoopCmd_t)))
            {
                LOG_NoopCmd((const LOG_NoopCmd_t *)SBBufPtr);
            }
            break;

        case LOG_RESET_COUNTERS_CC:
            if (LOG_VerifyCmdLength(&SBBufPtr->Msg, sizeof(LOG_ResetCountersCmd_t)))
            {
                LOG_ResetCountersCmd((const LOG_ResetCountersCmd_t *)SBBufPtr);
            }
            break;

        case LOG_PROCESS_CC:
            if (LOG_VerifyCmdLength(&SBBufPtr->Msg, sizeof(LOG_ProcessCmd_t)))
            {
                LOG_ProcessCmd((const LOG_ProcessCmd_t *)SBBufPtr);
            }
            break;

        case LOG_DISPLAY_PARAM_CC:
            if (LOG_VerifyCmdLength(&SBBufPtr->Msg, sizeof(LOG_DisplayParamCmd_t)))
            {
                LOG_DisplayParamCmd((const LOG_DisplayParamCmd_t *)SBBufPtr);
            }
            break;

        /* default case already found during FC vs length test */
        default:
            CFE_EVS_SendEvent(LOG_CC_ERR_EID, CFE_EVS_EventType_ERROR, "Invalid ground command code: CC = %d",
                              CommandCode);
            break;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the LOG    */
/*     command pipe.                                                          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void LOG_TaskPipe(const CFE_SB_Buffer_t *SBBufPtr)
{
    static CFE_SB_MsgId_t CMD_MID     = CFE_SB_MSGID_RESERVED;
    static CFE_SB_MsgId_t SEND_HK_MID = CFE_SB_MSGID_RESERVED;
    static CFE_SB_MsgId_t EVS_MID     = CFE_SB_MSGID_RESERVED;

    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

    /* cache the local MID Values here, this avoids repeat lookups */
    if (!CFE_SB_IsValidMsgId(CMD_MID))
    {
        CMD_MID     = CFE_SB_ValueToMsgId(LOG_CMD_MID);
        SEND_HK_MID = CFE_SB_ValueToMsgId(LOG_SEND_HK_MID);
        EVS_MID     = CFE_SB_ValueToMsgId(CFE_EVS_LONG_EVENT_MSG_MID);
    }

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    /* Process all SB messages */
    if (CFE_SB_MsgId_Equal(MsgId, SEND_HK_MID))
    {
        /* Housekeeping request */
        LOG_SendHkCmd((const LOG_SendHkCmd_t *)SBBufPtr);
    }
    else if (CFE_SB_MsgId_Equal(MsgId, CMD_MID))
    {
        /* Ground command */
        LOG_ProcessGroundCommand(SBBufPtr);
    }
    else if (CFE_SB_MsgId_Equal(MsgId, EVS_MID))
    {
        /* EVS event message receive */
        LOG_ProcessEventMsg((const CFE_EVS_LongEventTlm_t *)SBBufPtr);
    }
    else
    {
        /* Unknown command */
        CFE_EVS_SendEvent(LOG_MID_ERR_EID, CFE_EVS_EventType_ERROR, "LOG: invalid command packet,MID = 0x%x",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId));
    }
}
