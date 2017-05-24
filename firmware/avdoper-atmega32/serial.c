/*
 * Name: serial.c
 * Project: AVR-Doper
 * Author: Christian Starkjohann <cs@obdev.at>
 * Creation Date: 2006-07-10
 * Tabsize: 4
 * Copyright: (c) 2006 by Christian Starkjohann, all rights reserved.
 * License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 * Revision: $Id: serial.c 280 2007-03-20 12:03:11Z cs $
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "serial.h"
#include "hardware.h"

#if ENABLE_DEBUG_INTERFACE
ringBuffer_t    serialRingBuffer;

ISR(USART_RXC_vect)  /* runs with global interrupts disabled */
{
    uchar data = UDR;
    ringBufferWrite(&serialRingBuffer, data);
}
#endif
