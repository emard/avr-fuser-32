/*
 * Name: vreg.h
 * Project: AVR-Doper
 * Author: Christian Starkjohann <cs@obdev.at>
 * Creation Date: 2006-07-04
 * Tabsize: 4
 * Copyright: (c) 2006 by Christian Starkjohann, all rights reserved.
 * License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 * Revision: $Id: vreg.h 280 2007-03-20 12:03:11Z cs $
 */

/*
General Description:
This module implements the 12 V voltage regulator for high voltage programming
(in the ADC interrupt) and voltage reading of the target device supply voltage.
*/

#ifndef __vreg_h_included__
#define __vreg_h_included__

#include "utils.h"

void    vregInit(void);

#endif /* __vreg_h_included__ */
