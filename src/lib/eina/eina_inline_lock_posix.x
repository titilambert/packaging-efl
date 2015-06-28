/* EINA - EFL data type library
 * Copyright (C) 2011 Vincent Torri
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EINA_INLINE_LOCK_POSIX_X_
#define EINA_INLINE_LOCK_POSIX_X_

#ifdef _XOPEN_SOURCE
# define EINA_XOPEN_SOURCE _XOPEN_SOURCE
# undef _XOPEN_SOURCE
#endif
#define _XOPEN_SOURCE 600

#ifdef EINA_HAVE_POSIX_SPINLOCK
# include <sched.h>
#endif

#include <errno.h>
#ifndef __USE_UNIX98
# define __USE_UNIX98
# include <pthread.h>
# undef __USE_UNIX98
#else
# include <pthread.h>
#endif

#ifdef EINA_HAVE_OSX_SPINLOCK
# include <libkern/OSAtomic.h>
#endif

#include <semaphore.h>

#include <sys/time.h>
#include <stdio.h>

#ifdef EINA_HAVE_DEBUG_THREADS
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <execinfo.h>
#define EINA_LOCK_DEBUG_BT_NUM 64
typedef void (*Eina_Lock_Bt_Func) ();

#include "eina_inlist.h"
#endif

/* For cond_timedwait */
#include <time.h>
#include <sys/time.h>

typedef struct _Eina_Lock Eina_Lock;
typedef struct _Eina_RWLock Eina_RWLock;
typedef struct _Eina_Condition Eina_Condition;
typedef pthread_key_t Eina_TLS;

#if defined(EINA_HAVE_POSIX_SPINLOCK)
typedef pthread_spinlock_t Eina_Spinlock;
#elif defined(EINA_HAVE_OSX_SPINLOCK)
typedef OSSpinLock Eina_Spinlock;
#else
typedef Eina_Lock Eina_Spinlock;
#endif

#if defined(EINA_HAVE_OSX_SEMAPHORE)
/* OSX supports only named semaphores.
 * So, we need to be able to generate a unique string identifier for each
 * semaphore we want to create.
 * It seems reasonable to use a counter, which is incremented each time a
 * semaphore is created. However, it needs to be atomic...
 * It would be easier if we were using C11 with stdatomic, but I guess it
 * will just be fine without.
 * That's why there are two static variables below the struct */
struct _Eina_Semaphore
{
   sem_t       *sema;
   char         name[16];
};
typedef struct _Eina_Semaphore Eina_Semaphore;

static unsigned int _sem_ctr = 0;
static Eina_Spinlock _sem_ctr_lock = 0; // 0: not locked

#else
typedef sem_t Eina_Semaphore;
#endif

/** @privatesection  @{ */
struct _Eina_Lock
{
#ifdef EINA_HAVE_DEBUG_THREADS
   EINA_INLIST; /**< Keeps track of the threads waiting for the lock */
#endif
   pthread_mutex_t   mutex; /**< The mutex that handles the locking */
#ifdef EINA_HAVE_DEBUG_THREADS
   pthread_t         lock_thread_id; /**< The ID of the thread that currently has the lock */
   Eina_Lock_Bt_Func lock_bt[EINA_LOCK_DEBUG_BT_NUM]; /**< The function that will produce a backtrace on the thread that has the lock */
   int               lock_bt_num; /**< Number of addresses in the backtrace */
   Eina_Bool         locked : 1;  /**< Indicates locked or not locked */
#endif
};

struct _Eina_Condition
{
   Eina_Lock      *lock;      /**< The lock for this condition */
   pthread_cond_t  condition; /**< The condition variable */
#if defined(__clockid_t_defined)
   clockid_t       clkid;     /**< The attached clock for timedwait */
#endif
};

struct _Eina_RWLock
{
   pthread_rwlock_t mutex; /**< The mutex that handles the locking */
#ifdef EINA_HAVE_DEBUG_THREADS
   pthread_t        lock_thread_wid; /**< The ID of the thread that currently has the lock */
#endif
};
/** @} privatesection */

EAPI extern Eina_Bool _eina_threads_activated;

#ifdef EINA_HAVE_DEBUG_THREADS
EAPI extern int _eina_threads_debug;
EAPI extern pthread_t _eina_main_loop;
EAPI extern pthread_mutex_t _eina_tracking_lock;
EAPI extern Eina_Inlist *_eina_tracking;
#endif

static inline void
eina_lock_debug(const Eina_Lock *mutex)
{
#ifdef EINA_HAVE_DEBUG_THREADS
   printf("lock %p, locked: %i, by %ti\n",
          mutex, (int)mutex->locked, (ptrdiff_t)mutex->lock_thread_id);
   backtrace_symbols_fd((void **)mutex->lock_bt, mutex->lock_bt_num, 1);
#else
   (void) mutex;
#endif
}

static inline Eina_Bool
eina_lock_new(Eina_Lock *mutex)
{
   pthread_mutexattr_t attr;

#ifdef EINA_HAVE_DEBUG_THREADS
   if (!_eina_threads_activated)
     assert(pthread_equal(_eina_main_loop, pthread_self()));
#endif

   if (pthread_mutexattr_init(&attr) != 0)
     return EINA_FALSE;
   /* NOTE: PTHREAD_MUTEX_RECURSIVE is not allowed at all, you will break on/off
      feature for sure with that change. */
#ifdef EINA_HAVE_DEBUG_THREADS
   if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK) != 0)
     return EINA_FALSE;
   memset(mutex, 0, sizeof(Eina_Lock));
#endif
   if (pthread_mutex_init(&(mutex->mutex), &attr) != 0)
     return EINA_FALSE;

   pthread_mutexattr_destroy(&attr);

   return EINA_TRUE;
}

static inline void
eina_lock_free(Eina_Lock *mutex)
{
#ifdef EINA_HAVE_DEBUG_THREADS
   if (!_eina_threads_activated)
     assert(pthread_equal(_eina_main_loop, pthread_self()));
#endif

   pthread_mutex_destroy(&(mutex->mutex));
#ifdef EINA_HAVE_DEBUG_THREADS
   memset(mutex, 0, sizeof(Eina_Lock));
#endif
}

static inline Eina_Lock_Result
eina_lock_take(Eina_Lock *mutex)
{
   Eina_Lock_Result ret = EINA_LOCK_FAIL;
   int ok;

#ifdef EINA_HAVE_ON_OFF_THREADS
   if (!_eina_threads_activated)
     {
#ifdef EINA_HAVE_DEBUG_THREADS
        assert(pthread_equal(_eina_main_loop, pthread_self()));
#endif
        return EINA_LOCK_SUCCEED;
     }
#endif

#ifdef EINA_HAVE_DEBUG_THREADS
   if (_eina_threads_debug)
     {
        struct timeval t0, t1;
        int dt;

        gettimeofday(&t0, NULL);
        ok = pthread_mutex_lock(&(mutex->mutex));
        gettimeofday(&t1, NULL);

        dt = (t1.tv_sec - t0.tv_sec) * 1000000;
        if (t1.tv_usec > t0.tv_usec)
           dt += (t1.tv_usec - t0.tv_usec);
        else
           dt -= t0.tv_usec - t1.tv_usec;
        dt /= 1000;

        if (dt > _eina_threads_debug) abort();
     }
   else
     {
#endif
        ok = pthread_mutex_lock(&(mutex->mutex));
#ifdef EINA_HAVE_DEBUG_THREADS
     }
#endif

   if (ok == 0) ret = EINA_LOCK_SUCCEED;
   else if (ok == EDEADLK)
     {
        printf("ERROR ERROR: DEADLOCK on lock %p\n", mutex);
        eina_lock_debug(mutex);
        ret = EINA_LOCK_DEADLOCK; // magic
#ifdef EINA_HAVE_DEBUG_THREADS
        if (_eina_threads_debug) abort();
#endif
     }

#ifdef EINA_HAVE_DEBUG_THREADS
   mutex->locked = 1;
   mutex->lock_thread_id = pthread_self();
   mutex->lock_bt_num = backtrace((void **)(mutex->lock_bt), EINA_LOCK_DEBUG_BT_NUM);

   pthread_mutex_lock(&_eina_tracking_lock);
   _eina_tracking = eina_inlist_append(_eina_tracking,
                                       EINA_INLIST_GET(mutex));
   pthread_mutex_unlock(&_eina_tracking_lock);
#endif

   return ret;
}

static inline Eina_Lock_Result
eina_lock_take_try(Eina_Lock *mutex)
{
   Eina_Lock_Result ret = EINA_LOCK_FAIL;
   int ok;

#ifdef EINA_HAVE_ON_OFF_THREADS
   if (!_eina_threads_activated)
     {
#ifdef EINA_HAVE_DEBUG_THREADS
        assert(pthread_equal(_eina_main_loop, pthread_self()));
#endif
        return EINA_LOCK_SUCCEED;
     }
#endif

#ifdef EINA_HAVE_DEBUG_THREADS
   if (!_eina_threads_activated)
     assert(pthread_equal(_eina_main_loop, pthread_self()));
#endif

   ok = pthread_mutex_trylock(&(mutex->mutex));
   if (ok == 0) ret = EINA_LOCK_SUCCEED;
   else if (ok == EDEADLK)
     {
        printf("ERROR ERROR: DEADLOCK on trylock %p\n", mutex);
        ret = EINA_LOCK_DEADLOCK; // magic
     }
#ifdef EINA_HAVE_DEBUG_THREADS
   if (ret == EINA_LOCK_SUCCEED)
     {
        mutex->locked = 1;
        mutex->lock_thread_id = pthread_self();
        mutex->lock_bt_num = backtrace((void **)(mutex->lock_bt), EINA_LOCK_DEBUG_BT_NUM);

        pthread_mutex_lock(&_eina_tracking_lock);
        _eina_tracking = eina_inlist_append(_eina_tracking,
                                            EINA_INLIST_GET(mutex));
        pthread_mutex_unlock(&_eina_tracking_lock);
     }
#endif
   return ret;
}

static inline Eina_Lock_Result
eina_lock_release(Eina_Lock *mutex)
{
   Eina_Lock_Result ret;

#ifdef EINA_HAVE_ON_OFF_THREADS
   if (!_eina_threads_activated)
     {
#ifdef EINA_HAVE_DEBUG_THREADS
        assert(pthread_equal(_eina_main_loop, pthread_self()));
#endif
        return EINA_LOCK_SUCCEED;
     }
#endif

#ifdef EINA_HAVE_DEBUG_THREADS
   pthread_mutex_lock(&_eina_tracking_lock);
   _eina_tracking = eina_inlist_remove(_eina_tracking,
                                       EINA_INLIST_GET(mutex));
   pthread_mutex_unlock(&_eina_tracking_lock);

   mutex->locked = 0;
   mutex->lock_thread_id = 0;
   memset(mutex->lock_bt, 0, EINA_LOCK_DEBUG_BT_NUM * sizeof(Eina_Lock_Bt_Func));
   mutex->lock_bt_num = 0;
#endif
   ret = (pthread_mutex_unlock(&(mutex->mutex)) == 0) ?
      EINA_LOCK_SUCCEED : EINA_LOCK_FAIL;
   return ret;
}

static inline Eina_Bool
eina_condition_new(Eina_Condition *cond, Eina_Lock *mutex)
{
   pthread_condattr_t attr;

#ifdef EINA_HAVE_DEBUG_THREADS
   assert(mutex != NULL);
   if (!_eina_threads_activated)
     assert(pthread_equal(_eina_main_loop, pthread_self()));
   memset(cond, 0, sizeof (Eina_Condition));
#endif

   cond->lock = mutex;
   pthread_condattr_init(&attr);

   /* OSX doesn't provide clockid_t or clock_gettime. */
#if defined(__clockid_t_defined)
   cond->clkid = (clockid_t) 0;
   /* We try here to chose the best clock for cond_timedwait */
# if defined(CLOCK_MONOTONIC_RAW)
   if (!pthread_condattr_setclock(&attr, CLOCK_MONOTONIC_RAW))
     cond->clkid = CLOCK_MONOTONIC_RAW;
# endif
# if defined(CLOCK_MONOTONIC)
   if (!cond->clkid && !pthread_condattr_setclock(&attr, CLOCK_MONOTONIC))
     cond->clkid = CLOCK_MONOTONIC;
# endif
# if defined(CLOCK_REALTIME)
   if (!cond->clkid && !pthread_condattr_setclock(&attr, CLOCK_REALTIME))
     cond->clkid = CLOCK_REALTIME;
# endif
#endif

   if (pthread_cond_init(&cond->condition, &attr) != 0)
     {
        pthread_condattr_destroy(&attr);
#ifdef EINA_HAVE_DEBUG_THREADS
        if (errno == EBUSY)
          printf("eina_condition_new on already initialized Eina_Condition\n");
#endif
        return EINA_FALSE;
     }

   pthread_condattr_destroy(&attr);
   return EINA_TRUE;
}

static inline void
eina_condition_free(Eina_Condition *cond)
{
#ifdef EINA_HAVE_DEBUG_THREADS
   if (!_eina_threads_activated)
     assert(pthread_equal(_eina_main_loop, pthread_self()));
#endif

   pthread_cond_destroy(&(cond->condition));
#ifdef EINA_HAVE_DEBUG_THREADS
   memset(cond, 0, sizeof (Eina_Condition));
#endif
}

static inline Eina_Bool
eina_condition_wait(Eina_Condition *cond)
{
   Eina_Bool r;

#ifdef EINA_HAVE_DEBUG_THREADS
   assert(_eina_threads_activated);
   assert(cond->lock != NULL);

   pthread_mutex_lock(&_eina_tracking_lock);
   _eina_tracking = eina_inlist_remove(_eina_tracking,
				       EINA_INLIST_GET(cond->lock));
   pthread_mutex_unlock(&_eina_tracking_lock);
#endif

   r = pthread_cond_wait(&(cond->condition),
			 &(cond->lock->mutex)) == 0 ? EINA_TRUE : EINA_FALSE;

#ifdef EINA_HAVE_DEBUG_THREADS
   pthread_mutex_lock(&_eina_tracking_lock);
   _eina_tracking = eina_inlist_append(_eina_tracking,
				       EINA_INLIST_GET(cond->lock));
   pthread_mutex_unlock(&_eina_tracking_lock);
#endif

   return r;
}

static inline Eina_Bool
eina_condition_timedwait(Eina_Condition *cond, double t)
{
   struct timespec ts;
   time_t sec;
   long nsec;
   int err;

   if (t < 0)
     {
        errno = EINVAL;
        return EINA_FALSE;
     }

#if defined(__clockid_t_defined)
   if (cond->clkid)
     {
        if (clock_gettime(cond->clkid, &ts) != 0)
          return EINA_FALSE;
     }
   else
#endif
     {
        /* Obsolete for Linux.
         * TODO: use pthread_cond_timedwait_relative_np for OSX. */
        struct timeval tv;
        if (gettimeofday(&tv, NULL) != 0)
          return EINA_FALSE;

        ts.tv_sec = tv.tv_sec;
        ts.tv_nsec = tv.tv_usec * 1000L;
     }

#ifdef EINA_HAVE_DEBUG_THREADS
   assert(_eina_threads_activated);
   assert(cond->lock != NULL);

   pthread_mutex_lock(&_eina_tracking_lock);
   _eina_tracking = eina_inlist_remove(_eina_tracking,
				       EINA_INLIST_GET(cond->lock));
   pthread_mutex_unlock(&_eina_tracking_lock);
#endif

   sec = (time_t)t;
   nsec = (t - (double) sec) * 1000000000L;
   ts.tv_sec += sec;
   ts.tv_nsec += nsec;
   if (ts.tv_nsec > 1000000000L)
     {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000L;
     }

   err = pthread_cond_timedwait(&(cond->condition),
                                &(cond->lock->mutex),
                                &ts);

#ifdef EINA_HAVE_DEBUG_THREADS
   pthread_mutex_lock(&_eina_tracking_lock);
   _eina_tracking = eina_inlist_append(_eina_tracking,
				       EINA_INLIST_GET(cond->lock));
   pthread_mutex_unlock(&_eina_tracking_lock);
#endif

   return (!err) ? EINA_TRUE : EINA_FALSE;
}

static inline Eina_Bool
eina_condition_broadcast(Eina_Condition *cond)
{
#ifdef EINA_HAVE_DEBUG_THREADS
   assert(cond->lock != NULL);
#endif

   return pthread_cond_broadcast(&(cond->condition)) == 0 ? EINA_TRUE : EINA_FALSE;
}

static inline Eina_Bool
eina_condition_signal(Eina_Condition *cond)
{
#ifdef EINA_HAVE_DEBUG_THREADS
   assert(cond->lock != NULL);
#endif

   return pthread_cond_signal(&(cond->condition)) == 0 ? EINA_TRUE : EINA_FALSE;
}

static inline Eina_Bool
eina_rwlock_new(Eina_RWLock *mutex)
{
#ifdef EINA_HAVE_DEBUG_THREADS
   if (!_eina_threads_activated)
     assert(pthread_equal(_eina_main_loop, pthread_self()));
#endif

   if (pthread_rwlock_init(&(mutex->mutex), NULL) != 0)
     return EINA_FALSE;
   return EINA_TRUE;
}

static inline void
eina_rwlock_free(Eina_RWLock *mutex)
{
#ifdef EINA_HAVE_DEBUG_THREADS
   if (!_eina_threads_activated)
     assert(pthread_equal(_eina_main_loop, pthread_self()));
#endif

   pthread_rwlock_destroy(&(mutex->mutex));
}

static inline Eina_Lock_Result
eina_rwlock_take_read(Eina_RWLock *mutex)
{
#ifdef EINA_HAVE_ON_OFF_THREADS
   if (!_eina_threads_activated)
     {
#ifdef EINA_HAVE_DEBUG_THREADS
        assert(pthread_equal(_eina_main_loop, pthread_self()));
#endif
        return EINA_LOCK_SUCCEED;
     }
#endif

   if (pthread_rwlock_rdlock(&(mutex->mutex)) != 0)
     return EINA_LOCK_FAIL;
   return EINA_LOCK_SUCCEED;
}

static inline Eina_Lock_Result
eina_rwlock_take_write(Eina_RWLock *mutex)
{
#ifdef EINA_HAVE_ON_OFF_THREADS
   if (!_eina_threads_activated)
     {
#ifdef EINA_HAVE_DEBUG_THREADS
        assert(pthread_equal(_eina_main_loop, pthread_self()));
#endif
        return EINA_LOCK_SUCCEED;
     }
#endif

   if (pthread_rwlock_wrlock(&(mutex->mutex)) != 0)
     return EINA_LOCK_FAIL;
   return EINA_LOCK_SUCCEED;
}

static inline Eina_Lock_Result
eina_rwlock_release(Eina_RWLock *mutex)
{
#ifdef EINA_HAVE_ON_OFF_THREADS
   if (!_eina_threads_activated)
     {
#ifdef EINA_HAVE_DEBUG_THREADS
        assert(pthread_equal(_eina_main_loop, pthread_self()));
#endif
        return EINA_LOCK_SUCCEED;
     }
#endif

   if (pthread_rwlock_unlock(&(mutex->mutex)) != 0)
     return EINA_LOCK_FAIL;
   return EINA_LOCK_SUCCEED;
}

static inline Eina_Bool
eina_tls_cb_new(Eina_TLS *key, Eina_TLS_Delete_Cb delete_cb)
{
   if (pthread_key_create(key, delete_cb) != 0)
      return EINA_FALSE;
   return EINA_TRUE;
}

static inline Eina_Bool
eina_tls_new(Eina_TLS *key)
{
   return eina_tls_cb_new(key, NULL);
}

static inline void 
eina_tls_free(Eina_TLS key)
{
   pthread_key_delete(key);
}

static inline void * 
eina_tls_get(Eina_TLS key)
{
   return pthread_getspecific(key);
}

static inline Eina_Bool 
eina_tls_set(Eina_TLS key, const void *data)
{
   if (pthread_setspecific(key, data) != 0)
      return EINA_FALSE;
   return EINA_TRUE;
}


#ifdef EINA_HAVE_PTHREAD_BARRIER
typedef struct _Eina_Barrier Eina_Barrier;

struct _Eina_Barrier
{
   pthread_barrier_t barrier;
};

static inline Eina_Bool
eina_barrier_new(Eina_Barrier *barrier, int needed)
{
   if (!pthread_barrier_init(&(barrier->barrier), NULL, needed))
     return EINA_TRUE;
   return EINA_FALSE;
}

static inline void
eina_barrier_free(Eina_Barrier *barrier)
{
   pthread_barrier_destroy(&(barrier->barrier));
}

static inline Eina_Bool
eina_barrier_wait(Eina_Barrier *barrier)
{
   pthread_barrier_wait(&(barrier->barrier));
   return EINA_TRUE;
}

#else
#include "eina_inline_lock_barrier.x"
#endif

static inline Eina_Bool
eina_spinlock_new(Eina_Spinlock *spinlock)
{
#if defined(EINA_HAVE_POSIX_SPINLOCK)
   return pthread_spin_init(spinlock, PTHREAD_PROCESS_PRIVATE) == 0 ? EINA_TRUE : EINA_FALSE;
#elif defined(EINA_HAVE_OSX_SPINLOCK)
   /* OSSpinLock is an integer type.  The convention is that unlocked is
    * zero, and locked is nonzero. */
   *spinlock = 0;
   return EINA_LOCK_SUCCEED;
#else
   return eina_lock_new(spinlock);
#endif
}

static inline Eina_Lock_Result
eina_spinlock_take(Eina_Spinlock *spinlock)
{
#if defined(EINA_HAVE_POSIX_SPINLOCK)
   int t;

   do {
      t = pthread_spin_trylock(spinlock);
      if (t != 0)
        {
           if (errno == EBUSY) sched_yield();
           else if (errno == EDEADLK) return EINA_LOCK_DEADLOCK;
        }
   } while (t != 0);

   return EINA_LOCK_SUCCEED;
#elif defined(EINA_HAVE_OSX_SPINLOCK)
   /* void OSSpinLockLock(OSSpinLock *lock);
    * Will spin if the lock is already held, but employs various strategies to
    * back off, making it immune to most priority-inversion livelocks. */
   OSSpinLockLock(spinlock);
   return EINA_LOCK_SUCCEED;
#else
   return eina_lock_take(spinlock);
#endif
}

static inline Eina_Lock_Result
eina_spinlock_take_try(Eina_Spinlock *spinlock)
{
#if defined(EINA_HAVE_POSIX_SPINLOCK)
   int t;

   t = pthread_spin_trylock(spinlock);
   return t ? EINA_LOCK_FAIL : EINA_LOCK_SUCCEED;
#elif defined(EINA_HAVE_OSX_SPINLOCK)
   /* bool OSSpinLockTry(OSSpinLock *lock);
    * Immediately returns false if the lock was held, true if it took the
    * lock.  It does not spin. */
   return (OSSpinLockTry(spinlock)) ? EINA_LOCK_SUCCEED : EINA_LOCK_FAIL;
#else
   return eina_lock_take_try(spinlock);
#endif
}

static inline Eina_Lock_Result
eina_spinlock_release(Eina_Spinlock *spinlock)
{
#if defined(EINA_HAVE_POSIX_SPINLOCK)
   return pthread_spin_unlock(spinlock) ? EINA_LOCK_FAIL : EINA_LOCK_SUCCEED;
#elif defined(EINA_HAVE_OSX_SPINLOCK)
   /* void OSSpinLockUnlock(OSSpinLock *lock);
    * Unconditionally unlocks the lock by zeroing it. */
   OSSpinLockUnlock(spinlock);
   return EINA_LOCK_SUCCEED;
#else
   return eina_lock_release(spinlock);
#endif
}

static inline void
eina_spinlock_free(Eina_Spinlock *spinlock)
{
#if defined(EINA_HAVE_POSIX_SPINLOCK)
   pthread_spin_destroy(spinlock);
#elif defined(EINA_HAVE_OSX_SPINLOCK)
   /* Not applicable */
   (void) spinlock;
#else
   eina_lock_free(spinlock);
#endif
}

static inline Eina_Bool
eina_semaphore_new(Eina_Semaphore *sem, int count_init)
{
   if (!sem || (count_init < 0))
     return EINA_FALSE;

#if defined(EINA_HAVE_OSX_SEMAPHORE)
   /* Atomic increment to generate the unique identifier */
   eina_spinlock_take(&_sem_ctr_lock);
   ++_sem_ctr;
   eina_spinlock_release(&_sem_ctr_lock);

   snprintf(sem->name, sizeof(sem->name), "/eina_sem_%u", _sem_ctr);
   sem_unlink(sem->name);
   sem->sema = sem_open(sem->name, O_CREAT, 0644, count_init);
   return (sem->sema == SEM_FAILED) ? EINA_FALSE : EINA_TRUE;
#else
   return (sem_init(sem, 1, count_init) == 0) ? EINA_TRUE : EINA_FALSE;
#endif
}

static inline Eina_Bool
eina_semaphore_free(Eina_Semaphore *sem)
{
   if (!sem)
     return EINA_FALSE;

#if defined(EINA_HAVE_OSX_SEMAPHORE)
   return ((sem_close(sem->sema) == 0) &&
           (sem_unlink(sem->name)) == 0) ? EINA_TRUE : EINA_FALSE;
#else
   return (sem_destroy(sem) == 0) ? EINA_TRUE : EINA_FALSE;
#endif
}

static inline Eina_Bool
eina_semaphore_lock(Eina_Semaphore *sem)
{
   Eina_Bool ok = EINA_FALSE;

   if (!sem)
     return EINA_FALSE;

   for (;;)
     {
        if (
#if defined(EINA_HAVE_OSX_SEMAPHORE)
            sem_wait(sem->sema)
#else
            sem_wait(sem)
#endif
            == 0)
          {
             ok = EINA_TRUE;
             break;
          }
        else
          {
             if (errno != EINTR)
               break;
          }
     }
   return ok;
}

static inline Eina_Bool
eina_semaphore_release(Eina_Semaphore *sem, int count_release EINA_UNUSED)
{
   if (!sem)
     return EINA_FALSE;

#if defined(EINA_HAVE_OSX_SEMAPHORE)
   return (sem_post(sem->sema) == 0) ? EINA_TRUE : EINA_FALSE;
#else
   return (sem_post(sem) == 0) ? EINA_TRUE : EINA_FALSE;
#endif
}

#undef _XOPEN_SOURCE
// This is necessary to let third party still define this macro
#ifdef EINA_XOPEN_SOURCE
# define _XOPEN_SOURCE EINA_XOPEN_SOURCE
#endif

#ifdef EINA_HAVE_OSX_SPINLOCK
/* The inclusion of libkern/OSAtomic.h is a mess because it includes stdbool
 * which #defines bool. #undef bool is not sufficient because then other
 * headers (dlfcn.h) require it and #include stdbool.h to get it. It is
 * therefore important to "undo" the whole stdbool.h inclusion. */
# undef true
# undef false
# undef bool
# undef __bool_true_false_are_defined
# undef _STDBOOL_H_ // OSX SDK
# undef __STDBOOL_H // Clang 5.1
# undef _STDBOOL_H  // GCC
#endif


#endif
