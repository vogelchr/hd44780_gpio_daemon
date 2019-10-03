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

#include "statuspages.h"
#include "hd44780.h"
#include "hd44780_font.h"
#include "hd44780_display.h"

#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <error.h>
#include <assert.h>

#include <event2/event.h>

#define STATUSPAGE_NUM_FMTS 10

static const char * const clock_formats[STATUSPAGE_NUM_FMTS] = {
	/* without day of week */
	"%d. %b %Y ___%H:%M:%S _______",
	"___ %d. %b %Y%H:%M:%S _______",
	"%d. %b %Y __________ %H:%M:%S",
	"___ %d. %b %Y_______ %H:%M:%S",

	/* with day of week */
	"%d. %b %Y ___%H:%M:%S ___ %a",
	"___ %d. %b %Y%H:%M:%S ___ %a",
	"%d. %b %Y %a%H:%M:%S _______",
	"%a_%d. %b %Y%H:%M:%S _______",
	"%d. %b %Y _____ %a  %H:%M:%S",
	"___ %d. %b %Y%a ___ %H:%M:%S",
};

#define STATUSPAGE_CTR_MAX 20

static int statuspage_ctr=0;

static void
statuspage_cb(evutil_socket_t fd, short flags, void *arg)
{
	static char clock_fmt[33];
	unsigned int fmt_no;
	static struct timespec tsp;
	static struct tm stm;
	static unsigned char buf[33];
	int i,n;
	char *p;
	char glyph;

	(void) fd;
	(void) flags;
	(void) arg;

	if (statuspage_ctr++ >= 20) {
		statuspage_ctr = 0;
		fmt_no = rand() % STATUSPAGE_NUM_FMTS;
		strncpy(clock_fmt, clock_formats[fmt_no], sizeof(clock_fmt));

		/* count number of "_" in string */
		n=0;
		for (p=clock_fmt; *p; p++) {
			if (*p == '_')
				n++;
		}

		switch (((unsigned int)rand()) % 3) {
		case 0: glyph = HD44780_FONT_HEART; break;
		case 1: glyph = HD44780_FONT_SMILE; break;
		case 2: glyph = HD44780_FONT_WINDMILL; break;
		}
			
		/* insert a random windmill character */
		n = rand()%n;
		for (i=0, p=clock_fmt; *p; p++) {
			if (*p == '_') {
				if (i == n)
					*p = glyph;
				else
					*p = ' ';
				i++;
			}
		}

	}

	clock_gettime(CLOCK_REALTIME, &tsp);

	localtime_r(&tsp.tv_sec, &stm);
	strftime((char *)buf, sizeof(buf), clock_fmt, &stm);



	hd44780_write_byte(HD44780_DDADDR(0x00), HD44780_CMD);
	hd44780_write_buf(buf, 16);
	hd44780_write_byte(HD44780_DDADDR(0x40), HD44780_CMD);
	hd44780_write_buf(buf+16, 16);
}

static struct event *statuspage_evt;

void statuspages_on(struct event_base *evtbase)
{
	struct timeval timeout = { .tv_sec=1, .tv_usec=0 };

	if (statuspage_evt)
		return;

	statuspage_evt = event_new(evtbase, -1/*fd*/, EV_PERSIST, statuspage_cb, event_self_cbarg());
	event_add(statuspage_evt, &timeout);

	hd44780_display_clear();

	/* on, cursor off, cursor blink off */
	hd44780_write_byte(HD44780_ONOFF(1, 0, 0), HD44780_CMD);
	hd44780_font_windmill(evtbase, 1);

	statuspage_ctr = STATUSPAGE_CTR_MAX;
}

void statuspages_off()
{
	if (!statuspage_evt)
		return;

	/* turn off windmill for now */
	hd44780_font_windmill(event_get_base(statuspage_evt), 0);

	event_del(statuspage_evt);
	event_free(statuspage_evt);
	statuspage_evt = NULL;

	/* on, cursor on, cursor blink on */
	hd44780_write_byte(HD44780_ONOFF(1, 1, 1), HD44780_CMD);
	hd44780_display_clear();
}
