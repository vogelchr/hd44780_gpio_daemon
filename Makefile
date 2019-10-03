CFLAGS=-Wall -Wextra -ggdb -g3 -Os
CPPFLAGS=
LIBS=-levent

# allow compilation on my x86-pc
ifeq ($(shell uname -m), x86_64)
	CPPFLAGS=-DDISABLE_WIRINGPI
else
	LIBS += -lwiringPi
endif

all : hd44780_gpio_daemon

OBJECTS := hd44780_gpio_daemon.o hd44780.o hd44780_font.o statuspages.o tcp_server.o hd44780_display.o

hd44780_gpio_daemon : $(OBJECTS)
	$(CC) -o $@ $^ $(LIBS)

ifneq ($(MAKECMDGOALS),clean)
include $(OBJECTS:.o=.d)
endif

%.d : %.c
	$(CC) $(CPPFLAGS) -MM -o $@ $^

.PHONY : clean remote install
clean :
	rm -f *~ *.d *.o hd44780_gpio_daemon

remote :
	rsync -av --exclude "*.o" --exclude "*.d" --exclude hd44780_gpio_daemon \
		--exclude ".*" . --delete rpi0-cvogel:hd44780_gpio_daemon
	ssh rpi0-cvogel cd hd44780_gpio_daemon \; make

install : hd44780_gpio_daemon
	install -m755 hd44780_gpio_daemon /usr/local/sbin

	@echo "***"
	@echo "*** The next command may fail."
	@echo "***"
	@echo ""
	-systemctl stop hd44780_gpio_daemon

	install -m644 hd44780_gpio_daemon.service /etc/systemd/system
	systemctl daemon-reload

	systemctl enable hd44780_gpio_daemon
	systemctl start hd44780_gpio_daemon
