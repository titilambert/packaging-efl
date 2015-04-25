#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#ifdef __SUNPRO_C
# include <ieeefp.h>
#endif

#ifdef HAVE_ISFINITE
# define ECORE_FINITE(t)  isfinite(t)
#else
# ifdef _MSC_VER
#  define ECORE_FINITE(t) _finite(t)
# else
#  define ECORE_FINITE(t) finite(t)
# endif
#endif

#define FIX_HZ 1

#ifdef FIX_HZ
# ifndef _MSC_VER
#  include <sys/param.h>
# endif
# ifndef HZ
#  define HZ 100
# endif
#endif

#ifdef HAVE_EVIL
# include <Evil.h>
#endif

#ifdef HAVE_ESCAPE
# include <Escape.h>
#endif

#ifdef HAVE_EXOTIC
# include <Exotic.h>
#endif

/*
 * On Windows, pipe() is implemented with sockets.
 * Contrary to Linux, Windows uses different functions
 * for sockets and fd's: write() is for fd's and send
 * is for sockets. So I need to put some win32 code
 * here. I can't think of a solution where the win32
 * code is in Evil and not here.
 */

#define PIPE_FD_INVALID -1

#ifdef _WIN32

# include <winsock2.h>

# define pipe_write(fd, buffer, size) send((fd), (char *)(buffer), size, 0)
# define pipe_read(fd, buffer, size)  recv((fd), (char *)(buffer), size, 0)
# define pipe_close(fd)               closesocket(fd)
# define PIPE_FD_ERROR   SOCKET_ERROR

#else

# include <unistd.h>
# include <fcntl.h>

# define pipe_write(fd, buffer, size) write((fd), buffer, size)
# define pipe_read(fd, buffer, size)  read((fd), buffer, size)
# define pipe_close(fd)               close(fd)
# define PIPE_FD_ERROR   -1

#endif /* ! _WIN32 */

#include "Ecore.h"
#include "ecore_private.h"

/* How of then we should retry to write to the pipe */
#define ECORE_PIPE_WRITE_RETRY 6

struct _Ecore_Pipe
{
                     ECORE_MAGIC;
   int               fd_read;
   int               fd_write;
   Ecore_Fd_Handler *fd_handler;
   const void       *data;
   Ecore_Pipe_Cb     handler;
   unsigned int      len;
   int               handling;
   size_t            already_read;
   void             *passed_data;
   int               message;
   Eina_Bool         delete_me : 1;
};
GENERIC_ALLOC_SIZE_DECLARE(Ecore_Pipe);

static Eina_Bool _ecore_pipe_read(void             *data,
                                  Ecore_Fd_Handler *fd_handler);

EAPI Ecore_Pipe *
ecore_pipe_add(Ecore_Pipe_Cb handler,
               const void   *data)
{
   Ecore_Pipe *p;

   _ecore_lock();
   p = _ecore_pipe_add(handler, data);
   _ecore_unlock();

   return p;
}

EAPI void *
ecore_pipe_del(Ecore_Pipe *p)
{
   void *r;
   if (!p) return NULL;
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);
   _ecore_lock();
   r = _ecore_pipe_del(p);
   _ecore_unlock();
   return r;
}

EAPI void
ecore_pipe_read_close(Ecore_Pipe *p)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   _ecore_lock();
   if (!ECORE_MAGIC_CHECK(p, ECORE_MAGIC_PIPE))
     {
        ECORE_MAGIC_FAIL(p, ECORE_MAGIC_PIPE, "ecore_pipe_read_close");
        goto out;
     }
   if (p->fd_handler)
     {
        _ecore_main_fd_handler_del(p->fd_handler);
        p->fd_handler = NULL;
     }
   if (p->fd_read != PIPE_FD_INVALID)
     {
        pipe_close(p->fd_read);
        p->fd_read = PIPE_FD_INVALID;
     }
out:
   _ecore_unlock();
}

EAPI int
ecore_pipe_read_fd(Ecore_Pipe *p)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(PIPE_FD_INVALID);
   return p->fd_read;
}

EAPI void
ecore_pipe_freeze(Ecore_Pipe *p)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   _ecore_lock();
   if (!ECORE_MAGIC_CHECK(p, ECORE_MAGIC_PIPE))
     {
        ECORE_MAGIC_FAIL(p, ECORE_MAGIC_PIPE, "ecore_pipe_read_freeze");
        goto out;
     }
   if (p->fd_handler)
     {
        _ecore_main_fd_handler_del(p->fd_handler);
        p->fd_handler = NULL;
     }
out:
   _ecore_unlock();
}

EAPI void
ecore_pipe_thaw(Ecore_Pipe *p)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   _ecore_lock();
   if (!ECORE_MAGIC_CHECK(p, ECORE_MAGIC_PIPE))
     {
        ECORE_MAGIC_FAIL(p, ECORE_MAGIC_PIPE, "ecore_pipe_read_thaw");
        goto out;
     }
   if (!p->fd_handler && p->fd_read != PIPE_FD_INVALID)
     {
        p->fd_handler = ecore_main_fd_handler_add(p->fd_read,
                                                  ECORE_FD_READ,
                                                  _ecore_pipe_read,
                                                  p,
                                                  NULL, NULL);
     }
out:
   _ecore_unlock();
}

EAPI int
ecore_pipe_wait(Ecore_Pipe *p,
                int         message_count,
                double      wait)
{
   int r;
   _ecore_lock();
   r = _ecore_pipe_wait(p, message_count, wait);
   _ecore_unlock();
   return r;
}

EAPI void
ecore_pipe_write_close(Ecore_Pipe *p)
{
   _ecore_lock();
   if (!ECORE_MAGIC_CHECK(p, ECORE_MAGIC_PIPE))
     {
        ECORE_MAGIC_FAIL(p, ECORE_MAGIC_PIPE, "ecore_pipe_write_close");
        goto out;
     }
   if (p->fd_write != PIPE_FD_INVALID)
     {
        pipe_close(p->fd_write);
        p->fd_write = PIPE_FD_INVALID;
     }
out:
   _ecore_unlock();
}

EAPI int
ecore_pipe_write_fd(Ecore_Pipe *p)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(PIPE_FD_INVALID);
   return p->fd_write;
}

EAPI Eina_Bool
ecore_pipe_write(Ecore_Pipe  *p,
                 const void  *buffer,
                 unsigned int nbytes)
{
   ssize_t ret;
   size_t already_written = 0;
   int retry = ECORE_PIPE_WRITE_RETRY;
   Eina_Bool ok = EINA_FALSE;

   _ecore_lock();
   if (!ECORE_MAGIC_CHECK(p, ECORE_MAGIC_PIPE))
     {
        ECORE_MAGIC_FAIL(p, ECORE_MAGIC_PIPE, "ecore_pipe_write");
        goto out;
     }

   if (p->delete_me) goto out;

   if (p->fd_write == PIPE_FD_INVALID) goto out;

   /* First write the len into the pipe */
   do
     {
        ret = pipe_write(p->fd_write, &nbytes, sizeof(nbytes));
        if (ret == sizeof(nbytes))
          {
             retry = ECORE_PIPE_WRITE_RETRY;
             break;
          }
        else if (ret > 0)
          {
             /* XXX What should we do here? */
              ERR("The length of the data was not written complete"
                  " to the pipe");
              goto out;
          }
        else if (ret == PIPE_FD_ERROR && errno == EPIPE)
          {
             pipe_close(p->fd_write);
             p->fd_write = PIPE_FD_INVALID;
             goto out;
          }
        else if (ret == PIPE_FD_ERROR && errno == EINTR)
          /* try it again */
          ;
        else
          {
             ERR("An unhandled error (ret: %zd errno: %d)"
                 "occurred while writing to the pipe the length",
                 ret, errno);
          }
     }
   while (retry--);

   if (retry != ECORE_PIPE_WRITE_RETRY) goto out;

   /* and now pass the data to the pipe */
   do
     {
        ret = pipe_write(p->fd_write,
                         ((unsigned char *)buffer) + already_written,
                         nbytes - already_written);

        if (ret == (ssize_t)(nbytes - already_written))
          {
            ok = EINA_TRUE;
            goto out;
          }
        else if (ret >= 0)
          {
             already_written -= ret;
             continue;
          }
        else if (ret == PIPE_FD_ERROR && errno == EPIPE)
          {
             pipe_close(p->fd_write);
             p->fd_write = PIPE_FD_INVALID;
             goto out;
          }
        else if (ret == PIPE_FD_ERROR && errno == EINTR)
          /* try it again */
          ;
        else
          {
             ERR("An unhandled error (ret: %zd errno: %d)"
                 "occurred while writing to the pipe the length",
                 ret, errno);
          }
     }
   while (retry--);

out:
   _ecore_unlock();
   return ok;
}

EAPI Ecore_Pipe *
ecore_pipe_full_add(Ecore_Pipe_Cb handler,
                    const void   *data,
                    int fd_read,
                    int fd_write,
                    Eina_Bool read_survive_fork,
                    Eina_Bool write_survive_fork)
{
   Ecore_Pipe *p = NULL;
   int fds[2];

   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);
   if (!handler) return NULL;

   p = ecore_pipe_calloc(1);
   if (!p) return NULL;

   if (fd_read == -1 &&
       fd_write == -1)
     {
        if (pipe(fds))
          {
             ecore_pipe_mp_free(p);
             return NULL;
          }
        fd_read = fds[0];
        fd_write = fds[1];
     }
   else
     {
        fd_read = fd_read == -1 ? PIPE_FD_INVALID : fd_read;
        fd_write = fd_write == -1 ? PIPE_FD_INVALID : fd_write;
     }

   ECORE_MAGIC_SET(p, ECORE_MAGIC_PIPE);
   p->fd_read = fd_read;
   p->fd_write = fd_write;
   p->handler = handler;
   p->data = data;

   if (!read_survive_fork)
     _ecore_fd_close_on_exec(fd_read);
   if (!write_survive_fork)
     _ecore_fd_close_on_exec(fd_write);

   if (fcntl(p->fd_read, F_SETFL, O_NONBLOCK) < 0) ERR("can't set pipe to NONBLOCK");
   p->fd_handler = ecore_main_fd_handler_add(p->fd_read,
                                             ECORE_FD_READ,
                                             _ecore_pipe_read,
                                             p,
                                             NULL, NULL);

   return p;
}

/* Private functions */
Ecore_Pipe *
_ecore_pipe_add(Ecore_Pipe_Cb handler,
                const void   *data)
{
   return ecore_pipe_full_add(handler, data,
                              -1, -1,
                              EINA_FALSE, EINA_FALSE);
}

void *
_ecore_pipe_del(Ecore_Pipe *p)
{
   void *data = NULL;

   if (!ECORE_MAGIC_CHECK(p, ECORE_MAGIC_PIPE))
     {
        ECORE_MAGIC_FAIL(p, ECORE_MAGIC_PIPE, "ecore_pipe_del");
        return NULL;
     }
   p->delete_me = EINA_TRUE;
   if (p->handling > 0) return (void *)p->data;
   if (p->fd_handler) _ecore_main_fd_handler_del(p->fd_handler);
   if (p->fd_read != PIPE_FD_INVALID) pipe_close(p->fd_read);
   if (p->fd_write != PIPE_FD_INVALID) pipe_close(p->fd_write);
   data = (void *)p->data;
   ecore_pipe_mp_free(p);
   return data;
}

int
_ecore_pipe_wait(Ecore_Pipe *p,
                 int         message_count,
                 double      wait)
{
   struct timeval tv, *t;
   fd_set rset;
   double end = 0.0;
   double timeout;
   int ret;
   int total = 0;

   EINA_MAIN_LOOP_CHECK_RETURN_VAL(-1);
   if (p->fd_read == PIPE_FD_INVALID)
     return -1;

   FD_ZERO(&rset);
   FD_SET(p->fd_read, &rset);

   if (wait >= 0.0)
     end = ecore_loop_time_get() + wait;
   timeout = wait;

   while (message_count > 0 && (timeout > 0.0 || wait <= 0.0))
     {
        if (wait >= 0.0)
          {
             /* finite() tests for NaN, too big, too small, and infinity.  */
              if ((!ECORE_FINITE(timeout)) || (timeout == 0.0))
                {
                   tv.tv_sec = 0;
                   tv.tv_usec = 0;
                }
              else if (timeout > 0.0)
                {
                   int sec, usec;
#ifdef FIX_HZ
                   timeout += (0.5 / HZ);
                   sec = (int)timeout;
                   usec = (int)((timeout - (double)sec) * 1000000);
#else
                   sec = (int)timeout;
                   usec = (int)((timeout - (double)sec) * 1000000);
#endif
                   tv.tv_sec = sec;
                   tv.tv_usec = usec;
                }
              t = &tv;
          }
        else
          {
             t = NULL;
          }

        ret = main_loop_select(p->fd_read + 1, &rset, NULL, NULL, t);

        if (ret > 0)
          {
             _ecore_pipe_read(p, NULL);
             message_count -= p->message;
             total += p->message;
             p->message = 0;
          }
        else if (ret == 0)
          {
             break;
          }
        else if (errno != EINTR)
          {
             close(p->fd_read);
             p->fd_read = PIPE_FD_INVALID;
             break;
          }

        if (wait >= 0.0)
          timeout = end - ecore_loop_time_get();
     }

   return total;
}

static void
_ecore_pipe_unhandle(Ecore_Pipe *p)
{
   p->handling--;
   if (p->delete_me)
     {
        _ecore_pipe_del(p);
     }
}

static void
_ecore_pipe_handler_call(Ecore_Pipe *p,
                         unsigned char *buf,
                         size_t len)
{
   void *data = (void*) p->data;

   // clear all values of pipe first.
   p->passed_data = NULL;
   p->already_read = 0;
   p->len = 0;
   p->message++;

   if (!p->delete_me)
     {
        _ecore_unlock();
        p->handler(data, buf, len);
        _ecore_lock();
     }

   // free p->passed_data
   free(buf);
}

static Eina_Bool
_ecore_pipe_read(void             *data,
                 Ecore_Fd_Handler *fd_handler EINA_UNUSED)
{
   Ecore_Pipe *p = (Ecore_Pipe *)data;
   int i;

   p->handling++;
   for (i = 0; i < 16; i++)
     {
        ssize_t ret;

        /* if we already have read some data we don't need to read the len
         * but to finish the already started job
         */
        if (p->len == 0)
          {
             /* read the len of the passed data */
              ret = pipe_read(p->fd_read, &p->len, sizeof(p->len));

     /* catch the non error case first */
              /* read amount ok - nothing more to do */
              if (ret == sizeof(p->len))
                ;
              else if (ret > 0)
                {
     /* we got more data than we asked for - definite error */
                    ERR("Only read %i bytes from the pipe, although"
                        " we need to read %i bytes.",
                        (int)ret, (int)sizeof(p->len));
                    _ecore_pipe_unhandle(p);
                    return ECORE_CALLBACK_CANCEL;
                }
              else if (ret == 0)
                {
     /* we got no data */
                    if (i == 0)
                      {
     /* no data on first try through means an error */
                          _ecore_pipe_handler_call(p, NULL, 0);
                          pipe_close(p->fd_read);
                          p->fd_read = PIPE_FD_INVALID;
                          p->fd_handler = NULL;
                          _ecore_pipe_unhandle(p);
                          return ECORE_CALLBACK_CANCEL;
                      }
                    else
                      {
     /* no data after first loop try is ok */
                          _ecore_pipe_unhandle(p);
                          return ECORE_CALLBACK_RENEW;
                      }
                }
#ifndef _WIN32
              else if ((ret == PIPE_FD_ERROR) &&
                       ((errno == EINTR) || (errno == EAGAIN)))
                {
                    _ecore_pipe_unhandle(p);
                   return ECORE_CALLBACK_RENEW;
                }
              else
                {
                   ERR("An unhandled error (ret: %i errno: %i [%s])"
                       "occurred while reading from the pipe the length",
                       (int)ret, errno, strerror(errno));
                    _ecore_pipe_unhandle(p);
                   return ECORE_CALLBACK_RENEW;
                }
#else
              else /* ret == PIPE_FD_ERROR is the only other case on Windows */
                {
                   if (WSAGetLastError() != WSAEWOULDBLOCK)
                     {
                        _ecore_pipe_handler_call(p, NULL, 0);
                        pipe_close(p->fd_read);
                        p->fd_read = PIPE_FD_INVALID;
                        p->fd_handler = NULL;
                        _ecore_pipe_unhandle(p);
                        return ECORE_CALLBACK_CANCEL;
                     }
                }
#endif
          }

        /* if somehow we got less than or equal to 0 we got an errnoneous
         * messages so call callback with null and len we got. this case should
         * never happen */
        if (p->len == 0)
          {
             _ecore_pipe_handler_call(p, NULL, 0);
             _ecore_pipe_unhandle(p);
             return ECORE_CALLBACK_RENEW;
          }

        /* we dont have a buffer to hold the data, so alloc it */
        if (!p->passed_data)
          {
             p->passed_data = malloc(p->len);
             /* alloc failed - error case */
             if (!p->passed_data)
               {
                  _ecore_pipe_handler_call(p, NULL, 0);
     /* close the pipe */
                  pipe_close(p->fd_read);
                  p->fd_read = PIPE_FD_INVALID;
                  p->fd_handler = NULL;
                  _ecore_pipe_unhandle(p);
                  return ECORE_CALLBACK_CANCEL;
               }
          }

        /* and read the passed data */
        ret = pipe_read(p->fd_read,
                        ((unsigned char *)p->passed_data) + p->already_read,
                        p->len - p->already_read);

        /* catch the non error case first */
        /* if we read enough data to finish the message/buffer */
        if (ret == (ssize_t)(p->len - p->already_read))
          _ecore_pipe_handler_call(p, p->passed_data, p->len);
        else if (ret > 0)
          {
             /* more data left to read */
              p->already_read += ret;
              _ecore_pipe_unhandle(p);
              return ECORE_CALLBACK_RENEW;
          }
        else if (ret == 0)
          {
             /* 0 bytes to read - could be more to read next select wake up */
              _ecore_pipe_unhandle(p);
              return ECORE_CALLBACK_RENEW;
          }
#ifndef _WIN32
        else if ((ret == PIPE_FD_ERROR) &&
                 ((errno == EINTR) || (errno == EAGAIN)))
          {
             _ecore_pipe_unhandle(p);
             return ECORE_CALLBACK_RENEW;
          }
        else
          {
             ERR("An unhandled error (ret: %zd errno: %d)"
                 "occurred while reading from the pipe the data",
                 ret, errno);
             _ecore_pipe_unhandle(p);
             return ECORE_CALLBACK_RENEW;
          }
#else
        else /* ret == PIPE_FD_ERROR is the only other case on Windows */
          {
             if (WSAGetLastError() != WSAEWOULDBLOCK)
               {
                  _ecore_pipe_handler_call(p, NULL, 0);
                  pipe_close(p->fd_read);
                  p->fd_read = PIPE_FD_INVALID;
                  p->fd_handler = NULL;
                  _ecore_pipe_unhandle(p);
                  return ECORE_CALLBACK_CANCEL;
               }
             else
               break;
          }
#endif
     }

   _ecore_pipe_unhandle(p);
   return ECORE_CALLBACK_RENEW;
}

