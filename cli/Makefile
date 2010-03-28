objects = ubus-listen ubus-call ubus-signal ubus-method

all: $(objects)

clean: 
	rm -f $(objects) common.o

$(objects): %: %.c common.h common.o
	$(CC) -g $(CFLAGS) common.o $< -o $@
