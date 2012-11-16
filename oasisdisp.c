/* oasisdisp.c
 *
 * Driver for Oasis 4-digit 7-segment LED display
 */

#include <msp430.h>
#include "oasisdisp.h"
#include <stdint.h>

const char oasisdisp_DIGITS[16] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x77, 0x7c, 0x58, 0x5e, 0x79, 0x71};

void spi_mode_2()
{
#ifdef __MSP430_HAS_USI__
	USICTL0 |= USILSB;
	USICTL1 &= ~USICKPH;
	USICKCTL |= USICKPL;
#endif

#ifdef __MSP430_HAS_USCI__
	UCB0CTL0 &= ~(UCCKPH | UCMSB);
	UCB0CTL0 |= UCCKPL;
#endif
}

void spi_mode_0()
{
#ifdef __MSP430_HAS_USI__
	USICTL0 &= ~USILSB;
	USICTL1 |= USICKPH;
	USICKCTL &= ~USICKPL;
#endif

#ifdef __MSP430_HAS_USCI__
	UCB0CTL0 |= UCCKPH | UCMSB;
	UCB0CTL0 &= ~UCCKPL;
#endif
}

// Wrapping this because we don't need this if we are using the msprf24 nRF24L01+ library...
#ifndef _MSPRF24_H_

// USI
#ifdef __MSP430_HAS_USI__
void spi_init()
{
	/* USI SPI setup */
	USICTL0 |= USISWRST;
	USICTL1 = 0;
	USICKCTL = USICKPL | USISSEL_2 | USIDIV_3;  // Clock source = SMCLK/1
	USICTL0 = USIPE7 | USIPE6 | USIPE5 | USIMST | USILSB | USIOE;
	USISR = 0x0000;
}

uint8_t spi_transfer(uint8_t inb)
{
	USISRL = inb;
	USICNT = 8;            // Start SPI transfer
	while ( !(USICTL1 & USIIFG) )
		;
	USICTL1 &= ~USIIFG;
	return USISRL;
}
#endif

// USCI_B
#ifdef __MSP430_HAS_USCI__
void spi_init()
{
	/* Configure ports on MSP430 device for USCI_B */
	P1SEL |= BIT5 | BIT6 | BIT7;
	P1SEL2 |= BIT5 | BIT6 | BIT7;

	/* USCI-B specific SPI setup */
	UCB0CTL1 |= UCSWRST;
	UCB0CTL0 = UCCKPL | UCMST | UCMODE_0 | UCSYNC;  // SPI mode 0, master
	UCB0BR0 = 0x01;  // SPI clocked at same speed as SMCLK
	UCB0BR1 = 0x00;
	UCB0CTL1 = UCSSEL_2;  // Clock = SMCLK, clear UCSWRST and enables USCI_B module.
}
uint8_t spi_transfer(uint8_t inb)
{
	UCB0TXBUF = inb;
	while ( !(IFG2 & UCB0RXIFG) )  // Wait for RXIFG indicating remote byte received via SOMI
		;
	return UCB0RXBUF;
}
#endif
//

#endif

void oasisdisp_init(volatile unsigned char *portout, unsigned char bit, uint8_t brightness)
{
	*portout &= ~bit;
	spi_transfer(0x02);
	*portout |= bit;
	*portout &= ~bit;
	spi_transfer(0x40);
	*portout |= bit;
	*portout &= ~bit;
	spi_transfer(0x88 | (brightness & 0x07));
	*portout |= bit;
}

void oasisdisp_off(volatile unsigned char *portout, unsigned char bit)
{
	*portout &= ~bit;
	spi_transfer(0x80);
	*portout |= bit;
}

void oasisdisp_write_digit(volatile unsigned char *portout, unsigned char bit, uint8_t digit, uint8_t numeral)
{
	if (digit > 3)
		return;
	*portout &= ~bit;
	spi_transfer(0xC0 + digit*2);
	spi_transfer(oasisdisp_DIGITS[numeral & 0x0F]);
	*portout |= bit;
}

void oasisdisp_write_space(volatile unsigned char *portout, unsigned char bit, uint8_t digit)
{
	if (digit > 3)
		return;
	*portout &= ~bit;
	spi_transfer(0xC0 + digit*2);
	spi_transfer(0x00);
	*portout |= bit;
}

void oasisdisp_write_dash(volatile unsigned char *portout, unsigned char bit, uint8_t digit)
{
	if (digit > 3)
		return;
	*portout &= ~bit;
	spi_transfer(0xC0 + digit*2);
	spi_transfer(0x40);
	*portout |= bit;
}

void oasisdisp_print_uint(volatile unsigned char *portout, unsigned char bit, unsigned int num)
{
	uint8_t force_zero = 0;

	if (num >= 10000) {  // Only 4 digits here, can't display it...
		num -= (num/10000)*10000;
		force_zero = 1;
	}
	if (num >= 1000 || force_zero) {
		oasisdisp_write_digit(portout, bit, 0, num/1000);
		num -= (num/1000)*1000;
		force_zero = 1;
	} else {
		oasisdisp_write_space(portout, bit, 0);
	}
	if (num >= 100 || force_zero) {
		oasisdisp_write_digit(portout, bit, 1, num/100);
		num -= (num/100)*100;
		force_zero = 1;
	} else {
		oasisdisp_write_space(portout, bit, 1);
	}
	if (num >= 10 || force_zero) {
		oasisdisp_write_digit(portout, bit, 2, num/10);
		num -= (num/10)*10;
		force_zero = 1;
	} else {
		oasisdisp_write_space(portout, bit, 2);
	}
	oasisdisp_write_digit(portout, bit, 3, num);
}

void oasisdisp_print_int(volatile unsigned char *portout, unsigned char bit, int num)
{
	uint8_t i, force_zero = 0, negative = 0;

	if (num < 0) {
		// Temporarily using 'negative' to store the digit index for the negative sign
		num = -num;
		if (num >= 100)
			negative = 0;
		else if (num >= 10)
			negative = 1;
		else
			negative = 2;
		oasisdisp_write_dash(portout, bit, negative);
		for (i = 0; i < negative; i++)
			oasisdisp_write_space(portout, bit, i);

		if (num >= 10000) {
			num -= (num/10000)*10000;
			force_zero = 1;
		}
		if (num >= 1000) {
			num -= (num/1000)*1000;
			force_zero = 1;
		}
		negative = 1;
	}
	if (num >= 10000) {  // Only 4 digits here, can't display it...
		num -= (num/10000)*10000;
		force_zero = 1;
	}
	if (!negative && (num >= 1000 || force_zero)) {
		oasisdisp_write_digit(portout, bit, 0, num/1000);
		num -= (num/1000)*1000;
		force_zero = 1;
	} else {
		if (!negative)
			oasisdisp_write_space(portout, bit, 0);
	}
	if (num >= 100 || force_zero) {
		oasisdisp_write_digit(portout, bit, 1, num/100);
		num -= (num/100)*100;
		force_zero = 1;
	} else {
		if (!negative)
			oasisdisp_write_space(portout, bit, 1);
	}
	if (num >= 10 || force_zero) {
		oasisdisp_write_digit(portout, bit, 2, num/10);
		num -= (num/10)*10;
		force_zero = 1;
	} else {
		if (!negative)
			oasisdisp_write_space(portout, bit, 2);
	}
	oasisdisp_write_digit(portout, bit, 3, num);
}

void oasisdisp_print_byte(volatile unsigned char *portout, unsigned char bit, uint8_t b)
{
	oasisdisp_write_space(portout, bit, 0);
	oasisdisp_write_space(portout, bit, 1);
	oasisdisp_write_digit(portout, bit, 2, (b >> 4) & 0x0F);
	oasisdisp_write_digit(portout, bit, 3, b & 0x0F);
}

void oasisdisp_print_word(volatile unsigned char *portout, unsigned char bit, uint16_t b)
{
	oasisdisp_write_digit(portout, bit, 0, (b >> 12) & 0x0F);
	oasisdisp_write_digit(portout, bit, 1, (b >> 8) & 0x0F);
	oasisdisp_write_digit(portout, bit, 2, (b >> 4) & 0x0F);
	oasisdisp_write_digit(portout, bit, 3, b & 0x0F);
}
