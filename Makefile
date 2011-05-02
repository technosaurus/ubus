# ubus - unix bus
# See COPYING for copyright and license details.

include config.mk

TARGETS = lib cli examples

all: options
	@echo building ${TARGETS}
	@for i in ${TARGETS}; \
	do \
		cd $$i; \
		make; \
		cd ..; \
	done

clean:
	@echo cleaning ${TARGETS}
	@for i in ${TARGETS}; \
	do \
		cd $$i; \
		make clean; \
		cd ..; \
	done

install:
	@echo installing lib and cli files
	@for i in lib cli; \
	do \
		cd $$i; \
		make install; \
		cd ..; \
	done

uninstall:
	@echo uninstalling lib and cli files
	@for i in lib cli; \
	do \
		cd $$i; \
		make uninstall; \
		cd ..; \
	done

.PHONY: all options clean dist install uninstall

