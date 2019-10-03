#ifndef HD44780_DISPLAY_H
#define HD44780_DISPLAY_H

extern void hd44780_display_linefeed();
extern void hd44780_display_putc(unsigned char c);
extern void hd44780_display_clear();
extern void hd44780_display_home();
extern void hd44780_display_goto(unsigned char ddaddr);
extern void hd44780_display_sync_ddaddr(); /* call after font redefinitions */

#endif