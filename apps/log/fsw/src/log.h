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

#define LOG_MAX_ENTRIES_PER_FILE 150    // 파일 하나 당 저장 로그 수

#define LOG_MASK_INFO (1 << 0)   // 001
#define LOG_MASK_ERR  (1 << 1)   // 010
#define LOG_MASK_CRIT (1 << 2)   // 100


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
