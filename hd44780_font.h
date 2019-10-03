#ifndef HD44780_FONT_H
#define HD44780_FONT_H

#define HD44780_FONT_HEART '\x04'    /* 3> */
#define HD44780_FONT_SMILE '\x05'    /* :-) */
#define HD44780_FONT_ELLIPSIS '\x06' /* ... */
#define HD44780_FONT_WINDMILL '\x07' /* rotating bar */
#define HD44780_FONT_NBYTES    8     /* 8 bytes per char */
#define HD44780_FONT_WINDMILL_NCHARS 9

/*
 *  [0] = 0bxxx54321      54321
 *  [1] = 0bxxxA9876      A9876
 *  [2] = 0bxxxFEDCB      FEDCB
 *  [3] = 0bxxxKJIHG      KJIHG
 *  [4] = 0bxxxPONML      PONML <- 5x7 character
 *  [5] = 0bxxxUTSRQ      UTSRQ
 *  [6] = 0bxxxZYXWV      ZYXWV
 *  [7] = 0bxxx@xxxx      @@@@@ <- underline
 */

extern void
hd44780_font_init();

extern void
hd44780_font_setchar(char c, const unsigned char *font);

struct event_base;
extern void
hd44780_font_windmill(struct event_base *evtbase, int onoff);

#endif