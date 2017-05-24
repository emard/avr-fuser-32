/* Name: main.c
 * Project: AVR-Doper
 * Author: Christian Starkjohann <cs@obdev.at>
 * Creation Date: 2006-06-21
 * Tabsize: 4
 * Copyright: (c) 2006 by Christian Starkjohann, all rights reserved.
 * License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 * Revision: $Id: main.c 702 2008-11-26 20:16:54Z cs $
 */

/*
General Description:
This module implements hardware initialization and the USB interface
*/

#include "hardware.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <string.h>

#include "utils.h"
#include "usbdrv.h"
#include "oddebug.h"
#include "stk500protocol.h"
#include "vreg.h"
#include "serial.h"
#include "timer.h"
#include "fusereset.h"

/* ------------------------------------------------------------------------- */

/* CDC class requests: */
enum {
    SEND_ENCAPSULATED_COMMAND = 0,
    GET_ENCAPSULATED_RESPONSE,
    SET_COMM_FEATURE,
    GET_COMM_FEATURE,
    CLEAR_COMM_FEATURE,
    SET_LINE_CODING = 0x20,
    GET_LINE_CODING,
    SET_CONTROL_LINE_STATE,
    SEND_BREAK
};

/* defines for 'requestType' */
#define REQUEST_TYPE_LINE_CODING    0   /* CDC GET/SET_LINE_CODING */
#define REQUEST_TYPE_HID_FIRST      1   /* first block of HID reporting */
#define REQUEST_TYPE_HID_SUBSEQUENT 2   /* subsequent block of HID reporting */
#define REQUEST_TYPE_HID_DEBUGDATA  3   /* read/write data from/to debug interface */
#define REQUEST_TYPE_VENDOR         4   /* vendor request for get/set debug data */

/* ------------------------------------------------------------------------- */

#if ENABLE_CDC_INTERFACE
static uchar    modeBuffer[7] = {0x80, 0x25, 0, 0, 0, 0, 8};  /* default: 9600 bps, 8n1 */
#   if USE_DCD_REPORTING
static uchar    intr3Status;
#   endif
#endif
#if ENABLE_HID_INTERFACE
static uchar    hidStatus;
#endif
static uchar    requestType;
static uchar    useHIDInterface;

#if ENABLE_CDC_INTERFACE
static const PROGMEM char deviceDescrCDC[] = {    /* USB device descriptor */
    18,         /* sizeof(usbDescriptorDevice): length of descriptor in bytes */
    USBDESCR_DEVICE,        /* descriptor type */
    0x10, 0x01,             /* USB version supported */
    0x02,                   /* device class: CDC */
    0,                      /* subclass */
    0,                      /* protocol */
    8,                      /* max packet size */
    USB_CFG_VENDOR_ID,      /* 2 bytes */
    0xe1, 0x05,             /* 2 bytes: shared PID for CDC-ACM devices */
    USB_CFG_DEVICE_VERSION, /* 2 bytes */
    1,                      /* manufacturer string index */
    2,                      /* product string index */
    3,                      /* serial number string index */
    1,                      /* number of configurations */
};

static const PROGMEM char configDescrCDC[] = {   /* USB configuration descriptor */
    9,          /* sizeof(usbDescriptorConfiguration): length of descriptor in bytes */
    USBDESCR_CONFIG,    /* descriptor type */
    67, 0,      /* total length of data returned (including inlined descriptors) */
    2,          /* number of interfaces in this configuration */
    1,          /* index of this configuration */
    0,          /* configuration name string index */
#if USB_CFG_IS_SELF_POWERED
    USBATTR_SELFPOWER,  /* attributes */
#else
    USBATTR_BUSPOWER,   /* attributes */
#endif
    USB_CFG_MAX_BUS_POWER/2,            /* max USB current in 2mA units */

    /* interface descriptors follow inline: */
    /* Interface Descriptor for CDC-ACM Control  */
    9,          /* sizeof(usbDescrInterface): length of descriptor in bytes */
    USBDESCR_INTERFACE, /* descriptor type */
    0,          /* index of this interface */
    0,          /* alternate setting for this interface */
    1,          /* endpoints excl 0: number of endpoint descriptors to follow */
    USB_CFG_INTERFACE_CLASS,    /* see usbconfig.h */
    USB_CFG_INTERFACE_SUBCLASS,
    USB_CFG_INTERFACE_PROTOCOL,
    0,          /* string index for interface */

    /* CDC Class-Specific descriptors */
    5,          /* sizeof(usbDescrCDC_HeaderFn): length of descriptor in bytes */
    0x24,       /* descriptor type */
    0,          /* Subtype: header functional descriptor */
    0x10, 0x01, /* CDC spec release number in BCD */

    4,          /* sizeof(usbDescrCDC_AcmFn): length of descriptor in bytes */
    0x24,       /* descriptor type */
    2,          /* Subtype: abstract control management functional descriptor */
    0x02,       /* capabilities: SET_LINE_CODING, GET_LINE_CODING, SET_CONTROL_LINE_STATE */

    5,          /* sizeof(usbDescrCDC_UnionFn): length of descriptor in bytes */
    0x24,       /* descriptor type */
    6,          /* Subtype: union functional descriptor */
    0,          /* CDC_COMM_INTF_ID: master interface (control) */
    1,          /* CDC_DATA_INTF_ID: slave interface (data) */

    5,          /* sizeof(usbDescrCDC_CallMgtFn): length of descriptor in bytes */
    0x24,       /* descriptor type */
    1,          /* Subtype: call management functional descriptor */
    0x03,       /* capabilities: allows management on data interface, handles call management by itself */
    1,          /* CDC_DATA_INTF_ID: interface used for call management */

    /* Endpoint Descriptor */
    7,          /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
    0x83,       /* IN endpoint number 3 */
    0x03,       /* attrib: Interrupt endpoint */
    8, 0,       /* maximum packet size */
    100,        /* in ms */

    /* Interface Descriptor for CDC-ACM Data  */
    9,          /* sizeof(usbDescrInterface): length of descriptor in bytes */
    USBDESCR_INTERFACE, /* descriptor type */
    1,          /* index of this interface */
    0,          /* alternate setting for this interface */
    2,          /* endpoints excl 0: number of endpoint descriptors to follow */
    0x0a,       /* Data Interface Class Codes */
    0,          /* interface subclass */
    0,          /* Data Interface Class Protocol Codes */
    0,          /* string index for interface */

    /* Endpoint Descriptor */
    7,          /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
    0x01,       /* OUT endpoint number 1 */
    0x02,       /* attrib: Bulk endpoint */
    HW_CDC_PACKET_SIZE, 0,  /* maximum packet size */
    0,          /* in ms */

    /* Endpoint Descriptor */
    7,          /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
    0x81,       /* IN endpoint number 1 */
    0x02,       /* attrib: Bulk endpoint */
    HW_CDC_PACKET_SIZE, 0,  /* maximum packet size */
    0,          /* in ms */
};
#endif /* ENABLE_CDC_INTERFACE */

#if ENABLE_HID_INTERFACE
static const PROGMEM char deviceDescrHID[] = {    /* USB device descriptor */
    18,         /* sizeof(usbDescriptorDevice): length of descriptor in bytes */
    USBDESCR_DEVICE,        /* descriptor type */
    0x01, 0x01,             /* USB version supported */
    0,                      /* device class: composite */
    0,                      /* subclass */
    0,                      /* protocol */
    8,                      /* max packet size */
    USB_CFG_VENDOR_ID,      /* 2 bytes */
    0xdf, 0x05,             /* 2 bytes: shared PID for HIDs */
    USB_CFG_DEVICE_VERSION, /* 2 bytes */
    1,                      /* manufacturer string index */
    2,                      /* product string index */
    0,                      /* serial number string index */
    1,                      /* number of configurations */
};

static const PROGMEM char configDescrHID[] = {    /* USB configuration descriptor */
    9,          /* sizeof(usbDescriptorConfiguration): length of descriptor in bytes */
    USBDESCR_CONFIG,/* descriptor type */
    18 + 7 + 9, 0,  /* total length of data returned (including inlined descriptors) */
    1,          /* number of interfaces in this configuration */
    1,          /* index of this configuration */
    0,          /* configuration name string index */
#if USB_CFG_IS_SELF_POWERED
    (1<<7) | USBATTR_SELFPOWER,  /* attributes */
#else
    (1<<7) | USBATTR_BUSPOWER,   /* attributes */
#endif
    USB_CFG_MAX_BUS_POWER/2,    /* max USB current in 2mA units */
/* interface descriptor follows inline: */

    9,          /* sizeof(usbDescrInterface): length of descriptor in bytes */
    USBDESCR_INTERFACE, /* descriptor type */
    0,          /* index of this interface */
    0,          /* alternate setting for this interface */
    USB_CFG_HAVE_INTRIN_ENDPOINT,   /* endpoints excl 0: number of endpoint descriptors to follow */
    3,          /* interface class: HID */
    0,          /* subclass */
    0,          /* protocol */
    0,          /* string index for interface */

    9,          /* sizeof(usbDescrHID): length of descriptor in bytes */
    USBDESCR_HID,   /* descriptor type: HID */
    0x01, 0x01, /* BCD representation of HID version */
    0x00,       /* target country code */
    0x01,       /* number of HID Report (or other HID class) Descriptor infos to follow */
    0x22,       /* descriptor type: report */
    USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH, 0,  /* total length of report descriptor */

    7,          /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
    0x81,       /* IN endpoint number 1 */
    0x03,       /* attrib: Interrupt endpoint */
    8, 0,       /* maximum packet size */
    USB_CFG_INTR_POLL_INTERVAL, /* in ms */
};

const PROGMEM char usbDescriptorHidReport[60] = {
    0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)

    0x85, 0x01,                    //   REPORT_ID (1)
    0x95, 0x0e,                    //   REPORT_COUNT (14)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0x85, 0x02,                    //   REPORT_ID (2)
    0x95, 0x1e,                    //   REPORT_COUNT (30)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0x85, 0x03,                    //   REPORT_ID (3)
    0x95, 0x3e,                    //   REPORT_COUNT (62)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0x85, 0x04,                    //   REPORT_ID (4)
    0x95, 0x7e,                    //   REPORT_COUNT (126)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0x85, 0x05,                    //   REPORT_ID (5) [for debug interface]
    0x95, 0x3e,                    //   REPORT_COUNT (62)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0xc0                           // END_COLLECTION
};
/* Note: REPORT_COUNT does not include report-ID byte */

#endif /* ENABLE_HID_INTERFACE */

/* ------------------------------------------------------------------------- */

uchar usbFunctionDescriptor(usbRequest_t *rq)
{
uchar *p = NULL, len = 0, useHID = useHIDInterface;

    if(useHID){
#if ENABLE_HID_INTERFACE
        if(rq->wValue.bytes[1] == USBDESCR_DEVICE){
            p = (uchar *)deviceDescrHID;
            len = sizeof(deviceDescrHID);
        }else if(rq->wValue.bytes[1] == USBDESCR_CONFIG){
            p = (uchar *)configDescrHID;
            len = sizeof(configDescrHID);
        }else{  /* must be HID descriptor */
            p = (uchar *)(configDescrHID + 18);
            len = 9;
        }
#endif
    }else{
#if ENABLE_CDC_INTERFACE
        if(rq->wValue.bytes[1] == USBDESCR_DEVICE){
            p = (uchar *)deviceDescrCDC;
            len = sizeof(deviceDescrCDC);
        }else{  /* must be config descriptor */
            p = (uchar *)configDescrCDC;
            len = sizeof(configDescrCDC);
        }
#endif
    }
    usbMsgPtr = p;
    return len;
}


/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

uchar usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;
uchar           rqType = rq->bmRequestType & USBRQ_TYPE_MASK;

    if(rqType == USBRQ_TYPE_CLASS){    /* class request type */
        if(useHIDInterface){
#if ENABLE_HID_INTERFACE
            if(rq->bRequest == USBRQ_HID_GET_REPORT || rq->bRequest == USBRQ_HID_SET_REPORT){
                hidStatus = rq->wValue.bytes[0];    /* store report ID */
                if(rq->wValue.bytes[0] == 5){   /* report ID */
                    requestType = REQUEST_TYPE_HID_DEBUGDATA;
                }else{
                    requestType = REQUEST_TYPE_HID_FIRST;
                }
                /* the driver counts the total number of bytes for us */
                return 0xff;
            }
#endif
        }else{
#if ENABLE_CDC_INTERFACE
            if(rq->bRequest == GET_LINE_CODING || rq->bRequest == SET_LINE_CODING){
                requestType = REQUEST_TYPE_LINE_CODING;
                return 0xff;
            }
#   if USE_DCD_REPORTING
            if(rq->bRequest == SET_CONTROL_LINE_STATE){
                /* Report serial state (carrier detect). On several Unix platforms,
                 * tty devices can only be opened when carrier detect is set.
                 */
                intr3Status = 2;
            }
#   endif
#endif
        }
    }
#if ENABLE_DEBUG_INTERFACE
    else if(rqType == USBRQ_TYPE_VENDOR){   /* vendor requests */
        if(rq->bRequest == 1){  /* transmit data */
            serialPutc(rq->wValue.bytes[0]);
        }else if(rq->bRequest == 2){
            requestType = REQUEST_TYPE_VENDOR;
            return 0xff;    /* handle in usbFunctionRead() */
        }
    }
#endif
    return 0;
}

uchar usbFunctionRead(uchar *data, uchar len)
{
    if(requestType == REQUEST_TYPE_LINE_CODING){
#if ENABLE_CDC_INTERFACE
        /* return the "virtual" configuration */
        memcpy(data, modeBuffer, 7);
        return 7;
#endif
    }
#if ENABLE_DEBUG_INTERFACE
    else if(requestType == REQUEST_TYPE_VENDOR){
        uchar cnt;
        for(cnt = 0; cnt < len && ringBufferHasData(&serialRingBuffer); cnt++){
            *data++ = ringBufferRead(&serialRingBuffer);
        }
        return cnt;
#if ENABLE_HID_INTERFACE
    }else if(requestType == REQUEST_TYPE_HID_DEBUGDATA){
        uchar *p = data, remaining;
        if(hidStatus == 5){ /* first call */
            *p++ = hidStatus;   /* report ID */
            *p++ = ringBufferCount(&serialRingBuffer);
            remaining = len - 2;
            hidStatus = 1;  /* continue with subsequent call */
        }else{
            remaining = len;
        }
        if(hidStatus){
            do{
                if(!ringBufferHasData(&serialRingBuffer)){
                    hidStatus = 0;
                    break;
                }
                *p++ = ringBufferRead(&serialRingBuffer);
            }while(--remaining);
        }
        return len;
#endif
    }
#endif
#if ENABLE_HID_INTERFACE
    else if(requestType == REQUEST_TYPE_HID_FIRST || requestType == REQUEST_TYPE_HID_SUBSEQUENT){
        uchar *p = data, remaining;
        if(requestType == REQUEST_TYPE_HID_FIRST){
            int cnt;
            *p++ = hidStatus;   /* report ID */
            cnt = stkGetTxCount();
            if(utilHi8(cnt)){
                *p = 255;
            }else{
                *p = cnt;     /* second byte is number of remaining bytes buffered */
            }
            p++;
            remaining = len - 2;
            requestType = REQUEST_TYPE_HID_SUBSEQUENT;
        }else{
            remaining = len;
        }
        if(hidStatus){
            do{
                int c = stkGetTxByte();
                if(c < 0){
                    hidStatus = 0;
                    break;
                }
                *p++ = c;
            }while(--remaining);
        }
        return len;
    }
#endif
    return 0;   /* error -> terminate transfer */
}

uchar usbFunctionWrite(uchar *data, uchar len)
{
    if(requestType == REQUEST_TYPE_LINE_CODING){
#if ENABLE_CDC_INTERFACE
        /* Don't know why data toggling is reset when line coding is changed, but it is... */
        USB_SET_DATATOKEN1(USBPID_DATA1);   /* enforce DATA0 token for next transfer */
        /* store the line configuration so that we can return it on request */
        memcpy(modeBuffer, data, 7);
        return 1;
#endif
    }
#if ENABLE_HID_INTERFACE
#if ENABLE_DEBUG_INTERFACE
    else if(requestType == REQUEST_TYPE_HID_DEBUGDATA){
        uchar *p = data, rval = len != 8;
        if(hidStatus == 5){ /* first call */
            hidStatus = -p[1]; /* second byte is data length */
            p += 2;
            len -= 2;
        }
        do{
            if(!hidStatus)
                break;
            serialPutc(*p++);
            hidStatus++;
        }while(--len);
        return rval;    /* the last packet must have 7 bytes insted of 8 */
    }
#endif
    else if(requestType == REQUEST_TYPE_HID_FIRST || requestType == REQUEST_TYPE_HID_SUBSEQUENT){
        uchar *p = data, rval = len != 8;
        if(requestType == REQUEST_TYPE_HID_FIRST){
            hidStatus = p[1]; /* second byte is data length */
            p += 2;
            len -= 2;
            requestType = REQUEST_TYPE_HID_SUBSEQUENT;
        }
        do{
            if(!hidStatus)
                break;
            stkSetRxChar(*p++);
            hidStatus--;
        }while(--len);
        return rval;    /* the last packet must have 7 bytes insted of 8 */
    }
#endif
    return 1;   /* error -> accept everything until end */
}

void usbFunctionWriteOut(uchar *data, uchar len)
{
#if ENABLE_CDC_INTERFACE
    do{ /* len must be at least 1 character, the driver discards zero sized packets */
        stkSetRxChar(*data++);
    }while(--len);
#endif
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

static void readInterfaceType(void)
{
#if ENABLE_HID_INTERFACE && ENABLE_CDC_INTERFACE
#if METABOARD_HARDWARE
    PORT_PIN_SET(HWPIN_JUMPER);                         /* set internal pull-up */
    _delay_us(1);
    useHIDInterface = !PORT_PIN_VALUE(HWPIN_JUMPER);    /* if jumper is set -> HID mode */
#else
#if 0
    PORT_DDR_SET(HWPIN_ISP_MOSI);
    PORT_PIN_SET(HWPIN_ISP_MOSI);
    PORT_DDR_CLR(HWPIN_ISP_MOSI);
    PORT_PIN_CLR(HWPIN_ISP_MOSI);   /* deactivate pullup */
#endif
    useHIDInterface = PORT_PIN_VALUE(HWPIN_JUMPER) ? 1 : 0;
#endif /* METABOARD_HARDWARE */
#elif ENABLE_HID_INTERFACE
    useHIDInterface = 1;
#elif !ENABLE_CDC_INTERFACE
#error "You must set either ENABLE_HID_INTERFACE or ENABLE_CDC_INTERFACE in hardware.h!"
#endif
}

/*
20 pin HVSP / PP connector:
    1 .... GND
    2 .... Vtarget
    3 .... HVSP SCI
    4 .... RESET
    16 ... HVSP SDO
    18 ... HVSP SII
    20 ... HVSP SDI

 * Timer usage:
 * Timer 0 [8 bit]:
 *   1/64 prescaler for timer interrupt
 * Timer 1 [16 bit]:
 *   PWM for voltage supply -> fastPWM mode
 *   f = 23.4 kHz -> prescaler = 1, 9 bit
 * Timer 2 [8 bit]:
 *   Clock generation for target device
 */

static void hardwareInit(void)
{
uchar   i;
uchar   portA = 0, portB = 0, portC = 0, portD = 0, ddrA = 0, ddrB = 0, ddrC = 0, ddrD = 0;

#if ENABLE_HVPROG
#if HW_VCC_INVERT
    UTIL_PBIT_SET(port, HWPIN_HVSP_SUPPLY);
#else
    UTIL_PBIT_CLR(port, HWPIN_HVSP_SUPPLY);
#endif
    UTIL_PBIT_SET(ddr, HWPIN_HVSP_SUPPLY);
    UTIL_PBIT_CLR(port, HWPIN_BOOST_OUT);
    UTIL_PBIT_CLR(ddr, HWPIN_BOOST_OUT);

#if HVPP_HARDWARE || FUSER_HARDWARE

    PORT_OUT(HWPORT_DATA) = 0; /* Clear digital pins 0-7 */
    PORT_DDR(HWPORT_DATA) = 0; /* set digital pins 0-7 as  inputs */

    UTIL_PBIT_CLR(ddr, HW_RDY);

    UTIL_PBIT_SET(ddr, HW_RSTH);  /* signal to level shifter for +12V  RESET */
    UTIL_PBIT_SET(ddr, HW_RSTL);  /* signal to level shifter for +12V !RESET */
    UTIL_PBIT_SET(ddr, HW_VCC);
    UTIL_PBIT_SET(ddr, HW_OE);
    UTIL_PBIT_SET(ddr, HW_WR);
    UTIL_PBIT_SET(ddr, HW_BS1);
    UTIL_PBIT_SET(ddr, HW_BS2);
    UTIL_PBIT_SET(ddr, HW_XA0);
    UTIL_PBIT_SET(ddr, HW_XA1);
    UTIL_PBIT_SET(ddr, HW_PAGEL);
    UTIL_PBIT_SET(ddr, HW_XTAL1);

    UTIL_PBIT_CLR(port, HWPIN_HVSP_HVRESET);
    UTIL_PBIT_SET(ddr, HWPIN_HVSP_HVRESET);

#endif

#if AVRDOPER_HARDWARE
    UTIL_PBIT_CLR(port, HWPIN_ADC_VTARGET);
    UTIL_PBIT_CLR(ddr, HWPIN_ADC_VTARGET);
    UTIL_PBIT_CLR(port, HWPIN_ADC_SMPS);
    UTIL_PBIT_CLR(ddr, HWPIN_ADC_SMPS);

#ifdef HWPIN_ISP_CLK
    UTIL_PBIT_CLR(port, HWPIN_ISP_CLK);
    UTIL_PBIT_SET(ddr, HWPIN_ISP_CLK);
#endif

#endif /* end avrdoper hardware */

#endif /* ENABLE_HVPROG */


#if METABOARD_HARDWARE
    UTIL_PBIT_SET(ddr, HWPIN_LED_RED);
    UTIL_PBIT_SET(port, HWPIN_LED_RED);
    /* keep all I/O pins and DDR bits for ISP on low level */
    UTIL_PBIT_CLR(port, HWPIN_ISP_SUPPLY1);
    UTIL_PBIT_CLR(port, HWPIN_ISP_SUPPLY2);
    UTIL_PBIT_SET(ddr, HWPIN_ISP_SUPPLY1);
    UTIL_PBIT_SET(ddr, HWPIN_ISP_SUPPLY2);


#ifdef HWPIN_ISP_DRIVER
    UTIL_PBIT_SET(port, HWPIN_ISP_TXD);
    UTIL_PBIT_SET(ddr, HWPIN_ISP_TXD);
    UTIL_PBIT_SET(port, HWPIN_ISP_RXD);
    UTIL_PBIT_CLR(ddr, HWPIN_ISP_RXD);


    UTIL_PBIT_CLR(port, HWPIN_ISP_RESET);
    UTIL_PBIT_SET(ddr, HWPIN_ISP_RESET);
#endif

#ifdef HWPIN_ISP_DRIVER
    UTIL_PBIT_CLR(port, HWPIN_ISP_DRIVER);
    UTIL_PBIT_SET(ddr, HWPIN_ISP_DRIVER);
#endif

#endif /* METABOARD_HARDWARE */


#if HVPP_HARDWARE || FUSER_HARDWARE
    UTIL_PBIT_SET(ddr, HWPIN_LED_RED);
    UTIL_PBIT_CLR(port, HWPIN_LED_RED);
    UTIL_PBIT_CLR(port, HWPIN_LED_GREEN);
    UTIL_PBIT_SET(ddr, HWPIN_LED_GREEN);

    UTIL_PBIT_CLR(port, HWPIN_ISP_SCK);
    UTIL_PBIT_CLR(ddr, HWPIN_ISP_SCK);
    UTIL_PBIT_CLR(port, HWPIN_ISP_MISO);
    UTIL_PBIT_CLR(ddr, HWPIN_ISP_MISO);
    UTIL_PBIT_CLR(port, HWPIN_ISP_MOSI);
    UTIL_PBIT_CLR(ddr, HWPIN_ISP_MOSI);
    UTIL_PBIT_CLR(port, HWPIN_ISP_RESET);
    UTIL_PBIT_CLR(ddr, HWPIN_ISP_RESET);

#if 0
    UTIL_PBIT_CLR(port, HWPIN_USB_DPLUS);
    UTIL_PBIT_CLR(ddr, HWPIN_USB_DPLUS);
    UTIL_PBIT_CLR(port, HWPIN_USB_DMINUS);
    UTIL_PBIT_CLR(ddr, HWPIN_USB_DMINUS);
#endif
    UTIL_PBIT_SET(port, HWPIN_JUMPER);
    UTIL_PBIT_CLR(ddr, HWPIN_JUMPER);
#endif








    PORTA = portA;
    DDRA = ddrA;
    PORTB = portB;
    DDRB = ddrB;
    PORTC = portC;
    DDRC = ddrC;
    PORTD = portD;
    DDRD = ddrD;

    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    i = 0;
    while(--i){         /* fake USB disconnect for > 250 ms */
        wdt_reset();
        _delay_ms(1);
    }
    usbDeviceConnect();
    /* timer 0 configuration: ~ 1.365 ms interrupt @ 12 MHz */
    TCCR0 = 3;              /* 1/64 prescaler */
#if 0
    TIMSK = (1 << TOIE0);   /* enable timer0 overflow interrupt */
#endif

#if USBASP_HARDWARE
    /* Do not configure Timer 1 for original USBasp hardware */
#elif METABOARD_HARDWARE
    /* timer 1 configuration (used for target clock): */
    TCCR1A = UTIL_BIN8(1000, 0010); /* OC1A = PWM out, OC1B disconnected */
    TCCR1B = UTIL_BIN8(0001, 1001); /* wgm 14: TOP = ICR1, prescaler = 1 */
    ICR1 = F_CPU / 1000000 - 1;     /* TOP value for 1 MHz */
    OCR1A = F_CPU / 2000000 - 1;    /* 50% duty cycle */
#else /* METABOARD_HARDWARE */
    /* timer 1 configuration (used for high voltage generator): 32 kHz PWM (9 bit) */
    TCCR1A = UTIL_BIN8(1000, 0010);  /* OC1A = PWM, OC1B disconnected, 9 bit */
    TCCR1B = UTIL_BIN8(0000, 1001);  /* 9 bit, prescaler=1 */
#if HVPP_HARDWARE || FUSER_HARDWARE
    OCR1A = 160;    /* 128: equals to duty cycle of 25% */
#else
    OCR1A = 1;      /* set duty cycle to minimum */
#endif
    
    /* timer 2 configuration (used for target clock) */
#ifdef TCCR2A
    TCCR2A = UTIL_BIN8(0000, 0010); /* OC2A disconnected, CTC mode (WGM 0,1) */
    TCCR2B = UTIL_BIN8(0000, 0001); /* prescaler=1, CTC mode (WGM 2) */
#else
    TCCR2 = UTIL_BIN8(0000, 1001);  /* OC2 disconnected, prescaler=1, CTC mode */
#endif
    OCR2 = 2;       /* should give 3 MHz clock */
#endif /* METABOARD_HARDWARE */
}

int main(void)
{
    /* disable JTAG first otherwise C port lines will not work */
    MCUCSR = 1<<JTD;
    MCUCSR = 1<<JTD;

    wdt_enable(WDTO_1S);
    odDebugInit();
    DBG1(0x00, 0, 0);
#if ENABLE_DEBUG_INTERFACE
    serialInit();
#endif
    readInterfaceType();
    hardwareInit();
    vregInit();
    usbInit();
    sei();
    DBG1(0x01, 0, 0);
    for(;;){    /* main event loop */
        wdt_reset();
        pushbutton_poll();
        usbPoll();
        stkPoll();
#if ENABLE_CDC_INTERFACE
        if(!useHIDInterface && usbInterruptIsReady()){
            static uchar sendEmptyFrame = 1, buffer[HW_CDC_PACKET_SIZE];
            /* start with empty frame because the host eats the first packet -- don't know why... */
            int c;
            uchar i = 0;
            while(i < HW_CDC_PACKET_SIZE && (c = stkGetTxByte()) >= 0){
                buffer[i++] = c;
            }
            if(i > 0 || sendEmptyFrame){
                sendEmptyFrame = i;     /* send an empty block after last data block to indicate transfer end */
                usbSetInterrupt(buffer, i);
            }
        }
#endif
#if USE_DCD_REPORTING && ENABLE_CDC_INTERFACE
        /* We need to report rx and tx carrier after open attempt */
        if(intr3Status != 0 && usbInterruptIsReady3()){
            static uchar serialStateNotification[8] = {0xa1, 0x20, 0, 0, 0, 0, 2, 0};
            static uchar serialStateData[2] = {3, 0};
            if(intr3Status == 2){
                usbSetInterrupt3(serialStateNotification, 8);
            }else{
                usbSetInterrupt3(serialStateData, 2);
            }
            intr3Status--;
        }
#endif
    }
    return 0;
}
