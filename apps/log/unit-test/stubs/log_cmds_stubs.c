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
 * Auto-Generated stub implementations for functions defined in log_cmds header
 */

#include "log_cmds.h"
#include "utgenstub.h"

/*
 * ----------------------------------------------------
 * Generated stub function for LOG_DisplayParamCmd()
 * ----------------------------------------------------
 */
CFE_Status_t LOG_DisplayParamCmd(const LOG_DisplayParamCmd_t *Msg)
{
    UT_GenStub_SetupReturnBuffer(LOG_DisplayParamCmd, CFE_Status_t);

    UT_GenStub_AddParam(LOG_DisplayParamCmd, const LOG_DisplayParamCmd_t *, Msg);

    UT_GenStub_Execute(LOG_DisplayParamCmd, Basic, NULL);

    return UT_GenStub_GetReturnValue(LOG_DisplayParamCmd, CFE_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for LOG_NoopCmd()
 * ----------------------------------------------------
 */
CFE_Status_t LOG_NoopCmd(const LOG_NoopCmd_t *Msg)
{
    UT_GenStub_SetupReturnBuffer(LOG_NoopCmd, CFE_Status_t);

    UT_GenStub_AddParam(LOG_NoopCmd, const LOG_NoopCmd_t *, Msg);

    UT_GenStub_Execute(LOG_NoopCmd, Basic, NULL);

    return UT_GenStub_GetReturnValue(LOG_NoopCmd, CFE_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for LOG_ProcessCmd()
 * ----------------------------------------------------
 */
CFE_Status_t LOG_ProcessCmd(const LOG_ProcessCmd_t *Msg)
{
    UT_GenStub_SetupReturnBuffer(LOG_ProcessCmd, CFE_Status_t);

    UT_GenStub_AddParam(LOG_ProcessCmd, const LOG_ProcessCmd_t *, Msg);

    UT_GenStub_Execute(LOG_ProcessCmd, Basic, NULL);

    return UT_GenStub_GetReturnValue(LOG_ProcessCmd, CFE_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for LOG_ResetCountersCmd()
 * ----------------------------------------------------
 */
CFE_Status_t LOG_ResetCountersCmd(const LOG_ResetCountersCmd_t *Msg)
{
    UT_GenStub_SetupReturnBuffer(LOG_ResetCountersCmd, CFE_Status_t);

    UT_GenStub_AddParam(LOG_ResetCountersCmd, const LOG_ResetCountersCmd_t *, Msg);

    UT_GenStub_Execute(LOG_ResetCountersCmd, Basic, NULL);

    return UT_GenStub_GetReturnValue(LOG_ResetCountersCmd, CFE_Status_t);
}

/*
 * ----------------------------------------------------
 * Generated stub function for LOG_SendHkCmd()
 * ----------------------------------------------------
 */
CFE_Status_t LOG_SendHkCmd(const LOG_SendHkCmd_t *Msg)
{
    UT_GenStub_SetupReturnBuffer(LOG_SendHkCmd, CFE_Status_t);

    UT_GenStub_AddParam(LOG_SendHkCmd, const LOG_SendHkCmd_t *, Msg);

    UT_GenStub_Execute(LOG_SendHkCmd, Basic, NULL);

    return UT_GenStub_GetReturnValue(LOG_SendHkCmd, CFE_Status_t);
}
