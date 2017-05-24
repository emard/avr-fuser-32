/*
 * Name: vreg.c
 * Project: AVR-Doper
 * Author: Christian Starkjohann <cs@obdev.at>
 * Creation Date: 2006-07-04
 * Tabsize: 4
 * Copyright: (c) 2006 by Christian Starkjohann, all rights reserved.
 * License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 * Revision: $Id: vreg.c 566 2008-04-26 14:21:47Z cs $
 */

#include "hardware.h"

#if ENABLE_HVPROG

#include <avr/io.h>
#include <avr/interrupt.h>
#include "vreg.h"
#include "stk500protocol.h" /* for stkParam */

#if HVPP_HARDWARE || FUSER_HARDWARE
/* hvpp hardware */

void    vregInit(void)
{
}

#else
/* avr-doper hardware */
#define VREG_REF        (uint)((2.06 / 2.56) * 1024)

#define CHANNEL_VREG    0
#define CHANNEL_TARGET  1

#define REFERENCE_BITS  UTIL_BIN8(1100, 0000)

#define PWM_RANGE   (1 << 9)
#define PWM_CENTER  (PWM_RANGE * 7 / 12)

#define DIVIDER (uchar)(22 * 1024 / ((33 + 22) * 25.6)) /* is coincidentally == 16 */
/* The divider above normalizes the ADC value to 10 * vTarget */

UTIL_INTERRUPT(ADC_vect) /* run with global interrupt enable */
{
uint    value = ADC;
uchar   muxVal = ADMUX & 0xf;

    if(muxVal == CHANNEL_VREG){
        int x = PWM_CENTER + ((VREG_REF - value) * 32);
        if(x <= 0){
            x = 1;
        }else if(x > PWM_RANGE - (PWM_RANGE / 5)){
            x = PWM_RANGE - (PWM_RANGE / 5);
        }
        OCR1A = x;
        ADMUX = REFERENCE_BITS | CHANNEL_TARGET;
    }else{  /* must be CHANNEL_TARGET */
        /* Since divider is coincidentally a power of 2, do the division here
         * in the interrupt. Otherwise create an accessor function which is
         * called when reading the parameter.
         */
        stkParam.s.vTarget = (value + DIVIDER/2) / DIVIDER;
        ADMUX = REFERENCE_BITS | CHANNEL_VREG;
    }
    ADCSRA |= 1 << ADSC;    /* start next conversion */    
}

void    vregInit(void)
{
#if 1
    ADMUX = REFERENCE_BITS | CHANNEL_VREG;
    ADCSRA = UTIL_BIN8(1000, 1101); /* enable ADC, enable interrupt, prescaler=32 */
    ADCSRA |= 1 << ADSC;    /* start conversion */
#endif
}

#endif /* end avr-doper hardware */

#else /* ENABLE_HVPROG */

void    vregInit(void)
{
}

#endif /* ENABLE_HVPROG */
