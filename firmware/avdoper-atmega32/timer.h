/*
 * Name: timer.h
 * Project: AVR-Doper
 * Author: Christian Starkjohann <cs@obdev.at>
 * Creation Date: 2006-06-26
 * Tabsize: 4
 * Copyright: (c) 2006 by Christian Starkjohann, all rights reserved.
 * License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 * Revision: $Id: timer.h 566 2008-04-26 14:21:47Z cs $
 */

/*
General Description:
This module implements functions to control timing behavior. It is based on
a timer interrupt.
*/

#ifndef __timer_h_included__
#define __timer_h_included__

#include <avr/io.h> /* for TCNT0 */
#include "utils.h"

extern volatile uchar   timerTimeoutCnt;
extern volatile uchar   timerLongTimeoutCnt;

/* ------------------------------------------------------------------------- */

#define TIMER_TICK_US       (64000000.0/(double)F_CPU)

void timerMsDelay(uchar ms);    /* ~1 ms precision, based on hardware timer */
void timerSetupTimeout(uchar msDuration);
void timerTicksDelay(uchar ticks);

#define TIMER_US_DELAY(us)  timerTicksDelay((uchar)((us)/TIMER_TICK_US))

/* ------------------------------------------------------------------------- */

static inline uchar   timerTimeoutOccurred(void)
{
    return timerTimeoutCnt == 0;
}

static inline void    timerSetupLongTimeout(uchar ms100Duration)
{
    timerLongTimeoutCnt = ms100Duration;
}

static inline uchar   timerLongTimeoutOccurred(void)
{
    return timerLongTimeoutCnt == 0;
}

/* ------------------------------------------------------------------------- */

#endif /* __timer_h_included__ */
