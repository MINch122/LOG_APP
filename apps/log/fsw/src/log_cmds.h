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
 *   This file contains the prototypes for the Log Ground Command-handling functions
 */

#ifndef LOG_CMDS_H
#define LOG_CMDS_H

/*
** Required header files.
*/
#include "cfe_error.h"
#include "log_msg.h"
#include "cfe_evs_msgstruct.h"

CFE_Status_t LOG_SendHkCmd(const LOG_SendHkCmd_t *Msg);
CFE_Status_t LOG_ResetCountersCmd(const LOG_ResetCountersCmd_t *Msg);
CFE_Status_t LOG_ProcessCmd(const LOG_ProcessCmd_t *Msg);
CFE_Status_t LOG_NoopCmd(const LOG_NoopCmd_t *Msg);
CFE_Status_t LOG_DisplayParamCmd(const LOG_DisplayParamCmd_t *Msg);
CFE_Status_t LOG_EnabledCmd(const LOG_EnableCmd_t *Msg);
CFE_Status_t LOG_DisabledCmd(const LOG_DisableCmd_t *Msg);
CFE_Status_t LOG_ProcessEventMsg(const CFE_EVS_LongEventTlm_t *Msg);
CFE_Status_t LOG_GetCurrentHeaderCmd(const LOG_GetCurrentHeaderCmd_t *Msg);
CFE_Status_t LOG_GetLogCountCmd(const LOG_GetLogCountCmd_t *Msg);

#endif /* LOG_CMDS_H */
