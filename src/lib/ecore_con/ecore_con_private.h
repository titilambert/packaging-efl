#ifndef _ECORE_CON_PRIVATE_H
#define _ECORE_CON_PRIVATE_H

#include "ecore_private.h"
#include "Ecore_Con.h"

#define ECORE_MAGIC_CON_SERVER             0x77665544
#define ECORE_MAGIC_CON_CLIENT             0x77556677
#define ECORE_MAGIC_CON_URL                0x77074255

#define ECORE_CON_TYPE 0x0f
#define ECORE_CON_SSL  0xf0
#define ECORE_CON_SUPER_SSL  0xf00

#if HAVE_GNUTLS
# include <gnutls/gnutls.h>
#elif HAVE_OPENSSL
# include <openssl/ssl.h>
#endif

#define READBUFSIZ 65536

extern int _ecore_con_log_dom;

#ifdef ECORE_CON_DEFAULT_LOG_COLOR
#undef ECORE_LOG_DEFAULT_LOG_COLOR
#endif
#define ECORE_CON_DEFAULT_LOG_COLOR EINA_COLOR_BLUE

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_ecore_con_log_dom, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_ecore_con_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_ecore_con_log_dom, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_ecore_con_log_dom, __VA_ARGS__)

#ifdef CRI
# undef CRI
#endif
#define CRI(...) EINA_LOG_DOM_CRIT(_ecore_con_log_dom, __VA_ARGS__)

typedef struct _Ecore_Con_Lookup Ecore_Con_Lookup;
typedef struct _Ecore_Con_Info Ecore_Con_Info;
typedef struct Ecore_Con_Socks Ecore_Con_Socks_v4;
typedef struct Ecore_Con_Socks_v5 Ecore_Con_Socks_v5;
typedef void (*Ecore_Con_Info_Cb)(void *data, Ecore_Con_Info *infos);

typedef enum _Ecore_Con_State
{
   ECORE_CON_CONNECTED,
   ECORE_CON_DISCONNECTED,
   ECORE_CON_INPROGRESS
} Ecore_Con_State;

typedef enum _Ecore_Con_Ssl_Error
{
   ECORE_CON_SSL_ERROR_NONE = 0,
   ECORE_CON_SSL_ERROR_NOT_SUPPORTED,
   ECORE_CON_SSL_ERROR_INIT_FAILED,
   ECORE_CON_SSL_ERROR_SERVER_INIT_FAILED,
   ECORE_CON_SSL_ERROR_SSL2_NOT_SUPPORTED
} Ecore_Con_Ssl_Error;

typedef enum _Ecore_Con_Ssl_Handshake
{
   ECORE_CON_SSL_STATE_DONE = 0,
   ECORE_CON_SSL_STATE_HANDSHAKING,
   ECORE_CON_SSL_STATE_INIT
} Ecore_Con_Ssl_State;

typedef enum Ecore_Con_Proxy_State
{  /* named PROXY instead of SOCKS in case some handsome and enterprising
    * developer decides to add HTTP CONNECT support
    */
   ECORE_CON_PROXY_STATE_DONE = 0,
   ECORE_CON_PROXY_STATE_RESOLVED,
   ECORE_CON_PROXY_STATE_INIT,
   ECORE_CON_PROXY_STATE_READ,
   ECORE_CON_PROXY_STATE_AUTH,
   ECORE_CON_PROXY_STATE_REQUEST,
   ECORE_CON_PROXY_STATE_CONFIRM,
} Ecore_Con_Proxy_State;

struct _Ecore_Con_Client_Data
{
#ifdef _WIN32
   SOCKET fd;
#else
   int fd;
#endif
   Ecore_Con_Server *host_server;
   void *data;
   Ecore_Fd_Handler *fd_handler;
   size_t buf_offset;
   Eina_Binbuf *buf;
   const char *ip;
   Eina_List *event_count;
   struct sockaddr *client_addr;
   int client_addr_len;
   double start_time;
   Ecore_Timer *until_deletion;
   double disconnect_time;
#if HAVE_GNUTLS
   gnutls_datum_t session_ticket;
   gnutls_session_t session;
#elif HAVE_OPENSSL
   SSL *ssl;
   int ssl_err;
#endif
   Ecore_Con_Ssl_State ssl_state;
   Eina_Bool handshaking : 1;
   Eina_Bool upgrade : 1; /* STARTTLS queued */
   Eina_Bool delete_me : 1; /* del event has been queued */
};

typedef struct _Ecore_Con_Client_Data Ecore_Con_Client_Data;

struct _Ecore_Con_Server_Data
{
#ifdef _WIN32
   SOCKET fd;
#else
   int fd;
#endif
   Ecore_Con_Type type;
   char *name;
   int port;
   char *path;
   void *data;
   Ecore_Fd_Handler *fd_handler;
   Eina_List *clients;
   unsigned int client_count;
   Eina_Binbuf *buf;
   size_t write_buf_offset;
   Eina_List *infos;
   Eina_List *event_count;
   int client_limit;
   pid_t ppid;
   /* socks */
   Ecore_Con_Socks *ecs;
   Ecore_Con_Proxy_State ecs_state;
   int ecs_addrlen;
   unsigned char ecs_addr[16];
   size_t ecs_buf_offset;
   Eina_Binbuf *ecs_buf;
   Eina_Binbuf *ecs_recvbuf;
   const char *proxyip;
   int proxyport;
   /* endsocks */
   const char *verify_name;
#if HAVE_GNUTLS
   gnutls_session_t session;
   gnutls_anon_client_credentials_t anoncred_c;
   gnutls_anon_server_credentials_t anoncred_s;
   gnutls_psk_client_credentials_t pskcred_c;
   gnutls_psk_server_credentials_t pskcred_s;
   gnutls_certificate_credentials_t cert;
   char *cert_file;
   gnutls_dh_params_t dh_params;
#elif HAVE_OPENSSL
   SSL_CTX *ssl_ctx;
   SSL *ssl;
   int ssl_err;
#endif
   double start_time;
   Ecore_Timer *until_deletion;
   double disconnect_time;
   double client_disconnect_time;
   const char *ip;
   Eina_Bool created : 1; /* @c EINA_TRUE if server is our listening server */
   Eina_Bool connecting : 1; /* @c EINA_FALSE if just initialized or connected */
   Eina_Bool handshaking : 1; /* @c EINA_TRUE if server is ssl handshaking */
   Eina_Bool upgrade : 1;  /* STARTTLS queued */
   Eina_Bool disable_proxy : 1; /* proxy should never be used with this connection */
   Eina_Bool ssl_prepared : 1;
   Eina_Bool use_cert : 1; /* @c EINA_TRUE if using certificate auth */
   Ecore_Con_Ssl_State ssl_state; /* current state of ssl handshake on the server */
   Eina_Bool verify : 1; /* @c EINA_TRUE if certificates will be verified */
   Eina_Bool verify_basic : 1; /* @c EINA_TRUE if certificates will be verified only against the hostname */
   Eina_Bool reject_excess_clients : 1;
   Eina_Bool delete_me : 1; /* del event has been queued */
#ifdef _WIN32
   Eina_Bool want_write : 1;
   Eina_Bool read_stop : 1;
   Eina_Bool read_stopped : 1;
   HANDLE pipe;
   HANDLE thread_read;
   HANDLE event_read;
   HANDLE event_peek;
   DWORD nbr_bytes;
#endif
};

typedef struct _Ecore_Con_Server_Data Ecore_Con_Server_Data;

struct _Ecore_Con_Info
{
   unsigned int size;
   struct addrinfo info;
   char ip[NI_MAXHOST];
   char service[NI_MAXSERV];
};

struct _Ecore_Con_Lookup
{
   Ecore_Con_Dns_Cb done_cb;
   const void *data;
};

struct Ecore_Con_Socks /* v4 */
{
   unsigned char version;

   const char *ip;
   int port;
   const char *username;
   unsigned int ulen;
   Eina_Bool lookup : 1;
   Eina_Bool bind : 1;
};

struct Ecore_Con_Socks_v5
{
   unsigned char version;

   const char *ip;
   int port;
   const char *username;
   unsigned int ulen;
   Eina_Bool lookup : 1;
   Eina_Bool bind : 1;
   /* v5 only */
   unsigned char method;
   const char *password;
   unsigned int plen;
};

#ifdef HAVE_SYSTEMD
extern int sd_fd_index;
extern int sd_fd_max;
#endif

extern Ecore_Con_Socks *_ecore_con_proxy_once;
extern Ecore_Con_Socks *_ecore_con_proxy_global;
void ecore_con_socks_init(void);
void ecore_con_socks_shutdown(void);
Eina_Bool ecore_con_socks_svr_init(Ecore_Con_Server *svr);
void ecore_con_socks_read(Ecore_Con_Server *svr, unsigned char *buf, int num);
void ecore_con_socks_dns_cb(const char *canonname, const char *ip, struct sockaddr *addr, int addrlen, Ecore_Con_Server *svr);
/* from ecore_con.c */
void ecore_con_server_infos_del(Ecore_Con_Server *svr, void *info);
void ecore_con_event_proxy_bind(Ecore_Con_Server *svr);
void ecore_con_event_server_data(Ecore_Con_Server *svr, unsigned char *buf, int num, Eina_Bool duplicate);
void ecore_con_event_server_del(Ecore_Con_Server *svr);
#define ecore_con_event_server_error(svr, error) _ecore_con_event_server_error((svr), (char*)(error), EINA_TRUE)
void _ecore_con_event_server_error(Ecore_Con_Server *svr, char *error, Eina_Bool duplicate);
void ecore_con_event_client_add(Ecore_Con_Client *cl);
void ecore_con_event_client_data(Ecore_Con_Client *cl, unsigned char *buf, int num, Eina_Bool duplicate);
void ecore_con_event_client_del(Ecore_Con_Client *cl);
void ecore_con_event_client_error(Ecore_Con_Client *cl, const char *error);
void _ecore_con_server_kill(Ecore_Con_Server *svr);
void _ecore_con_client_kill(Ecore_Con_Client *cl);
/* from ecore_local_win32.c */
#ifdef _WIN32
Eina_Bool ecore_con_local_listen(Ecore_Con_Server *svr);
Eina_Bool ecore_con_local_connect(Ecore_Con_Server *svr,
                            Eina_Bool (*cb_done)(void *data,
                                                 Ecore_Fd_Handler *fd_handler));
Eina_Bool ecore_con_local_win32_server_flush(Ecore_Con_Server *svr);
Eina_Bool ecore_con_local_win32_client_flush(Ecore_Con_Client *cl);
void      ecore_con_local_win32_server_del(Ecore_Con_Server *svr);
void      ecore_con_local_win32_client_del(Ecore_Con_Client *cl);
#else
/* from ecore_local.c */
int ecore_con_local_init(void);
int ecore_con_local_shutdown(void);
int ecore_con_local_connect(Ecore_Con_Server *svr,
                            Eina_Bool (*cb_done)(
                               void *data,
                               Ecore_Fd_Handler *fd_handler),
                            void *data);
int ecore_con_local_listen(Ecore_Con_Server *svr,
                           Eina_Bool (*cb_listen)(
                              void *data,
                              Ecore_Fd_Handler *fd_handler),
                           void *data);
#endif

/* from ecore_con_info.c */
int                 ecore_con_info_init(void);
int                 ecore_con_info_shutdown(void);
int                 ecore_con_info_tcp_connect(Ecore_Con_Server *svr,
                                               Ecore_Con_Info_Cb done_cb,
                                               void *data);
int                 ecore_con_info_tcp_listen(Ecore_Con_Server *svr,
                                              Ecore_Con_Info_Cb done_cb,
                                              void *data);
int                 ecore_con_info_udp_connect(Ecore_Con_Server *svr,
                                               Ecore_Con_Info_Cb done_cb,
                                               void *data);
int                 ecore_con_info_udp_listen(Ecore_Con_Server *svr,
                                              Ecore_Con_Info_Cb done_cb,
                                              void *data);
int                 ecore_con_info_mcast_listen(Ecore_Con_Server *svr,
                                                Ecore_Con_Info_Cb done_cb,
                                                void *data);
void                ecore_con_info_data_clear(void *info);

void ecore_con_event_server_add(Ecore_Con_Server *svr);


/* from ecore_con_ssl.c */
Ecore_Con_Ssl_Error ecore_con_ssl_init(void);
Ecore_Con_Ssl_Error ecore_con_ssl_shutdown(void);
Ecore_Con_Ssl_Error ecore_con_ssl_server_prepare(Ecore_Con_Server *svr, int ssl_type);
Ecore_Con_Ssl_Error ecore_con_ssl_server_init(Ecore_Con_Server *svr);
Ecore_Con_Ssl_Error ecore_con_ssl_server_shutdown(Ecore_Con_Server *svr);
int                 ecore_con_ssl_server_read(Ecore_Con_Server *svr,
                                              unsigned char *buf,
                                              int size);
int                 ecore_con_ssl_server_write(Ecore_Con_Server *svr,
                                               const unsigned char *buf,
                                               int size);
Ecore_Con_Ssl_Error ecore_con_ssl_client_init(Ecore_Con_Client *svr);
Ecore_Con_Ssl_Error ecore_con_ssl_client_shutdown(Ecore_Con_Client *svr);
int                 ecore_con_ssl_client_read(Ecore_Con_Client *svr,
                                              unsigned char *buf,
                                              int size);
int                 ecore_con_ssl_client_write(Ecore_Con_Client *svr,
                                               const unsigned char *buf,
                                               int size);

int                 ecore_con_info_get(Ecore_Con_Server *svr,
                                       Ecore_Con_Info_Cb done_cb,
                                       void *data,
                                       struct addrinfo *hints);


#define GENERIC_ALLOC_FREE_HEADER(TYPE, Type) \
  TYPE *Type##_alloc(void);		      \
  void Type##_free(TYPE *e);

GENERIC_ALLOC_FREE_HEADER(Ecore_Con_Event_Client_Add, ecore_con_event_client_add);
GENERIC_ALLOC_FREE_HEADER(Ecore_Con_Event_Client_Del, ecore_con_event_client_del);
GENERIC_ALLOC_FREE_HEADER(Ecore_Con_Event_Client_Write, ecore_con_event_client_write);
GENERIC_ALLOC_FREE_HEADER(Ecore_Con_Event_Client_Data, ecore_con_event_client_data);
GENERIC_ALLOC_FREE_HEADER(Ecore_Con_Event_Server_Error, ecore_con_event_server_error);
GENERIC_ALLOC_FREE_HEADER(Ecore_Con_Event_Client_Error, ecore_con_event_client_error);
GENERIC_ALLOC_FREE_HEADER(Ecore_Con_Event_Server_Add, ecore_con_event_server_add);
GENERIC_ALLOC_FREE_HEADER(Ecore_Con_Event_Server_Del, ecore_con_event_server_del);
GENERIC_ALLOC_FREE_HEADER(Ecore_Con_Event_Server_Write, ecore_con_event_server_write);
GENERIC_ALLOC_FREE_HEADER(Ecore_Con_Event_Server_Data, ecore_con_event_server_data);
GENERIC_ALLOC_FREE_HEADER(Ecore_Con_Event_Proxy_Bind, ecore_con_event_proxy_bind);

void ecore_con_mempool_init(void);
void ecore_con_mempool_shutdown(void);

#undef GENERIC_ALLOC_FREE_HEADER

#endif
