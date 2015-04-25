#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>

#ifdef _MSC_VER
# include <io.h>   /* for _get_osfhandle _lseek and _locking */
#endif

#include <sys/locking.h>

#include <winsock2.h> /* for ioctlsocket */

#include "Evil.h"


/*
 * port of fcntl function
 *
 */

int fcntl(int fd, int cmd, ...)
{
   va_list va;
   int     res = -1;

   va_start (va, cmd);

   if (cmd == F_GETFD)
     {
        HANDLE  h;
        DWORD flag;

        h = (HANDLE)_get_osfhandle(fd);
        if (h == INVALID_HANDLE_VALUE)
          return -1;

	if (!GetHandleInformation(h, &flag))
          {
             /* FIXME: should we close h ? MSDN seems to say that */
             return -1;
          }

	res = 0;
     }

   if (cmd == F_SETFD)
     {
        HANDLE  h;
        long flag;

        h = (HANDLE)_get_osfhandle(fd);
        if (h == INVALID_HANDLE_VALUE)
          return -1;

        flag = va_arg(va, long);
        if (flag == FD_CLOEXEC)
          {
             if (SetHandleInformation(h, HANDLE_FLAG_INHERIT, 0))
               res = 0;
          }
     }
   else if (cmd == F_GETFL)
     {
        /* does nothing*/
     }
   else if (cmd == F_SETFL)
     {
        long flag;

        flag = va_arg(va, long);
        if (flag & O_NONBLOCK)
          {
             u_long arg = 1;
             int    type;
             int    len;
             int    ret;

             len = (int)sizeof(int);
             ret = getsockopt((SOCKET)fd, SOL_SOCKET, SO_TYPE, (char *)&type, &len);
             if (!ret && (type == SOCK_STREAM))
               {
                  if (ioctlsocket((SOCKET)fd, FIONBIO, &arg) != SOCKET_ERROR)
                    res = 0;
               }
          }
     }
   else if ((cmd == F_SETLK) || (cmd == F_SETLKW))
     {
        struct flock *fl;
        off_t        length = 0;
        long         pos;

        fl = va_arg(va, struct flock *);

        if (fl->l_len == 0)
          {
             length = _lseek(fd, 0L, SEEK_END);
             if (length != -1L)
               res = 0;
          }
        fl->l_len = length - fl->l_start - 1;

        pos = _lseek(fd, fl->l_start, fl->l_whence);
        if (pos != -1L)
          res = 0;

        if ((fl->l_type == F_RDLCK) || (fl->l_type == F_WRLCK))
          {
             if (cmd == F_SETLK)
               res = _locking(fd, _LK_NBLCK, fl->l_len); /* if cannot be locked, we return immediatly */
             else /* F_SETLKW */
               res = _locking(fd, _LK_LOCK, fl->l_len); /* otherwise, we try several times */
          }

        if (fl->l_type == F_UNLCK)
          res = _locking(fd, _LK_UNLCK, fl->l_len);
     }

   va_end(va);

   return res;
}
