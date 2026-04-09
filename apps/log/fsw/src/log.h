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
 * Main header file for the Log application
 */

#ifndef LOG_H
#define LOG_H

/*
** Required header files.
*/
#include "cfe.h"
#include "cfe_config.h"

#include "log_mission_cfg.h"
#include "log_platform_cfg.h"

#include "log_perfids.h"
#include "log_msgids.h"
#include "log_msg.h"

#include "cfe_evs_msgids.h"
#include "cfe_evs_msgdefs.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define LOG_MAX_ENTRIES_PER_FILE 1000    // 파일 하나 당 저장 로그 수
#define LOG_SYNC_INTERVAL          50    // 로그 몇 개마다 sync를 쓸지


/************************************************************************
** Type Definitions
*************************************************************************/

/*
** 저장되는 로그 파일 헤더
*/
typedef struct
{
    uint32                      CreateTime;   // 파일 생성 시각
    uint32                       CloseTime;   // 파일 꽉 찬 시각
    uint32                       FileIndex;   // 파일 인덱스 (0, 1, 2, ...)
    uint16                       InfoCount;   // Info log 개수
    uint16                        ErrCount;   // Err log 개수
    uint16                       CritCount;   // Critical log 개수
    uint32                        FileSize;   // 파일 총 사이즈 (byte)
} LOG_FileHeader_t;


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

    /*
    ** Housekeeping telemetry packet...
    */
    LOG_HkTlm_t HkTlm;

    /*
    ** Run Status variable used in the main processing loop
    */
    uint32 RunStatus;

    /*
    ** Operational data (not reported in housekeeping)...
    */
    CFE_SB_PipeId_t CommandPipe;


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


/****************************************************************************/
/*
** Local function prototypes.
**
** Note: Except for the entry point (LOG_Main), these
**       functions are not called from any other source module.
*/
void         LOG_Main(void);
CFE_Status_t LOG_Init(void);

#endif /* LOG_H */
