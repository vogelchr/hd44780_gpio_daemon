#include "hd44780.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int
main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	hd44780_init();
}

