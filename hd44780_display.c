/*
    This file is part of hd44780_gpio_daemon.

    (c) 2019 by Christian Vogel <vogelchr@vogel.cx>

    hd44780_gpio_daemon is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    hd44780_gpio_daemon is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with hd44780_gpio_daemon.  If not, see <https://www.gnu.org/licenses/>.
*/


#include "hd44780_display.h"
#include "hd44780.h"
#include <stdio.h>

static unsigned int hd44780_display_ddaddr; /* cursor position */

unsigned int
hd44780_display_canonical_addr(unsigned int daddr)
{
	if (daddr <= 0x10) /* first line, incl. one after last char */
		return daddr;
	else if (daddr > 0x10 && daddr < 0x40)
		return 0x40; /* next line */
	else if (daddr >= 0x40 && daddr <= 0x50)
		return daddr; /* 2nd line */
	else
		return 0x0; /* wrap to next line */
}

void hd44780_display_linefeed()
{
	unsigned int daddr = hd44780_display_canonical_addr(hd44780_display_ddaddr);
	if (daddr <= 0x10) /* within 1st line */
		daddr = 0x40; /* move to 2nd line */
	else
		daddr = 0x00; /* move to 1st line */
	hd44780_write_byte(HD44780_DDADDR(daddr), HD44780_CMD);
	hd44780_display_ddaddr = daddr;
}

void hd44780_display_putc(unsigned char c)
{
	unsigned int daddr = hd44780_display_canonical_addr(hd44780_display_ddaddr);

	if (daddr == 0x10) /* end of 1st line */
		daddr = 0x40; /* move to 2nd line */
	else if (daddr == 0x50) /* end of 2nd line */
		daddr = 0x00; /* move to 1st line */

	if (daddr != hd44780_display_ddaddr)
		hd44780_write_byte(HD44780_DDADDR(daddr), HD44780_CMD);
	hd44780_write_byte(c, HD44780_DATA);	

	hd44780_display_ddaddr = hd44780_display_canonical_addr(daddr+1);
}

void
hd44780_display_goto(unsigned char daddr)
{
	daddr = hd44780_display_canonical_addr(daddr);
	if (daddr != hd44780_display_ddaddr)
		hd44780_write_byte(HD44780_DDADDR(daddr), HD44780_CMD);
	hd44780_display_ddaddr = daddr;
}

void hd44780_display_clear() {
	hd44780_display_ddaddr = 0;
	hd44780_write_byte(HD44780_CLEAR, HD44780_CMD);
	hd44780_write_byte(HD44780_HOME, HD44780_CMD);
}

void hd44780_display_home() {
	hd44780_display_ddaddr = 0;
	hd44780_write_byte(HD44780_HOME, HD44780_CMD);
}

void hd44780_display_sync_ddaddr() {
	unsigned int daddr = hd44780_display_canonical_addr(hd44780_display_ddaddr);
	hd44780_write_byte(HD44780_DDADDR(daddr), HD44780_CMD);
	hd44780_display_ddaddr = daddr;
}