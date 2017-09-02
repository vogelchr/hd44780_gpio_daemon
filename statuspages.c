#include "statuspages.h"
#include "hd44780.h"
#include "hd44780_font.h"
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

time_t clock_last_sec;

static char clock_fmt[64];

static int
statuspage_clock_update(useconds_t *usec)
{
	static struct timespec tsp;
	static struct tm stm;
	static unsigned int windmill = 0;
	static char buf[33];

	if (usec)
		*usec = 100000;

	clock_gettime(CLOCK_REALTIME, &tsp);
	if (tsp.tv_sec == clock_last_sec) {
		hd44780_font_windmill_advance(&windmill);
		return 0;
	}

	clock_last_sec = tsp.tv_sec;

	/* +----------------+
	 * |12. Jan 2017____|
	 * |09:17:58________|
	 * +----------------+
	 * */
	localtime_r(&tsp.tv_sec, &stm);
	strftime(buf, sizeof(buf), clock_fmt, &stm);

	hd44780_write_byte(HD44780_DDADDR(0x00), HD44780_CMD);
	hd44780_write_buf(buf, 16);
	hd44780_write_byte(HD44780_DDADDR(0x40), HD44780_CMD);
	hd44780_write_buf(buf+16, 16);

	hd44780_font_windmill_advance(&windmill);
	return 0;
}

static const char * const clock_formats[] = {
	/* without day of week */
	"%d. %b %Y____%H:%M:%S________",
	"____%d. %b %Y%H:%M:%S________",
	"%d. %b %Y____________%H:%M:%S",
	"____%d. %b %Y________%H:%M:%S",

	/* with day of week */
	"%d. %b %Y____%H:%M:%S_____%a",
	"____%d. %b %Y%H:%M:%S_____%a",
	"%d. %b %Y_%a%H:%M:%S________",
	"%a_%d. %b %Y%H:%M:%S________",
	"%d. %b %Y_______%a__%H:%M:%S",
	"____%d. %b %Y%a_____%H:%M:%S",
};

static int
statuspage_clock_init(void)
{
	char *p;
	int i,n;

	i = rand() % (sizeof(clock_formats)/sizeof(clock_formats[0]));
	strcpy(clock_fmt,clock_formats[i]);

	/* count number of "_" in string */
	n=0;
	for (p=clock_fmt; *p; p++) {
		if (*p == '_')
			n++;
	}

	/* insert a random windmill character */
	printf("%d/", n);
	n = rand()%n;
	printf("%d\n", n);
	for (i=0, p=clock_fmt; *p; p++) {
		if (*p == '_') {
			if (i == n)
				*p = HD44780_FONT_WINDMILL;
			else
				*p = ' ';
			i++;
		}
	}

	clock_last_sec = 0;
	hd44780_write_byte(HD44780_CLEAR, HD44780_CMD);

	return 0;
}

static struct statuspage statuspage_clock = {
	&statuspage_clock_init,
	&statuspage_clock_update
};

struct statuspage *statuspages_arr[] = {
	&statuspage_clock,
	NULL
};

struct statuspage **statuspages = statuspages_arr;
