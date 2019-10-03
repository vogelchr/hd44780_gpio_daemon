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

#include "hd44780_font.h"
#include "hd44780.h"

#include <event2/event.h>

static const unsigned char cgram_heart[2][HD44780_FONT_NBYTES] = {
[0]={
	0x00,  /* . . . . . */
	0x0a,  /* . * . * . */
	0x1f,  /* * * * * * */
	0x1f,  /* * * * * * */
	0x0e,  /* . * * * . */
	0x04,  /* . . * . . */
	0x00,  /* . . . . . */
	0x00,
},[1]={
	0x00,  /* . . . . . */
	0x0a,  /* . * . * . */
	0x15,  /* * . * . * */
	0x11,  /* * . . . * */
	0x0a,  /* . * . * . */
	0x04,  /* . . * . . */
	0x00,  /* . . . . . */
	0x00,
}};

static const unsigned char cgram_empty[HD44780_FONT_NBYTES] = {
	0x00,  /* . . . . . */
	0x1f,  /* * * * * * */
	0x11,  /* * . . . * */
	0x11,  /* * . . . * */
	0x11,  /* * . . . * */
	0x1f,  /* * * * * * */
	0x00,  /* . . . . . */
	0x00,
};

static const unsigned char cgram_smile[2][HD44780_FONT_NBYTES] = {
[0]={
	0x00,  /* . . . . . */
	0x1b,  /* * * . * * */
	0x1b,  /* * * . * * */
	0x00,  /* . . . . . */
	0x11,  /* * . . . * */
	0x0e,  /* . * * * . */
	0x00,  /* . . . . . */
	0x00,
},[1]={
	0x00,  /* . . . . . */
	0x12,  /* * . . * . */
	0x1b,  /* * * . * * */
	0x00,  /* . . . . . */
	0x11,  /* * . . . * */
	0x0e,  /* . * * * . */
	0x00,  /* . . . . . */
	0x00,
}};


static const unsigned char
cgram_ellipsis[HD44780_FONT_NBYTES] = {
	0x00, /* . . . . .  */ 
	0x00, /* . . . . .  */ 
	0x00, /* . . . . .  */ 
	0x00, /* . . . . .  */ 
	0x00, /* . . . . .  */ 
	0x15, /* * . * . *  */ 
	0x00, /* . . . . .  */
	0x00, /* . . . . .  */
};

static const unsigned char
cgram_windmill[HD44780_FONT_WINDMILL_NCHARS][HD44780_FONT_NBYTES] =
{[0]={	0x01,  /* . . . . *  0 */
	0x02,  /* . . . * . */
	0x04,  /* . . * . . */
	0x04,  /* . . * . . */
	0x04,  /* . . * . . */
	0x08,  /* . * . . . */
	0x10,  /* * . . . . */
	0x00
}, [1]={
	0x00,  /* . . . . .  1 */
	0x01,  /* . . . . * */
	0x02,  /* . . . * . */
	0x04,  /* . . * . . */
	0x08,  /* . * . . . */
	0x10,  /* * . . . . */
	0x00,  /* . . . . . */
	0x00
}, [2]={
	0x00,  /* . . . . .  2 */
	0x00,  /* . . . . . */
	0x03,  /* . . . * * */
	0x04,  /* . . * . . */
	0x18,  /* * * . . . */
	0x00,  /* . . . . . */
	0x00,  /* . . . . . */
	0x00
}, [3]={
	0x00,  /* . . . . .  3 */
	0x00,  /* . . . . . */
	0x00,  /* . . . . . */
	0x1f,  /* * * * * * */
	0x00,  /* . . . . . */
	0x00,  /* . . . . . */
	0x00,  /* . . . . . */
	0x00
}, [4]={
	0x00,  /* . . . . .  4 */
	0x00,  /* . . . . . */
	0x18,  /* * * . . . */
	0x04,  /* . . * . . */
	0x03,  /* . . . * * */
	0x00,  /* . . . . . */
	0x00,  /* . . . . . */
	0x00
}, [5]={
	0x00,  /* . . . . .  5 */
	0x10,  /* * . . . . */
	0x08,  /* . * . . . */
	0x04,  /* . . * . . */
	0x02,  /* . . . * . */
	0x01,  /* . . . . * */
	0x00,  /* . . . . . */
	0x00
}, [6]={
	0x10,  /* * . . . .  6 */
	0x08,  /* . * . . . */
	0x04,  /* . . * . . */
	0x04,  /* . . * . . */
	0x04,  /* . . * . . */
	0x02,  /* . . . * . */
	0x01,  /* . . . . * */
	0x00
}, [7]={
	0x08,  /* . * . . . 7 */
	0x08,  /* . * . . . */
	0x04,  /* . . * . . */
	0x04,  /* . . * . . */
	0x04,  /* . . * . . */
	0x02,  /* . . . * . */
	0x02,  /* . . . * . */
	0x00
}, [8]={
	0x04,  /* . . * . . 8 */
	0x04,  /* . . * . . */
	0x04,  /* . . * . . */
	0x04,  /* . . * . . */
	0x04,  /* . . * . . */
	0x04,  /* . . * . . */
	0x04,  /* . . * . . */
	0x00
}};

void
hd44780_font_setchar(char c, const unsigned char *font) {
	if (c>7)
		return;
	hd44780_write_byte(HD44780_CGADDR(HD44780_FONT_NBYTES*c), HD44780_CMD);
	hd44780_write_buf(font, 8);
}

void
hd44780_font_set_windmill_phase(unsigned i) {
	if (i>=HD44780_FONT_WINDMILL_NCHARS)
		i=0;
	hd44780_font_setchar(HD44780_FONT_WINDMILL,cgram_windmill[i]);

	if (i == 0) {
		hd44780_font_setchar(HD44780_FONT_HEART,cgram_heart[0]);
		hd44780_font_setchar(HD44780_FONT_SMILE,cgram_smile[0]);
	}
	if (i == 3)
		hd44780_font_setchar(HD44780_FONT_HEART,cgram_heart[1]);
	if (i == 6)
		hd44780_font_setchar(HD44780_FONT_SMILE,cgram_smile[1]);

}

void
hd44780_font_init() {
	int i;

	for (i=0; i<HD44780_FONT_HEART; i++)
		hd44780_font_setchar(i, cgram_empty);
	hd44780_font_setchar(HD44780_FONT_HEART, cgram_heart[0]);	
	hd44780_font_setchar(HD44780_FONT_SMILE, cgram_smile[0]);	
	hd44780_font_setchar(HD44780_FONT_ELLIPSIS, cgram_ellipsis);
	hd44780_font_setchar(HD44780_FONT_WINDMILL,cgram_windmill[0]);
}

static struct event *hd44780_font_windmill_evt;
static int hd44780_font_windmill_phase;

static void
hd44780_font_windmill_cb(evutil_socket_t fd, short flags, void *arg)
{
	(void) fd;
	(void) flags;
	(void) arg;

	hd44780_font_windmill_phase++;
	if (hd44780_font_windmill_phase >= HD44780_FONT_WINDMILL_NCHARS)
		hd44780_font_windmill_phase = 0;
	hd44780_font_set_windmill_phase(hd44780_font_windmill_phase);
}

void
hd44780_font_windmill(struct event_base *evtbase, int onoff)
{
	struct timeval timeout = { .tv_sec=0, .tv_usec=133000 };

	if (onoff) {
		if (hd44780_font_windmill_evt)
			return;
		hd44780_font_windmill_evt = event_new(evtbase, -1/*fd*/,
			EV_PERSIST, hd44780_font_windmill_cb, NULL);
		event_add(hd44780_font_windmill_evt, &timeout);
	} else {
		if (!hd44780_font_windmill_evt)
			return;
		event_del(hd44780_font_windmill_evt);
		event_free(hd44780_font_windmill_evt);
		hd44780_font_windmill_evt = NULL;

		hd44780_font_windmill_phase = 0;
		hd44780_font_set_windmill_phase(0);
	}

}