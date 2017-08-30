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
	int i;

	(void) argc;
	(void) argv;
	hd44780_init();
	hd44780_clear_mem();

	hd44780_font_setchar(0, cgram);


	while (1) {
		(statuspages[0]->fct_enter)();
		for (i=0; i<50; i++) {
			(statuspages[0]->fct_update)(&usec);
			usleep(usec);
		}
	}

	return 0;
}

