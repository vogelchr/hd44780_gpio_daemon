#include "hd44780.h"
#include "hd44780_font.h"
#include "statuspages.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>


static const char cgram[] = {
	0x00,  /* . . . . . */
	0x1b,  /* * * . * * */
	0x1b,  /* * * . * * */
	0x00,  /* . . . . . */
	0x11,  /* * . . . * */
	0x0e,  /* . * * * . */
	0x00,  /* . . . . . */
	0x00,
};

int
main(int argc, char **argv)
{
	useconds_t usec;
	useconds_t usec_per_statuspage;
	struct statuspage **p;
	int i;

	(void) argc;
	(void) argv;
	hd44780_init();
	hd44780_clear_mem();

	hd44780_font_setchar(0, cgram);

	p = statuspages;
	usec_per_statuspage = 0;

	while (1) {

		if (usec_per_statuspage <= 0) {
			printf("New statuspage.\n");
			p++;
			if (!*p)
				p = statuspages;
			usec_per_statuspage = 10000000;
			(*p)->fct_enter();
		}
		(*p)->fct_update(&usec);
		usleep(usec);
		usec_per_statuspage -= usec;
	}

	return 0;
}

