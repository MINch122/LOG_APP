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
 *   Specification for the LOG command and telemetry
 *   message data types.
 *
 * @note
 *   Constants and enumerated types related to these message structures
 *   are defined in log_msgdefs.h.
 */
#ifndef DEFAULT_LOG_MSGSTRUCT_H
#define DEFAULT_LOG_MSGSTRUCT_H

/************************************************************************
 * Includes
 ************************************************************************/

#include "log_mission_cfg.h"
#include "log_msgdefs.h"
#include "cfe_msg_hdr.h"

/*************************************************************************/

#pragma pack(push, 1)

/*
** The following commands all share the "NoArgs" format
**
** They are each given their own type name matching the command name, which
** allows them to change independently in the future without changing the prototype
** of the handler function
*/
typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} LOG_NoopCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} LOG_ResetCountersCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} LOG_ProcessCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t           CommandHeader; /**< \brief Command header */
    LOG_DisplayParam_Payload_t Payload;
} LOG_DisplayParamCmd_t;

/*************************************************************************/
/*
** Type definition (Log housekeeping)
*/

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} LOG_SendHkCmd_t;

typedef struct {
    CFE_MSG_TelemetryHeader_t  TelemetryHeader; /**< \brief Telemetry header */
    LOG_HkTlm_Payload_t Payload;         /**< \brief Telemetry payload */
} LOG_HkTlm_t;



/* * * * * * * * * * * * * * * * * */
/* 민창희가 만들었어요                */
/* * * * * * * * * * * * * * * * * */

/*
** 저장되는 로그 파일 헤더
*/
typedef struct {
    uint32                      CreateTime;   // 파일 생성 시각
    uint32                       CloseTime;   // 파일 꽉 찬 시각
    uint32                       FileIndex;   // 파일 인덱스 (0, 1, 2, ...)
    uint16                       InfoCount;   // Info log 개수
    uint16                        ErrCount;   // Err log 개수
    uint16                       CritCount;   // Critical log 개수
    uint32                        FileSize;   // 파일 총 사이즈 (byte)
} LOG_FileHeader_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} LOG_EnableCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} LOG_DisableCmd_t;


typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} LOG_GetCurrentHeaderCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} LOG_GetLogCountCmd_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
    LOG_QueryTime_Payload_t Payload;
} LOG_QueryTime_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
    LOG_QueryNumber_Payload_t Payload;
} LOG_QueryNumber;


/* Telemetry struct */
typedef struct {
    CFE_MSG_TelemetryHeader_t TelemetryHeader;
    LOG_FileHeader_t HeaderData;
} LOG_HeaderTlm_t;

typedef struct {
    CFE_MSG_TelemetryHeader_t TelemetryHeader;
    uint32 ScannedFiles;
    uint32 TotalEntries;
    uint32 TotalInfo;
    uint32 TotalErr;
    uint32 TotalCrit;
} LOG_CountTlm_t;

typedef struct {
    CFE_MSG_TelemetryHeader_t TelemetryHeader;

    uint16 TotalLogCount;       // 총 로그 개수
    uint16 TotalDataLength;    // 총 데이터 길이

    uint8 Payload[LOG_MAX_DOWNLINK_PAYLOAD_SIZE];
} LOG_QueryTlm_t;


/************************************************************************
** Type Definitions
*************************************************************************/

/*
** Global Data
*/
typedef struct
{
    /*
    ** Command interface counters...
    */
    uint8 CmdCounter;
    uint8 ErrCounter;

    // LOGGING ON / OFF
    uint8 LogEnabled;

    /* TELEMETRY PARCKET*/
    
    // Housekeeping telemetry packet...
    LOG_HkTlm_t HkTlm;

    // Header telemetry packet...
    LOG_HeaderTlm_t HdrTlm;

    // Log Count Telemetry packet
    LOG_CountTlm_t CntTlm;

    /*
    ** Run Status variable used in the main processing loop
    */
    uint32 RunStatus;

    /*
    ** Operational data (not reported in housekeeping)...
    */
    CFE_SB_PipeId_t CommandPipe;
    CFE_SB_PipeId_t EventPipe;


    /*
    ** 현재 파일 관련 정보
    */
    uint32                    CurrentIndex;   // 현재 파일 번호 log000, log001 ...
    uint32                      EntryCount;   // 현재 파일에 들어간 로그 개수
    LOG_FileHeader_t         CurrentHeader;   // 현재 파일의 헤더 정보

    int                                 Fd;   // POSIX file descriptor
    uint8                          *MapPtr;   // 매핑된 메모리 pointer

    CFE_TBL_Handle_t TblHandles[LOG_PLATFORM_NUMBER_OF_TABLES];
} LOG_Data_t;

/*
** Global data structure
*/
extern LOG_Data_t LOG_Data;

#pragma pack(pop)

#endif /* LOG_MSGSTRUCT_H */