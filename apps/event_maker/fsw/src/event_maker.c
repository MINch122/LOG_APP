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
 *   This file contains the source code for the Event Maker.
 */

/*
** Include Files:
*/
#include "event_maker.h"
#include "event_maker_cmds.h"
#include "event_maker_utils.h"
#include "event_maker_eventids.h"
#include "event_maker_dispatch.h"
#include "event_maker_tbl.h"
#include "event_maker_simulate.h"
#include "event_maker_version.h"

// #include "cfe_msgids.h" /* 1Hz tone subscription */


/*
** global data
*/
EVENT_MAKER_Data_t EVENT_MAKER_Data;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
/*                                                                            */
/* Application entry point and main process loop                              */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
void EVENT_MAKER_Main(void)
{
    CFE_Status_t     status;
    CFE_SB_Buffer_t *SBBufPtr;

    /*
    ** Create the first Performance Log entry
    */
    CFE_ES_PerfLogEntry(EVENT_MAKER_PERF_ID);

    /*
    ** Perform application-specific initialization
    ** If the Initialization fails, set the RunStatus to
    ** CFE_ES_RunStatus_APP_ERROR and the App will not enter the RunLoop
    */
    status = EVENT_MAKER_Init();
    if (status != CFE_SUCCESS)
    {
        EVENT_MAKER_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    status = EVENT_MAKER_InitSimulatedEvents();
    if (status != CFE_SUCCESS)
        EVENT_MAKER_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;

        /*
    ** Event Maker Runloop
    */
    while (CFE_ES_RunLoop(&EVENT_MAKER_Data.RunStatus) == true)
    {
        /*
        ** Performance Log Exit Stamp
        */
        CFE_ES_PerfLogExit(EVENT_MAKER_PERF_ID);

        /* Pend on receipt of command packet */
        status = CFE_SB_ReceiveBuffer(&SBBufPtr, EVENT_MAKER_Data.CommandPipe, CFE_SB_PEND_FOREVER);

        /*
        ** Performance Log Entry Stamp
        */
        CFE_ES_PerfLogEntry(EVENT_MAKER_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            EVENT_MAKER_TaskPipe(SBBufPtr);
        }
        else
        {
            CFE_EVS_SendEvent(EVENT_MAKER_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "EVENT MAKER: SB Pipe Read Error, App Will Exit");

            EVENT_MAKER_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    /*
    ** Performance Log Exit Stamp
    */
    CFE_ES_PerfLogExit(EVENT_MAKER_PERF_ID);

    CFE_ES_ExitApp(EVENT_MAKER_Data.RunStatus);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* Initialization                                                             */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t EVENT_MAKER_Init(void)
{
    CFE_Status_t status;
    char         VersionString[EVENT_MAKER_CFG_MAX_VERSION_STR_LEN];

    /* Zero out the global data structure */
    memset(&EVENT_MAKER_Data, 0, sizeof(EVENT_MAKER_Data));

    EVENT_MAKER_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    EVENT_MAKER_Data.event_enable = 1;

    /*
    ** Register the events
    */
    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Event Maker: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);
    }
    else
    {
        /*
         ** Initialize housekeeping packet (clear user data area).
         */
        CFE_MSG_Init(CFE_MSG_PTR(EVENT_MAKER_Data.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(EVENT_MAKER_HK_TLM_MID),
                     sizeof(EVENT_MAKER_Data.HkTlm));

        /*
         ** Create Software Bus message pipe.
         */
        status = CFE_SB_CreatePipe(&EVENT_MAKER_Data.CommandPipe, EVENT_MAKER_PLATFORM_PIPE_DEPTH,
                                   EVENT_MAKER_PLATFORM_PIPE_NAME);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(EVENT_MAKER_CR_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Event Maker: Error creating SB Command Pipe, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to Housekeeping request commands
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(EVENT_MAKER_SEND_HK_MID), EVENT_MAKER_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(EVENT_MAKER_SUB_HK_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Event Maker: Error Subscribing to HK request, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to ground command packets
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(EVENT_MAKER_CMD_MID), EVENT_MAKER_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(EVENT_MAKER_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Event Maker: Error Subscribing to Commands, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {

        status = CFE_TBL_Register(&EVENT_MAKER_Data.TblHandle, "SimulatedEvents", sizeof(EVENT_MAKER_SimulatedEventEntry_t) * EVENT_MAKER_PLATFORM_SIMULATED_EVENT_ENTRIES,
                                  CFE_TBL_OPT_DEFAULT, NULL);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(EVENT_MAKER_TABLE_REG_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Event Maker: Error Registering Simulated Event Table, RC = 0x%08lX", (unsigned long)status);
        }
        else
        {
            status = CFE_TBL_Load(EVENT_MAKER_Data.TblHandle, CFE_TBL_SRC_FILE, EVENT_MAKER_PLATFORM_TABLE_FILE);
        }

        CFE_Config_GetVersionString(VersionString, EVENT_MAKER_CFG_MAX_VERSION_STR_LEN, "Event Maker", EVENT_MAKER_VERSION,
                                    EVENT_MAKER_BUILD_CODENAME, EVENT_MAKER_LAST_OFFICIAL);

        CFE_EVS_SendEvent(EVENT_MAKER_INIT_INF_EID, CFE_EVS_EventType_INFORMATION, "Event Maker Initialized.%s",
                          VersionString);
    }

    return status;
}
