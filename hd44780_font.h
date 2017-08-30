#ifndef HD44780_FONT_H
#define HD44780_FONT_H

#define HD44780_FONT_WINDMILL '\x07'
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
hd44780_font_setchar(char c, const char *font);

extern void
hd44780_font_set_windmill_phase(unsigned i);

extern void
hd44780_font_windmill_advance(unsigned *i);


#endif