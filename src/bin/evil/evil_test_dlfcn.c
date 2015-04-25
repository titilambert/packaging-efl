#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <dlfcn.h>

#include <Evil.h>

#include "evil_suite.h"
#include "evil_test_dlfcn.h"


typedef int (*_evil_init)(void);
typedef int (*_evil_shutdwon)(void);


static int
test_dlfcn_test_dlopen(void)
{
   void *handle;

   handle = dlopen("libevil-1.dll", 0);
   if (!handle)
     return 0;

   if (dlclose(handle))
     return 0;

   return 1;
}

static int
test_dlfcn_test_dlsym(void)
{
   void          *handle;
   _evil_init     sym_init;
   _evil_shutdwon sym_shutdown;

   handle = dlopen("libevil-1.dll", 0);
   if (!handle)
     return 0;

   sym_init = dlsym(handle, "evil_init");
   if (!sym_init)
     {
        dlclose(handle);
        return 0;
     }

   sym_shutdown = dlsym(handle, "evil_shutdown");
   if (!sym_shutdown)
     {
        dlclose(handle);
        return 0;
     }

   if (dlclose(handle))
     return 0;

   return 1;
}

static int
test_dlfcn_tests_run(suite *s)
{
   int res;

   res  = test_dlfcn_test_dlopen();
   res &= test_dlfcn_test_dlsym();

   return res;
}

int
test_dlfcn(suite *s)
{

   return test_dlfcn_tests_run(s);
}
