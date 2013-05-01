# ubus - unix bus
# See COPYING for copyright and license details.

include ../config.mk

EXAMPLES = echo clock

all: options ${EXAMPLES} ../lib/libubus.a

options:
	@echo ubus examples build options
	@echo "  CFLAGS   = ${CFLAGS}"
	@echo "  LDFLAGS  = ${LDFLAGS} -lubus"
	@echo "  CC       = ${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

${EXAMPLES}: ${EXAMPLES:=.o}
	${CC} -o $@ ${@:=.o} ${LDFLAGS} -lubus

clean: 
	rm -f *.o *.a ${BINS} ${EXAMPLES}

.PHONY: all options clean

