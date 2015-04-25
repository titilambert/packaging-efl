#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#ifdef HAVE_EVIL
# include <Evil.h>
#endif
#ifdef __linux__
# include <sys/syscall.h>
#endif
#include "evas_common_private.h"
#include "evas_private.h"
#include "Evas.h"

static int _threads_max = 0;

typedef struct _Evas_Preload_Pthread_Worker Evas_Preload_Pthread_Worker;
typedef struct _Evas_Preload_Pthread_Data Evas_Preload_Pthread_Data;

typedef void (*_evas_preload_pthread_func)(void *data);

struct _Evas_Preload_Pthread_Worker
{
   EINA_INLIST;

   _evas_preload_pthread_func func_heavy;
   _evas_preload_pthread_func func_end;
   _evas_preload_pthread_func func_cancel;
   void *data;
   Eina_Bool cancel : 1;
};

struct _Evas_Preload_Pthread_Data
{
   Eina_Thread thread;
};

static int _threads_count = 0;
static Evas_Preload_Pthread_Worker *_workers = NULL;

static LK(_mutex);

static void
_evas_preload_thread_end(void *data)
{
   Evas_Preload_Pthread_Data *pth = data;
   Evas_Preload_Pthread_Data *p = NULL;

   if ((p = eina_thread_join(pth->thread))) free(p);
   else return;
   eina_threads_shutdown();
}

static void
_evas_preload_thread_done(void *target EINA_UNUSED, Evas_Callback_Type type EINA_UNUSED, void *event_info)
{
   Evas_Preload_Pthread_Worker *work = event_info;
   if (work->cancel)
     {
        if (work->func_cancel) work->func_cancel(work->data);
     }
   else
      work->func_end(work->data);

   free(work);
}

static void *
_evas_preload_thread_worker(void *data, Eina_Thread thread EINA_UNUSED)
{
   Evas_Preload_Pthread_Data *pth = data;
   Evas_Preload_Pthread_Worker *work;

on_error:
   for (;;)
     {
        LKL(_mutex);
        if (!_workers)
          {
             LKU(_mutex);
             break;
          }

        work = _workers;
        _workers = EINA_INLIST_CONTAINER_GET(eina_inlist_remove(EINA_INLIST_GET(_workers), EINA_INLIST_GET(_workers)), Evas_Preload_Pthread_Worker);
        LKU(_mutex);

        if (work->func_heavy) work->func_heavy(work->data);
        evas_async_events_put(pth, 0, work, _evas_preload_thread_done);
     }

   LKL(_mutex);
   if (_workers)
     {
        LKU(_mutex);
        goto on_error;
     }
   _threads_count--;
   LKU(_mutex);

   // dummy worker to wake things up
   work = malloc(sizeof(Evas_Preload_Pthread_Worker));
   if (!work) return NULL;

   work->data = pth;
   work->func_heavy = NULL;
   work->func_end = (_evas_preload_pthread_func) _evas_preload_thread_end;
   work->func_cancel = NULL;
   work->cancel = EINA_FALSE;

   evas_async_events_put(pth, 0, work, _evas_preload_thread_done);
   return pth;
}

void
_evas_preload_thread_init(void)
{
   _threads_max = eina_cpu_count();
   if (_threads_max < 1) _threads_max = 1;

   LKI(_mutex);
}

void
_evas_preload_thread_shutdown(void)
{
   /* FIXME: If function are still running in the background, should we kill them ? */
   Evas_Preload_Pthread_Worker *work;

   /* Force processing of async events. */
   evas_async_events_process();
   LKL(_mutex);
   while (_workers)
     {
        work = _workers;
        _workers = EINA_INLIST_CONTAINER_GET(eina_inlist_remove(EINA_INLIST_GET(_workers), EINA_INLIST_GET(_workers)), Evas_Preload_Pthread_Worker);
        if (work->func_cancel) work->func_cancel(work->data);
        free(work);
     }
   LKU(_mutex);

   LKD(_mutex);
}

Evas_Preload_Pthread *
evas_preload_thread_run(void (*func_heavy) (void *data),
                        void (*func_end) (void *data),
                        void (*func_cancel) (void *data),
                        const void *data)
{
   Evas_Preload_Pthread_Worker *work;
   Evas_Preload_Pthread_Data *pth;

   work = malloc(sizeof(Evas_Preload_Pthread_Worker));
   if (!work)
     {
        func_cancel((void *)data);
        return NULL;
     }

   work->func_heavy = func_heavy;
   work->func_end = func_end;
   work->func_cancel = func_cancel;
   work->cancel = EINA_FALSE;
   work->data = (void *)data;

   LKL(_mutex);
   _workers = (Evas_Preload_Pthread_Worker *)eina_inlist_append(EINA_INLIST_GET(_workers), EINA_INLIST_GET(work));
   if (_threads_count == _threads_max)
     {
        LKU(_mutex);
        return (Evas_Preload_Pthread *)work;
     }
   LKU(_mutex);

   /* One more thread could be created. */
   pth = malloc(sizeof(Evas_Preload_Pthread_Data));
   if (!pth) goto on_error;

   eina_threads_init();

   if (eina_thread_create(&pth->thread, EINA_THREAD_BACKGROUND, -1, _evas_preload_thread_worker, pth))
     {
        LKL(_mutex);
        _threads_count++;
        LKU(_mutex);
        return (Evas_Preload_Pthread*)work;
     }

   eina_threads_shutdown();

on_error:
   LKL(_mutex);
   if (_threads_count == 0)
     {
        _workers = EINA_INLIST_CONTAINER_GET(eina_inlist_remove(EINA_INLIST_GET(_workers), EINA_INLIST_GET(work)), Evas_Preload_Pthread_Worker);
        LKU(_mutex);
        if (work->func_cancel) work->func_cancel(work->data);
        free(work);
        return NULL;
     }
   LKU(_mutex);
   return NULL;
}

Eina_Bool
evas_preload_thread_cancel(Evas_Preload_Pthread *thread)
{
   Evas_Preload_Pthread_Worker *work;

   if (!thread) return EINA_TRUE;
   LKL(_mutex);
   EINA_INLIST_FOREACH(_workers, work)
     {
        if (work == (Evas_Preload_Pthread_Worker *)thread)
          {
             _workers = EINA_INLIST_CONTAINER_GET(eina_inlist_remove(EINA_INLIST_GET(_workers), EINA_INLIST_GET(work)), Evas_Preload_Pthread_Worker);
             LKU(_mutex);
             if (work->func_cancel) work->func_cancel(work->data);
             free(work);
             return EINA_TRUE;
          }
     }
   LKU(_mutex);

   /* Delay the destruction */
   work = (Evas_Preload_Pthread_Worker *)thread;
   work->cancel = EINA_TRUE;
   return EINA_FALSE;
}
