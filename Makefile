BINS= ubus ubus-connect ubus-signal
EXAMPLES= examples/echo examples/clock

all: .obj cli examples

examples: $(EXAMPLES)
examples/%: examples/%.c libubus.a
	$(CC) -static  $(LDLAGS) $< ./libubus.a -o $@

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
install: ubus
	cp libubus.a /usr/lib/
	cp lib/ubus.h /usr/include/
	cp ubus ubus_
	mv ubus_ /usr/bin/ubus
