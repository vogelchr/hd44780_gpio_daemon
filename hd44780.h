#ifndef HD44780_H
#define HD44780_H

#include <stdlib.h>  /* size_t */

/* 
 * https://www.sparkfun.com/datasheets/LCD/HD44780.pdf
 * Table 6 Instructions
 *
 * Entry mode set (HD44780_ENTRYMODE(id,s))
 *  Set cursor move direction and specify display shift.
 *   id  1=increment, 0=decrement
 *    s  1=accompany display shift
 *
 * Display on off (HD44780_ONOFF(d,c,b))
 *  Turn display(d) on/off, cursor(c) on/off, cursor character blink(b)
 *
 * Cursor of Display Shift (HD44780_CURS(sc,rl))
 *  Moves cursor and shifts display without changing DDRAM contents
 *   sc  1=display shift,0=cursor move
 *   rl  1=shift to right, 0=shift to left 
 *
 * Function Set HD44780_FUNC(dl,n,f)
 *  Set interface data length (dl) number of displayl ines (n) and font (f).
 *    dl  1=8 bit, 0=4 bit
 *    n   1=2 lines, 0=1 line
 *    f   1=5x10 dots, 0=5x8 dots
 *
 */

/* commands */

#define HD44780_BIT(n,cond)   ((cond)?(1<<(n)):0)
#define HD44780_CLEAR                 0x01
#define HD44780_HOME                  0x02
#define HD44780_ENTRYMODE(id, s)     (0x04|HD44780_BIT(1,id)|HD44780_BIT(0,s))
#define HD44780_ONOFF(d,c,b)         (0x08|HD44780_BIT(2,d) |HD44780_BIT(1,c) |HD44780_BIT(0,b))
#define HD44780_CURS(sc,rl)          (0x10|HD44780_BIT(3,sc)|HD44780_BIT(2,rl))
#define HD44780_FUNC(dl,n,f)         (0x20|HD44780_BIT(4,dl)|HD44780_BIT(3,n) |HD44780_BIT(2,f))
#define HD44780_CGADDR(a)            (0x40|((a)&0x3f))
#define HD44780_DDADDR(a)            (0x80|((a)&0x7f))

enum HD44780_ISCMD {
	HD44780_CMD=0,  /* RS = 0 : command */
	HD44780_DATA=1  /* RS = 1 : data */
};

extern int
hd44780_init();

/* write a 8-bit command or 8-bit data to the display */
extern void
hd44780_write_byte(unsigned char v, enum HD44780_ISCMD iscmd);

/* write a buffer of data */
extern void
hd44780_write_buf(const unsigned char *v, size_t len);

extern void
hd44780_noritake_brightness(unsigned int c);

#endif
