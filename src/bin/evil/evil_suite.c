#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#include "Evil.h"
#include "evil_suite.h"
#include "evil_test_dlfcn.h"
#include "evil_test_environment.h"
#include "evil_test_gettimeofday.h"
#include "evil_test_link.h"
#include "evil_test_memcpy.h"
#include "evil_test_mkstemp.h"
#include "evil_test_pipe.h"
#include "evil_test_print.h"
#include "evil_test_realpath.h"
#include "evil_test_util.h"


typedef int(*function)(suite *s);

struct test
{
   const char *name;
   function    fct;
};

struct list
{
  void *data;
   int  succeed;
  list *next;
};

struct suite
{
   LARGE_INTEGER freq;
   LARGE_INTEGER start;
   LARGE_INTEGER end;

   list         *first;
   list         *l;

   int           tests_count;
   int           tests_success;
};


static suite *
suite_new(void)
{
   suite *s;

   s = (suite *)malloc(sizeof(suite));
   if (!s) return NULL;

   if (!QueryPerformanceFrequency(&s->freq))
     {
        free(s);
        return NULL;
     }

   s->first = NULL;
   s->l = NULL;

   s->tests_count = 0;
   s->tests_success = 0;

   return s;
}

static void
suite_del(suite *s)
{
   list *l;
   list *tmp;

   if (!s) return;

   l = s->first;
   while (l)
     {
        tmp = l->next;
        free(l->data);
        free(l);
        l = tmp;
     }

   free(s);
}

void
suite_time_start(suite *s)
{
   QueryPerformanceCounter(&s->start);
}

void
suite_time_stop(suite *s)
{
   QueryPerformanceCounter(&s->end);
}

double
suite_time_get(suite *s)
{
   return (double)(s->end.QuadPart - s->start.QuadPart) / (double)s->freq.QuadPart;
}

static void
suite_test_add(suite *s, const char *name, function fct)
{
   test *t;
   list *l;

   t = (test *)malloc(sizeof(test));
   if (!t) return;

   l = (list *)malloc(sizeof(list));
   if (!l)
     {
        free(t);
        return;
     }

   t->name = name;
   t->fct = fct;

   l->data = t;
   l->succeed = 0;
   l->next = NULL;

   if (!s->first) s->first = l;

   if (!s->l)
     s->l = l;
   else
     {
        s->l->next = l;
        s->l =l;
     }
}

static void
suite_run(suite *s)
{
   list *l;

   l = s->first;
   while (l)
     {
        test *t;

        t = (test *)l->data;
        l->succeed = t->fct(s);
        printf("%s test: %s\n", t->name, l->succeed ? "success" : "failure");
        s->tests_count++;
        if (l->succeed)
          s->tests_success++;
        l = l->next;
     }
}

static void
suite_show(suite *s)
{
   printf ("\n%d/%d tests passed (%d%%)\n",
           s->tests_success,
           s->tests_count,
           (100 * s->tests_success) / s->tests_count);
}

int
main(void)
{
   test tests[] = {
     { "dlfcn       ",  test_dlfcn },
     { "environment ",  test_environment },
     { "gettimeofday",  test_gettimeofday },
     { "link        ",  test_link },
     { "mkstemp     ",  test_mkstemp },
     { "pipe        ",  test_pipe },
     { "print       ",  test_print },
     { "realpath    ",  test_realpath },
     { "util        ",  test_util },
/*      { "memcpy      ",  test_memcpy }, */
     { NULL,            NULL },
   };
   suite *s;
   int    i;

   if (!evil_init())
     return EXIT_FAILURE;

   s = suite_new();
   if (!s)
     {
        evil_shutdown();
        return EXIT_FAILURE;
     }

   for (i = 0; tests[i].name; ++i)
     {
        suite_test_add(s, tests[i].name, tests[i].fct);
     }

   suite_run(s);

   suite_show(s);

   suite_del(s);
   evil_shutdown();

   return EXIT_SUCCESS;
}
