/*
 * Name: hvprog.h
 * Project: AVR-Doper
 * Author: Christian Starkjohann <cs@obdev.at>
 * Creation Date: 2006-07-07
 * Tabsize: 4
 * Copyright: (c) 2006 by Christian Starkjohann, all rights reserved.
 * License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 * Revision: $Id: hvprog.h 280 2007-03-20 12:03:11Z cs $
 */

/*
General Description:
This module implements the STK500v2 primitives for High Voltage serial and
parallel programming. Functions accept parameters directly from the input data
stream and prepare results for the output data stream, where appropriate.
*/

#ifndef __hvprog_h_included__
#define __hvprog_h_included__

#include "stk500protocol.h"

/* control lines for high voltage serial (and parallel) programming */
/*
    7       6       5       4    |  3       2       1       0
    n/a     XA1     XA0     BS1  |  /WR     /OE     BS2     PAGEL
*/
#define HVCTL_PAGEL (1 << 0)
#define HVCTL_BS2   (1 << 1)
#define HVCTL_nOE   (1 << 2)
#define HVCTL_nWR   (1 << 3)
#define HVCTL_BS1   (1 << 4)
#define HVCTL_XA0   (1 << 5)
#define HVCTL_XA1   (1 << 6)
#define HVCTL_NA    (1 << 7)

/* actions: */
#define HV_ADDR     0
#define HV_DATA     HVCTL_XA0
#define HV_CMD      HVCTL_XA1
#define HV_NONE     (HVCTL_XA1 | HVCTL_XA0)
#define HV_PAGEL    (HVCTL_XA1 | HVCTL_XA0 | HVCTL_PAGEL)

/* bytes: */
#define HV_LOW      0
#define HV_HIGH     HVCTL_BS1
#define HV_EXT      HVCTL_BS2
#define HV_EXT2     (HVCTL_BS1 | HVCTL_BS2)

#define HVCTL_WRFUSE_LOW      0
#define HVCTL_WRFUSE_HIGH     HVCTL_BS1
#define HVCTL_WRFUSE_EXT      HVCTL_BS2
#define HVCTL_WRLOCK          0

#define HVCTL_RDFUSE_LOW      0
#define HVCTL_RDFUSE_HIGH     (HVCTL_BS1 | HVCTL_BS2)
#define HVCTL_RDFUSE_EXT      HVCTL_BS2
#define HVCTL_RDLOCK          HVCTL_BS1

/* modes: */
#define HV_READ     HVCTL_nWR
#define HV_WRITE    HVCTL_nOE
#define HV_NORW     (HVCTL_nWR | HVCTL_nOE)

#define HVCTL(action, byte, mode)   ((action) | (byte) | (mode))


/* high voltage parallel and serial programming commands */
#define HVCMD_CHIP_ERASE    0x80
#define HVCMD_WRITE_FUSE    0x40
#define HVCMD_WRITE_LOCK    0x20
#define HVCMD_WRITE_FLASH   0x10
#define HVCMD_WRITE_EEPROM  0x11
#define HVCMD_READ_SIGCAL   0x08
#define HVCMD_READ_FUSELCK  0x04
#define HVCMD_READ_FLASH    0x02
#define HVCMD_READ_EEPROM   0x03
#define HVCMD_NOP           0x00



void    hvspEnterProgmode(stkEnterProgHvsp_t *param);
void    hvspLeaveProgmode(stkLeaveProgHvsp_t *param);
uchar   hvspChipErase(stkChipEraseHvsp_t *param);
uchar   hvspProgramMemory(stkProgramFlashHvsp_t *param, uchar isEeprom);
uint    hvspReadMemory(stkReadFlashHvsp_t *param, stkReadFlashHvspResult_t *result, uchar isEeprom);
uchar   hvspProgramFuse(stkProgramFuseHvsp_t *param);
uchar   hvspProgramLock(stkProgramFuseHvsp_t *param);
uchar   hvspReadFuse(stkReadFuseHvsp_t *param);
uchar   hvspReadLock(void);
uchar   hvspReadSignature(stkReadFuseHvsp_t *param);
uchar   hvspReadOsccal(void);

void    ppEnterProgmode(stkEnterProgPp_t *param);
void    ppLeaveProgmode(stkLeaveProgPp_t *param);
uchar   ppChipErase(stkChipErasePp_t *param);
#define ppProgramMemory(param, isEeprom)        hvspProgramMemory(param, isEeprom)
#define ppReadMemory(param, result, isEeprom)   hvspReadMemory(param, result, isEeprom)
uchar   ppProgramFuse(stkProgramFusePp_t *param);
uchar   ppProgramLock(stkProgramFusePp_t *param);
#define ppReadFuse(param)                       hvspReadFuse(param)
#define ppReadLock()                            hvspReadLock()
#define ppReadSignature(param)                  hvspReadSignature(param)
#define ppReadOsccal()                          hvspReadOsccal()

/* low level routines for fusereset */
uchar   hvReadSignature(uchar addr, uchar highLow);
uchar   hvReadFuse(uchar highLow);
uchar   hvProgramFuse(uchar value, uchar cmd, uchar highLow);
uchar   hvChipErase(uchar eraseTime);
void    hvEnterProgmode();
void    hvLeaveProgmode();
void    hvSetProgmode(uint8_t is_parallel);

#endif /* __hvprog_h_included__ */
