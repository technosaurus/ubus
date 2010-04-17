#ifndef NO_SELECT
#include <sys/select.h>
#endif

typedef void ubus_t;
typedef void ubus_chan_t;

typedef enum{
    UBUS_INIT,
    UBUS_ERROR,
    UBUS_LURKING,
    UBUS_CONNECTED,
    UBUS_READY,
    UBUS_EOF,
    UBUS_ACCEPTING
}  UBUS_STATUS;

typedef enum{
    UBUS_NO_ACTIVATE_FLAGS =0,
    UBUS_IGNORE_INBOUND    =1,
}  UBUS_ACTIVATE_FLAGS;

ubus_t *      ubus_create     (const char * uri);
int           ubus_fd         (ubus_t *);
ubus_chan_t * ubus_accept     (ubus_t *);
void          ubus_destroy    (ubus_t *);

int           ubus_broadcast  (ubus_t *, const void * buff, int len);
ubus_chan_t * ubus_ready_chan (ubus_t *);
ubus_chan_t * ubus_fresh_chan (ubus_t *);

#ifndef NO_SELECT
int           ubus_select_all     (ubus_t * , fd_set * );
void          ubus_activate_all   (ubus_t * , fd_set * ,int flags);
#endif

ubus_chan_t * ubus_connect    (const char * uri);
UBUS_STATUS   ubus_status     (ubus_chan_t *);
int           ubus_chan_fd    (ubus_chan_t *);
UBUS_STATUS   ubus_activate   (ubus_chan_t *);
int           ubus_write      (ubus_chan_t *, const void * buff, int len);
int           ubus_read       (ubus_chan_t *, void * buff, int len);
void          ubus_close      (ubus_chan_t *);
void          ubus_disconnect (ubus_chan_t *);
