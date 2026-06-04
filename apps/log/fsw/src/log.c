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
#include "log_cmds.h"
#include "log_utils.h"
#include "log_eventids.h"
#include "log_dispatch.h"
#include "log_tbl.h"
#include "log_version.h"

/*
** global data
*/
LOG_Data_t LOG_Data;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
/*                                                                            */
/* Application entry point and main process loop                              */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
void LOG_Main(void)
{
    CFE_Status_t     status;
    CFE_SB_Buffer_t *SBBufPtr;

    /*
    ** Create the first Performance Log entry
    */
    CFE_ES_PerfLogEntry(LOG_PERF_ID);

    /*
    ** Perform application-specific initialization
    ** If the Initialization fails, set the RunStatus to
    ** CFE_ES_RunStatus_APP_ERROR and the App will not enter the RunLoop
    */
    status = LOG_Init();
    if (status != CFE_SUCCESS)
    {
        LOG_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    /*
    ** Log Runloop
    */
    while (CFE_ES_RunLoop(&LOG_Data.RunStatus) == true)
    {
        /*
        ** Performance Log Exit Stamp
        */
        CFE_ES_PerfLogExit(LOG_PERF_ID);

        /* Pend on receipt of command packet */
        status = CFE_SB_ReceiveBuffer(&SBBufPtr, LOG_Data.CommandPipe, CFE_SB_PEND_FOREVER);

        /*
        ** Performance Log Entry Stamp
        */
        CFE_ES_PerfLogEntry(LOG_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            LOG_TaskPipe(SBBufPtr);
        }
        else
        {
            CFE_EVS_SendEvent(LOG_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "LOG: SB Pipe Read Error, App Will Exit");

            LOG_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    /*
    ** Performance Log Exit Stamp
    */
    CFE_ES_PerfLogExit(LOG_PERF_ID);

    CFE_ES_ExitApp(LOG_Data.RunStatus);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* Initialization                                                             */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t LOG_Init(void)
{
    CFE_Status_t status;
    char         VersionString[LOG_CFG_MAX_VERSION_STR_LEN];

    /* Zero out the global data structure */
    memset(&LOG_Data, 0, sizeof(LOG_Data));

    // LOGGING on
    LOG_Data.LogEnabled = 1;

    LOG_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    /*
    ** Register the events
    */
    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Log: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);
    }
    else
    {
        /*
         ** Initialize housekeeping packet (clear user data area).
         */
        CFE_MSG_Init(CFE_MSG_PTR(LOG_Data.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(LOG_HK_TLM_MID),
                     sizeof(LOG_Data.HkTlm));

        /*
         ** Initialize HeaderTlm packet
         */
        CFE_MSG_Init(CFE_MSG_PTR(LOG_Data.HdrTlm.TelemetryHeader), CFE_SB_ValueToMsgId(LOG_HDR_TLM_MID),
                     sizeof(LOG_HeaderTlm_t));

        /*
         ** Initialize Count command Tlm packet
         */
        CFE_MSG_Init(CFE_MSG_PTR(LOG_Data.CntTlm.TelemetryHeader),  CFE_SB_ValueToMsgId(LOG_CNT_TLM_MID), 
                     sizeof(LOG_CountTlm_t));

        /*
         ** Create Software Bus message pipe.
         */
        status = CFE_SB_CreatePipe(&LOG_Data.CommandPipe, LOG_PLATFORM_PIPE_DEPTH,
                                   LOG_PLATFORM_PIPE_NAME);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(LOG_CR_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Log: Error creating SB Command Pipe, RC = 0x%08lX", (unsigned long)status);
        }

        status = CFE_SB_CreatePipe(&LOG_Data.EventPipe, LOG_PLATFORM_EVENT_PIPE_DEPTH,
                                    LOG_PLATFORM_EVENT_PIPE_NAME);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(LOG_CR_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Log: Error creating SB Command Pipe, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to Housekeeping request commands
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(LOG_SEND_HK_MID), LOG_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(LOG_SUB_HK_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Log: Error Subscribing to HK request, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to ground command packets
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(LOG_CMD_MID), LOG_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(LOG_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Log: Error Subscribing to Commands, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to EVS Event message
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CFE_EVS_LONG_EVENT_MSG_MID), LOG_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(LOG_SUB_EVS_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Log: Error Subscribing to EVS events, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Register Example Table(s)
        */
        status = CFE_TBL_Register(&LOG_Data.TblHandles[0], "ExampleTable", sizeof(LOG_ExampleTable_t),
                                  CFE_TBL_OPT_DEFAULT, LOG_TblValidationFunc);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(LOG_TABLE_REG_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Log: Error Registering Example Table, RC = 0x%08lX", (unsigned long)status);
        }
        else
        {
            status = CFE_TBL_Load(LOG_Data.TblHandles[0], CFE_TBL_SRC_FILE, LOG_PLATFORM_TABLE_FILE);
        }

        CFE_Config_GetVersionString(VersionString, LOG_CFG_MAX_VERSION_STR_LEN, "Log", LOG_VERSION,
                                    LOG_BUILD_CODENAME, LOG_LAST_OFFICIAL);

        CFE_EVS_SendEvent(LOG_INIT_INF_EID, CFE_EVS_EventType_INFORMATION, "Log Initialized.%s",
                          VersionString);
    }

    return status;
}

/* 민창희가 만들었어요(with AI) */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         SB에서부터 오는 event message를 저장하기 위한 함수                      */
/*         LINUX system call (POSIX) used: open, close, mmap                  */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */

CFE_Status_t LOG_ProcessEventMsg(const CFE_EVS_LongEventTlm_t *Msg)
{
    char Filename[64];
    uint32 TotalFileSize;
    uint32 WriteOffset;

    OS_printf("[LOG APP] I receive an Event Mesagge. OK!\n");

    // LOG Enabled 확인
    if (LOG_Data.LogEnabled != 1)
    {
        OS_printf("[LOG APP] BUT LOG Disabled\n");
        return CFE_SUCCESS;
    }

    // Debug 메시지 무시
    if (Msg->Payload.PacketID.EventType == CFE_EVS_EventType_DEBUG)
    {
        return CFE_SUCCESS;
    }

    // 전체 파일 크기 = 헤더 + (최대 허용 메시지 개수 * 메시지 하나의 구조체 크기)
    TotalFileSize = sizeof(LOG_FileHeader_t) + (LOG_MAX_ENTRIES_PER_FILE * sizeof(CFE_EVS_LongEventTlm_t));

    // 파일 생성, mmap 초기화
    if (LOG_Data.MapPtr == NULL)
    {
        snprintf(Filename, sizeof(Filename), "./cf/log%03d.bin", (int)LOG_Data.CurrentIndex);

        // 표준 POSIX 사용 (mmap과 연동)
        LOG_Data.Fd = open(Filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (LOG_Data.Fd < 0)
        {
            OS_printf("[LOG APP] ERROR: Failed to open %s\n", Filename);
            return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
        }

        // ftruncate
        if (ftruncate(LOG_Data.Fd, TotalFileSize) == -1)
        {
            close(LOG_Data.Fd);
            return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
        }

        // memory mapping
        LOG_Data.MapPtr = (uint8 *)mmap(NULL, TotalFileSize, PROT_READ | PROT_WRITE, MAP_SHARED, LOG_Data.Fd, 0);
        if (LOG_Data.MapPtr == MAP_FAILED)
        {
            LOG_Data.MapPtr = NULL;
            close(LOG_Data.Fd);
            return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
        }

        // 헤더 구조체를 메모리 맨 앞에 복사
        memset(&LOG_Data.CurrentHeader, 0, sizeof(LOG_FileHeader_t));
        LOG_Data.CurrentHeader.CreateTime = CFE_TIME_GetTime().Seconds;
        LOG_Data.CurrentHeader.FileIndex = LOG_Data.CurrentIndex;
        LOG_Data.CurrentHeader.FileSize = sizeof(LOG_FileHeader_t);
        LOG_Data.EntryCount = 0;

        memcpy(LOG_Data.MapPtr, &LOG_Data.CurrentHeader, sizeof(LOG_FileHeader_t));
    }

    // 메모리에 데이터 복사
    WriteOffset = sizeof(LOG_FileHeader_t) + (LOG_Data.EntryCount * sizeof(CFE_EVS_LongEventTlm_t));

    memcpy(LOG_Data.MapPtr + WriteOffset, Msg, sizeof(CFE_EVS_LongEventTlm_t));

    // 헤더 상태 업데이트
    LOG_Data.EntryCount++;
    LOG_Data.CurrentHeader.FileSize += sizeof(CFE_EVS_LongEventTlm_t);

    if (Msg->Payload.PacketID.EventType == CFE_EVS_EventType_CRITICAL)
    {
        LOG_Data.CurrentHeader.CritCount++;
    }
    else if (Msg->Payload.PacketID.EventType == CFE_EVS_EventType_ERROR)
    {
        LOG_Data.CurrentHeader.ErrCount++;
    }
    else
    {
        LOG_Data.CurrentHeader.InfoCount++;
    }
    memcpy(LOG_Data.MapPtr, &LOG_Data.CurrentHeader, sizeof(LOG_FileHeader_t));

    // 다 채웠을 때 - 파일 닫기, 초기화
    if (LOG_Data.EntryCount >= LOG_MAX_ENTRIES_PER_FILE)
    {
        LOG_Data.CurrentHeader.CloseTime = CFE_TIME_GetTime().Seconds;
        memcpy(LOG_Data.MapPtr, &LOG_Data.CurrentHeader, sizeof(LOG_FileHeader_t));

        msync(LOG_Data.MapPtr, TotalFileSize, MS_SYNC);

        munmap(LOG_Data.MapPtr, TotalFileSize);
        close(LOG_Data.Fd);

        LOG_Data.MapPtr = NULL;
        LOG_Data.CurrentIndex++;
    }

    return CFE_SUCCESS;
}