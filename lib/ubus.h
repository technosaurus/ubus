typedef void ubus_t;
typedef void ubus_chan_t;

typedef enum{
    UBUS_INIT,
    UBUS_ERROR,
    UBUS_LURKING,
    UBUS_CONNECTED,
    UBUS_READY,
    UBUS_EOF,
}  UBUS_STATUS;

ubus_t *      ubus_create     (const char * uri);
int           ubus_fd         (ubus_t *);
ubus_chan_t * ubus_accept     (ubus_t *);
void          ubus_destroy    (ubus_t *);

ubus_chan_t * ubus_connect    (const char * uri);
UBUS_STATUS   ubus_status     (ubus_chan_t *);
int           ubus_chan_fd    (ubus_chan_t *);
UBUS_STATUS   ubus_activate   (ubus_chan_t *);
int           ubus_write      (ubus_chan_t *, const void * buff, int len);
int           ubus_read       (ubus_chan_t *, void * buff, int len);
void          ubus_close      (ubus_chan_t *);
void          ubus_disconnect (ubus_chan_t *);


