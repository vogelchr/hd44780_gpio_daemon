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

#include "tcp_server.h"
#include "hd44780_font.h"
#include "hd44780.h"
#include "hd44780_display.h"
#include "statuspages.h"

#include <arpa/inet.h>

#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>

#include <event2/event.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event-config.h>

/* the tcp server understands the following commands:	
	ESC c                 --> clear display, cursor to home
	ESC h                 --> cursor to home
	ESC $ <cmd>           --> send command <cmd> to display (see datasheet)
	ESC g <char> <8bytes> --> redefine character <char> (0..7)
	ESG p <hex> <hex>     --> goto position 00..0f 1st line, 40..4f 2nd line
	ESC q                 --> drop connection
	ESC \n <any>          --> just ignore
	ESC b x               --> set brightness \0..\3 or 0..3
	ESC C 0/1/2           --> cursor on/off/blink
	ESC <everythingelse>  --> write <everythingelse> to display as char
	ESC w 0/1             --> enable/disable windmill rotation, heart blink
*/

enum tcp_server_conn_state {
	TCP_SERVER_CONN_STATE_DATA, /* processing normal data */
	TCP_SERVER_CONN_STATE_ESC,  /* received ESC */
	TCP_SERVER_CONN_STATE_ESC_CMD, /* received ESC c, wait for cmd byte */
	TCP_SERVER_CONN_STATE_ESC_GLYPH, /* received ESC g, wait for glyph num */
	TCP_SERVER_CONN_STATE_ESC_GLYPH_DATA, /* received ESC g <num>, wait for glyph data */
	TCP_SERVER_CONN_STATE_ESC_GOTO_MSB,
	TCP_SERVER_CONN_STATE_ESC_GOTO_LSB,
	TCP_SERVER_CONN_STATE_ESC_NL,
	TCP_SERVER_CONN_STATE_ESC_BRIGHT,
	TCP_SERVER_CONN_STATE_ESC_CURSOR,
	TCP_SERVER_CONN_STATE_ESC_WINDMILL,
};

struct tcp_server_global {
	unsigned int nconns;
};

/* user data */
struct tcp_server_conn {
	struct tcp_server_global *server;

	enum tcp_server_conn_state state;
	unsigned char esc_cmd;

	unsigned char glyph;
	unsigned char cgrambuf[HD44780_FONT_NBYTES+1];
	unsigned int cgram_nwrite;
	unsigned int disp_ptr;
	unsigned char ddaddr;
	int drop; /* set to 1 to drop this connection */
};

static unsigned char
hex2bin(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return 10 + (c - 'a');
	if (c >= 'A' && c <= 'F')
		return 10 + (c - 'A');
	return 0;
}

static void
tcp_server_conn_close(struct bufferevent *bev,struct tcp_server_conn *conn) {
	conn->server->nconns--;
	if (conn->server->nconns == 0)
		statuspages_on(bufferevent_get_base(bev));
	free(conn);
	bufferevent_free(bev);
}

static int
tcp_server_conn_eat_char(struct bufferevent *bev, struct tcp_server_conn *conn, unsigned char c)
{
	switch (conn->state) {
	case TCP_SERVER_CONN_STATE_DATA:
		if (c == '\r') /* ignore CR */
			break;
		else if (c == '\n')
			hd44780_display_linefeed();
		else if (c == '\033')
			conn->state = TCP_SERVER_CONN_STATE_ESC;
		else
			hd44780_display_putc(c);
		break;
	case TCP_SERVER_CONN_STATE_ESC:
		conn->state = TCP_SERVER_CONN_STATE_DATA;
		if (c == 'c')
			hd44780_display_clear();
		else if (c == 'h')
			hd44780_display_home();
		else if (c == '$')
			conn->state = TCP_SERVER_CONN_STATE_ESC_CMD;
		else if (c == 'g')
			conn->state = TCP_SERVER_CONN_STATE_ESC_GLYPH;
		else if (c == 'p')
			conn->state = TCP_SERVER_CONN_STATE_ESC_GOTO_MSB;
		else if (c == 'q')
			conn->drop = 1;
		else if (c == 'b')
			conn->state = TCP_SERVER_CONN_STATE_ESC_BRIGHT;
		else if (c == 'C')
			conn->state = TCP_SERVER_CONN_STATE_ESC_CURSOR;
		else if (c == 'w')
			conn->state = TCP_SERVER_CONN_STATE_ESC_WINDMILL;
		else if (c == '\n' || c == '\r')
			conn->state = TCP_SERVER_CONN_STATE_ESC_NL;
		else
			hd44780_display_putc(c);
		break;
	case TCP_SERVER_CONN_STATE_ESC_CMD:
		hd44780_write_byte(c, HD44780_CMD);
		conn->state = TCP_SERVER_CONN_STATE_DATA;
		break;
	case TCP_SERVER_CONN_STATE_ESC_GLYPH:
		if (c > 7) {
			conn->state = TCP_SERVER_CONN_STATE_DATA;
			break;
		}
		conn->glyph = c;
		conn->state = TCP_SERVER_CONN_STATE_ESC_GLYPH_DATA;
		conn->cgram_nwrite = 0;
		break;
	case TCP_SERVER_CONN_STATE_ESC_GLYPH_DATA:
		conn->cgrambuf[conn->cgram_nwrite++] = c;
		if (conn->cgram_nwrite >= HD44780_FONT_NBYTES) {
			hd44780_font_setchar(conn->glyph, conn->cgrambuf);
			conn->state = TCP_SERVER_CONN_STATE_DATA;
		}
		break;
	case TCP_SERVER_CONN_STATE_ESC_GOTO_MSB:
		conn->ddaddr = hex2bin(c) << 4;
		conn->state = TCP_SERVER_CONN_STATE_ESC_GOTO_LSB;
		break;
	case TCP_SERVER_CONN_STATE_ESC_GOTO_LSB:
		conn->ddaddr |= hex2bin(c);
		conn->state = TCP_SERVER_CONN_STATE_DATA;
		hd44780_display_goto(conn->ddaddr);
		break;
	/* to be able to enter text without newline, <esc> NR X or <esc> CR X
	   will just be swallowed */
	case TCP_SERVER_CONN_STATE_ESC_NL:
		conn->state = TCP_SERVER_CONN_STATE_DATA;
		break;
	case TCP_SERVER_CONN_STATE_ESC_BRIGHT:
		if (c >= '0' && c <= '3')
			c -= '0';
		if (c <= 3)
			hd44780_noritake_brightness(c);
		conn->state = TCP_SERVER_CONN_STATE_DATA;
		break;
	case TCP_SERVER_CONN_STATE_ESC_CURSOR:
		if (c == 0 || c == '0') /* display on, cursor off, cursor blink off */	
			hd44780_write_byte(HD44780_ONOFF(1, 0, 0), HD44780_CMD);
		if (c == 1 || c == '1') /* display on, cursor on, blink off */	
			hd44780_write_byte(HD44780_ONOFF(1, 1, 0), HD44780_CMD);
		if (c == 2 || c == '2') /* display on, cursor off, blink on */	
			hd44780_write_byte(HD44780_ONOFF(1, 0, 1), HD44780_CMD);
		conn->state = TCP_SERVER_CONN_STATE_DATA;
		break;
	case TCP_SERVER_CONN_STATE_ESC_WINDMILL:
		if (c == 0 || c == '0') /* disable windmill rotation */	
			hd44780_font_windmill(bufferevent_get_base(bev), 0);
		if (c == 1 || c == '1') /* disable windmill rotation */	
			hd44780_font_windmill(bufferevent_get_base(bev), 1);
		conn->state = TCP_SERVER_CONN_STATE_DATA;
		break;
	}

	return 0;
}

static void
tcp_server_read_cb(struct bufferevent *bev, void *ctx)
{
	struct tcp_server_conn *conn = (struct tcp_server_conn*) ctx;
	unsigned char c;
	while (1) {
		if (bufferevent_read(bev, &c, 1) == 0)
			break;
		tcp_server_conn_eat_char(bev, conn, c);
	}
	if (conn->drop)
		tcp_server_conn_close(bev, conn);
}

static void
tcp_server_write_cb(struct bufferevent *bev, void *ctx)
{
	struct tcp_server_conn *conn = (struct tcp_server_conn*) ctx;

	(void) bev;
	(void) ctx;
	(void) conn;
}

static void
tcp_server_event_cb(struct bufferevent *bev, short what, void *ctx)
{
	struct tcp_server_conn *conn = (struct tcp_server_conn*) ctx;
	tcp_server_conn_close(bev, conn);
}


/* connection listener callback */
static void
tcp_server_accept_cb(struct evconnlistener *listener, evutil_socket_t fd,
	struct sockaddr *sa, int socklen, void *arg)
{
	struct event_base *base = NULL;
	struct bufferevent *bev = NULL;
	struct tcp_server_conn *conn = NULL;
	struct tcp_server_global *server = (struct tcp_server_global*)arg;


	(void) sa;
	(void) socklen;
	(void) arg;

	conn = malloc(sizeof(struct tcp_server_conn));
	memset(conn, '\0', sizeof(*conn));

	conn->server = server;
	conn->state = TCP_SERVER_CONN_STATE_DATA;

	base = evconnlistener_get_base(listener);
	if(!(bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE)))
		goto err_out;

	if (server->nconns == 0)
		statuspages_off();
	server->nconns++;

	bufferevent_setcb(bev, tcp_server_read_cb, tcp_server_write_cb,
		tcp_server_event_cb, conn/*ctx*/);
	bufferevent_enable(bev, EV_READ /*|EV_WRITE*/);

	return;
err_out:
	if (conn)
		free(conn);
	bufferevent_free(bev);
	return;
}

int
tcp_server_init(struct event_base *evtbase, int listen_portno)
{
	struct sockaddr_in listen_addr = { };
	struct evconnlistener *ecl;

	struct tcp_server_global *server;

	server = malloc(sizeof(struct tcp_server_global));
	memset(server, '\0', sizeof(*server));

	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = INADDR_ANY;
	listen_addr.sin_port = htons(listen_portno);

	ecl = evconnlistener_new_bind(evtbase, tcp_server_accept_cb, server,
		LEV_OPT_CLOSE_ON_FREE|LEV_OPT_CLOSE_ON_EXEC|LEV_OPT_REUSEABLE,
		5/*backlog*/,
		(struct sockaddr*) &listen_addr, sizeof(listen_addr));
	if (!ecl) {
		perror("Cannot listen on socket");
		return -1;
	}

	return 0;
}