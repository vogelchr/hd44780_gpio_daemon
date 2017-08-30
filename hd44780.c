#include "hd44780.h"
#include <unistd.h>
#include <wiringPi.h>
#include <stdio.h>
#include <string.h>

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

	digitalWrite(HD44780_GPIO_nE, 0);
	digitalWrite(HD44780_GPIO_RS, 0);
	digitalWrite(HD44780_GPIO_D4, 0);
	digitalWrite(HD44780_GPIO_D5, 0);
	digitalWrite(HD44780_GPIO_D6, 0);
	digitalWrite(HD44780_GPIO_D7, 0);

	return 0;
}

/* D4..D7 is connected, so we write the upper 4 bits! */
static void
hd44780_write_nibble(unsigned char v, useconds_t sleep_after) {
//	fprintf(stderr,"Nibble %01x.\n", v>>4);
	digitalWrite(HD44780_GPIO_D7, !!(v & 0x80));
	digitalWrite(HD44780_GPIO_D6, !!(v & 0x40));
	digitalWrite(HD44780_GPIO_D5, !!(v & 0x20));
	digitalWrite(HD44780_GPIO_D4, !!(v & 0x10));
	digitalWrite(HD44780_GPIO_nE, 1);
	usleep(1);
	digitalWrite(HD44780_GPIO_nE, 0);
	usleep(sleep_after);
}

/* set the register select line, avoid GPIO operation if cached value matches */
static void
hd44780_set_rs(enum HD44780_ISCMD rs) {
	rs = !!rs; /* collapse to 0 or 1 */
//	fprintf(stderr,"RS=%d\n", rs);
	if (hd44780_rs_state != rs) {
		hd44780_rs_state = rs;
		digitalWrite(HD44780_GPIO_RS, rs);
	}
}

/* initialize the LCD to 4-bit mode and turn it on */
static void
hd44780_init_lcd()
{
	hd44780_set_rs(HD44780_CMD);

	/* try to bring display to 8-bit mode first, but the first instruction
	   may be the 2nd half of a 0x03 (return home) operation which is
	   particularly slow. Therefore add a gratious amount of usleep(). */

	hd44780_write_nibble(0x30, 4700); /*   }  LCD controller may be in 4bit   */
	hd44780_write_nibble(0x30, 4700); /* } }  mode and any two of them will   */
	hd44780_write_nibble(0x30, 4700); /* }    put it in 8-bit mode.           */

	hd44780_write_nibble(0x20,   50); /* }    put it in 4-bit mode.           */

	/* now display is in 8-bit mode for sure */
	/* 0:4-bit, 1:2 lines, 0:5x8 font */
	hd44780_write_byte(HD44780_FUNC(0, 1, 0), HD44780_CMD);

	/* one byte directly following the FUNCTION command controls
	 * the brightness (on a Noritake VFD Display). Set to low intensity. */
	hd44780_write_byte(3, HD44780_DATA);

	/* on, cursor off, cursor blink off */
	hd44780_write_byte(HD44780_ONOFF(1, 0, 0), HD44780_CMD);

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
	useconds_t usec;

	if ((iscmd == HD44780_CMD) & (
		(v & 0xfe) == HD44780_HOME || 
		 v         == HD44780_CLEAR
	))
		usec=4500;
	else
		usec=50;

	hd44780_set_rs(iscmd);
	hd44780_write_nibble( v       & 0xf0, 1);     /* MSB */
	hd44780_write_nibble((v << 4) & 0xf0, usec);  /* LSB */
}

/* write a buffer of data */
extern void
hd44780_write_buf(const char *v, size_t len) {
	size_t i;

	for (i=0; i<len ; i++)
		hd44780_write_byte(v[i], HD44780_DATA);
}

void
hd44780_clear_mem()
{
	char buf[64];
	memset(buf,'\0', sizeof(buf));
	hd44780_write_byte(HD44780_DDADDR(0x00), HD44780_CMD);
	hd44780_write_buf(buf, 16);
	hd44780_write_byte(HD44780_DDADDR(0x40), HD44780_CMD);
	hd44780_write_buf(buf, 16);
	hd44780_write_byte(HD44780_CGADDR(0x00), HD44780_CMD);
	hd44780_write_buf(buf, 64);
};