#ifndef STATUSPAGES_H
#define STATUSPAGES_H

struct event_base;

extern void statuspages_on(struct event_base *evtbase);
extern void statuspages_off();

#endif