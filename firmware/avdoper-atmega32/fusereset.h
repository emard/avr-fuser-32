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

#ifndef __fusereset_h_included__
#define __fusereset_h_included__

struct fuses {
  uint8_t low,high,extended,lock;
};

struct fuserec {
  uint8_t signature[3];    /* 3 byte signature - all FF's means end record */
  struct fuses fuse, mask; /* fuse bits values and mask (presence of the bits) */  
};

void pushbutton_poll(void);

#endif /* __fusereset_h_included__ */
