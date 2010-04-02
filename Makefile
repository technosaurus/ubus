ubus: .obj/ubus.o libubus.a
	$(CC) -static  $(LDLAGS) $< libubus.a -o $@
.obj/ubus.o: .obj cli/ubus.c
	$(CC) -I./lib/ $(CFLAGS) -c cli/ubus.c -o $@

libubus.a: .obj/libubus.o 
	$(AR) rcs libubus.a .obj/libubus.o

.obj/libubus.o: .obj lib/ubus.c lib/ubus.h lib/util.c
	$(CC) $(CFLAGS) -c lib/ubus.c -o .obj/libubus.o

.obj:
	mkdir .obj

clean: 
	rm -rf .obj
	rm -f ubus libubus.a
install: ubus
	cp libubus.a /usr/lib/
	cp lib/ubus.h /usr/include/
	cp ubus ubus_
	mv ubus_ /usr/bin/ubus
