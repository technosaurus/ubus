typedef void ubus_t;
typedef void ubus_chan_t;

ubus_t *      ubus_create     (const char * uri);
int           ubus_fd         (ubus_t *);
ubus_chan_t * ubus_accept     (ubus_t *);
void          ubus_destroy    (ubus_t *);
ubus_chan_t * ubus_connect    (const char * uri);
int           ubus_chan_fd    (ubus_chan_t *);
int           ubus_write      (ubus_chan_t *, const void * buff, int len);
int           ubus_read       (ubus_chan_t *, void * buff, int len);
void          ubus_disconnect (ubus_chan_t *);


