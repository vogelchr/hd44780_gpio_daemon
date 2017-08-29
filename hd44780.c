#include "hd44780.h"
#include <unistd.h>
#include <wiringPi.h>


#define HD44780_GPIO_RS 17
#define HD44780_GPIO_nE 27
#define HD44780_GPIO_D4 22
#define HD44780_GPIO_D5 23
#define HD44780_GPIO_D6 24
#define HD44780_GPIO_D7 25

enum HD44780_ISCMD hd44780_rs_state = HD44780_CMD;

int
hd44780_init_gpio() {
	wiringPiSetupGpio();

	pinMode(HD44780_GPIO_RS, OUTPUT);
	pinMode(HD44780_GPIO_nE, OUTPUT);
	pinMode(HD44780_GPIO_D4, OUTPUT);
	pinMode(HD44780_GPIO_D5, OUTPUT);
	pinMode(HD44780_GPIO_D6, OUTPUT);
	pinMode(HD44780_GPIO_D7, OUTPUT);

	digitalWrite(HD44780_GPIO_nE, 1);
	digitalWrite(HD44780_GPIO_RS, 0);
	digitalWrite(HD44780_GPIO_D4, 0);
	digitalWrite(HD44780_GPIO_D5, 0);
	digitalWrite(HD44780_GPIO_D6, 0);
	digitalWrite(HD44780_GPIO_D7, 0);

	return 0;
}

/* D4..D7 is connected, so we write the upper 4 bits! */
static void
hd44780_write_nibble(unsigned char v) {
	digitalWrite(HD44780_GPIO_D7, !!(v & 0x80));
	digitalWrite(HD44780_GPIO_D6, !!(v & 0x40));
	digitalWrite(HD44780_GPIO_D5, !!(v & 0x20));
	digitalWrite(HD44780_GPIO_D4, !!(v & 0x10));
	usleep(1);
	digitalWrite(HD44780_GPIO_nE, 0);
	usleep(1);
	digitalWrite(HD44780_GPIO_nE, 1);
}

/* set the register select line, avoid GPIO operation if cached value matches */
static void
hd44780_set_rs(enum HD44780_ISCMD rs) {
	rs = !!rs; /* collapse to 0 or 1 */
	if (hd44780_rs_state != rs) {
		hd44780_rs_state = rs;
		digitalWrite(HD44780_GPIO_RS, rs);
	}
}

/* initialize the LCD to 4-bit mode and turn it on */
static void
hd44780_init_lcd()
{
	hd44780_set_rs(HD44780_DATA);
	hd44780_set_rs(HD44780_CMD);
	usleep(1);

	/* try to bring display to 8-bit mode first, but the first instruction
	   may be the 2nd half of a 0x03 (return home) operation which is
	   particularly slow. Therefore add a gratious amount of usleep(). */

	hd44780_write_nibble(0x30); /*   }  of these three, two may be      */
	usleep(4700);               /*   :  0x33 in 4-bit mode or three     */
	hd44780_write_nibble(0x30); /* } }  of them 0x3X in 8-bit mode      */
	hd44780_write_nibble(0x30); /* }                                    */
	usleep(150);

	/* now display is in 8-bit mode for sure */
	/* 0:4-bit, 1:2 lines, 0:5x8 font */
	hd44780_write_byte(HD44780_FUNC(0, 1, 0), HD44780_CMD);

	/* on, cursor on, cursor blink */
	hd44780_write_byte(HD44780_ONOFF(1, 1, 1), HD44780_CMD);

	/* increment (left to right), no display shift */
	hd44780_write_byte(HD44780_ENTRYMODE(1,0), HD44780_CMD);

	/* clear display */
	hd44780_write_byte(HD44780_CLEAR, HD44780_CMD);
	hd44780_write_byte(HD44780_HOME, HD44780_CMD);
}

int
hd44780_init() {
	int i;

	if ((i=hd44780_init_gpio()) != 0)
		return i;
	hd44780_rs_state = 0;
	hd44780_init_lcd();
	return 0;
}

/* write a 8-bit command or 8-bit data to the display */
void
hd44780_write_byte(unsigned char v, enum HD44780_ISCMD iscmd)
{
	hd44780_set_rs(iscmd);
	hd44780_write_nibble( v       & 0xf0); /* MSB */
	hd44780_write_nibble((v << 4) & 0xf0); /* LSB */
	if ((iscmd == HD44780_CMD) & (
		(v & 0xfe) == HD44780_HOME || 
		 v         == HD44780_CLEAR
	))
		usleep(4500); /* these commands are *SLOW* */
	else
		usleep(50);
}

/* write a buffer of data */
extern void
hd44780_write_buf(unsigned char *v, size_t len) {
	size_t i;

	for (i=0; i<len ; i++)
		hd44780_write_byte(v[i], HD44780_DATA);
}
