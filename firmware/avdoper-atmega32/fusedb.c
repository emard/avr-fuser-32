/* Name: main.c
 * License: GNU GPL
 * Project: HVPP fuse reset
 * Author: Emard
 */

#include <avr/eeprom.h>
#include <stdint.h>

#include "fusereset.h"

/* declare default values of the fuese in eeprom.
** 1024 byte eeprom in atmega32 fits 112 devices
** of 9 byte each
*/
const struct fuserec EEMEM fusedb[] = {
  {
    { 0x1e, 0x96, 0x84 }, /* device atmega64M1 */
    { 0x62, 0xD9, 0xFF, 0xFF }, /* default fuse value LO,HI,EX,LOCK */
    { 0xff, 0xff, 0x3f, 0xFF }, /* presence of the fuses ff-allpresent, 00-no fuse byte */
  },
  {
    { 0x1e, 0x94, 0x83 }, /* device at90pwm316 */
    { 0x62, 0xDF, 0xF9, 0xFF }, /* default fuse value LO,HI,EX,LOCK */
    { 0xff, 0xff, 0xf7, 0xFF }, /* presence of the fuses ff-allpresent, 00-no fuse byte */
  },
  {
    { 0x1e, 0x93, 0x07 }, /* device atmega8 */
    { 0xE1, 0xD9, 0xFF, 0xFF }, /* default fuse value LO,HI,EX,LOCK */
    { 0xff, 0xff, 0x00, 0xFF }, /* presence of the fuses ff-allpresent, 00-no fuse byte */
  },
  {
    { 0x1e, 0x94, 0x03 }, /* device atmega16 */
    { 0xE1, 0xD9, 0xFF, 0xFF }, /* default fuse value LO,HI,EX,LOCK */
    { 0xff, 0xff, 0x00, 0xFF }, /* presence of the fuses ff-allpresent, 00-no fuse byte */
  },
  {
    { 0x1e, 0x95, 0x02 }, /* device atmega32 */
    { 0xE1, 0xD9, 0xE1, 0xFF }, /* default fuse value LO,HI,EX,LOCK */
    { 0xff, 0xff, 0x00, 0xFF }, /* presence of the fuses ff-allpresent, 00-no fuse byte */
  },
  {
    { 0x1e, 0x92, 0x05 }, /* device atmega48 */
    { 0x62, 0xDF, 0xFF, 0xFF }, /* default fuse value LO,HI,EX,LOCK */
    { 0xff, 0xff, 0xFF, 0xFF }, /* presence of the fuses ff-allpresent, 00-no fuse byte */
  },
  {
    { 0x1e, 0x95, 0x0F }, /* device atmega328p */
    { 0x62, 0xD9, 0xFF, 0xFF }, /* default fuse value LO,HI,EX,LOCK */
    { 0xff, 0xff, 0x07, 0xFF }, /* presence of the fuses ff-allpresent, 00-no fuse byte */
  },
  {
    { 0x1e, 0x94, 0x04 }, /* device atmega162 */
    { 0x62, 0x99, 0xFF, 0xFF }, /* default fuse value LO,HI,EX,LOCK */
//    { 0x62, 0x99, 0xFF, 0xFF }, /* default fuse value LO,HI,EX,LOCK */
//    { 0xff, 0xff, 0x1E, 0x3F }, /* presence of the fuses ff-allpresent, 00-no fuse byte */
    { 0xFF, 0xFF, 0x1E, 0xFF }, /* presence of the fuses ff-allpresent, 00-no fuse byte */
  },
  {
    { 0x1e, 0x93, 0x0c }, /* device attiny84 */
    { 0x62, 0xDF, 0xFF, 0xFF }, /* default fuse value LO,HI,EX,LOCK */
    { 0xff, 0xff, 0x01, 0xFF }, /* presence of the fuses ff-allpresent, 00-no fuse byte */
  },
  {
    { 0x1e, 0x93, 0x0b }, /* device attiny85 */
    { 0x62, 0xDF, 0xFF, 0xFF }, /* default fuse value LO,HI,EX,LOCK */
    { 0xff, 0xff, 0x01, 0xFF }, /* presence of the fuses ff-allpresent, 00-no fuse byte */
  },
  {
    { 0xff, 0xff, 0xff }, /* terminator - end record */
    { 0xff, 0xff, 0xff, 0xFF },
    { 0xff, 0xff, 0xff, 0xFF },
  }
};
