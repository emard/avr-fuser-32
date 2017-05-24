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
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <string.h> /* memcmp functions */
#include "timer.h"
#include "utils.h"
#include "hvprog.h"
#include "fusereset.h"
#include "fusedb.h"

uint8_t startbutton = 0;
uint16_t waitpush = 0; /* no of cycles to wait for next button push */

uint8_t signature[3]; /* current device signature */
struct fuses fuses[1]; /* current device fuses */
struct fuserec default_fuserec; /* default fuserec from database */

uint8_t iseof(uint8_t s[])
{
  return (s[0] & s[1] & s[2]) == 0xFF;
}

/* consult database in EEPROM of 
** device IDs and factory default fuses
*/
void search_device_db(uint8_t s[], struct fuses *f)
{
  uint8_t i, found = 0;

  struct fuserec *fuserec = &default_fuserec;
  
  if( iseof(s) )
  {
    /* no device connected, turn off both leds */
    PORT_PIN_CLR(HWPIN_LED_RED);
    PORT_PIN_CLR(HWPIN_LED_GREEN);
    return;
  }
  
  PORT_PIN_SET(HWPIN_LED_RED); /* turn on red */

  /* loop limit: number of records that fit max size of eeprom */
  for(i = 0; i < E2END/sizeof(struct fuserec); i++)
  {
    eeprom_read_block(fuserec, &(fusedb[i]), sizeof(struct fuserec));
    if(iseof(fuserec->signature))
      break; /* exit for-loop immediately */
    if( memcmp(s, fuserec->signature, sizeof(fuserec->signature)) == 0)
    {
      found = 1;
      break; /* exit for loop immediately */
    }
  }
  
  if(found)
  {
    /* turn on green led, indicates that device is found in database */
    PORT_PIN_SET(HWPIN_LED_GREEN);
    /* if there're no lock bits and device fuses are equal to those in database, turn off red */
    /* on atmega162, low and high fuse are sometimes incorrectly read */
    if(   ( 0
            | ((f->low      ^ fuserec->fuse.low)      & fuserec->mask.low)
//            | ((f->high     ^ fuserec->fuse.high)     & fuserec->mask.high)
            | ((f->extended ^ fuserec->fuse.extended) & fuserec->mask.extended)
            | ((f->lock     ^ fuserec->fuse.lock)     & fuserec->mask.lock)
          ) == 0)
        PORT_PIN_CLR(HWPIN_LED_RED);
  }
}

/* reads device's signature */
void read_signature(uint8_t s[])
{
  uint8_t i = 0;

  hvEnterProgmode();
  for(i = 0; i < 3; i++)
    s[i] = hvReadSignature(i, HV_LOW);
  hvLeaveProgmode();
}


void read_fuses(struct fuses *fuses)
{
  hvEnterProgmode();
  fuses->high =      hvReadFuse(HVCTL_RDFUSE_HIGH);
  fuses->low =       hvReadFuse(HVCTL_RDFUSE_LOW);
  fuses->extended =  hvReadFuse(HVCTL_RDFUSE_EXT);
  fuses->lock =      hvReadFuse(HVCTL_RDLOCK);
  hvLeaveProgmode();
}

void write_fuses(struct fuses *fuses)
{
  hvEnterProgmode();
  hvProgramFuse(fuses->low,      HVCMD_WRITE_FUSE, HVCTL_WRFUSE_LOW);
  hvProgramFuse(fuses->high,     HVCMD_WRITE_FUSE, HVCTL_WRFUSE_HIGH);
  hvProgramFuse(fuses->extended, HVCMD_WRITE_FUSE, HVCTL_WRFUSE_EXT);
  hvProgramFuse(fuses->lock,     HVCMD_WRITE_LOCK, HVCTL_WRLOCK);
  hvLeaveProgmode();
}

void read_device_info(void)
{
  uint8_t i;
  
  for(i = 0; i < 2; i++)
  {
    hvSetProgmode(i); /* hvsp = 0 or hvpp = 1 */
    /* probing hvsp on atmega162 will set lock bits to 0xC0 */
    // timerMsDelay(1); /* wait enough to 12V to stabilize */
    read_signature(signature);
    if(signature[0] == 0x1e)
    {
      read_fuses(fuses);
      search_device_db(signature, fuses);
      return;
    }
  }
}

void read_device_info_pponly(void)
{
  uint8_t i;

  for(i = 1; i < 2; i++)
  {
    hvSetProgmode(i); /* hvsp = 0 or hvpp = 1 */
    // timerMsDelay(1); /* wait enough to 12V to stabilize */
    read_signature(signature);
#if 1
    if(signature[0] == 0x1e)
    {
      read_fuses(fuses);
      search_device_db(signature, fuses);
      return;
    }
#endif
  }

}

#define TEST 0

void reset_fuses(void)
{
#if TEST
  uint16_t i;
  
  for(i = 0; i < 1000; i++)
  {
    wdt_reset();
    read_device_info_pponly();
    if(PORT_PIN_VALUE(HWPIN_LED_RED) != 0)
    {
      if(fuses->high == 0xff)
        PORT_PIN_CLR(HWPIN_LED_GREEN);
      return;
    }
  }
  return;
#endif
  read_device_info();
  if(PORT_PIN_VALUE(HWPIN_LED_RED) == 0 || PORT_PIN_VALUE(HWPIN_LED_GREEN) == 0)
    return;
  /* continue if LED is yellow RED+GREEN */
  if( ((fuses->lock ^ default_fuserec.fuse.lock) & default_fuserec.mask.lock) != 0)
  {
    hvEnterProgmode();
    hvChipErase(80);
    hvLeaveProgmode();
  }
  write_fuses(&(default_fuserec.fuse));
  read_device_info();
}


void pushbutton_poll(void)
{
  if(waitpush)
  {
    waitpush--;
    return;
  }
  if(startbutton == 0)
  {
    if(PORT_PIN_VALUE(HWPIN_JUMPER) == 0)
    {
      reset_fuses();
      startbutton = 1;
      waitpush = 10000;
    }
  }
  else
  {
    if(PORT_PIN_VALUE(HWPIN_JUMPER) != 0)
    {
      PORT_PIN_CLR(HWPIN_LED_RED);
      PORT_PIN_CLR(HWPIN_LED_GREEN);
      startbutton = 0;
      waitpush = 10000;
    }
  }
}

#endif /* ENABLE_HVPROG */
