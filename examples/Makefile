# ubus - unix bus
# See COPYING for copyright and license details.

include ../config.mk

EXAMPLES = echo clock

all: options ${EXAMPLES}

options:
	@echo ubus build options
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${EXAMPLES}: ${EXAMPLES:=.o}
	@echo CC -o $@
	@${CC} -o $@ $< ${LDFLAGS}

clean: 
	@echo cleaning
	@rm -f *.o *.a ${BINS} ${EXAMPLES}

.PHONY: all options clean

