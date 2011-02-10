# ubus - unix bus
# See COPYING for copyright and license details.

include config.mk

BINS=ubus ubus-connect ubus-signal
EXAMPLES=examples/echo examples/clock

all: .obj cli examples

examples: $(EXAMPLES)
examples/%: examples/%.c libubus.a
	$(CC) -static -Ilib $(LDLAGS) $< ./libubus.a -o $@

cli: $(BINS)
%: .obj/%.o libubus.a
	$(CC) -static  $(LDLAGS) $< libubus.a -o $@
.obj/%.o: cli/%.c
	$(CC) -static -I./lib/ $(CFLAGS) -c $< -o $@
libubus.a: .obj/libubus.o
	$(AR) rcs libubus.a .obj/libubus.o
.obj/libubus.o: lib/ubus.c lib/ubus.h lib/util.c
	$(CC) $(CFLAGS) -c lib/ubus.c -o .obj/libubus.o

.obj:
	mkdir .obj

clean: 
	rm -rf .obj
	rm -f ubus libubus.a
	rm -f $(BINS) $(EXAMPLES)
install: $(BINS)
	cp libubus.a ${PREFIX}/lib/
	cp lib/ubus.h ${PREFIX}/include/
	mkdir .tmp
	cp $(BINS) .tmp/
	mv .tmp/*  ${PREFIX}/bin/
	rm -rf .tmp
