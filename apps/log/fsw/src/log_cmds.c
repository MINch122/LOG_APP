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
CFE_Status_t LOG_SendHkCmd(const LOG_SendHkCmd_t *Msg) {
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
/* LOG NOOP commands                                                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t LOG_NoopCmd(const LOG_NoopCmd_t *Msg) {
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
CFE_Status_t LOG_ResetCountersCmd(const LOG_ResetCountersCmd_t *Msg) {
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
CFE_Status_t LOG_ProcessCmd(const LOG_ProcessCmd_t *Msg) {
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
CFE_Status_t LOG_DisplayParamCmd(const LOG_DisplayParamCmd_t *Msg) {
    LOG_Data.CmdCounter++;

    CFE_EVS_SendEvent(LOG_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "LOG: ValU32=%lu, ValI16=%d, ValStr=%s", (unsigned long)Msg->Payload.ValU32,
                      (int)Msg->Payload.ValI16, Msg->Payload.ValStr);

    return CFE_SUCCESS;
}


/* 민창희가 만들었어요(with AI) */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* LOG_ENABLE:                                                                */
/*     Logging enable command                                                 */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t LOG_EnabledCmd(const LOG_EnableCmd_t *Msg) {
    LOG_Data.CmdCounter++;
    LOG_Data.LogEnabled = 1;

    CFE_EVS_SendEvent(LOG_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION, "Logging Enabled");

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* LOG_DISABLE:                                                                */
/*     Logging disable command                                                 */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t LOG_DisabledCmd(const LOG_DisableCmd_t *Msg) {
    LOG_Data.CmdCounter++;
    LOG_Data.LogEnabled = 0;

    CFE_EVS_SendEvent(LOG_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION, "Logging Disabled");

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* GET_CURRENT_HEADER:                                                        */
/*     Gets the header of current log file                                    */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t LOG_GetCurrentHeaderCmd(const LOG_GetCurrentHeaderCmd_t *Msg) {
    LOG_Data.CmdCounter++;

    // 열려 있는 파일 있는지 확인
    if (LOG_Data.MapPtr == NULL)
    {
        OS_printf("[LOG APP] There is No opened file.\n");
        LOG_Data.ErrCounter++;
        
        CFE_EVS_SendEvent(LOG_CMD_ERR_EID, CFE_EVS_EventType_ERROR, "LOG: GET_CURRENT_HEADER failed. no opened file.");

        return CFE_SUCCESS;
    }

    LOG_Data.HdrTlm.HeaderData = LOG_Data.CurrentHeader;

    CFE_SB_TimeStampMsg(CFE_MSG_PTR(LOG_Data.HdrTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(LOG_Data.HdrTlm.TelemetryHeader), true);

    // debug print
    OS_printf("[LOG APP] GET_CURRENT_HEADER Success.\n");
    OS_printf("[LOG APP] --- Current File Header Status ---\n");
    OS_printf("CreateTime: %u, CloseTime: %u, FileIndex: %u\n",
              (unsigned int)LOG_Data.CurrentHeader.CreateTime,
              (unsigned int)LOG_Data.CurrentHeader.CloseTime,
              (unsigned int)LOG_Data.CurrentHeader.FileIndex);
    OS_printf("InfoCnt: %u, ErrCnt: %u, CritCnt: %u, FileSize: %u bytes\n",
              (unsigned int)LOG_Data.CurrentHeader.InfoCount,
              (unsigned int)LOG_Data.CurrentHeader.ErrCount,
              (unsigned int)LOG_Data.CurrentHeader.CritCount,
              (unsigned int)LOG_Data.CurrentHeader.FileSize);

    CFE_EVS_SendEvent(LOG_CMD_INF_EID, CFE_EVS_EventType_INFORMATION, "GET_CURRENT_HEADER command processed successfully");

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* GET_LOG_COUNT:                                                             */
/*     Gets the LOG stats: total, info, err, crit count                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t LOG_GetLogCountCmd(const LOG_GetLogCountCmd_t *Msg) {
    LOG_Data.CmdCounter++;

    int fd;
    char Filename[64];
    LOG_FileHeader_t TempHeader;
    ssize_t bytesRead;

    // 누적 시작
    uint32 TotalEntries = 0;
    uint32 TotalInfo = 0;
    uint32 TotalErr = 0;
    uint32 TotalCrit = 0;
    uint32 ScannedFiles = 0;
    
    OS_printf("[LOG APP] Log Counting Start...\n");

    // 0번 파일부터 순회
    for (uint32 i = 0; i <= LOG_Data.CurrentIndex; i++)
    {
        snprintf(Filename, sizeof(Filename), "./cf/log%03d.bin", (int)i);

        // 파일 열기
        fd = open(Filename, O_RDONLY);
        if (fd < 0)
        {
            // 파일이 없으면 무시하고 다음으로
            continue;
        }

        // 헤더 읽기 - 헤더만 읽으면 count할 수 있음
        bytesRead = read(fd, &TempHeader, sizeof(LOG_FileHeader_t));
        close(fd);

        // 누적 계산
        if (bytesRead == sizeof(LOG_FileHeader_t))
        {
            if (i < LOG_Data.CurrentIndex)
            {
                TotalEntries += LOG_MAX_ENTRIES_PER_FILE; // 이미 닫힌 파일은 최대 개수
            }
            else
            {
                TotalEntries += LOG_Data.EntryCount; // 닫히지 않은 파일(=현재 파일)은 현재 엔트리 카운트
            }

            TotalInfo    += TempHeader.InfoCount;
            TotalErr     += TempHeader.ErrCount;
            TotalCrit    += TempHeader.CritCount;
            ScannedFiles ++;
        }
    }

    // Tlm 패킷 작성
    LOG_Data.CntTlm.ScannedFiles = ScannedFiles;
    LOG_Data.CntTlm.TotalEntries = TotalEntries;
    LOG_Data.CntTlm.TotalInfo    = TotalInfo;
    LOG_Data.CntTlm.TotalErr     = TotalErr;
    LOG_Data.CntTlm.TotalCrit    = TotalCrit;

    // 패킷 전송
    CFE_SB_TimeStampMsg(CFE_MSG_PTR(LOG_Data.CntTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(LOG_Data.CntTlm.TelemetryHeader), true);

    // debug print
    OS_printf("[LOG APP] --- Disk Log Statistics ---\n");
    OS_printf("Files Scanned: %u\n", (unsigned int)ScannedFiles);
    OS_printf("Total Logs   : %u\n", (unsigned int)TotalEntries);
    OS_printf("Info: %u, Err: %u, Crit: %u\n", (unsigned int)TotalInfo,
                (unsigned int)TotalErr, (unsigned int)TotalCrit);

    CFE_EVS_SendEvent(LOG_CMD_INF_EID, CFE_EVS_EventType_INFORMATION, "GET_LOG_COUNT command processed successfully");

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* GET LOG                                                                    */
/*     Gets the LOG                                                           */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* QUERY_TIME:                                                                */
/*     Gets the LOG according to time (start - end)                           */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t LOG_QueryTimeCmd(const LOG_QueryTimeCmd_t *Msg) {

    uint32 req_start = Msg->Payload.start_time;
    uint32 req_end   = Msg->Payload.end_time;
    uint8 req_mask   = Msg->Payload.query_level;

    // tlm pkt 초기화
    LOG_QueryTlm_t QueryPkt;
    memset(&QueryPkt, 0, sizeof(LOG_QueryTlm_t));
    CFE_MSG_Init(CFE_MSG_PTR(QueryPkt.TelemetryHeader), CFE_SB_ValueToMsgId(LOG_REP_TLM_MID), sizeof(LOG_QueryTlm_t));

    uint16 current_offset = 0; // payload 내에서 현재 쓰기 위치
    uint16 log_count = 0;  // 담은 로그 개수
    char filename[64];

    // 인덱스 0번 파일부터 현재 작성 중인 파일까지
    for (uint32 file_idx = 0; file_idx <= LOG_Data.CurrentIndex; file_idx++) {
        snprintf(filename, sizeof(filename), "./cf/log%03d.bin", (int)file_idx);

        // POSIX
        int fd = open(filename, O_RDONLY);
        if (fd < 0) {
            continue; // 파일이 없거나 열 수 없으면 패스
        }

        // 헤더 읽기
        LOG_FileHeader_t header;
        if (read(fd, &header, sizeof(LOG_FileHeader_t)) != sizeof(LOG_FileHeader_t)) {
            close(fd);
            continue;
        }

        // ------------ 시간 범위 확인 ------------------
        // CloseTime이 req_start보다 과거면 바ㅏ로 패스, 단 CloseTime = 0(즉 현재 파일)은 예외처리
        if (header.CloseTime > 0 && header.CloseTime < req_start) {
            close(fd);
            continue;
        }
        // CreateTime이 req_end보다 미래면 바ㅏ로 탐색 종료
        if(header.CreateTime > req_end) {
            close(fd);
            break;
        }

        // ----------- 00 무시 ------------
        // ftruncate로 파일 크기 고정 -> 끝까지 읽으면 안됨: 헤더 읽고 실제 쓰인 로그 개수 확인
        uint32 valid_cnt = header.InfoCount + header.ErrCount + header.CritCount;

        for (uint32 i = 0; i < valid_cnt; i++) {
            CFE_EVS_LongEventTlm_t raw_msg;
            if (read(fd, &raw_msg, sizeof(CFE_EVS_LongEventTlm_t)) != sizeof(CFE_EVS_LongEventTlm_t)) {
                break;
            }
            // 개별 메시지 시간 검사
            CFE_TIME_SysTime_t log_sys_time;
            CFE_MSG_GetMsgTime(CFE_MSG_PTR(raw_msg.TelemetryHeader), &log_sys_time);
            if (log_sys_time.Seconds < req_start || log_sys_time.Seconds > req_end) {
                continue;
            }

            // 개별 메시지 level 검사(mask)
            uint8 level_mask = 0;
            if (raw_msg.Payload.PacketID.EventType == CFE_EVS_EventType_INFORMATION) {
                level_mask = LOG_MASK_INFO; // (1 << 0)
            } else if (raw_msg.Payload.PacketID.EventType == CFE_EVS_EventType_ERROR) {
                level_mask = LOG_MASK_ERR;  // (1 << 1)
            } else if (raw_msg.Payload.PacketID.EventType == CFE_EVS_EventType_CRITICAL) {
                level_mask = LOG_MASK_CRIT; // (1 << 2)
            }

            if ((level_mask & req_mask) == 0) {
                continue;
            }


            // Packing
            // 앱이름 + EVS msg
            char combined_msg[256];
            snprintf(combined_msg, sizeof(combined_msg), "[%s] %s", raw_msg.Payload.PacketID.AppName, raw_msg.Payload.Message);

            uint16 actual_length = (uint16)strlen(combined_msg);

            // 남은 공간 확인
            if (current_offset + sizeof(uint16) + actual_length > LOG_MAX_DOWNLINK_PAYLOAD_SIZE) {
                close(fd);
                goto SEND_PACKET;
            }

            // 데이터 복붙
            memcpy(&QueryPkt.Payload[current_offset], &actual_length, sizeof(uint16));
            current_offset += sizeof(uint16);
            memcpy(&QueryPkt.Payload[current_offset], combined_msg, actual_length);
            current_offset += actual_length;

            log_count ++;
        }

        close(fd);  // 현재 파일 다 읽음
    }

SEND_PACKET:
    // 메타데이터 업데이트
    QueryPkt.TotalLogCount = log_count;
    QueryPkt.TotalDataLength = current_offset;

    CFE_Status_t Status;

    if (log_count > 0) {
        Status = LOG_logging(&QueryPkt);
        if (Status == CFE_SUCCESS) {
            CFE_EVS_SendEvent(LOG_CMD_INF_EID, CFE_EVS_EventType_INFORMATION,
                              "QUERY_TIME: Downlinked %d logs across files", log_count);
        }
        else {
            CFE_EVS_SendEvent(LOG_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "QUERY_TIME: Failed to downlink log query results");
        }
    }
    else {
        Status = CFE_SUCCESS;
        CFE_EVS_SendEvent(LOG_CMD_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "QUERY_TIME: No logs found matching criteria");
    }

    return Status;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* QUERY_NUMBER:                                                              */
/*     Gets the N LOGs starting from the first log at or after start_time     */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t LOG_QueryNumberCmd(const LOG_QueryNumber_t *Msg) {

    uint32 req_start = (uint32)Msg->Payload.start_time;
    uint16 req_count = Msg->Payload.log_number;
    uint8 req_mask   = Msg->Payload.query_level;

    // tlm pkt 초기화
    LOG_QueryTlm_t QueryPkt;
    memset(&QueryPkt, 0, sizeof(LOG_QueryTlm_t));
    CFE_MSG_Init(CFE_MSG_PTR(QueryPkt.TelemetryHeader), CFE_SB_ValueToMsgId(LOG_REP_TLM_MID), sizeof(LOG_QueryTlm_t));

    uint16 current_offset = 0;
    uint16 log_count = 0;
    char filename[64];
    uint8 started = 0;  // 0 = false, 1 = true: req_start 시각부터 카운트 시작하도록

    // 인덱스 0번 파일부터 현재 작성 중인 파일까지
    for (uint32 file_idx = 0; file_idx <= LOG_Data.CurrentIndex; file_idx++) {
        snprintf(filename, sizeof(filename), "./cf/log%03d.bin", (int)file_idx);

        // POSIX
        int fd = open(filename, O_RDONLY);
        if (fd < 0) {
            continue;   // 파일이 없거나 열 수 없으면 패스
        }

        // 헤더 읽기
        LOG_FileHeader_t header;
        if (read(fd, &header, sizeof(LOG_FileHeader_t)) != sizeof(LOG_FileHeader_t)) {
            close(fd);
            continue;
        }

        // 파일 닫힌 시각이 요청 시작 시각보다 과거면 바ㅏㅏ로 패스
        if (header.CloseTime > 0 && header.CloseTime < req_start) {
            close(fd);
            continue; 
        }

        uint32 valid_cnt = header.InfoCount + header.ErrCount + header.CritCount;

        for (uint32 i = 0; i < valid_cnt; i++) {
            CFE_EVS_LongEventTlm_t raw_msg;
            if (read(fd, &raw_msg, sizeof(CFE_EVS_LongEventTlm_t)) != sizeof(CFE_EVS_LongEventTlm_t)) {
                break;
            }

            CFE_TIME_SysTime_t log_sys_time;
            CFE_MSG_GetMsgTime(CFE_MSG_PTR(raw_msg.TelemetryHeader), &log_sys_time);

            // 시작 조건 확인
            if (started == 0) {
                if (log_sys_time.Seconds < req_start) {
                    continue;   // 아직 시작 아님
                }
                started = 1; // 지금부터 카운트 시작
            }

            // 개별 메시지 level 검사(mask)
            uint8 level_mask = 0;
            if (raw_msg.Payload.PacketID.EventType == CFE_EVS_EventType_INFORMATION) {
                level_mask = LOG_MASK_INFO;
            } else if (raw_msg.Payload.PacketID.EventType == CFE_EVS_EventType_ERROR) {
                level_mask = LOG_MASK_ERR;
            } else if (raw_msg.Payload.PacketID.EventType == CFE_EVS_EventType_CRITICAL) {
                level_mask = LOG_MASK_CRIT;
            }

            if ((level_mask & req_mask) == 0) {
                continue;
            }

            // Packing
            // 앱이름 + EVS msg
            char combined_msg[256];
            snprintf(combined_msg, sizeof(combined_msg), "[%s] %s", raw_msg.Payload.PacketID.AppName, raw_msg.Payload.Message);

            uint16 actual_length = (uint16)strlen(combined_msg);

            // 남은 공간 확인
            if (current_offset + sizeof(uint16) + actual_length > LOG_MAX_DOWNLINK_PAYLOAD_SIZE) {
                close(fd);
                goto SEND_NUMBER_PACKET;
            }

            // 데이터 복붙
            memcpy(&QueryPkt.Payload[current_offset], &actual_length, sizeof(uint16));
            current_offset += sizeof(uint16);
            memcpy(&QueryPkt.Payload[current_offset], combined_msg, actual_length);
            current_offset += actual_length;

            log_count++;

            // 로그 개수 도달했으면 종료
            if (log_count >= req_count) {
                close(fd);
                goto SEND_NUMBER_PACKET;
            }
        }

        close(fd);
    }

SEND_NUMBER_PACKET:
    // 메타데이터 업데이트
    QueryPkt.TotalLogCount = log_count;
    QueryPkt.TotalDataLength = current_offset;

    CFE_Status_t Status;

    if (log_count > 0) {
        Status = LOG_logging(&QueryPkt);
        if (Status == CFE_SUCCESS) {
            CFE_EVS_SendEvent(LOG_CMD_INF_EID, CFE_EVS_EventType_INFORMATION,
                              "QUERY_NUMBER: Downlinked %d logs across files", log_count);
        }
        else {
            CFE_EVS_SendEvent(LOG_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "QUERY_NUMBER: Failed to downlink log query results");
        }
    }
    else {
        Status = CFE_SUCCESS;
        CFE_EVS_SendEvent(LOG_CMD_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "QUERY_NUMBER: No logs found matching criteria");
    }

    return Status;
}
