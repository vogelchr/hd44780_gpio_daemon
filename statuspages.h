#ifndef STATUSPAGES_H
#define STATUSPAGES_H

#include <unistd.h>

#define STATUSPAGES_STAY   0  /* please keep this statuspage shown for now */
#define STATUSPAGES_NEXT   1  /* please advance to the next statuspage */

struct statuspage {
	int (*fct_enter)(void);
	int (*fct_update)(useconds_t *wait_usec);
};

extern struct statuspage **statuspages;


#endif