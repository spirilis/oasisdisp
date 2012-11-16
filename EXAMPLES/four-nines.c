/* Count-Up-Fast
 * Connect to an Oasis SPI LED display and count from 0 to 65535
 */

#include <msp430.h>
#include <stdint.h>
#include "oasisdisp.h"

int main()
{
	WDTCTL = WDTPW | WDTHOLD;
	DCOCTL = CALDCO_16MHZ;
	BCSCTL1 = CALBC1_16MHZ;
	BCSCTL2 = DIVS_2;  // SMCLK = 8MHz
	BCSCTL3 = LFXT1S_2;  // ACLK = VLO
	__delay_cycles(8000);     // Wait for VLO settling
	while (BCSCTL3 & LFXT1OF)
		;

	P1DIR |= BIT1;
	P1OUT |= BIT1;
	P1SEL &= ~BIT1;
	P1SEL2 &= ~BIT1;

	spi_init();
	spi_mode_2();
	spi_transfer(0x00);  // Strawman xfer to cover G2452 errata USI5

	oasisdisp_init(&P1OUT, BIT1, 7);

	oasisdisp_print_int(&P1OUT, BIT1, 9999);

	LPM4;
	return 0;
}
