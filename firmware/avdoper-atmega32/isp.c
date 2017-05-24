/*
 * Name: isp.c
 * Project: AVR-Doper
 * Author: Christian Starkjohann <cs@obdev.at>
 * Creation Date: 2006-06-21
 * Tabsize: 4
 * Copyright: (c) 2006 by Christian Starkjohann, all rights reserved.
 * License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 * Revision: $Id: isp.c 566 2008-04-26 14:21:47Z cs $
 */

#include "hardware.h"
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include "utils.h"
#include <util/delay.h>
#include "isp.h"
#include "timer.h"
#include "oddebug.h"

static uchar    ispClockDelay;
static uchar    cmdBuffer[4];

#define CLI()
#define SEI()

/* ------------------------------------------------------------------------- */
/* We disable interrupts while transfer a byte. This ensures that we execute
 * at nominal speed, in spite of aggressive USB polling.
 */
static uchar ispBlockTransfer(uchar *block, uchar len)
{
uchar   cnt, shift = 0, port, delay = ispClockDelay;

/* minimum clock pulse width:
 * 5 + 4 * delay clock cycles           -> Tmin = 750 ns
 * total clock period: 12 + 8 * delay   -> fmax = 600 kHz
 */
/*    DBG2(0x40, block, len); */
    CLI();
    port = PORT_OUT(HWPIN_ISP_MOSI) & ~(1 << PORT_BIT(HWPIN_ISP_MOSI));
    while(len--){   /* len may be 0 */
        cnt = 8;
        shift = *block++;
        do{
            if(shift & 0x80){
                port |= (1 << PORT_BIT(HWPIN_ISP_MOSI));
            }
            PORT_OUT(HWPIN_ISP_MOSI) = port;
            SEI();
            timerTicksDelay(delay);
            CLI();
            PORT_PIN_SET(HWPIN_ISP_SCK);    /* <-- data clocked by device */
            shift <<= 1;
            port &= ~(1 << PORT_BIT(HWPIN_ISP_MOSI));
#if METABOARD_HARDWARE
            if(PORT_PIN_VALUE(HWPIN_ISP_MISO))  /* no driver in this hardware */
                shift |= 1;
#else /* METABOARD_HARDWARE */
#ifdef HWPIN_ISP_DRIVER
            if(!PORT_PIN_VALUE(HWPIN_ISP_MISO)) /* driver is inverting */
                shift |= 1;
#else
            if(PORT_PIN_VALUE(HWPIN_ISP_MISO))  /* no driver in this hardware */
                shift |= 1;
#endif
#endif /* METABOARD_HARDWARE */
            SEI();
            timerTicksDelay(delay);
            CLI();
            PORT_PIN_CLR(HWPIN_ISP_SCK);    /* <-- device changes data */
        }while(--cnt);
    }
    SEI();
/*    DBG2(0x41, &shift, 1); */
    return shift;
}

/* ------------------------------------------------------------------------- */

static void ispAttachToDevice(uchar stk500Delay, uchar stabDelay)
{
#if !METABOARD_HARDWARE /* on metaboard, we use the jumper to select CDC vs. HID mode */
    if(!PORT_PIN_VALUE(HWPIN_JUMPER)){      /* Jumper is set -> request clock below 8 kHz */
        ispClockDelay = (uchar)(70/TIMER_TICK_US);   /* 140 us -> 7.14 kHz clock rate */
    }else
#endif
    if(stk500Delay == 0){ /* 1.8 MHz nominal */
        ispClockDelay = 0;
    }else if(stk500Delay == 1){ /* 460 kHz nominal */
        ispClockDelay = 0;
    }else if(stk500Delay == 2){ /* 115 kHz nominal */
        ispClockDelay = 1;
    }else if(stk500Delay == 3){ /* 58 kHz nominal */
        ispClockDelay = 2;
    }else{
        /* from STK500v2 spec: stk500Delay = 1/(24 * SCK / 7.37 MHz) - 10/12
         * definition of ispClockDelay = 1 + 1/(SCK/MHz * 2 * TIMER_TICK_US)
         * ispClockDelay = 1 + (stk500Delay + 10/12) * 12 / (TIMER_TICK_US * 7.37)
         */
#if F_CPU > 14000000L    /* ~ 16 MHz */
        ispClockDelay = (stk500Delay + 1)/2 - (stk500Delay + 1)/8 + (stk500Delay + 1)/32;
#else   /* must be 12 MHz */
        ispClockDelay = (stk500Delay + 1)/4 + (stk500Delay + 1)/16;
#endif
    }
#if METABOARD_HARDWARE
    /* turn on power for programming sockets: */
    PORT_OUT(HWPIN_ISP_SUPPLY1) |= _BV(PORT_BIT(HWPIN_ISP_SUPPLY1)) | _BV(PORT_BIT(HWPIN_ISP_SUPPLY2));
#ifdef HWPIN_ISP_CLK
    PORT_DDR_SET(HWPIN_ISP_CLK);    /* turn on optional clock for programming socket */
#endif
    PORT_DDR_SET(HWPIN_ISP_SCK);    /* enable programming I/O pins */
    PORT_DDR_SET(HWPIN_ISP_MOSI);
    timerMsDelay(200);              /* allow device to power up */
    PORT_DDR_SET(HWPIN_ISP_RESET);
    timerMsDelay(stabDelay);
    timerTicksDelay(ispClockDelay); /* stabDelay may have been 0 */
    PORT_PIN_SET(HWPIN_ISP_RESET);  /* give positive RESET pulse */
    timerTicksDelay(ispClockDelay);
    PORT_PIN_CLR(HWPIN_ISP_RESET);  /* bring RESET back low */
#else /* METABOARD_HARDWARE */
    /* setup initial condition: SCK, MOSI = 0 */
    PORT_OUT(HWPIN_ISP_SCK) &= ~((1 << PORT_BIT(HWPIN_ISP_SCK)) | (1 << PORT_BIT(HWPIN_ISP_MOSI)));
#ifdef HWPIN_ISP_DRIVER
    PORT_PIN_SET(HWPIN_ISP_RESET);  /* set RESET */
    PORT_DDR_CLR(HWPIN_ISP_DRIVER); /* make input: use internal pullup to control driver */
    PORT_PIN_SET(HWPIN_ISP_DRIVER); /* attach to device: */
    TCCR2 |= (1 << COM20);  /* set toggle on compare match mode -> activate clock */
    timerMsDelay(stabDelay);
    timerTicksDelay(ispClockDelay); /* stabDelay may have been 0 */
    /* We now need to give a positive pulse on RESET since we can't guarantee
     * that SCK was low during power up (according to instructions in Atmel's
     * data sheets).
     */
    PORT_PIN_CLR(HWPIN_ISP_RESET);  /* give a positive RESET pulse */
    timerTicksDelay(ispClockDelay);
    PORT_PIN_SET(HWPIN_ISP_RESET);
#else
    /* connected directly without driver */
    PORT_DDR_SET(HWPIN_ISP_SCK);    /* enable programming I/O pins */
    PORT_DDR_SET(HWPIN_ISP_MOSI);
    timerMsDelay(200);              /* allow device to power up */
    PORT_DDR_SET(HWPIN_ISP_RESET);
    timerMsDelay(stabDelay);
    timerTicksDelay(ispClockDelay); /* stabDelay may have been 0 */
    PORT_PIN_SET(HWPIN_ISP_RESET);  /* give positive RESET pulse */
    timerTicksDelay(ispClockDelay);
    PORT_PIN_CLR(HWPIN_ISP_RESET);  /* bring RESET back low */
#endif
#endif /* METABOARD_HARDWARE */
}

static void ispDetachFromDevice(uchar removeResetDelay)
{
#if METABOARD_HARDWARE
    PORT_DDR_CLR(HWPIN_ISP_SCK);
    PORT_DDR_CLR(HWPIN_ISP_MOSI);
#ifdef HWPIN_ISP_CLK
    PORT_DDR_CLR(HWPIN_ISP_CLK);    /* turn off optional clock for programming socket */
#endif
    PORT_DDR_CLR(HWPIN_ISP_RESET);
    PORT_PIN_CLR(HWPIN_ISP_SCK);    /* ensure we have no pull-ups active */
    PORT_PIN_CLR(HWPIN_ISP_MOSI);
    /* turn off optional power supply for programming socket: */
    PORT_OUT(HWPIN_ISP_SUPPLY1) &= ~(_BV(PORT_BIT(HWPIN_ISP_SUPPLY1)) | _BV(PORT_BIT(HWPIN_ISP_SUPPLY2)));
#else /* METABOARD_HARDWARE */
    PORT_OUT(HWPIN_ISP_SCK) &= ~((1 << PORT_BIT(HWPIN_ISP_SCK)) | (1 << PORT_BIT(HWPIN_ISP_MOSI)));
#ifdef HWPIN_ISP_DRIVER
    PORT_PIN_SET(HWPIN_ISP_RESET);
    timerMsDelay(removeResetDelay);
    TCCR2 &= ~(1 << COM20);  /* clear toggle on compare match mode */
    PORT_PIN_CLR(HWPIN_ISP_DRIVER); /* detach from device */
    PORT_DDR_SET(HWPIN_ISP_DRIVER); /* set pin level to low-Z 0 */
    PORT_PIN_CLR(HWPIN_ISP_RESET);
#else
    PORT_DDR_CLR(HWPIN_ISP_RESET);
    PORT_PIN_CLR(HWPIN_ISP_SCK);    /* ensure we have no pull-ups active */
    PORT_PIN_CLR(HWPIN_ISP_MOSI);
    timerMsDelay(removeResetDelay);
    TCCR2 &= ~(1 << COM20);  /* clear toggle on compare match mode */
    PORT_PIN_SET(HWPIN_ISP_RESET);
#endif
#endif /* METABOARD_HARDWARE */
}

/* ------------------------------------------------------------------------- */

uchar   ispEnterProgmode(stkEnterProgIsp_t *param)
{
uchar   i, rval;

    ispAttachToDevice(stkParam.s.sckDuration, param->stabDelay);
    timerMsDelay(param->cmdExeDelay);
    /* we want for(i = param->synchLoops; i--;), but avrdude sends synchLoops == 0 */
    for(i = 32; i--;){
        wdt_reset();
        rval = ispBlockTransfer(param->cmd, param->pollIndex);
        if(param->pollIndex < 4)
            ispBlockTransfer(param->cmd + param->pollIndex, 4 - param->pollIndex);
        if(rval == param->pollValue){   /* success: we are in sync */
            return STK_STATUS_CMD_OK;
        }
        /* insert one clock pulse and try again: */
        PORT_PIN_SET(HWPIN_ISP_SCK);
        timerTicksDelay(ispClockDelay);
        PORT_PIN_CLR(HWPIN_ISP_SCK);
        timerTicksDelay(ispClockDelay);
    }
    ispDetachFromDevice(0);
    return STK_STATUS_CMD_FAILED;   /* failure */
}

void    ispLeaveProgmode(stkLeaveProgIsp_t *param)
{
    ispDetachFromDevice(param->preDelay);
    timerMsDelay(param->postDelay);
    PORT_DDR_CLR(HWPIN_ISP_SCK);    /* disable I/O pins, disconnect */
    PORT_DDR_CLR(HWPIN_ISP_MOSI);
    PORT_DDR_CLR(HWPIN_ISP_MISO);
    PORT_DDR_CLR(HWPIN_ISP_RESET);
    PORT_PIN_CLR(HWPIN_ISP_SCK);
    PORT_PIN_CLR(HWPIN_ISP_MOSI);
    PORT_PIN_CLR(HWPIN_ISP_MISO);
    PORT_PIN_CLR(HWPIN_ISP_RESET);

    PORT_PIN_CLR(HWPIN_LED_RED);
    PORT_PIN_CLR(HWPIN_LED_GREEN);
}

/* ------------------------------------------------------------------------- */

static uchar    deviceIsBusy(void)
{
    cmdBuffer[0] = 0xf0;
    cmdBuffer[1] = 0;
    return ispBlockTransfer(cmdBuffer, 4) & 1;
}

static uchar    waitUntilReady(uchar msTimeout)
{
    timerSetupTimeout(msTimeout);
    while(deviceIsBusy()){
        if(timerTimeoutOccurred())
            return STK_STATUS_RDY_BSY_TOUT;
    }
    return STK_STATUS_CMD_OK;
}

/* ------------------------------------------------------------------------- */

uchar   ispChipErase(stkChipEraseIsp_t *param)
{
uchar   maxDelay = param->eraseDelay;
uchar   rval = STK_STATUS_CMD_OK;

    PORT_PIN_CLR(HWPIN_LED_GREEN);
    PORT_PIN_SET(HWPIN_LED_RED);

    ispBlockTransfer(param->cmd, 4);
    if(param->pollMethod != 0){
        if(maxDelay < 10)   /* allow at least 10 ms */
            maxDelay = 10;
        rval = waitUntilReady(maxDelay);
    }else{
        timerMsDelay(maxDelay);
    }
    return rval;
}

/* ------------------------------------------------------------------------- */

uchar   ispProgramMemory(stkProgramFlashIsp_t *param, uchar isEeprom)
{
utilWord_t  numBytes;
uchar       rval = STK_STATUS_CMD_OK;
uchar       valuePollingMask, rdyPollingMask;
uint        i;

    PORT_PIN_CLR(HWPIN_LED_GREEN);
    PORT_PIN_SET(HWPIN_LED_RED);

    numBytes.bytes[1] = param->numBytes[0];
    numBytes.bytes[0] = param->numBytes[1];
    if(param->mode & 1){    /* page mode */
        valuePollingMask = 0x20;
        rdyPollingMask = 0x40;
    }else{                  /* word mode */
        valuePollingMask = 4;
        rdyPollingMask = 8;
    }
    if(!isEeprom && stkAddress.bytes[3] & 0x80){
        cmdBuffer[0] = 0x4d;    /* load extended address */
        cmdBuffer[1] = 0x00;
        cmdBuffer[2] = stkAddress.bytes[2];
        cmdBuffer[3] = 0x00;
        ispBlockTransfer(cmdBuffer, 4);
    }
    for(i = 0; rval == STK_STATUS_CMD_OK && i < numBytes.word; i++){
        uchar x;
        wdt_reset();
        cmdBuffer[1] = stkAddress.bytes[1];
        cmdBuffer[2] = stkAddress.bytes[0];
        cmdBuffer[3] = param->data[i];
        x = param->cmd[0];
        if(!isEeprom){
            x &= ~0x08;
            if((uchar)i & 1){
                x |= 0x08;
                stkIncrementAddress();
            }
        }else{
            stkIncrementAddress();
        }
        cmdBuffer[0] = x;
#if 0   /* does not work this way... */
        if(cmdBuffer[3] == 0xff && !(param->mode & 1) && !isEeprom)   /* skip 0xff in word mode */
            continue;
#endif
        ispBlockTransfer(cmdBuffer, 4);
        if(param->mode & 1){            /* is page mode */
            if(i < numBytes.word - 1 || !(param->mode & 0x80))
                continue;               /* not last byte written */
            cmdBuffer[0] = param->cmd[1];     /* write program memory page */
            ispBlockTransfer(cmdBuffer, 4);
        }
        /* poll for ready after each byte (word mode) or page (page mode) */
        if(param->mode & valuePollingMask){ /* value polling */
            uchar d = param->data[i];
            if(d == param->poll[0] || d == param->poll[1]){ /* must use timed polling */
                timerMsDelay(param->delay);
            }else{
                uchar x = param->cmd[2];     /* read flash */
                x &= ~0x08;
                if((uchar)i & 1){
                    x |= 0x08;
                }
                cmdBuffer[0] = x;
                timerSetupTimeout(param->delay);
                while(ispBlockTransfer(cmdBuffer, 4) != d){
                    if(timerTimeoutOccurred()){
                        rval = STK_STATUS_CMD_TOUT;
                        break;
                    }
                }
            }
        }else if(param->mode & rdyPollingMask){ /* rdy/bsy polling */
            rval = waitUntilReady(param->delay);
        }else{                          /* must be timed delay */
            timerMsDelay(param->delay);
        }
    }
    return rval;
}

/* ------------------------------------------------------------------------- */

uint    ispReadMemory(stkReadFlashIsp_t *param, stkReadFlashIspResult_t *result, uchar isEeprom)
{
utilWord_t  numBytes;
uchar       *p, cmd0;
uint        i;

    PORT_PIN_CLR(HWPIN_LED_RED);
    PORT_PIN_SET(HWPIN_LED_GREEN);

    cmdBuffer[3] = 0;
    if(!isEeprom && stkAddress.bytes[3] & 0x80){
        cmdBuffer[0] = 0x4d;    /* load extended address */
        cmdBuffer[1] = 0x00;
        cmdBuffer[2] = stkAddress.bytes[2];
        ispBlockTransfer(cmdBuffer, 4);
    }
    numBytes.bytes[1] = param->numBytes[0];
    numBytes.bytes[0] = param->numBytes[1];
    p = result->data;
    result->status1 = STK_STATUS_CMD_OK;
    cmd0 = param->cmd;
    for(i = 0; i < numBytes.word; i++){
        wdt_reset();
        cmdBuffer[1] = stkAddress.bytes[1];
        cmdBuffer[2] = stkAddress.bytes[0];
        if(!isEeprom){
            if((uchar)i & 1){
                cmd0 |= 0x08;
                stkIncrementAddress();
            }else{
                cmd0 &= ~0x08;
            }
        }else{
            stkIncrementAddress();
        }
        cmdBuffer[0] = cmd0;
        *p++ = ispBlockTransfer(cmdBuffer, 4);
    }
    *p = STK_STATUS_CMD_OK; /* status2 */
    return numBytes.word + 2;
}

/* ------------------------------------------------------------------------- */

uchar   ispProgramFuse(stkProgramFuseIsp_t *param)
{
    PORT_PIN_CLR(HWPIN_LED_GREEN);
    PORT_PIN_SET(HWPIN_LED_RED);

    ispBlockTransfer(param->cmd, 4);
    return STK_STATUS_CMD_OK;
}

/* ------------------------------------------------------------------------- */

uchar   ispReadFuse(stkReadFuseIsp_t *param)
{
uchar   rval;

    PORT_PIN_CLR(HWPIN_LED_RED);
    PORT_PIN_SET(HWPIN_LED_GREEN);

    rval = ispBlockTransfer(param->cmd, param->retAddr);
    if(param->retAddr < 4)
        ispBlockTransfer(param->cmd + param->retAddr, 4 - param->retAddr);
    return rval;
}

/* ------------------------------------------------------------------------- */

uint    ispMulti(stkMultiIsp_t *param, stkMultiIspResult_t *result)
{
uchar   cnt1, i, *p;

    PORT_PIN_CLR(HWPIN_LED_RED);
    PORT_PIN_SET(HWPIN_LED_GREEN);

    cnt1 = param->numTx;
    if(cnt1 > param->rxStartAddr)
        cnt1 = param->rxStartAddr;
    ispBlockTransfer(param->txData, cnt1);

    p = result->rxData;
    for(i = 0; i < param->numTx - cnt1; i++){
        uchar b = ispBlockTransfer(&param->txData[cnt1] + i, 1);
        if(i < param->numRx)
            *p++ = b;
        wdt_reset();
    }
    
    for(; i < param->numRx; i++){
        cmdBuffer[0] = 0;
        *p++ = ispBlockTransfer(cmdBuffer, 1);
        wdt_reset();
    }
    *p = result->status1 = STK_STATUS_CMD_OK;
    return (uint)param->numRx + 2;
}

/* ------------------------------------------------------------------------- */
