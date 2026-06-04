#include "event_maker_simulate.h"
#include "event_maker.h"
#include "event_maker_utils.h"
#include "event_maker_tbl.h"
#include "event_maker_eventids.h"

#include <stdio.h>

typedef enum {
    EVENT_EMPTY     = 0,
    EVENT_ACTIVE    = 1,
    EVENT_ONCE      = 2,
    EVENT_INACTIVE  = 3,
} SimulatedEventEntryState_t;

typedef struct {
    uint8_t EntryState;
    uint32  NextTicks;
    uint32  Ticks;
    uint32  Period;
    uint32  Counter;
} SimulatedEventStateEntry_t;

typedef struct {
    osal_id_t TimerId;
    osal_id_t EventSem;
    uint32    TimerAccuracy;
    SimulatedEventStateEntry_t Entry[EVENT_MAKER_PLATFORM_SIMULATED_EVENT_ENTRIES];
} SimulatedEventState_t;

static SimulatedEventState_t EventState;

// static void debug_print_event_states(void)
// {
//     for (int i = 0; i < EVENT_MAKER_PLATFORM_SIMULATED_EVENT_ENTRIES; ++i) {
//         if (EventState.Entry[i].EntryState != EVENT_EMPTY)
//         printf("Event %d: State %d, NextTicks %u, Ticks %u, Period %u, Counter %u\n", i, EventState.Entry[i].EntryState,
//                EventState.Entry[i].NextTicks, EventState.Entry[i].Ticks, EventState.Entry[i].Period, EventState.Entry[i].Counter);
//     }
// }

static bool isValidEventType(uint8_t EventType) {
    return (EventType == CFE_EVS_EventType_DEBUG) ||
           (EventType == CFE_EVS_EventType_INFORMATION) ||
           (EventType == CFE_EVS_EventType_ERROR) ||
           (EventType == CFE_EVS_EventType_CRITICAL);
}

static void SimulatedEventTimerCallback(osal_id_t TimerId)
{
    OS_BinSemGive(EventState.EventSem);
}

static uint32 RandomPeriodAdder(uint16 Deviation, uint32 NextTicks)
{
    int32 RandomAdder = EVENT_MAKER_ZeroCenteredNormal(Deviation);
    if (RandomAdder < 0 && (uint32)(-RandomAdder) > NextTicks) {
        return 0;
    }
    else {
        return NextTicks + RandomAdder;
    }
}

int32 EVENT_MAKER_InitSimulatedEvents(void)
{
    int32 Status;
    EVENT_MAKER_SimulatedEventEntry_t* EventEntry;

    Status = CFE_TBL_GetAddress((void**)&EventEntry, EVENT_MAKER_Data.TblHandle);
    if (Status != CFE_SUCCESS && Status != CFE_TBL_INFO_UPDATED) {
        CFE_EVS_SendEvent(EVENT_MAKER_TABLE_ACCESS_ERR_EID, CFE_EVS_EventType_ERROR, "Event Maker: Error accessing Simulated Event Table, RC = 0x%08lX", (unsigned long)Status);
        return Status;
    }

    memset(&EventState, 0, sizeof(EventState));

    Status = OS_BinSemCreate(&EventState.EventSem, "EventSimSem", 0, 0);
    if (Status != OS_SUCCESS) {
        CFE_EVS_SendEvent(EVENT_MAKER_SEM_ERR_EID, CFE_EVS_EventType_ERROR, "Event Maker: Error creating binary semaphore, RC = 0x%08lX", (unsigned long)Status);
        return Status;
    }

    Status = OS_TimerCreate(&EventState.TimerId, "EVT_SIM", &EventState.TimerAccuracy, SimulatedEventTimerCallback);
    if (Status != OS_SUCCESS) {
        CFE_EVS_SendEvent(EVENT_MAKER_TIMER_ERR_EID, CFE_EVS_EventType_ERROR, "Event Maker: Error creating timer, RC = 0x%08lX", (unsigned long)Status);
        return Status;
    }

    CFE_EVS_SendEvent(EVENT_MAKER_TIMER_CREATED_INF_EID, CFE_EVS_EventType_INFORMATION, "Event Maker: Timer accuracy is %u us", (unsigned int)EventState.TimerAccuracy);

    for (int i = 0; i < EVENT_MAKER_PLATFORM_SIMULATED_EVENT_ENTRIES; i++) {
        if (isValidEventType(EventEntry[i].EventType) && EventEntry[i].EventID > 0) {
            EventState.Entry[i].EntryState = EventEntry[i].Period > 0 ? EVENT_ACTIVE : EVENT_ONCE;
            EventState.Entry[i].Period = EventEntry[i].Period;
            if (EventState.Entry[i].EntryState == EVENT_ACTIVE && EventState.Entry[i].Period < EventState.TimerAccuracy / 1000) {
                CFE_EVS_SendEvent(EVENT_MAKER_TIMER_ERR_EID, CFE_EVS_EventType_INFORMATION, "Event Maker: Simulated event %d has a period less than the timer accuracy", i);
                EventState.Entry[i].Period = EventState.TimerAccuracy;
            }
            EventState.Entry[i].NextTicks = EventEntry[i].Offset + EventState.Entry[i].Period;
            EventState.Entry[i].NextTicks *= EVENT_MAKER_PLATFORM_SIMULATED_EVENT_TICK_PER_SEC;
            EventState.Entry[i].NextTicks /= 1000;
        }
    }

    CFE_TBL_ReleaseAddress(EVENT_MAKER_Data.TblHandle);

    Status = CFE_ES_CreateChildTask(&EVENT_MAKER_Data.SimTaskId,
                                    "EventSimTask",
                                    EVENT_MAKER_SimulateEvents,
                                    NULL,
                                    CFE_PLATFORM_ES_DEFAULT_STACK_SIZE,
                                    20, 0);
    if (Status != CFE_SUCCESS) {
        CFE_EVS_SendEvent(EVENT_MAKER_CHILD_CREATE_ERR_EID, CFE_EVS_EventType_ERROR, "Event Maker: Error creating child task, RC = 0x%08lX", (unsigned long)Status);
        return Status;
    }

    Status = OS_TimerSet(EventState.TimerId, 1000000/EVENT_MAKER_PLATFORM_SIMULATED_EVENT_TICK_PER_SEC,
                                             1000000/EVENT_MAKER_PLATFORM_SIMULATED_EVENT_TICK_PER_SEC);
    if (Status != OS_SUCCESS) {
        CFE_EVS_SendEvent(EVENT_MAKER_TIMER_ERR_EID, CFE_EVS_EventType_ERROR, "Event Maker: Error setting timer, RC = 0x%08lX", (unsigned long)Status);
        return Status;
    }

    return Status;
}

void EVENT_MAKER_SimulateEvents(void)
{
    int32                          Status;
    EVENT_MAKER_SimulatedEventEntry_t* EventEntry;

    Status = CFE_TBL_GetAddress((void**)&EventEntry, EVENT_MAKER_Data.TblHandle);
    if (Status != CFE_SUCCESS) {
        CFE_EVS_SendEvent(EVENT_MAKER_TABLE_ACCESS_ERR_EID, CFE_EVS_EventType_ERROR, "Event Maker: Error accessing Simulated Event Table, RC = 0x%08lX", (unsigned long)Status);
        return;
    }

    while (1) {
        Status = OS_BinSemTake(EventState.EventSem);
        if (Status != OS_SUCCESS) {
            CFE_EVS_SendEvent(EVENT_MAKER_SEM_ERR_EID, CFE_EVS_EventType_ERROR, "Event Maker: Error taking binary semaphore, RC = 0x%08lX", (unsigned long)Status);
            return;
        }

        if (EVENT_MAKER_Data.event_enable == 0) {
            OS_printf("[EVENT_MAKER] Disabled.\n");
            break;
        }

        /* Process each entry in the table */
        for (int i = 0; i < EVENT_MAKER_PLATFORM_SIMULATED_EVENT_ENTRIES; ++i) {

            if (EventState.Entry[i].EntryState == EVENT_ACTIVE || EventState.Entry[i].EntryState == EVENT_ONCE) {

                if (EventState.Entry[i].Ticks++ < EventState.Entry[i].NextTicks)
                    /* Not ready yet. */
                    continue;
                
                if (EventEntry[i].EveryNth == 0 ||
                        EventState.Entry[i].Counter++ % EventEntry[i].EveryNth == 0) {
                    /* Fire the event. */
                    CFE_EVS_SendEvent(EventEntry[i].EventID, EventEntry[i].EventType, "%s", EventEntry[i].Message);
                }

                EventState.Entry[i].Ticks = 0;

                if (EventState.Entry[i].EntryState == EVENT_ONCE) {
                    /* One-time event. Done. */
                    EventState.Entry[i].EntryState = EVENT_INACTIVE;
                    EventState.Entry[i].NextTicks = 0;
                }
                else {
                    /* Update the next ticks. */
                    EventState.Entry[i].NextTicks = RandomPeriodAdder(EventEntry[i].PeriodDeviationMs, EventState.Entry[i].Period);
                    EventState.Entry[i].NextTicks = EventState.Entry[i].NextTicks * EVENT_MAKER_PLATFORM_SIMULATED_EVENT_TICK_PER_SEC / 1000;
                }
            }
        }
    }
    /* Must never reach here. */
    return;
}
