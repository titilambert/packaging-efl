#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#undef WIN32_LEAN_AND_MEAN

#include "Evil.h"
#include "evil_private.h"


static int      _evil_init_count = 0;

extern LONGLONG _evil_time_freq;
extern LONGLONG _evil_time_count;
extern long     _evil_time_second;

int
evil_init(void)
{
   SYSTEMTIME    st;
   LARGE_INTEGER freq;
   LARGE_INTEGER count;
   WORD          second = 59;

   if (++_evil_init_count != 1)
     return _evil_init_count;

   if (!QueryPerformanceFrequency(&freq))
       return 0;

   _evil_time_freq = freq.QuadPart;

   /* be sure that second + 1 != 0 */
   while (second == 59)
     {
        GetSystemTime(&st);
        second = st.wSecond;
     }

   /* retrieve the tick corresponding to the time we retrieve above */
   while (1)
     {
        GetSystemTime(&st);
        QueryPerformanceCounter(&count);
        if (st.wSecond == second + 1)
          break;
     }

   _evil_time_second = _evil_systemtime_to_time(st);
   if (_evil_time_second < 0)
     return --_evil_init_count;

   _evil_time_count = count.QuadPart;

   if (!evil_sockets_init())
     return --_evil_init_count;

   return _evil_init_count;
}

int
evil_shutdown(void)
{
   /* _evil_init_count should not go below zero. */
   if (_evil_init_count < 1)
     {
        printf("Evil shutdown called without calling evil init.\n");
        return 0;
     }

   if (--_evil_init_count != 0)
     return _evil_init_count;

   evil_sockets_shutdown();

   return _evil_init_count;
}
