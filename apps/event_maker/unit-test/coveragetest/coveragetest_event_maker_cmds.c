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

/*
** File: coveragetest_event_maker.c
**
** Purpose:
** Coverage Unit Test cases for the EVENT_MAKER Application
**
** Notes:
** This implements various test cases to exercise all code
** paths through all functions defined in the EVENT_MAKER application.
**
** It is primarily focused at providing examples of the various
** stub configurations, hook functions, and wrapper calls that
** are often needed when coercing certain code paths through
** complex functions.
*/

/*
 * Includes
 */

#include "event_maker_coveragetest_common.h"
#include "event_maker.h"
#include "event_maker_dispatch.h"
#include "event_maker_cmds.h"

/*
**********************************************************************************
**          TEST CASE FUNCTIONS
**********************************************************************************
*/

void Test_EVENT_MAKER_ReportHousekeeping(void)
{
    /*
     * Test Case For:
     * void EVENT_MAKER_ReportHousekeeping( const CFE_SB_CmdHdr_t *Msg )
     */
    CFE_MSG_Message_t *MsgSend;
    CFE_MSG_Message_t *MsgTimestamp;

    /* Set up to capture send message address */
    UT_SetDataBuffer(UT_KEY(CFE_SB_TransmitMsg), &MsgSend, sizeof(MsgSend), false);

    /* Set up to capture timestamp message address */
    UT_SetDataBuffer(UT_KEY(CFE_SB_TimeStampMsg), &MsgTimestamp, sizeof(MsgTimestamp), false);

    /* Call unit under test, NULL pointer confirms command access is through APIs */
    EVENT_MAKER_SendHkCmd(NULL);

    /* Confirm message sent*/
    UtAssert_STUB_COUNT(CFE_SB_TransmitMsg, 1);
    UtAssert_ADDRESS_EQ(MsgSend, &EVENT_MAKER_Data.HkTlm);

    /* Confirm timestamp msg address */
    UtAssert_STUB_COUNT(CFE_SB_TimeStampMsg, 1);
    UtAssert_ADDRESS_EQ(MsgTimestamp, &EVENT_MAKER_Data.HkTlm);

    /*
     * Confirm that the CFE_TBL_Manage() call was done
     */
    UtAssert_STUB_COUNT(CFE_TBL_Manage, 1);
}

void Test_EVENT_MAKER_NoopCmd(void)
{
    /*
     * Test Case For:
     * void EVENT_MAKER_NoopCmd( const EVENT_MAKER_Noop_t *Msg )
     */
    EVENT_MAKER_NoopCmd_t TestMsg;
    UT_CheckEvent_t      EventTest;

    memset(&TestMsg, 0, sizeof(TestMsg));

    /* test dispatch of NOOP */
    UT_CHECKEVENT_SETUP(&EventTest, EVENT_MAKER_NOOP_INF_EID, NULL);

    UtAssert_INT32_EQ(EVENT_MAKER_NoopCmd(&TestMsg), CFE_SUCCESS);

    /*
     * Confirm that the event was generated
     */
    UtAssert_UINT32_EQ(EventTest.MatchCount, 1);
}

void Test_EVENT_MAKER_ResetCountersCmd(void)
{
    /*
     * Test Case For:
     * void EVENT_MAKER_ResetCounters( const EVENT_MAKER_ResetCounters_t *Msg )
     */
    EVENT_MAKER_ResetCountersCmd_t TestMsg;
    UT_CheckEvent_t               EventTest;

    memset(&TestMsg, 0, sizeof(TestMsg));

    UT_CHECKEVENT_SETUP(&EventTest, EVENT_MAKER_RESET_INF_EID, "EVENT_MAKER: RESET command");

    UtAssert_INT32_EQ(EVENT_MAKER_ResetCountersCmd(&TestMsg), CFE_SUCCESS);

    /*
     * Confirm that the event was generated
     */
    UtAssert_UINT32_EQ(EventTest.MatchCount, 1);
}

void Test_EVENT_MAKER_ProcessCmd(void)
{
    /*
     * Test Case For:
     * void  EVENT_MAKER_ProcessCmd( const EVENT_MAKER_ProcessCmd_t *Msg )
     */
    EVENT_MAKER_ProcessCmd_t   TestMsg;
    EVENT_MAKER_ExampleTable_t TestTblData;
    void *                    TblPtr = &TestTblData;

    memset(&TestTblData, 0, sizeof(TestTblData));
    memset(&TestMsg, 0, sizeof(TestMsg));

    /* Provide some table data for the EVENT_MAKER_Process() function to use */
    TestTblData.Int1 = 40;
    TestTblData.Int2 = 50;
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblPtr, sizeof(TblPtr), false);
    UtAssert_INT32_EQ(EVENT_MAKER_ProcessCmd(&TestMsg), CFE_SUCCESS);

    /*
     * This only needs to account for the call to CFE_ES_WriteToSysLog() directly
     * invoked by the unit under test.   Note that in this build environment, the
     * EVENT_MAKER_GetCrc() function is a stub.
     */
    UtAssert_STUB_COUNT(CFE_ES_WriteToSysLog, 1);

    /*
     * Confirm that the CFE_TBL_GetAddress() call was done
     */
    UtAssert_STUB_COUNT(CFE_TBL_GetAddress, 1);

    /*
     */
    /*
     * Configure the CFE_TBL_GetAddress function to return an error.
     * Exercise the error return path.
     * Error at this point should add only one additional call to
     * CFE_ES_WriteToSysLog() through the CFE_TBL_GetAddress() error path.
     */
    UT_SetDefaultReturnValue(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_ERR_UNREGISTERED);
    UtAssert_INT32_EQ(EVENT_MAKER_ProcessCmd(&TestMsg), CFE_TBL_ERR_UNREGISTERED);
    UtAssert_STUB_COUNT(CFE_ES_WriteToSysLog, 2);

    /*
     * Configure CFE_TBL_ReleaseAddress() to return an error, exercising the
     * error return path.
     * Confirm two additional calls to CFE_ES_WriteToSysLog() - one
     * reporting the table values, and one through the CFE_TBL_ReleaseAddress()
     * error path.
     */
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblPtr, sizeof(TblPtr), false);
    UT_SetDefaultReturnValue(UT_KEY(CFE_TBL_GetAddress), CFE_SUCCESS);
    UT_SetDefaultReturnValue(UT_KEY(CFE_TBL_ReleaseAddress), CFE_TBL_ERR_NO_ACCESS);
    UtAssert_INT32_EQ(EVENT_MAKER_ProcessCmd(&TestMsg), CFE_TBL_ERR_NO_ACCESS);
    UtAssert_STUB_COUNT(CFE_ES_WriteToSysLog, 4);
}

void Test_EVENT_MAKER_DisplayParamCmd(void)
{
    /*
     * Test Case For:
     * void  EVENT_MAKER_DisplayParamCmd( const EVENT_MAKER_DisplayParamCmd_t *Msg )
     */
    EVENT_MAKER_DisplayParamCmd_t TestMsg;
    UT_CheckEvent_t              EventTest;

    memset(&TestMsg, 0, sizeof(TestMsg));

    UT_CHECKEVENT_SETUP(&EventTest, EVENT_MAKER_VALUE_INF_EID, "EVENT_MAKER: ValU32=%lu, ValI16=%d, ValStr=%s");
    TestMsg.Payload.ValU32 = 10;
    TestMsg.Payload.ValI16 = -4;
    snprintf(TestMsg.Payload.ValStr, sizeof(TestMsg.Payload.ValStr), "Hello");

    UtAssert_INT32_EQ(EVENT_MAKER_DisplayParamCmd(&TestMsg), CFE_SUCCESS);
    /*
     * Confirm that the event was generated
     */
    UtAssert_UINT32_EQ(EventTest.MatchCount, 1);
}

/*
 * Register the test cases to execute with the unit test tool
 */
void UtTest_Setup(void)
{
    ADD_TEST(EVENT_MAKER_ReportHousekeeping);
    ADD_TEST(EVENT_MAKER_NoopCmd);
    ADD_TEST(EVENT_MAKER_ResetCountersCmd);
    ADD_TEST(EVENT_MAKER_ProcessCmd);
    ADD_TEST(EVENT_MAKER_DisplayParamCmd);
}
