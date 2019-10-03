hd44780_gpio_daemon

A daemon to drive a HD44780 compatible alphanumeric display connected to the GPIOs of
a raspberry pi. I am using a Noritake CU16025 Vacuum Fluorescence Display.

Currently a 16x2 display is hardcoded.

Hardware Connections:

Connect the following pins to your raspberry PI GPIO.

HD44780   Raspberry Pi, Connector P1 (GPIO)
---------:---------------------------------
1 GND     GND
2 Vcc     +5V (Pin 2, Pin 4)
3 -       -
4 RS      GPIO 17 (Pin 11)
5 R/~W    GND
6 ~E      GPIO 27 (Pin 13)
7 D0      -
8 D1      -
9 D2      -
10 D3     -
11 D4     GPIO 22 (Pin 15)
12 D5     GPIO 23 (Pin 16)
13 D6     GPIO 24 (Pin 18)
14 D7     GPIO 25 (Pin 22)

To change this mapping, edit hd44780.c.

Compilation:

Type make. You need to have wiringPi and libevent2 installed.

Running "make install" will copy the executable to /usr/local/sbin, and a
systemd unit file to /etc/system/systemd. The unit file will also be enabled
and started. You should now see a clock display on your LCD or VFD.

The program will listen on a socket (by default on port 54321, can be changed)
with the "-p" program option. If you connect to this socket, you can send data
that gets shown on the screen. A few ESC control codes are understood.

   ESC c              clears the display
   ESC h              send cursor to the upper left corner of the display
   ESC $ cmd          send a raw command byte to the display
   ESC g <c> <data..> redefine user-configurable glyph by sending ESC, g, then
                      the glyph to redefine (0..7) and then 8 bytes of data
                      corresponding to the graphics bitmap of this glyph.
                      The first byte defines the uppermost line, with the
                      following bytes defining successive lines. The display
                      only supports 7 lines, the last byte defines whether the
                      underscore segment is activated. Columns are LSB right.
   ESC p <hex> <hex>  goto position 00..1f (first line) and 40..5f (2nd line)
   ESC q              close TCP connection

   ESC \n <x> 
   ESC \r <x>         these two commands are just ignored, it's useful when
                      typing in stuff on the keyboard where ENTER generates
                      \r\n (or \n\r) and one wants to drop this data

   ESC b <x>          set brignthess from 0..3 (either byte 0..3 or '0'..'3')
                      0 is brigtest, 3 is dimmest, this is special to
                      Noritake VFDs

   ESC C <x>          disable(0)/enable(1) cursor (or set it to blink(2))

   ESC w <x>          disable(0)/enable(1) animation of windmill
                      (and heart, and smiley)

   ESC <x>            for all undefined escape codes, <x> is just sent to
                      the display as data

Default glyphs:

   \4 is a heart      3>
   \5 is a smiley     :-)
   \6 is an ellipsis  ...
   \7 is a windmill   (cycles through /, -, |, \, ...)

When enabling windmill animation (ESC w 1), the heart, smiley and windmill
user defined glyphs in the character generator RAM are constantly updated.

Video:

Here's a video of the program in action: https://www.youtube.com/watch?v=DgQFyl9hzSU
