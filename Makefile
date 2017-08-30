CFLAGS=-Wall -Wextra -ggdb -g3 -Os
CPPFLAGS=
LIBS=-lwiringPi

all : hd44780_gpio_daemon

OBJECTS := hd44780_gpio_daemon.o hd44780.o hd44780_font.o statuspages.o

hd44780_gpio_daemon : $(OBJECTS)
	$(CC) -o $@ $^ $(LIBS)

ifneq ($(MAKECMDGOALS),clean)
include $(OBJECTS:.o=.d)
endif

%.d : %.c
	$(CC) $(CPPFLAGS) -MM -o $@ $^

.PHONY : clean
clean :
	rm -f *~ *.d *.o hd44780_gpio_daemon