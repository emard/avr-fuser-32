# AVR-Fuser 32

3-in-1 Resurrector programmer for AVR chips

    HVPP: High-Voltage Parallel Programmer
    HVSP: High-Voltage Serial Programmer
    ISP:  In-System Programmer (SPI)

Based on [AVR-Doper](https://www.obdev.at/products/vusb/avrdoper.html)
firmware running on ATMEGA32 in DIP-40 socket.

AVRDUDE compatible, USB powered, onboard 5V->12V converter,
10-pin ISP header, 20-pin HVPP/HVSP header and 
adapter PCB for many popular DIP packages.

Quick mode without PC and avrdude: Pushbutton
erases chip to factory default state.

Complete with firmware, bootloader, kicad schematics and PCB 
for the programmer and sockets adapter.

Supported devices (I tried many but not all of them :)

    ATMEGA 8, 16, 32, 162, 168P, 328P, 16M1, 32M1, 64M1, 8515, 8535
    ATTINY 11, 13, 15, 26, 84, 85, 2313, 4313

# Remark

This is my old project, just uplodaded to github in order
not to get lost, without trying it to compile or open 
since a long time.

May need some fitting for new avr-gcc and kicad 4.x
