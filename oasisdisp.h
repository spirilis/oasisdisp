/* oasisdisp.h
 *
 * Oasis SPI LED driver library
 */

#ifndef _OASISDISP_H_
#define _OASISDISP_H_

#include <stdint.h>

void spi_mode_2();
void spi_mode_0();

#ifndef _MSPRF24_H_
void spi_init();
uint8_t spi_transfer(uint8_t inb);
#endif

/* The volatile unsigned char * plus unsigned char passed includes the SFR for the Port attached
 * to the STBY (SPI Chip Select) line of the target display.  Pass this along to each call.
 *
 * Note that in order to talk to this display after we've been talking to other SPI devices, it may
 *   be necessary to wrap these calls in spi_mode_2() and run spi_mode_0() at the end to reconfigure
 *   for whichever other devices we have (e.g. nRF24L01+).
 */

void oasisdisp_init(volatile unsigned char *, unsigned char, uint8_t brightness);  // Brightness: 0-7
void oasisdisp_off(volatile unsigned char *, unsigned char);
void oasisdisp_write_digit(volatile unsigned char *, unsigned char, uint8_t, uint8_t);
void oasisdisp_write_space(volatile unsigned char *, unsigned char, uint8_t);
void oasisdisp_write_dash(volatile unsigned char *, unsigned char, uint8_t);

void oasisdisp_print_uint(volatile unsigned char *, unsigned char, unsigned int);
void oasisdisp_print_int(volatile unsigned char *, unsigned char, int);

void oasisdisp_print_byte(volatile unsigned char *, unsigned char, uint8_t);   // Hex dump of a single byte
void oasisdisp_print_word(volatile unsigned char *, unsigned char, uint16_t);  // Hex dump of two bytes

#endif
