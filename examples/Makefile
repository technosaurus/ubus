# ubus - unix bus
# See COPYING for copyright and license details.

include ../config.mk

EXAMPLES = echo clock

all: options ${EXAMPLES} ../lib/libubus.so

options:
	@echo ubus examples build options
	@echo "  CFLAGS   = ${CFLAGS}"
	@echo "  LDFLAGS  = ${LDFLAGS} -lubus"
	@echo "  CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${EXAMPLES}: ${EXAMPLES:=.o}
	@echo LD  $@
	@${CC} -o $@ $< ${LDFLAGS} -lubus

clean: 
	@echo cleaning
	@rm -f *.o *.a ${BINS} ${EXAMPLES}

.PHONY: all options clean

