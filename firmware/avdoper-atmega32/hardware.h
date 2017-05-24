/*
 * Name: hardware.h
 * Project: AVR-Doper
 * Author: Christian Starkjohann <cs@obdev.at>
 * Creation Date: 2006-07-05
 * Tabsize: 4
 * Copyright: (c) 2006 by Christian Starkjohann, all rights reserved.
 * License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 * Revision: $Id: hardware.h 566 2008-04-26 14:21:47Z cs $
 */

/*
General Description:
This module defines hardware properties and configuration choices.
*/

#ifndef __hardware_h_included__
#define __hardware_h_included__

/* configuration options: */
#define USE_DCD_REPORTING       0
/* If this option is defined to 1, the driver will report carrier detect when
 * the serial device is opened. This is useful on some Unix platforms to allow
 * using the /dev/tty* instead of /dev/cu* devices.
 * Setting this option to 0 saves a couple of bytes in flash and RAM memory.
 */
#define ENABLE_DEBUG_INTERFACE  0
/* If this option is defined to 1, the device buffers serial data (read from
 * the the ISP connector) and makes it available through vendor specific
 * requests to host based software. This is useful to display debug information
 * sent by the target.
 * Setting this option to 0 saves a couple of bytes in flash memory.
 */
#define ENABLE_HID_INTERFACE    1
/* If this option is defined to 1, the device implements a custom HID type
 * interface to send and receive STK500 serial data. If both, the CDC-ACM and
 * HID interface are enabled, a jumper selects which one is used.
 */
#define ENABLE_CDC_INTERFACE    0
/* If this option is defined to 1, the device implements a CDC-ACM modem,
 * emulating the behavior of the STK500 board on a virtual COM port.
 */
#define ENABLE_HVPROG           1
/* If this option is defined to 1, the high voltage programmer is enabled.
 */
#define HW_CDC_PACKET_SIZE      8
/* Size of bulk transfer packets. The standard demands 8 bytes, but we may
 * be better off with less. Try smaller values if the communication hangs.
 */
#define HW_DEBUG_BAUDRATE       19200
/* Bit-rate for the UART (for reading debug data from the target).
 */
#ifndef F_CPU
#error "F_CPU must be defined in compiler command line!"
/* #define F_CPU                   12000000 */
/* Our CPU frequency.
 */
#endif


/*
Port        | Function                | dir | value
------------+-------------------------+-----+-------
PORT B
  0         | HVSP Supply               [O]    1
  1 OC1A    | SMPS                      [O]    0
  2         | HVSP RESETHV / LEDHV      [O]    0
  3 OC2     | HVSP SCI / ISP target clk [O]    0
  4         | ISP driver enable         [O]    0
  5         | LED Prog active           [O]    1
  6 XTAL1   | XTAL
  7 XTAL2   | XTAL
PORT C (ADC)
  0         | SMPS feedback             [i]    0
  1         | ISP voltage sense         [i]    0
  2         | ISP SCK                   [O]    0
  3         | ISP MISO                  [I]    1
  4         | ISP MOSI                  [O]    0
  5         | ISP RESET / HVSP RESET    [O]    0
  6 RESET   | Reset
  7 n/a     | *
PORT D
  0 RxD     | ISP TxD                   [I]    1
  1 TxD     | ISP RxD                   [I]    1
  2 Int0    | USB D+                    [i]    0
  3 Int1    | USB D-                    [i]    0
  4 T0      | JUMPER Low Speed          [I]    1
  5 T1      | HVSP SII (PPD1)           [I]    1
  6 AIN0    | HVSP SDI (PPD0)           [I]    1
  7 AIN1    | HVSP SDO (PPD2)           [I]    1
*/

/* The following defines can be used with the PORT_* macros from utils.h */

#if USBASP_HARDWARE         /* USBasp hardware from www.fischl.de/usbasp/ */

#undef ENABLE_HVPROG
#define ENABLE_HVPROG       0
#define METABOARD_HARDWARE  1   /* most settings are the same as for metaboard */

#define HWPIN_LED           C, 1
#define HWPIN_ISP_SUPPLY1   C, 3    /* these pins are NC on USBasp */
#define HWPIN_ISP_SUPPLY2   C, 4

#define HWPIN_ISP_RESET     B, 2
#define HWPIN_ISP_MOSI      B, 3
#define HWPIN_ISP_MISO      B, 4
#define HWPIN_ISP_SCK       B, 5

#define HWPIN_ISP_TXD       D, 0
#define HWPIN_ISP_RXD       D, 1
#define HWPIN_USB_DPLUS     B, 1
#define HWPIN_USB_DMINUS    B, 0
#define HWPIN_JUMPER        C, 2

#elif METABOARD_HARDWARE    /* Metaboard hardware from www.obdev.at/goto?t=metaboard */

#undef ENABLE_HVPROG
#define ENABLE_HVPROG       0

#define HWPIN_LED           C, 1
#define HWPIN_ISP_SUPPLY1   C, 3
#define HWPIN_ISP_SUPPLY2   C, 4

#define HWPIN_ISP_CLK       B, 1
#define HWPIN_ISP_RESET     B, 2
#define HWPIN_ISP_MOSI      B, 3
#define HWPIN_ISP_MISO      B, 4
#define HWPIN_ISP_SCK       B, 5

#define HWPIN_ISP_TXD       D, 0
#define HWPIN_ISP_RXD       D, 1
#define HWPIN_USB_DPLUS     D, 2
#define HWPIN_USB_DMINUS    D, 4
#define HWPIN_JUMPER        D, 7

#elif HVPP_HARDWARE         /* HVPP fuse reset hardware */
/***********************************************************/

/* comment out if we don't have ISP driver */
/*
#define HWPIN_ISP_DRIVER    B, 4
*/
#define HWPIN_LED_RED       B, 0
#define HWPIN_LED_GREEN     B, 1

#define HWPIN_ISP_SCK       B,7
#define HWPIN_ISP_MISO      B,5
#define HWPIN_ISP_MOSI      B,6
#define HWPIN_ISP_RESET     B,4
#define HWPIN_ISP_TXD       D,0
#define HWPIN_ISP_RXD       D,1
#define HWPIN_ISP_CLK       D,7

#define HWPIN_USB_DPLUS     D, 2
#define HWPIN_USB_DMINUS    D, 3

/* start button */
#define HWPIN_START         B,3

/* jumper is actually the start button */
#define HWPIN_JUMPER        HWPIN_START

/* RSTH pulls reset up to +12V */
#define HW_RSTH             D, 4
/* RSTL pulls reset down to 0V */
#define HW_RSTL             D, 6

/* VCC +5V supply for target chip */
#define HW_VCC              B,2
/* 0-non inverting vcc hardware, 1-inverting vcc hardware */
#define NW_VCC_INVERT       0

/* 8-bit data port */
#define HWPORT_DATA         A,0

/* normal (non-inverting) logic */
#define HW_BS2              C,0
#define HW_RDY              C,1
#define HW_OE               C,2
#define HW_WR               C,3
#define HW_BS1              C,4
#define HW_XA0              C,5
#define HW_XA1              C,6
#define HW_PAGEL            C,7
#define HW_XTAL1            D,7  /* clk */
/*
0: PAGEL and BS2 are correctly routed in hardware
1: routelines should route PAGEL and BS2 
*/
#define HW_ROUTE_PAGEL_BS2    1

/* port that enables output for 12V generator */
#define HWPIN_BOOST_OUT     D, 5

#define HWPIN_HVSP_HVRESET  HW_RSTH
#define HWPIN_HVSP_RESET    HW_RSTL
#define HWPIN_HVSP_SUPPLY   HW_VCC
#define HWPIN_HVSP_SDI      A, 0
#define HWPIN_HVSP_SII      A, 1
#define HWPIN_HVSP_SDO      A, 2
#define HWPIN_HVSP_SCI      HWPIN_ISP_CLK

#define HWPIN_HVSP_PRGEN0   HW_BS1
#define HWPIN_HVSP_PRGEN1   HW_XA0
#define HWPIN_HVSP_PRGEN2   HW_XA1
/***********************************************************/
#elif FUSER_HARDWARE
/* comment out if we don't have ISP driver */
/*
#define HWPIN_ISP_DRIVER    B, 4
*/
#define HWPIN_LED_RED       B, 0
#define HWPIN_LED_GREEN     B, 1

#define HWPIN_ISP_SCK       B,7
#define HWPIN_ISP_MISO      B,5
#define HWPIN_ISP_MOSI      B,6
#define HWPIN_ISP_RESET     B,4
#define HWPIN_ISP_TXD       D,0
#define HWPIN_ISP_RXD       D,1
#define HWPIN_ISP_CLK       D,7

#define HWPIN_USB_DPLUS     D, 2
#define HWPIN_USB_DMINUS    D, 3

/* start button */
#define HWPIN_START         B,3

/* jumper is actually the start button */
#define HWPIN_JUMPER        HWPIN_START

/* RSTH pulls reset up to +12V */
#define HW_RSTH             D, 4
/* RSTL pulls reset down to 0V */
#define HW_RSTL             D, 6

/* VCC +5V supply for target chip */
#define HW_VCC              B,2
/* 0-non inverting vcc hardware, 1-inverting vcc hardware */
#define HW_VCC_INVERT       1

/* 8-bit data port */
#define HWPORT_DATA         A,0

/* normal (non-inverting) logic */
#define HW_PAGEL            C,0
#define HW_BS2              C,1
#define HW_OE               C,2
#define HW_WR               C,3
#define HW_BS1              C,4
#define HW_XA0              C,5
#define HW_XA1              C,6
#define HW_RDY              C,7
#define HW_XTAL1            D,7  /* clk */
/*
0: PAGEL and BS2 are correctly routed in hardware
1: routelines should move PAGEL and BS2 
*/
#define HW_ROUTE_PAGEL_BS2    0

/* port that enables output for 12V generator */
#define HWPIN_BOOST_OUT     D, 5

#define HWPIN_HVSP_HVRESET  HW_RSTH
#define HWPIN_HVSP_RESET    HW_RSTL
#define HWPIN_HVSP_SUPPLY   HW_VCC
#define HWPIN_HVSP_SDI      A, 0
#define HWPIN_HVSP_SII      A, 1
#define HWPIN_HVSP_SDO      A, 2
#define HWPIN_HVSP_SCI      HWPIN_ISP_CLK

#define HWPIN_HVSP_PRGEN0   HW_BS1
#define HWPIN_HVSP_PRGEN1   HW_XA0
#define HWPIN_HVSP_PRGEN2   HW_XA1
/***********************************************************/
#else                       /* Standard AVR-Doper hardware */

#define HWPIN_HVSP_SUPPLY   B, 0
#define HWPIN_SMPS_OUT      B, 1
#define HWPIN_HVSP_HVRESET  B, 2
#define HWPIN_HVSP_SCI      B, 3
#define HWPIN_ISP_CLK       B, 3
/* comment out if we don't have ISP driver */
/*
#define HWPIN_ISP_DRIVER    B, 4
*/
#define HWPIN_LED           B, 5

#define HWPIN_ADC_SMPS      C, 0
#define HWPIN_ADC_VTARGET   C, 1
#define HWPIN_ISP_SCK       C, 2
#define HWPIN_ISP_MISO      C, 3
#define HWPIN_ISP_MOSI      C, 4
#define HWPIN_ISP_RESET     C, 5
#define HWPIN_HVSP_RESET    C, 5

#define HWPIN_ISP_TXD       D, 0
#define HWPIN_ISP_RXD       D, 1
#define HWPIN_USB_DPLUS     D, 2
#define HWPIN_USB_DMINUS    D, 3
#define HWPIN_JUMPER        D, 4
#define HWPIN_HVSP_SII      D, 5
#define HWPIN_HVSP_SDI      D, 6
#define HWPIN_HVSP_SDO      D, 7

#endif

/* Device compatibility: Allow both, ATMega8 and ATMega88/168. The following
 * macros (almost) mimic an ATMega8 based on the ATMega88/168 defines.
 */
#include <avr/io.h>

#ifdef TCCR2A
#   define TCCR2    TCCR2A
#   define COM20    COM2A0
#   define OCR2     OCR2A
#   define HW_SET_T2_PRESCALER(value)   (TCCR2B = (TCCR2B & ~7) | (value & 7))
#   define HW_GET_T2_PRESCALER()        (TCCR2B & 7)
#else
#   define HW_SET_T2_PRESCALER(value)   (TCCR2 = (TCCR2 & ~7) | (value & 7))
#   define HW_GET_T2_PRESCALER()        (TCCR2 & 7)
#endif

#ifdef TCCR0B
#   define TCCR0    TCCR0B
#   define TIMSK    TIMSK0
#endif

#ifdef UBRR0L
#   define UBRRH    UBRR0H
#   define UBRRL    UBRR0L
#   define UCSRA    UCSR0A
#   define UCSRB    UCSR0B
#   define UDR      UDR0
#   define RXEN     RXEN0
#   define TXEN     TXEN0
#   define UDRE     UDRE0
#   define RXCIE    RXCIE0
#endif



#endif /* __hardware_h_included__ */
