/*
 * Name: timer.c
 * Project: AVR-Doper
 * Author: Christian Starkjohann <cs@obdev.at>
 * Creation Date: 2006-06-26
 * Tabsize: 4
 * Copyright: (c) 2006 by Christian Starkjohann, all rights reserved.
 * License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 * Revision: $Id: timer.c 566 2008-04-26 14:21:47Z cs $
 */

#include "hardware.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include "timer.h"

volatile uchar  timerTimeoutCnt;
volatile uchar  timerLongTimeoutCnt;

/* ------------------------------------------------------------------------- */

void timerTicksDelay(uchar ticks)
{
char    d, until;

    // cli();
#ifdef SFIOR
    SFIOR = _BV(PSR10);     /* reset prescaler */
#else
    GTCCR = _BV(PSRSYNC);
#endif
    until = TCNT0 + ticks;
    // sei();
    do{
        d = TCNT0 - until;  /* enforce 8 bit arithmetics */
    }while(d < 0);
}

/* ------------------------------------------------------------------------- */

void timerMsDelay(uchar ms)
{
  uint16_t i;
  for(i = 0; i < 8* ((uint16_t) ms); i++)
  {
    TIMER_US_DELAY(125);
  }
}


/* ------------------------------------------------------------------------- */

/* main configures Timer 0 with 1/64 prescaler
 * -> 12MHz/64 = 187.5kHz --> 5.333us
 * overflow every 256 counts = 732.422Hz --> 1.365ms
 */
ISR(TIMER0_OVF_vect)   /* should run with global interrupt enabled */
{
   static uchar    prescaler = 1;
   if(timerTimeoutCnt != 0)
        timerTimeoutCnt--;
   if(--prescaler == 0){   /* scale down to 10 Hz */
        prescaler = (uchar)((double)F_CPU / 64 / 256 / 10 + 0.5);
        if(timerLongTimeoutCnt != 0)
            timerLongTimeoutCnt--;
   }
}

/* ------------------------------------------------------------------------- */

void    timerSetupTimeout(uchar msDuration)
{
/* We can either have 12 Mhz clock or roughly 16 MHz (15/16/16.5). The
 * following is precise enough.
 */
#if F_CPU < 14000000
    msDuration -= msDuration >> 2;  /* approximate milliseconds with our timebase */
    /* 1 - 1/4 = 0.75; 1.365 ms * 0.75 = 1.024 ms --> 2.4% deviation */
#endif
    msDuration++;   /* Our resolution is 1.4 ms. Add one unit to avoid almost zero delays for low values */
    timerTimeoutCnt = msDuration;
}

/* ------------------------------------------------------------------------- */
