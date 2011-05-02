
#ifndef __UBUS_UTIL_H__
#define __UBUS_UTIL_H_

#include <sys/un.h>

#include "ubus.h"

typedef struct ubus_channel ubus_channel;
typedef struct ubus_service ubus_service;
struct ubus_service {
	ubus_channel *chanlist;
	int fd;
	struct sockaddr_un local;
};

struct ubus_channel {
	UBUS_STATUS status;
	int fd;
	int inotify;
	ubus_service *service;
	struct sockaddr_un remote;

	// todo: safe bytes, make those flags
	int activated;
	int isnew;

	struct ubus_channel *next;
	struct ubus_channel *prev;
};

int mkpath(const char *s, mode_t mode);
int mksocketpath(const char *s);
ubus_channel *ubus_client_add(ubus_service *service, ubus_channel *c);
ubus_channel *ubus_client_del(ubus_service *service, ubus_channel *c);

#endif

