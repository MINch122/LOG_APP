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
 *   This file contains the source code for the Log utility functions
 */

/*
** Include Files:
*/
#include <string.h>
#include "log.h"
#include "log_eventids.h"
#include "log_tbl.h"
#include "log_utils.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Verify contents of First Example Table buffer contents                  */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CFE_Status_t LOG_TblValidationFunc(void *TblData)
{
    CFE_Status_t               ReturnCode = CFE_SUCCESS;
    LOG_ExampleTable_t *TblDataPtr = (LOG_ExampleTable_t *)TblData;

    /*
    ** Log Example Table Validation
    */
    if (TblDataPtr->Int1 > LOG_PLATFORM_TBL_ELEMENT_1_MAX)
    {
        /* First element is out of range, return an appropriate error code */
        ReturnCode = LOG_PLATFORM_TABLE_OUT_OF_RANGE_ERR_CODE;
    }

    return ReturnCode;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Output CRC                                                      */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void LOG_GetCrc(const char *TableName)
{
    CFE_Status_t   status;
    uint32         Crc;
    CFE_TBL_Info_t TblInfoPtr;

    status = CFE_TBL_GetInfo(&TblInfoPtr, TableName);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Log: Error Getting Example Table Info");
    }
    else
    {
        Crc = TblInfoPtr.Crc;
        CFE_ES_WriteToSysLog("Log: CRC: 0x%08lX\n\n", (unsigned long)Crc);
    }
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                             */
/* LOG logging helper                                                          */
/*                                                                             */
/* Splits LOG query telemetry into 200-byte SB packets including the message   */
/* header, preserving complete log entry boundaries.                           */
/*                                                                             */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CFE_Status_t LOG_logging(const LOG_QueryTlm_t *OriginalPkt) {

    const size_t MaxTotalPacketSize = 200;  // 최대 전송 가능 사이즈 (헤더 + 메타데이터 + 페이로드)
    const size_t HeaderSize = sizeof(CFE_MSG_TelemetryHeader_t);
    // 사이즈 계산
    const size_t MetaSize = sizeof(OriginalPkt->TotalLogCount) + 
                            sizeof(OriginalPkt->TotalDataLength) + 
                            sizeof(OriginalPkt->Index);
    const size_t MaxChunkPayload = MaxTotalPacketSize - HeaderSize - MetaSize;

    const uint8 *PayloadPtr = OriginalPkt->Payload;
    size_t Offset = 0;
    uint8 PacketIndex = 0;
    
    // 원본 총 길이를 별도 변수에 저장 (루프 보호용)
    const size_t OriginalTotalLength = OriginalPkt->TotalDataLength; 

    if (HeaderSize + MetaSize >= MaxTotalPacketSize) {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    // 별도 변수를 이용하여 while 루프 돌기
    while (Offset < OriginalTotalLength) {
        size_t ChunkEnd = Offset;
        size_t ChunkLimit = Offset + MaxChunkPayload;

        if (ChunkLimit > OriginalTotalLength) {
            ChunkLimit = OriginalTotalLength;
        }

        // 로그 사이 경계 탐색
        while (ChunkEnd < ChunkLimit) {
            if (ChunkEnd + sizeof(uint16) > OriginalTotalLength) break;

            uint16 EntryLength;
            memcpy(&EntryLength, &PayloadPtr[ChunkEnd], sizeof(EntryLength));
            size_t NextEntry = ChunkEnd + sizeof(EntryLength) + EntryLength;

            if (NextEntry > ChunkLimit || NextEntry > OriginalTotalLength) break;
            ChunkEnd = NextEntry;
        }

        if (ChunkEnd == Offset) {
            // 단일 로그가 너무 클 때의 예외처리
            uint16 EntryLength;
            memcpy(&EntryLength, &PayloadPtr[Offset], sizeof(EntryLength));
            size_t SingleEntrySize = sizeof(EntryLength) + EntryLength;

            if (SingleEntrySize > MaxChunkPayload) {
                CFE_EVS_SendEvent(LOG_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                                  "LOG_logging failed; entry exceeds %u bytes",
                                  (unsigned int)MaxChunkPayload);
                return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
            }
            ChunkEnd = Offset + SingleEntrySize;
        }

        // 전송용 패킷 생성
        LOG_QueryTlm_t TxPkt;
        memset(&TxPkt, 0, sizeof(TxPkt));
        
        // 헤더 복사
        TxPkt.TelemetryHeader = OriginalPkt->TelemetryHeader;

        // 메타데이터 입력
        TxPkt.TotalLogCount = OriginalPkt->TotalLogCount;         // 전체 로그 개수
        TxPkt.TotalDataLength = (uint16_t)(ChunkEnd - Offset);    // 현재 조각 길이
        
        // 지상국을 위한 플래그: 마지막 패킷이면 Index에 0xFF 입력. 패킷은 255개 미만이지 않을까??
        if (OriginalTotalLength <= MaxChunkPayload) {
            TxPkt.Index = 0;                     // 단일 패킷
        } 
        else { // 분할 패킷
            if (ChunkEnd >= OriginalTotalLength) {
                TxPkt.Index = 0xFF;              // 마지막 패킷
            } else {
                TxPkt.Index = PacketIndex + 1;   // 1부터 시작
            }
        }

        // 데이터 복사
        memcpy(TxPkt.Payload, &PayloadPtr[Offset], TxPkt.TotalDataLength);

        // 사이즈 계산 & 전송
        size_t ActualPacketSize = HeaderSize + MetaSize + TxPkt.TotalDataLength;
        CFE_MSG_SetSize(CFE_MSG_PTR(TxPkt.TelemetryHeader), ActualPacketSize);
        CFE_SB_TimeStampMsg(CFE_MSG_PTR(TxPkt.TelemetryHeader));
        CFE_SB_TransmitMsg(CFE_MSG_PTR(TxPkt.TelemetryHeader), true);

        // DEBUG: 전송 패킷 정보 저장
        int dbg_fd = open("./cf/debug_tx_pkt.bin", O_CREAT | O_WRONLY | O_APPEND, 0666);
        if (dbg_fd >= 0) {
            // 구조체 전체 크기(sizeof)가 아니라, '실제 꽉 찬 데이터 크기'만큼 기록
            write(dbg_fd, &TxPkt, ActualPacketSize);
            close(dbg_fd);
        }

        // 다음 루프 준비
        Offset = ChunkEnd;
        PacketIndex++;
    }

    return CFE_SUCCESS;
}