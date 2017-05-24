/*
 * Name: serial.h
 * Project: AVR-Doper
 * Author: Christian Starkjohann <cs@obdev.at>
 * Creation Date: 2006-07-10
 * Tabsize: 4
 * Copyright: (c) 2006 by Christian Starkjohann, all rights reserved.
 * License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 * Revision: $Id: serial.h 280 2007-03-20 12:03:11Z cs $
 */

/*
General Description:
This module implements a ring buffer for receiving UART data, an interrupt
handler to fill this ring buffer with received data and an interface to
read data from the ring buffer.
*/

#ifndef __serial_h_included__
#define __serial_h_included__

#include "hardware.h"   /* for F_CPU */
#include "utils.h"      /* for uchar etc */

/* Set the buffer size so that we have at least ~ 100 bytes for the stack! */
#define SERIAL_BUFFER_SIZE  200

/* ------------------------------------------------------------------------- */
/* ------------------- simple ring buffer implementation ------------------- */
/* ------------------------------------------------------------------------- */

typedef struct ringBuffer{
    uchar   readIndex;
    uchar   writeIndex;
    uchar   buffer[SERIAL_BUFFER_SIZE];
}ringBuffer_t;

extern ringBuffer_t serialRingBuffer;

/* ------------------------------------------------------------------------- */

static inline void  ringBufferWrite(ringBuffer_t *rb, uchar c)
{
uchar   newWriteIndex = rb->writeIndex + 1;

    if(newWriteIndex >= SERIAL_BUFFER_SIZE)
        newWriteIndex = 0;
    if(newWriteIndex != rb->readIndex){ /* not yet full */
        rb->buffer[rb->writeIndex] = c;
        rb->writeIndex = newWriteIndex;
    }
}

static inline uchar ringBufferHasData(ringBuffer_t *rb)
{
    return rb->readIndex != rb->writeIndex;
}

static inline uchar ringBufferCount(ringBuffer_t *rb)
{
uchar   x = rb->writeIndex - rb->readIndex;

    if(rb->writeIndex >= rb->readIndex)
        return x;
    return x + SERIAL_BUFFER_SIZE;
}

static inline uchar ringBufferRead(ringBuffer_t *rb)
{
    uchar c = rb->buffer[rb->readIndex];
    rb->readIndex++;
    if(rb->readIndex >= SERIAL_BUFFER_SIZE)
        rb->readIndex = 0;
    return c;
}

/* ------------------------------------------------------------------------- */

static inline void  serialInit(void)
{
    UBRRL = F_CPU / (HW_DEBUG_BAUDRATE * 16L) - 1;
    UCSRB = (1 << TXEN) | (1 << RXEN) | (1 << RXCIE);   /* enable rx, tx and rx interrupt */
}

static inline void  serialPutc(uchar c)
{
    while(!(UCSRA & (1 << UDRE)));    /* wait for data register empty */
    UDR = c;
}

#endif /* __serial_h_included__ */
