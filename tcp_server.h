#ifndef TCP_SERVER_H
#define TCP_SERVER_H

struct event_base;
extern int tcp_server_init(struct event_base *evtbase, int listen_portno);

#endif