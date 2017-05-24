/*
 * Name: isp.h
 * Project: AVR-Doper
 * Author: Christian Starkjohann <cs@obdev.at>
 * Creation Date: 2006-06-21
 * Tabsize: 4
 * Copyright: (c) 2006 by Christian Starkjohann, all rights reserved.
 * License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 * Revision: $Id: isp.h 280 2007-03-20 12:03:11Z cs $
 */

/*
General Description:
This module implements the STK500v2 primitives for In System Programming.
Functions accept parameters directly from the input data stream and prepare
results for the output data stream, where appropriate.
*/

#ifndef __isp_h_included__
#define __isp_h_included__

#include "stk500protocol.h"

#ifndef uchar
#define uchar   unsigned char
#endif

#ifndef uint
#define uint    unsigned int
#endif

uchar   ispEnterProgmode(stkEnterProgIsp_t *param);
void    ispLeaveProgmode(stkLeaveProgIsp_t *param);
uchar   ispChipErase(stkChipEraseIsp_t *param);
uchar   ispProgramMemory(stkProgramFlashIsp_t *param, uchar isEeprom);
uint    ispReadMemory(stkReadFlashIsp_t *param, stkReadFlashIspResult_t *result, uchar isEeprom);
uchar   ispProgramFuse(stkProgramFuseIsp_t *param);
uchar   ispReadFuse(stkReadFuseIsp_t *param);
uint    ispMulti(stkMultiIsp_t *param, stkMultiIspResult_t *result);


#endif  /* __isp_h_included__ */
