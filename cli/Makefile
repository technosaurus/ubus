objects = ubus-listen ubus-call ubus-signal

all: $(objects)

clean: 
	rm -f $(objects) common.o

$(objects): %: %.c common.h common.o
	$(CC) $(CFLAGS) common.o $< -o $@
