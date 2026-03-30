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
 *   This file contains the source code for the Event Maker utility functions
 */

/*
** Include Files:
*/
#include "event_maker.h"
#include "event_maker_eventids.h"
#include "event_maker_tbl.h"
#include "event_maker_utils.h"

#include <stdlib.h>

/* approximated normal distribution [-std, std] */
int32 EVENT_MAKER_ZeroCenteredNormal(uint16 Deviation)
{
    if (Deviation == 0)
        return 0;

    int32 Random = 0;
    for (int i = 0; i < 12; ++i)
        Random += rand() % (Deviation * 2 + 1) - Deviation;
    Random /= 2;

    // clamp
    if (Random > (int32)Deviation)
        Random = Deviation;
    else if (Random < -(int32)Deviation)
        Random = -Deviation;

    return Random;
}

