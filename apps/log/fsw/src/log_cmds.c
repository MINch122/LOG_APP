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
 *   This file contains the source code for the Log Ground Command-handling functions
 */

/*
** Include Files:
*/
#include "log.h"
#include "log_cmds.h"
#include "log_msgids.h"
#include "log_eventids.h"
#include "log_version.h"
#include "log_tbl.h"
#include "log_utils.h"
#include "log_msg.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the Apps     */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
CFE_Status_t LOG_SendHkCmd(const LOG_SendHkCmd_t *Msg)
{
    int i;

    /*
    ** Get command execution counters...
    */
    LOG_Data.HkTlm.Payload.CommandErrorCounter = LOG_Data.ErrCounter;
    LOG_Data.HkTlm.Payload.CommandCounter      = LOG_Data.CmdCounter;

    /*
    ** Send housekeeping telemetry packet...
    */
    CFE_SB_TimeStampMsg(CFE_MSG_PTR(LOG_Data.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(LOG_Data.HkTlm.TelemetryHeader), true);

    /*
    ** Manage any pending table loads, validations, etc.
    */
    for (i = 0; i < LOG_PLATFORM_NUMBER_OF_TABLES; i++)
    {
        CFE_TBL_Manage(LOG_Data.TblHandles[i]);
    }

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         SB에서부터 오는 event message를 저장하기 위한 함수                      */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */

CFE_Status_t LOG_ProcessEventMsg(const CFE_EVS_LongEventTlm_t *Msg)
{
    char Filename[64];
    uint32 TotalFileSize;
    uint32 WriteOffset;

    OS_printf("[LOG APP] I receive an Event Mesagge. OK!\n");

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

    // 상태 업데이트
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* LOG NOOP commands                                                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t LOG_NoopCmd(const LOG_NoopCmd_t *Msg)
{
    LOG_Data.CmdCounter++;

    CFE_EVS_SendEvent(LOG_NOOP_INF_EID, CFE_EVS_EventType_INFORMATION, "LOG: NOOP command %s",
                      LOG_VERSION);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
CFE_Status_t LOG_ResetCountersCmd(const LOG_ResetCountersCmd_t *Msg)
{
    LOG_Data.CmdCounter = 0;
    LOG_Data.ErrCounter = 0;

    CFE_EVS_SendEvent(LOG_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "LOG: RESET command");

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function Process Ground Station Command                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
CFE_Status_t LOG_ProcessCmd(const LOG_ProcessCmd_t *Msg)
{
    CFE_Status_t               Status;
    void *                     TblAddr;
    LOG_ExampleTable_t *TblPtr;
    const char *               TableName = "LOG.ExampleTable";

    /* Log Use of Example Table */
    LOG_Data.CmdCounter++;
    Status = CFE_TBL_GetAddress(&TblAddr, LOG_Data.TblHandles[0]);
    if (Status < CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Log: Fail to get table address: 0x%08lx", (unsigned long)Status);
    }
    else
    {
        TblPtr = TblAddr;
        CFE_ES_WriteToSysLog("Log: Example Table Value 1: %d  Value 2: %d", TblPtr->Int1, TblPtr->Int2);

        LOG_GetCrc(TableName);

        Status = CFE_TBL_ReleaseAddress(LOG_Data.TblHandles[0]);
        if (Status != CFE_SUCCESS)
        {
            CFE_ES_WriteToSysLog("Log: Fail to release table address: 0x%08lx", (unsigned long)Status);
        }
        else
        {
        }
    }

    return Status;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* A simple example command that displays a passed-in value                   */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t LOG_DisplayParamCmd(const LOG_DisplayParamCmd_t *Msg)
{
    LOG_Data.CmdCounter++;
    CFE_EVS_SendEvent(LOG_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "LOG: ValU32=%lu, ValI16=%d, ValStr=%s", (unsigned long)Msg->Payload.ValU32,
                      (int)Msg->Payload.ValI16, Msg->Payload.ValStr);

    return CFE_SUCCESS;
}
