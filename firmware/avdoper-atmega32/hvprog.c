/*
 * Name: hvprog.c
 * Project: AVR-Doper
 * Author: Christian Starkjohann <cs@obdev.at>
 * Creation Date: 2006-07-07
 * Tabsize: 4
 * Copyright: (c) 2006 by Christian Starkjohann, all rights reserved.
 * License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 * Revision: $Id: hvprog.c 566 2008-04-26 14:21:47Z cs $
 */

#include "hardware.h"

#if ENABLE_HVPROG
#include <avr/io.h>
#include <avr/wdt.h>
#include <string.h> /* NULL */
#include "timer.h"
#include "utils.h"
#include "hvprog.h"


/* This module can handle high voltage serial and parallel programming. Serial
 * programming can be understood as a serial interface to parallel programming:
 * SDI represents programming data and SII a serial input for the control
 * input lines /OE, /WR, BS1, XA0, XA1, PAGEL, and BS2.
 * In order to handle both with similar code, we define the function
 * hvSetControlAndData() which takes care of the underlying mechanism.
 */

/* ------------------------------------------------------------------------- */

static uchar    progModeIsPp;   /* use parallel programming primitives */
static uchar    hvPollTimeout;

/*
Implementing High Voltage Parallel Programming:
4 functions have to be implemented for HVPP:
ppEnterProgmode()
    This function brings the port lines into the state required for programming.
ppLeaveProgmode()
    This function turns off all signals to the target device.
ppExecute()
    This function reads data, sets data and sets control lines according to
    the parameters passed to the function.
ppPoll()
    This function polls for "programming ready".
You may have to add global variables to communicate additional parameters
such as e.g. write pulse width. See the hvsp* function implementation for
more information.
*/

/* ------------------------------------------------------------------------- */

// #define CLKDELAY() asm("nop"); /* too fast for atmega32 */
// #define CLKDELAY() TIMER_US_DELAY(1) /* to fast for big adapter v17 */
#define CLKDELAY() TIMER_US_DELAY(5)
// #define CLKDELAY() TIMER_US_DELAY(100)
// #define CLKDELAY() timerMsDelay(1)
// #define CLKDELAY() clkdelay()

void clkdelay(void)
{
  uint16_t i;
  
  for(i = 0; i < 1000; i++)
    asm("nop");
}

static uchar hvspExecute(uchar ctlLines, uchar data)
{
uchar   cnt, r = 0;

    PORT_PIN_CLR(HWPIN_HVSP_SII);
    PORT_PIN_CLR(HWPIN_HVSP_SDI);
    CLKDELAY();
    PORT_PIN_SET(HWPIN_HVSP_SCI);
    cnt = 8;
    CLKDELAY();
    PORT_PIN_CLR(HWPIN_HVSP_SCI);
    do{
        r <<= 1;
        if(PORT_PIN_VALUE(HWPIN_HVSP_SDO))
            r |= 1;
        if(data & 0x80)
            PORT_PIN_SET(HWPIN_HVSP_SDI);
        else
            PORT_PIN_CLR(HWPIN_HVSP_SDI);
        if(ctlLines & 0x80)
            PORT_PIN_SET(HWPIN_HVSP_SII);
        else
            PORT_PIN_CLR(HWPIN_HVSP_SII);
        CLKDELAY();
        ctlLines <<= 1;
        PORT_PIN_SET(HWPIN_HVSP_SCI);
        data <<= 1;
        CLKDELAY();
        PORT_PIN_CLR(HWPIN_HVSP_SCI);
    }while(--cnt);
    PORT_PIN_CLR(HWPIN_HVSP_SII);
    PORT_PIN_CLR(HWPIN_HVSP_SDI);
    /* clock out two zeros */
    CLKDELAY();
    PORT_PIN_SET(HWPIN_HVSP_SCI);
    CLKDELAY();
    PORT_PIN_CLR(HWPIN_HVSP_SCI);
    CLKDELAY();
    PORT_PIN_SET(HWPIN_HVSP_SCI);
    CLKDELAY();
    PORT_PIN_CLR(HWPIN_HVSP_SCI);
    CLKDELAY();
    return r;
}

/* route from avr hvsp to our hardware hvpp */
uint8_t routelines(uint8_t lines)
{
#if HW_ROUTE_PAGEL_BS2
  uint8_t routed = lines & ~(HVCTL_BS2 | HVCTL_PAGEL | HVCTL_NA);
  /* BS2, PAGEL are wired differently, HW_RDY is on HVCTL BS2 */

  if( (lines & HVCTL_BS2) != 0)
    routed |= (1 << PORT_BIT(HW_BS2));

  if( (lines & HVCTL_PAGEL) != 0)
    routed |= (1 << PORT_BIT(HW_PAGEL));
  
  return routed;
#else
  /* set RDY bit for pull up */
  return lines | (1 << PORT_BIT(HW_RDY));
#endif
}

static uint8_t hvppExecute(uchar ctlLines, uchar data)
{
  uint8_t readbyte = PORT_IN(HWPORT_DATA);
  /* read byte which is left from previous command */
  
  if( (ctlLines & HVCTL_nOE) == 0)
  {
    /* before target chip will be configured to output something 
    ** set data lines to input (high impedance pull up) 
    */
    PORT_DDR(HWPORT_DATA) = 0;    /* input */
    PORT_OUT(HWPORT_DATA) = 0xFF; /* all to pull up */
    // CLKDELAY();
  }
  /* set control lines */  
  PORT_OUT(HW_OE) = routelines(ctlLines);
  if( (ctlLines & HVCTL_nOE) != 0)
  {
    /* after target chip was configured to input something,
    ** set data lines to output 
    */
    // CLKDELAY();
    PORT_DDR(HWPORT_DATA) = 0xFF; /* output */
  }
    
  if( (ctlLines & HV_NONE) != HV_NONE)
  {
    PORT_OUT(HWPORT_DATA) = data; /* set data */
    /* pulse clock */
    CLKDELAY();
    PORT_PIN_SET(HW_XTAL1);
    CLKDELAY();
    PORT_PIN_CLR(HW_XTAL1);
  }
  return readbyte;
}

/* This function applies 'data' to the data lines, 'ctlLines' to the control
 * lines and returns the status of the data lines BEFORE any control lines
 * were changed. These somewhat strange semantics are required for
 * compatibility with HV serial programming.
 */
static uchar hvSetControlAndData(uchar ctlLines, uchar data)
{
    if(progModeIsPp)
      return hvppExecute(ctlLines, data);
    else
      return hvspExecute(ctlLines, data);
}

/* ------------------------------------------------------------------------- */

#if 0
void    hvspEnterProgmode(stkEnterProgHvsp_t *param)
{
    progModeIsPp = 0;

    TCCR2 &= ~(1 << COM20);             /* clear toggle on compare match mode why ??? */

    PORT_PIN_CLR(HWPIN_HVSP_SDI);
    PORT_DDR_SET(HWPIN_HVSP_SDI);
    PORT_PIN_CLR(HWPIN_HVSP_SII);
    PORT_DDR_SET(HWPIN_HVSP_SII);
    PORT_PIN_CLR(HWPIN_HVSP_SCI);
    PORT_DDR_SET(HWPIN_HVSP_SCI);
    PORT_PIN_CLR(HWPIN_HVSP_SDO);
    PORT_DDR_SET(HWPIN_HVSP_SDO);

    PORT_PIN_CLR(HWPIN_HVSP_PRGEN0);
    PORT_PIN_CLR(HWPIN_HVSP_PRGEN1);
    PORT_PIN_CLR(HWPIN_HVSP_PRGEN2);
    PORT_DDR_SET(HWPIN_HVSP_PRGEN0);
    PORT_DDR_SET(HWPIN_HVSP_PRGEN1);
    PORT_DDR_SET(HWPIN_HVSP_PRGEN2);


    PORT_PIN_CLR(HWPIN_HVSP_HVRESET);
    PORT_PIN_SET(HWPIN_HVSP_RESET);   /* load reset to avoid initial voltage jump */

    PORT_DDR_SET(HWPIN_HVSP_HVRESET);
    PORT_DDR_SET(HWPIN_HVSP_RESET);

    PORT_DDR_SET(HWPIN_BOOST_OUT);    /* generate 12V */

#if HW_VCC_INVERT
    PORT_PIN_SET(HWPIN_HVSP_SUPPLY);  /* remove 5V supply */
#else
    PORT_PIN_CLR(HWPIN_HVSP_SUPPLY);  /* remove 5V supply */
#endif
    PORT_DDR_SET(HWPIN_HVSP_SUPPLY);

    timerMsDelay(1); /* wait enough to 12V to stabilize and target to turn off without supply */

    PORT_PIN_CLR(HWPIN_HVSP_RESET);
    PORT_PIN_SET(HWPIN_HVSP_HVRESET);
    
    TIMER_US_DELAY(10);

#if HW_VCC_INVERT
    PORT_PIN_CLR(HWPIN_HVSP_SUPPLY); /* turn on 5V supply after RESET 12V */
#else
    PORT_PIN_SET(HWPIN_HVSP_SUPPLY); /* turn on 5V supply after RESET 12V */
#endif
    
    TIMER_US_DELAY(20);  /* wait enough before removing PRGEN */
    PORT_DDR_CLR(HWPIN_HVSP_SDO); /* change direction SDO, now input */
    PORT_PIN_SET(HWPIN_HVSP_SDO); /* weak pull up. normally set */
    /* if by set SDO weak pull up device id is 0xFFFFFF
    ** and   clr SDO weak pull up device id is 0x000000 or some random bits
    ** it means that device didn't enter hvsp mode
    */

    PORT_DDR_CLR(HWPIN_HVSP_PRGEN0);    /* release output to prevent contention**/
    PORT_DDR_CLR(HWPIN_HVSP_PRGEN1);
    PORT_DDR_CLR(HWPIN_HVSP_PRGEN2);
    /* must enable weak pull ups */
    PORT_PIN_SET(HWPIN_HVSP_PRGEN0);
    PORT_PIN_SET(HWPIN_HVSP_PRGEN1);
    PORT_PIN_SET(HWPIN_HVSP_PRGEN2);
    /* datasheet says 50us minimum, but really
    ** it needs 100us for device to start responding
    */
    TIMER_US_DELAY(150);
}
#endif

#if 0
/* This is the new mechanism to enter prog mode which is closer to the method
 * described in the ATTiny45 data sheet, but it seems to fail on some of the
 * ATTiny45. We therefore stick with the old method, but leave this code
 * for reference.
 */
void    hvspEnterProgmode(stkEnterProgHvsp_t *param)
{
uchar i;

    progModeIsPp = 0;

    TCCR2 &= ~(1 << COM20);             /* clear toggle on compare match mode */

    PORT_PIN_CLR(HWPIN_HVSP_HVRESET);
    PORT_PIN_SET(HWPIN_HVSP_RESET);     /* load reset to avoid initial voltage jump */
    PORT_DDR_SET(HWPIN_BOOST_OUT);
#if HW_VCC_INVERT
    PORT_PIN_SET(HWPIN_HVSP_SUPPLY);
#else
    PORT_PIN_CLR(HWPIN_HVSP_SUPPLY);
#endif

    PORT_PIN_CLR(HWPIN_HVSP_PRGEN0);
    PORT_PIN_CLR(HWPIN_HVSP_PRGEN1);
    PORT_PIN_CLR(HWPIN_HVSP_PRGEN2);
    PORT_DDR_SET(HWPIN_HVSP_PRGEN0);
    PORT_DDR_SET(HWPIN_HVSP_PRGEN1);
    PORT_DDR_SET(HWPIN_HVSP_PRGEN2);

    PORT_PIN_CLR(HWPIN_HVSP_SDI);
    PORT_DDR_SET(HWPIN_HVSP_SDI);
    PORT_PIN_CLR(HWPIN_HVSP_SII);
    PORT_DDR_SET(HWPIN_HVSP_SII);
    PORT_PIN_CLR(HWPIN_HVSP_SCI);
    PORT_DDR_SET(HWPIN_HVSP_SCI);
    PORT_PIN_CLR(HWPIN_HVSP_SDO);
    PORT_DDR_SET(HWPIN_HVSP_SDO);

    timerMsDelay(2);

    PORT_PIN_CLR(HWPIN_HVSP_HVRESET);
    PORT_PIN_SET(HWPIN_HVSP_RESET);

#if 1
    for(i = 0; i < 6; i++){    /* ATTiny[248]4 data sheet says: toggle SCI at least 6 times */
        PORT_PIN_SET(HWPIN_HVSP_SCI); /* this toggles output! */
        CLKDELAY();
        PORT_PIN_CLR(HWPIN_HVSP_SCI);
        CLKDELAY();
    }
#endif
    PORT_PIN_CLR(HWPIN_HVSP_RESET);
    PORT_PIN_SET(HWPIN_HVSP_HVRESET);

    TIMER_US_DELAY(10);
#if HW_VCC_INVERT
    PORT_PIN_CLR(HWPIN_HVSP_SUPPLY);
#else
    PORT_PIN_SET(HWPIN_HVSP_SUPPLY);
#endif
    TIMER_US_DELAY(30);

    PORT_DDR_CLR(HWPIN_HVSP_PRGEN0);    /* release output to prevent contention**/
    PORT_DDR_CLR(HWPIN_HVSP_PRGEN1);
    PORT_DDR_CLR(HWPIN_HVSP_PRGEN2);
    PORT_DDR_CLR(HWPIN_HVSP_SDO);       /* prevent contention */
    PORT_PIN_SET(HWPIN_HVSP_PRGEN0);
    PORT_PIN_SET(HWPIN_HVSP_PRGEN1);
    PORT_PIN_SET(HWPIN_HVSP_PRGEN2);
    PORT_PIN_SET(HWPIN_HVSP_SDO);
    TIMER_US_DELAY(50);
}
#endif

#if 1
/* original code from avr-doper works for attiny84, attiny85 */
void    hvspEnterProgmode(stkEnterProgHvsp_t *param)
{
    progModeIsPp = 0;
    uint8_t i;

    PORT_PIN_CLR(HWPIN_HVSP_HVRESET);   /* remove 12V reset */
    PORT_DDR_SET(HWPIN_HVSP_HVRESET);   /* output for hvreset pin just in case */
    PORT_PIN_SET(HWPIN_HVSP_RESET);     /* reset active (0V) to avoid initial voltage jump */
    PORT_DDR_SET(HWPIN_HVSP_RESET);     /* output for reset pin just in case */
    PORT_DDR_SET(HWPIN_BOOST_OUT);      /* generate 12V */
#if HW_VCC_INVERT
    PORT_PIN_SET(HWPIN_HVSP_SUPPLY);    /* remove 5V supply from chip */
#else
    PORT_PIN_CLR(HWPIN_HVSP_SUPPLY);    /* remove 5V supply from chip */
#endif
    
    PORT_PIN_CLR(HWPIN_HVSP_PRGEN0);
    PORT_PIN_CLR(HWPIN_HVSP_PRGEN1);
    PORT_PIN_CLR(HWPIN_HVSP_PRGEN2);
    PORT_DDR_CLR(HWPIN_HVSP_PRGEN0);
    PORT_DDR_CLR(HWPIN_HVSP_PRGEN1);
    PORT_DDR_CLR(HWPIN_HVSP_PRGEN2);

    PORT_PIN_CLR(HWPIN_HVSP_SDI);
    PORT_DDR_SET(HWPIN_HVSP_SDI);
    PORT_PIN_CLR(HWPIN_HVSP_SII);
    PORT_DDR_SET(HWPIN_HVSP_SII);
    PORT_PIN_CLR(HWPIN_HVSP_SCI);
    PORT_DDR_SET(HWPIN_HVSP_SCI);
    PORT_PIN_CLR(HWPIN_HVSP_SDO);
    PORT_DDR_CLR(HWPIN_HVSP_SDO);

    timerMsDelay(40); /* let capactitor charge to 12V */

    TCCR2 &= ~(1 << COM20);             /* clear toggle on compare match mode */
    PORT_PIN_SET(HWPIN_HVSP_RESET);     /* activate reset to 0V */
    #if HW_VCC_INVERT
    PORT_PIN_SET(HWPIN_HVSP_SUPPLY);    /* remove 5V supply from chip */
    #else
    PORT_PIN_CLR(HWPIN_HVSP_SUPPLY);    /* remove power supply */
    #endif
    PORT_DDR_SET(HWPIN_HVSP_SDO);
    /* wait enough for target to loose power */
    TIMER_US_DELAY(40);
    PORT_PIN_CLR(HWPIN_HVSP_RESET);     /* release reset from 0V (later it needs to rise towards 12V) */
    for(i = 0; i < 100; i++)
    {
      /* keep retrying to enable HVSP_SUPPLY because output is weak, 
      ** directly from AVR (should be done with a transistor) 
      ** and can collide with target VCC capacitor and bounce off
      */
      #if HW_VCC_INVERT
      PORT_PIN_CLR(HWPIN_HVSP_SUPPLY);    /* activate power supply */
      #else
      PORT_PIN_SET(HWPIN_HVSP_SUPPLY);    /* activate power supply */
      #endif
    }
    PORT_PIN_SET(HWPIN_HVSP_HVRESET); /* jump to 12V reset */
    TIMER_US_DELAY(10);
    PORT_DDR_CLR(HWPIN_HVSP_SDO);  /* set SDO to input to avoid collision */
    PORT_PIN_SET(HWPIN_HVSP_SDO);  /* enable SDO pull up */
    // PORT_PIN_CLR(HWPIN_HVSP_SDO);  /* disable SDO pull up */
    /* if by set SDO weak pull up device id is 0xFFFFFF
    ** and   clr SDO weak pull up device id is 0x000000 or some random bits
    ** it means that device didn't enter hvsp mode
    */
    TIMER_US_DELAY(300);
}
#endif


void    hvspLeaveProgmode(stkLeaveProgHvsp_t *param)
{
    PORT_PIN_CLR(HWPIN_HVSP_HVRESET);
    PORT_PIN_SET(HWPIN_HVSP_RESET);

    PORT_PIN_CLR(HWPIN_HVSP_SDI);
    PORT_DDR_CLR(HWPIN_HVSP_SDI);
    PORT_PIN_CLR(HWPIN_HVSP_SII);
    PORT_DDR_CLR(HWPIN_HVSP_SII);
    PORT_PIN_CLR(HWPIN_HVSP_SCI);
    PORT_DDR_CLR(HWPIN_HVSP_SCI);
    PORT_PIN_CLR(HWPIN_HVSP_SDO);
    PORT_DDR_CLR(HWPIN_HVSP_SDO);

    #if HW_VCC_INVERT
    PORT_PIN_SET(HWPIN_HVSP_SUPPLY);
    #else
    PORT_PIN_CLR(HWPIN_HVSP_SUPPLY);
    #endif
    PORT_DDR_CLR(HWPIN_BOOST_OUT);
#if 0
    PORT_PIN_CLR(HWPIN_HVSP_RESET);
#endif
    PORT_PIN_CLR(HWPIN_LED_RED);
    PORT_PIN_CLR(HWPIN_LED_GREEN);
}

/* ------------------------------------------------------------------------- */

#if 0
/* help functions for parallel program mode from hvpp.c
** in hvpp.c they work, here they don't work...
*/
/* xa1 = 1 loads command
** xa1 = 0 loads address
*/
void sendca(uint8_t command)  // Send command or address to target AVR
{
  PORT_PIN_SET(HW_OE); /* disable output */
  PORT_DDR(HWPORT_DATA) = 0xFF; /* set digital pins 0-7 as outputs */
  PORT_OUT(HWPORT_DATA) = command;
  // Set controls for command mode
  PORT_PIN_CLR(HW_XA0);
  PORT_PIN_CLR(HW_BS1);
  timerMsDelay(1);
  PORT_PIN_SET(HW_XTAL1);  // pulse XTAL to send command to target
  timerMsDelay(1);
  PORT_PIN_CLR(HW_XTAL1);
}

void sendcmd(uint8_t command)  // Send command to target AVR
{
  PORT_PIN_SET(HW_XA1); /* load command */
  sendca(command);
}

void sendaddr(uint8_t addr)  // Send command to target AVR
{
  PORT_PIN_CLR(HW_XA1); /* load address */
  sendca(addr);
}

/* reads device's signature */
uint8_t read_signature1(uint8_t i)
{
    uint8_t value;
   
    sendcmd(0x08); /* read signature */
    sendaddr(i);   /* the address */
    PORT_DDR(HWPORT_DATA) = 0; /* set digital pins 0-7 as inputs */
    PORT_OUT(HWPORT_DATA) = 0xFF; /* enable pull ups */
    PORT_PIN_CLR(HW_OE);  /* activate avr output byte */
    PORT_PIN_CLR(HW_BS1);
    timerMsDelay(1);
    value = PORT_IN(HWPORT_DATA); /* read byte */
    PORT_PIN_SET(HW_OE); /* deactivate avr output */
    return value;
}
#endif

#if 1
void    ppEnterProgmode(stkEnterProgPp_t *param)
{
  progModeIsPp = 1;

  PORT_DDR(HWPORT_DATA) = 0; /* set digital pins 0-7 as  inputs */
  PORT_OUT(HWPORT_DATA) = 0;    /* Clear digital pins 0-7 */

  PORT_DDR_CLR(HW_RDY);

  PORT_DDR_SET(HW_RSTH);  /* signal to level shifter for +12V  RESET */
  PORT_DDR_SET(HW_RSTL);  /* signal to level shifter for +12V !RESET */

  PORT_DDR_SET(HWPIN_BOOST_OUT); /* generate 12V */
  PORT_PIN_CLR(HW_RSTH); /* this disconnects +12V */
  PORT_PIN_SET(HW_RSTL); /* this pulls down RESET to 0V */

  PORT_DDR_SET(HW_VCC);
  PORT_DDR_SET(HW_OE);
  PORT_DDR_SET(HW_WR);
  PORT_DDR_SET(HW_BS1);
  PORT_DDR_SET(HW_BS2);
  PORT_DDR_SET(HW_XA0);
  PORT_DDR_SET(HW_XA1);
  PORT_DDR_SET(HW_PAGEL);
  PORT_DDR_SET(HW_XTAL1);

  PORT_PIN_CLR(HW_RDY); /* no weak pull up */

  /* remove power supply */
#if HW_VCC_INVERT
  PORT_PIN_SET(HW_VCC);
#else
  PORT_PIN_CLR(HW_VCC);
#endif

  // Initialize all pins including those important to enter programming mode

  // Enter programming mode
  PORT_PIN_CLR(HW_WR);   /* remove !OE and !WR because it will drive VCC line */
  PORT_PIN_CLR(HW_OE);

  /* drive all pins to 0V to avoid powering thru them
  ** and some pines must be 0V according to datasheet 
  */
  PORT_PIN_CLR(HW_PAGEL);
  PORT_PIN_CLR(HW_XA1);
  PORT_PIN_CLR(HW_XA0);
  PORT_PIN_CLR(HW_BS1);
  PORT_PIN_CLR(HW_BS2);

  PORT_PIN_CLR(HW_XTAL1);
  
  /* start timed sequence for HVPP */
  timerMsDelay(5); /* wait for 12V to stabilize and target to turn off without supply */

  /* Apply VCC to start programming process */
#if HW_VCC_INVERT
  PORT_PIN_CLR(HW_VCC);  
#else
  PORT_PIN_SET(HW_VCC);  
#endif

  PORT_PIN_SET(HW_RDY);  /* weak pull up for RDY */

  TIMER_US_DELAY(30);    /* wait 30 us (20-60 us after VCC) */
  
  PORT_PIN_CLR(HW_RSTL); /* remove reset pull down */
  PORT_PIN_SET(HW_WR);   /* apply !OE and !WR */
  PORT_PIN_SET(HW_OE);
  PORT_PIN_SET(HW_RSTH); /* Apply 12V to !RESET thru level shifter */

  timerMsDelay(1);
  /* Now we're in programming mode */
}
#else
/* as described for atmega32 */
void    ppEnterProgmode(stkEnterProgPp_t *param)
{
  uint8_t i;
  
  progModeIsPp = 1;

  PORT_DDR(HWPORT_DATA) = 0; /* set digital pins 0-7 as  inputs */
  PORT_OUT(HWPORT_DATA) = 0;    /* Clear digital pins 0-7 */

  PORT_DDR_CLR(HW_RDY);

  PORT_DDR_SET(HW_RSTH);  /* signal to level shifter for +12V  RESET */
  PORT_DDR_SET(HW_RSTL);  /* signal to level shifter for +12V !RESET */

  PORT_DDR_SET(HWPIN_BOOST_OUT); /* generate 12V */
  PORT_PIN_CLR(HW_RSTH); /* this disconnects +12V */
  PORT_PIN_SET(HW_RSTL); /* this pulls down RESET to 0V */

  PORT_DDR_SET(HW_VCC);
  PORT_DDR_SET(HW_OE);
  PORT_DDR_SET(HW_WR);
  PORT_DDR_SET(HW_BS1);
  PORT_DDR_SET(HW_BS2);
  PORT_DDR_SET(HW_XA0);
  PORT_DDR_SET(HW_XA1);
  PORT_DDR_SET(HW_PAGEL);
  PORT_DDR_SET(HW_XTAL1);

  PORT_PIN_CLR(HW_RDY); /* no weak pull up */

  /* remove power supply */
#if HW_VCC_INVERT
  PORT_PIN_SET(HW_VCC);
#else
  PORT_PIN_CLR(HW_VCC);
#endif

  // Initialize all pins including those important to enter programming mode

  // Enter programming mode
  PORT_PIN_CLR(HW_WR);   /* remove !OE and !WR because it will drive VCC line */
  PORT_PIN_CLR(HW_OE);

  /* drive all pins to 0V to avoid powering thru them
  ** and some pines must be 0V according to datasheet 
  */
  PORT_PIN_CLR(HW_PAGEL);
  PORT_PIN_CLR(HW_XA1);
  PORT_PIN_CLR(HW_XA0);
  PORT_PIN_CLR(HW_BS1);
  PORT_PIN_CLR(HW_BS2);

  PORT_PIN_CLR(HW_XTAL1);
  
  /* start timed sequence for HVPP */
  timerMsDelay(1); /* wait for 12V to stabilize and target to turn off without supply */

  /* Apply VCC to start programming process */
#if HW_VCC_INVERT
  PORT_PIN_CLR(HW_VCC);
#else
  PORT_PIN_SET(HW_VCC);  
#endif

  PORT_PIN_SET(HW_RDY);  /* weak pull up for RDY */

  /* pulse clock at least 16 times, otherwise errors by reading fuses */
  for(i = 0; i < 16; i++)
  {
    PORT_PIN_SET(HW_XTAL1);
    CLKDELAY();
    PORT_PIN_CLR(HW_XTAL1);
    CLKDELAY();
  }

  TIMER_US_DELAY(30);    /* wait 30 us (20-60 us after VCC) */
  
  PORT_PIN_CLR(HW_RSTL); /* remove pull down */
  PORT_PIN_SET(HW_WR);   /* apply !OE and !WR */
  PORT_PIN_SET(HW_OE);
  PORT_PIN_SET(HW_RSTH); /* Apply 12V to !RESET thru level shifter */

  timerMsDelay(1);
  /* Now we're in programming mode */
}
#endif


void    ppLeaveProgmode(stkLeaveProgPp_t *param)
{
  // Exit programming mode
  PORT_PIN_CLR(HW_RSTH);  /* turn off +12V */
  PORT_PIN_SET(HW_RSTL);  /* turn on pull down */

  PORT_PIN_CLR(HW_OE);
  PORT_PIN_CLR(HW_WR);
  PORT_PIN_CLR(HW_PAGEL);
  PORT_PIN_CLR(HW_XA1);
  PORT_PIN_CLR(HW_XA0);
  PORT_PIN_CLR(HW_BS1);
  PORT_PIN_CLR(HW_BS2);

  PORT_DDR_CLR(HW_OE);
  PORT_DDR_CLR(HW_WR);
  PORT_DDR_CLR(HW_PAGEL);
  PORT_DDR_CLR(HW_XA1);
  PORT_DDR_CLR(HW_XA0);
  PORT_DDR_CLR(HW_BS1);
  PORT_DDR_CLR(HW_BS2);

  // Turn off all outputs
  PORT_OUT(HWPORT_DATA)  = 0x00;

  PORT_DDR(HWPORT_DATA)  = 0x00; /* remove output */

  timerMsDelay(1); /* device must reset */

  /* remove power supply */
#if HW_VCC_INVERT
  PORT_PIN_SET(HW_VCC);
#else
  PORT_PIN_CLR(HW_VCC);
#endif

  PORT_DDR_CLR(HWPIN_BOOST_OUT); /* turn off 12V generator */

  PORT_PIN_CLR(HWPIN_LED_RED);
  PORT_PIN_CLR(HWPIN_LED_GREEN);

  /* device must be either left without power long enough
  */
  timerMsDelay(1); 
}

void hvSetProgmode(uint8_t is_parallel)
{
  hvPollTimeout = 90;
  progModeIsPp = is_parallel;
}

void hvEnterProgmode()
{
  void *param = NULL;
  
  if(progModeIsPp)
    ppEnterProgmode(param);
  else
    hvspEnterProgmode(param);
}

void hvLeaveProgmode()
{
  void *param = NULL;
  
  if(progModeIsPp)
    ppLeaveProgmode(param);
  else
    hvspLeaveProgmode(param);

}



/* ------------------------------------------------------------------------- */

static uchar    hvspPoll(void)
{
  uchar   rval = STK_STATUS_CMD_OK;

  timerSetupTimeout(hvPollTimeout);
  while(!PORT_PIN_VALUE(HWPIN_HVSP_SDO)){
    if(timerTimeoutOccurred()){
      rval = STK_STATUS_CMD_TOUT;
      break;
    }
  }
  return rval;
}

static uchar    ppPoll(void)
{
  uchar   rval = STK_STATUS_CMD_OK;

  timerSetupTimeout(hvPollTimeout);
  while(!PORT_PIN_VALUE(HW_RDY)){
    if(timerTimeoutOccurred()){
      rval = STK_STATUS_CMD_TOUT;
      break;
    }
  }
  return rval;
}

static uchar    hvPoll(void)
{
    if(progModeIsPp)
      return ppPoll();
    else
      return hvspPoll();
}

/* ------------------------------------------------------------------------- */

uchar   hvChipErase(uchar eraseTime)
{
    uchar rval = STK_STATUS_CMD_OK;

    PORT_PIN_CLR(HWPIN_LED_GREEN);
    PORT_PIN_SET(HWPIN_LED_RED);

    hvSetControlAndData(HVCTL(HV_CMD,  HV_LOW, HV_NORW), HVCMD_CHIP_ERASE);
    hvSetControlAndData(HVCTL(HV_NONE, HV_LOW, HV_WRITE), 0);
    hvSetControlAndData(HVCTL(HV_NONE, HV_LOW, HV_NORW), 0);
    if(hvPollTimeout){
        rval = hvPoll();
    }else{
        timerMsDelay(eraseTime);
    }
    hvSetControlAndData(HVCTL(HV_CMD, HV_LOW, HV_NORW), HVCMD_NOP);
    return rval;
}

uchar   hvspChipErase(stkChipEraseHvsp_t *param)
{
    PORT_PIN_CLR(HWPIN_LED_GREEN);
    PORT_PIN_SET(HWPIN_LED_RED);

    hvPollTimeout = param->pollTimeout;
    return hvChipErase(param->eraseTime);
}

uchar   ppChipErase(stkChipErasePp_t *param)
{
    PORT_PIN_CLR(HWPIN_LED_GREEN);
    PORT_PIN_SET(HWPIN_LED_RED);
    /* ### set pulse width to global variable */
    hvPollTimeout = param->pollTimeout;
    return hvChipErase(10);
}

/* ------------------------------------------------------------------------- */

#define MODEMASK_PAGEMODE   1
#define MODEMASK_LAST_PAGE  0x40
#define MODEMASK_FLASH_PAGE 0x80

/* len == 0 means 256 bytes */
static uchar    hvProgramMemory(uchar *data, uchar len, uchar mode, uchar isEeprom)
{
uchar   x, pageMask = 0xff, rval = STK_STATUS_CMD_OK;

    x = -(mode >> 1) & 7;
    while(x--)  /* pageMask >>= x is less efficient */
        pageMask >>= 1;
    if(!isEeprom)
        pageMask >>= 1;
    hvSetControlAndData(HVCTL(HV_CMD, HV_LOW, HV_NORW), isEeprom ? HVCMD_WRITE_EEPROM : HVCMD_WRITE_FLASH);
    do{
        wdt_reset();
        hvSetControlAndData(HVCTL(HV_ADDR, HV_LOW, HV_NORW), stkAddress.bytes[0]);
        if(mode & MODEMASK_PAGEMODE){
            hvSetControlAndData(HVCTL(HV_DATA, HV_LOW, HV_NORW), *data++);
            if(isEeprom){
                hvSetControlAndData(HVCTL(HV_PAGEL, HV_LOW, HV_NORW), 0);
                hvSetControlAndData(HVCTL(HV_NONE, HV_LOW, HV_NORW), 0);
            }else{
                hvSetControlAndData(HVCTL(HV_DATA, HV_HIGH, HV_NORW), *data++);
                hvSetControlAndData(HVCTL(HV_PAGEL, HV_HIGH, HV_NORW), 0);
                hvSetControlAndData(HVCTL(HV_NONE, HV_HIGH, HV_NORW), 0);
            }
            x = stkAddress.bytes[0] + 1;    /* enforce byte wide operation */
            if((x & pageMask) == 0 && (mode & MODEMASK_FLASH_PAGE)){
                hvSetControlAndData(HVCTL(HV_ADDR, HV_HIGH, HV_NORW), stkAddress.bytes[1]);
                hvSetControlAndData(HVCTL(HV_NONE, HV_LOW, HV_WRITE), 0);
                hvSetControlAndData(HVCTL(HV_NONE, HV_LOW, HV_NORW), 0);
                rval = hvPoll();
            }
        }else{
            hvSetControlAndData(HVCTL(HV_ADDR, HV_HIGH, HV_NORW), stkAddress.bytes[1]);
            hvSetControlAndData(HVCTL(HV_DATA, HV_LOW, HV_NORW), *data++);
            if(isEeprom){
                hvSetControlAndData(HVCTL(HV_PAGEL, HV_LOW, HV_NORW), 0);
                hvSetControlAndData(HVCTL(HV_NONE, HV_LOW, HV_WRITE), 0);
                hvSetControlAndData(HVCTL(HV_NONE, HV_LOW, HV_NORW), 0);
                if((rval = hvPoll()) != STK_STATUS_CMD_OK)
                    break;
            }else{
                hvSetControlAndData(HVCTL(HV_NONE, HV_LOW, HV_WRITE), 0);
                hvSetControlAndData(HVCTL(HV_NONE, HV_LOW, HV_NORW), 0);
                if((rval = hvPoll()) != STK_STATUS_CMD_OK)
                    break;
                hvSetControlAndData(HVCTL(HV_DATA, HV_HIGH, HV_NORW), *data++);
                hvSetControlAndData(HVCTL(HV_NONE, HV_HIGH, HV_WRITE), 0);
                hvSetControlAndData(HVCTL(HV_NONE, HV_HIGH, HV_NORW), 0);
            }
            if((rval = hvPoll()) != STK_STATUS_CMD_OK)
                break;
        }
        stkIncrementAddress();
        if(!isEeprom && !--len) /* should not happen, but we would hang indefinitely */
            break;
    }while(--len);
    if(!(mode & MODEMASK_PAGEMODE) || (mode & MODEMASK_LAST_PAGE))
        hvSetControlAndData(HVCTL(HV_CMD, HV_LOW, HV_NORW), HVCMD_NOP);
    return rval;
}

uchar   hvspProgramMemory(stkProgramFlashHvsp_t *param, uchar isEeprom)
{
    PORT_PIN_CLR(HWPIN_LED_GREEN);
    PORT_PIN_SET(HWPIN_LED_RED);
    hvPollTimeout = param->pollTimeout;
    return hvProgramMemory(param->data, param->numBytes[1], param->mode, isEeprom);
}

/* ------------------------------------------------------------------------- */

static void hvReadMemory(uchar *data, uint len, uchar isEeprom)
{
    hvSetControlAndData(HVCTL(HV_CMD, HV_LOW, HV_NORW), isEeprom ? HVCMD_READ_EEPROM : HVCMD_READ_FLASH);
    while(len-- > 0){
        wdt_reset();
        hvSetControlAndData(HVCTL(HV_ADDR, HV_LOW, HV_NORW), stkAddress.bytes[0]);
        hvSetControlAndData(HVCTL(HV_ADDR, HV_HIGH, HV_NORW), stkAddress.bytes[1]);
        hvSetControlAndData(HVCTL(HV_NONE, HV_LOW, HV_READ), 0);
        *data++ = hvSetControlAndData(HVCTL(HV_NONE, HV_LOW, HV_NORW), 0);
        if(!isEeprom){
            hvSetControlAndData(HVCTL(HV_NONE, HV_HIGH, HV_READ), 0);
            *data++ = hvSetControlAndData(HVCTL(HV_NONE, HV_HIGH, HV_NORW), 0);
            len--;
        }
        stkIncrementAddress();
    }
    *data++ = STK_STATUS_CMD_OK; /* status2 */
}

uint    hvspReadMemory(stkReadFlashHvsp_t *param, stkReadFlashHvspResult_t *result, uchar isEeprom)
{
utilWord_t  numBytes;

    PORT_PIN_CLR(HWPIN_LED_RED);
    PORT_PIN_SET(HWPIN_LED_GREEN);

    numBytes.bytes[1] = param->numBytes[0];
    numBytes.bytes[0] = param->numBytes[1];
    result->status1 = STK_STATUS_CMD_OK;
    hvReadMemory(result->data, numBytes.word, isEeprom);
    return numBytes.word + 2;
}

/* ------------------------------------------------------------------------- */

uchar    hvProgramFuse(uchar value, uchar cmd, uchar highLow)
{
    hvSetControlAndData(HVCTL(HV_CMD,  HV_LOW,  HV_NORW),  cmd);
    hvSetControlAndData(HVCTL(HV_DATA, HV_LOW,  HV_NORW),  value);
    hvSetControlAndData(HVCTL(HV_NONE, highLow, HV_WRITE), 0);
    hvSetControlAndData(HVCTL(HV_NONE, highLow, HV_NORW),  0);
    hvPoll();
    hvSetControlAndData(HVCTL(HV_CMD, HV_LOW, HV_NORW), HVCMD_NOP);
    return STK_STATUS_CMD_OK; /* atmega8 Poll timeouts here but fuses are written correctly */
}

uchar   hvspProgramFuse(stkProgramFuseHvsp_t *param)
{
uchar   highLow;

    PORT_PIN_CLR(HWPIN_LED_GREEN);
    PORT_PIN_SET(HWPIN_LED_RED);

    hvPollTimeout = param->pollTimeout;
    if(param->fuseAddress == 0){
        highLow = HV_LOW;
    }else if(param->fuseAddress == 1){
        highLow = HV_HIGH;
    }else{
        highLow = HV_EXT;
    }
    return hvProgramFuse(param->fuseByte, HVCMD_WRITE_FUSE, highLow);
}

uchar   hvspProgramLock(stkProgramFuseHvsp_t *param)
{
    PORT_PIN_CLR(HWPIN_LED_GREEN);
    PORT_PIN_SET(HWPIN_LED_RED);

    hvPollTimeout = param->pollTimeout;
    return hvProgramFuse(param->fuseByte, HVCMD_WRITE_LOCK, HV_LOW);
}

uchar   ppProgramFuse(stkProgramFusePp_t *param)
{
uchar   highLow;

    PORT_PIN_CLR(HWPIN_LED_GREEN);
    PORT_PIN_SET(HWPIN_LED_RED);

    /* ### set pulse width to global variable */
    hvPollTimeout = param->pollTimeout;
    if(param->address == 0){
        highLow = HV_LOW;
    }else if(param->address == 1){
        highLow = HV_HIGH;
    }else if(param->address == 2){
        highLow = HV_EXT;
    }else{
        highLow = HV_EXT2;
    }
    return hvProgramFuse(param->data, HVCMD_WRITE_FUSE, highLow);
}

uchar   ppProgramLock(stkProgramFusePp_t *param)
{
    PORT_PIN_CLR(HWPIN_LED_GREEN);
    PORT_PIN_SET(HWPIN_LED_RED);

    /* ### set pulse width to global variable */
    hvPollTimeout = param->pollTimeout;
    return hvProgramFuse(param->data, HVCMD_WRITE_LOCK, HV_LOW);
}

/* ------------------------------------------------------------------------- */

uchar    hvReadFuse(uchar highLow)
{
    PORT_PIN_CLR(HWPIN_LED_RED);
    PORT_PIN_SET(HWPIN_LED_GREEN);

    hvSetControlAndData(HVCTL(HV_CMD, HV_LOW, HV_NORW), HVCMD_READ_FUSELCK);
    hvSetControlAndData(HVCTL(HV_NONE, highLow, HV_READ), 0);
    return hvSetControlAndData(HVCTL(HV_NONE, highLow, HV_NORW), 0);
}

uchar   hvspReadFuse(stkReadFuseHvsp_t *param)
{
uchar   highLow;

    PORT_PIN_CLR(HWPIN_LED_RED);
    PORT_PIN_SET(HWPIN_LED_GREEN);

    if(param->fuseAddress == 0){
        highLow = HV_LOW;
    }else if(param->fuseAddress == 1){
        highLow = HV_EXT2;
    }else if(param->fuseAddress == 2){
        highLow = HV_EXT;
    }else{
        return STK_STATUS_CMD_FAILED;   /* ### not implemented yet -- which data sheet documents this? */
    }
    return hvReadFuse(highLow);
}

uchar   hvspReadLock(void)
{
    return hvReadFuse(HV_HIGH);
}

/* ------------------------------------------------------------------------- */

uchar    hvReadSignature(uchar addr, uchar highLow)
{
    PORT_PIN_CLR(HWPIN_LED_RED);
    PORT_PIN_SET(HWPIN_LED_GREEN);
#if 0
 /* alternative code - specific PP commands that work on this hardware 
 ** used to test if the hardware works before testing complex HVSP HVPP code
 */
 if(progModeIsPp)
 {
   return read_signature1(addr);
 }
 else
#endif
 {
    /* this works for both HVSP and HVPP */
    hvSetControlAndData(HVCTL(HV_CMD, HV_LOW, HV_NORW), HVCMD_READ_SIGCAL);
    hvSetControlAndData(HVCTL(HV_ADDR, HV_LOW, HV_NORW), addr);
    hvSetControlAndData(HVCTL(HV_NONE, highLow, HV_READ), 0);
    return hvSetControlAndData(HVCTL(HV_NONE, highLow, HV_NORW), 0);
 }
}

uchar   hvspReadSignature(stkReadFuseHvsp_t *param)
{
    PORT_PIN_CLR(HWPIN_LED_RED);
    PORT_PIN_SET(HWPIN_LED_GREEN);
    return hvReadSignature(param->fuseAddress, HV_LOW);
}

uchar   hvspReadOsccal(void)
{
    PORT_PIN_CLR(HWPIN_LED_RED);
    PORT_PIN_SET(HWPIN_LED_GREEN);
    return hvReadSignature(0, HV_HIGH);
}

/* ------------------------------------------------------------------------- */

#endif /* ENABLE_HVPROG */
