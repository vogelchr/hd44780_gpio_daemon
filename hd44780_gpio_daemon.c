/*
    This file is part of hd44780_gpio_daemon.

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


#include "tcp_server.h"
#include "hd44780.h"
#include "hd44780_font.h"
#include "hd44780_display.h"
#include "statuspages.h"

#include <event2/event.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define DEFAULT_TCP_PORT 54321

static struct event_base *evtbase;
static uint16_t port = DEFAULT_TCP_PORT;

void
usage(char *argv0) {
	fprintf(stderr, "Usage: %s [options]\n", argv0);
	fprintf(stderr, "\t-h        Show this usage.\n");
	fprintf(stderr, "\t-p PORT   listen on TCP port PORT, default %u\n",
		DEFAULT_TCP_PORT);
}

int
main(int argc, char **argv)
{
	int i;

	while ((i=getopt(argc, argv, "hp:")) != -1) {
		switch(i) {
		case 'h':
			usage(argv[0]);
			exit(1);
		case 'p':
			port = strtoul(optarg, NULL, 0);
			break;
		}
	}

	fprintf(stderr, "Listening in TCP port %u.\n", port);

	evtbase = event_base_new();
	tcp_server_init(evtbase, 1234);

	hd44780_init();
	hd44780_font_init();
	hd44780_display_sync_ddaddr();

	statuspages_on(evtbase);
	event_base_dispatch(evtbase);
	return 0;
}

