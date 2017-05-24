/*
 * Name: stk500protocol.h
 * Project: AVR-Doper
 * Author: Christian Starkjohann <cs@obdev.at>
 * Creation Date: 2006-06-19
 * Tabsize: 4
 * Copyright: (c) 2006 by Christian Starkjohann, all rights reserved.
 * License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 * Revision: $Id: stk500protocol.h 566 2008-04-26 14:21:47Z cs $
 */

/*
General Description:
This module implements parsing of the STK500v2 protocol and assembly of
response messages. It calls functions in the isp and hvprog modules to
perform the real work.
*/

#ifndef __stk500protocol_h_included__
#define __stk500protocol_h_included__

#include "utils.h"

extern void stkIncrementAddress(void);

extern void stkSetRxChar(uchar c);
extern int stkGetTxByte(void);
extern int stkGetTxCount(void);
extern void stkPoll(void);  /* must be called from main loop */

typedef union{
    uchar   bytes[32];
    struct{
        int     buildVersionLow;
        uchar   reserved1[14];
        uchar   hardwareVersion;
        uchar   softwareVersionMajor;
        uchar   softwareVersionMinor;
        uchar   reserved2;
        uchar   vTarget;
        uchar   vRef;
        uchar   oscPrescale;
        uchar   oscCmatch;
        uchar   sckDuration;
        uchar   reserved3;
        uchar   topcardDetect;
        uchar   reserved4;
        uchar   status;
        uchar   data;
        uchar   resetPolarity;
        uchar   controllerInit;
    }       s;
}stkParam_t;

/* ------------------------------------------------------------------------- */

extern utilDword_t  stkAddress;
extern stkParam_t   stkParam;

/* ------------------------------------------------------------------------- */
/* ----------------------- command parameter structs ----------------------- */
/* ------------------------------------------------------------------------- */

typedef struct stkEnterProgIsp{
    uchar   timeout;
    uchar   stabDelay;
    uchar   cmdExeDelay;
    uchar   synchLoops;
    uchar   byteDelay;
    uchar   pollValue;
    uchar   pollIndex;
    uchar   cmd[4];
}stkEnterProgIsp_t;

typedef struct stkLeaveProgIsp{
    uchar   preDelay;
    uchar   postDelay;
}stkLeaveProgIsp_t;

typedef struct stkChipEraseIsp{
    uchar   eraseDelay;
    uchar   pollMethod;
    uchar   cmd[4];
}stkChipEraseIsp_t;

typedef struct stkProgramFlashIsp{
    uchar   numBytes[2];
    uchar   mode;
    uchar   delay;
    uchar   cmd[3];
    uchar   poll[2];
    uchar   data[1];    /* actually more data than 1 byte */
}stkProgramFlashIsp_t;

typedef struct stkReadFlashIsp{
    uchar   numBytes[2];
    uchar   cmd;
}stkReadFlashIsp_t;

typedef struct stkReadFlashIspResult{
    uchar   status1;
    uchar   data[1];    /* actually more than 1 byte */
    /* uchar status2 */
}stkReadFlashIspResult_t;

typedef struct stkProgramFuseIsp{
    uchar   cmd[4];
}stkProgramFuseIsp_t;

typedef struct stkReadFuseIsp{
    uchar   retAddr;
    uchar   cmd[4];
}stkReadFuseIsp_t;

typedef struct stkMultiIsp{
    uchar   numTx;
    uchar   numRx;
    uchar   rxStartAddr;
    uchar   txData[1];  /* actually more than 1 byte */
}stkMultiIsp_t;

typedef struct stkMultiIspResult{
    uchar   status1;
    uchar   rxData[1];  /* potentially more than 1 byte */
    /* uchar status2 */
}stkMultiIspResult_t;

/* ------------------------------------------------------------------------- */

typedef struct stkEnterProgHvsp{
    uchar   stabDelay;
    uchar   cmdExeDelay;
    uchar   synchCycles;
    uchar   latchCycles;
    uchar   toggleVtg;
    uchar   powerOffDelay;
    uchar   resetDelay1;
    uchar   resetDelay2;
}stkEnterProgHvsp_t;

typedef struct stkLeaveProgHvsp{
    uchar   stabDelay;
    uchar   resetDelay;
}stkLeaveProgHvsp_t;

typedef struct stkChipEraseHvsp{
    uchar   pollTimeout;
    uchar   eraseTime;
}stkChipEraseHvsp_t;

typedef struct stkProgramFlashHvsp{
    uchar   numBytes[2];
    uchar   mode;
    uchar   pollTimeout;
    uchar   data[1];    /* actually more data than 1 byte */
}stkProgramFlashHvsp_t;

typedef struct stkReadFLashHvsp{
    uchar   numBytes[2];
}stkReadFlashHvsp_t;

#define stkReadFlashHvspResult_t    stkReadFlashIspResult_t

typedef struct stkProgramFuseHvsp{
    uchar   fuseAddress;
    uchar   fuseByte;
    uchar   pollTimeout;
}stkProgramFuseHvsp_t;

typedef struct stkReadFuseHvsp{
    uchar   fuseAddress;
}stkReadFuseHvsp_t;

/* ------------------------------------------------------------------------- */

typedef struct stkEnterProgPp{
    uchar   stabDelay;
    uchar   progModeDelay;
    uchar   latchCycles;
    uchar   toggleVtg;
    uchar   powerOffDelay;
    uchar   resetDelayMs;
    uchar   resetDelayUs;
}stkEnterProgPp_t;

#define stkLeaveProgPp_t        stkLeaveProgHvsp_t

typedef struct stkChipErasePp{
    uchar   pulseWidth;
    uchar   pollTimeout;
}stkChipErasePp_t;

#define stkProgramFlashPp_t     stkProgramFlashHvsp_t

#define stkReadFlashPp_t        stkReadFlashHvsp_t
#define stkReadFlashPpResult_t  stkReadFlashHvspResult_t

typedef struct stkProgramFusePp{
    uchar   address;
    uchar   data;
    uchar   pulseWidth;
    uchar   pollTimeout;
}stkProgramFusePp_t;

#define stkReadFusePp_t         stkReadFuseHvsp_t

/* ------------------------------------------------------------------------- */

#define STK_STX     27
#define STK_TOKEN   14

/* =================== [ STK general command constants ] =================== */

#define STK_CMD_SIGN_ON                         0x01
#define STK_CMD_SET_PARAMETER                   0x02
#define STK_CMD_GET_PARAMETER                   0x03
#define STK_CMD_SET_DEVICE_PARAMETERS           0x04
#define STK_CMD_OSCCAL                          0x05
#define STK_CMD_LOAD_ADDRESS                    0x06
#define STK_CMD_FIRMWARE_UPGRADE                0x07


/* =================== [ STK ISP command constants ] =================== */

#define STK_CMD_ENTER_PROGMODE_ISP              0x10
#define STK_CMD_LEAVE_PROGMODE_ISP              0x11
#define STK_CMD_CHIP_ERASE_ISP                  0x12
#define STK_CMD_PROGRAM_FLASH_ISP               0x13
#define STK_CMD_READ_FLASH_ISP                  0x14
#define STK_CMD_PROGRAM_EEPROM_ISP              0x15
#define STK_CMD_READ_EEPROM_ISP                 0x16
#define STK_CMD_PROGRAM_FUSE_ISP                0x17
#define STK_CMD_READ_FUSE_ISP                   0x18
#define STK_CMD_PROGRAM_LOCK_ISP                0x19
#define STK_CMD_READ_LOCK_ISP                   0x1A
#define STK_CMD_READ_SIGNATURE_ISP              0x1B
#define STK_CMD_READ_OSCCAL_ISP                 0x1C
#define STK_CMD_SPI_MULTI                       0x1D

/* =================== [ STK PP command constants ] =================== */

#define STK_CMD_ENTER_PROGMODE_PP               0x20
#define STK_CMD_LEAVE_PROGMODE_PP               0x21
#define STK_CMD_CHIP_ERASE_PP                   0x22
#define STK_CMD_PROGRAM_FLASH_PP                0x23
#define STK_CMD_READ_FLASH_PP                   0x24
#define STK_CMD_PROGRAM_EEPROM_PP               0x25
#define STK_CMD_READ_EEPROM_PP                  0x26
#define STK_CMD_PROGRAM_FUSE_PP                 0x27
#define STK_CMD_READ_FUSE_PP                    0x28
#define STK_CMD_PROGRAM_LOCK_PP                 0x29
#define STK_CMD_READ_LOCK_PP                    0x2A
#define STK_CMD_READ_SIGNATURE_PP               0x2B
#define STK_CMD_READ_OSCCAL_PP                  0x2C    

#define STK_CMD_SET_CONTROL_STACK               0x2D

/* =================== [ STK HVSP command constants ] =================== */

#define STK_CMD_ENTER_PROGMODE_HVSP             0x30
#define STK_CMD_LEAVE_PROGMODE_HVSP             0x31
#define STK_CMD_CHIP_ERASE_HVSP                 0x32
#define STK_CMD_PROGRAM_FLASH_HVSP              0x33
#define STK_CMD_READ_FLASH_HVSP                 0x34
#define STK_CMD_PROGRAM_EEPROM_HVSP             0x35
#define STK_CMD_READ_EEPROM_HVSP                0x36
#define STK_CMD_PROGRAM_FUSE_HVSP               0x37
#define STK_CMD_READ_FUSE_HVSP                  0x38
#define STK_CMD_PROGRAM_LOCK_HVSP               0x39
#define STK_CMD_READ_LOCK_HVSP                  0x3A
#define STK_CMD_READ_SIGNATURE_HVSP             0x3B
#define STK_CMD_READ_OSCCAL_HVSP                0x3C

/* =================== [ STK status constants ] =================== */

/* Success */
#define STK_STATUS_CMD_OK                       0x00

/* Warnings */
#define STK_STATUS_CMD_TOUT                     0x80
#define STK_STATUS_RDY_BSY_TOUT                 0x81
#define STK_STATUS_SET_PARAM_MISSING            0x82

/* Errors */
#define STK_STATUS_CMD_FAILED                   0xC0
#define STK_STATUS_CKSUM_ERROR                  0xC1
#define STK_STATUS_CMD_UNKNOWN                  0xC9

/* =================== [ STK parameter constants ] =================== */
#define STK_PARAM_BUILD_NUMBER_LOW              0x80
#define STK_PARAM_BUILD_NUMBER_HIGH             0x81
#define STK_PARAM_HW_VER                        0x90
#define STK_PARAM_SW_MAJOR                      0x91
#define STK_PARAM_SW_MINOR                      0x92
#define STK_PARAM_VTARGET                       0x94
#define STK_PARAM_VADJUST                       0x95
#define STK_PARAM_OSC_PSCALE                    0x96
#define STK_PARAM_OSC_CMATCH                    0x97
#define STK_PARAM_SCK_DURATION                  0x98
#define STK_PARAM_TOPCARD_DETECT                0x9A
#define STK_PARAM_STATUS                        0x9C
#define STK_PARAM_DATA                          0x9D
#define STK_PARAM_RESET_POLARITY                0x9E
#define STK_PARAM_CONTROLLER_INIT               0x9F

/* =================== [ STK answer constants ] =================== */

#define STK_ANSWER_CKSUM_ERROR                  0xB0


#endif /* __stk500protocol_h_included__ */
