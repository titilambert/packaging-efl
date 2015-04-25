#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>

#ifdef HAVE_SYS_PRCTL_H
# include <sys/prctl.h>
#endif

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#include <Efl.h>

#include "Ecore.h"
#include "ecore_private.h"

#define MY_CLASS ECORE_EXE_CLASS

#include "ecore_exe_private.h"

/* TODO: Something to let people build a command line and does auto escaping -
 *
 * ecore_exe_snprintf()
 *
 *   OR
 *
 * cmd = ecore_exe_comand_parameter_append(cmd, "firefox");
 * cmd = ecore_exe_comand_parameter_append(cmd, "http://www.foo.com/bar.html?baz=yes");
 * each parameter appended is one argument, and it gets escaped, quoted, and
 * appended with a preceding space.  The first is the command off course.
 */

struct _ecore_exe_dead_exe
{
   pid_t pid;
   char *cmd;
};

static inline void _ecore_exe_exec_it(const char     *exe_cmd,
                                      Ecore_Exe_Flags flags);
static Eina_Bool   _ecore_exe_data_generic_handler(void             *data,
                                                   Ecore_Fd_Handler *fd_handler,
                                                   Ecore_Exe_Flags   flags);
static Eina_Bool            _ecore_exe_data_error_handler(void             *data,
                                                          Ecore_Fd_Handler *fd_handler);
static Eina_Bool            _ecore_exe_data_read_handler(void             *data,
                                                         Ecore_Fd_Handler *fd_handler);
static Eina_Bool            _ecore_exe_data_write_handler(void             *data,
                                                          Ecore_Fd_Handler *fd_handler);
static void                 _ecore_exe_flush(Ecore_Exe *obj);
static Ecore_Exe           *_ecore_exe_is_it_alive(pid_t pid);
static Eina_Bool            _ecore_exe_make_sure_its_dead(void *data);
static Eina_Bool            _ecore_exe_make_sure_its_really_dead(void *data);
static void                 _ecore_exe_dead_attach(Ecore_Exe *obj);

static const char *shell = NULL;

/* FIXME: This errno checking stuff should be put elsewhere for everybody to use.
 * For now it lives here though, just to make testing easier.
 */
static int _ecore_exe_check_errno(int         result,
                                  const char *file,
                                  int         line);

#define E_IF_NO_ERRNO(result, foo, ok)                                                           \
  while (((ok) = _ecore_exe_check_errno((result) = (foo), __FILE__, __LINE__)) == -1) sleep(1);  \
          if (ok)

#define E_NO_ERRNO(result, foo, ok) \
  while (((ok) = _ecore_exe_check_errno((result) = (foo), __FILE__, __LINE__)) == -1) sleep(1)

#define E_IF_NO_ERRNO_NOLOOP(result, foo, ok) \
  if (((ok) = _ecore_exe_check_errno((result) = (foo), __FILE__, __LINE__)))

static int
_ecore_exe_check_errno(int         result,
                       const char *file EINA_UNUSED,
                       int         line EINA_UNUSED)
{
   int saved_errno = errno;

   if (result == -1)
   {
      perror("*** errno reports ");
/* What is currently supported -
 *
 *   pipe
 *     EFAULT  Argument is not valid.
 *     EMFILE  Too many file descriptors used by process.
 *     ENFILE  Too many open files by system.
 *   read
 *     EAGAIN  No data now, try again.
 *     EBADF   This is not an fd that can be read.
 *     EFAULT  This is not a valid buffer.
 *     EINTR   Interupted by signal, try again.
 *     EINVAL  This is not an fd that can be read.
 *     EIO     I/O error.
 *     EISDIR  This is a directory, and cannot be read.
 *     others  Depending on what sort of thing we are reading from.
 *   close
 *     EBADF   This is not an fd that can be closed.
 *     EINTR   Interupted by signal, try again.
 *     EIO     I/O error.
 *   dup2
 *     EBADF   This is not an fd that can be dup2'ed.
 *     EBUSY   Race condition between open() and dup()
 *     EINTR   Interupted by signal, try again.
 *     EMFILE  Too many file descriptors used by process.
 *   fcntl
 *     EACCES, EAGAIN  Locked or mapped by something else, try again later.
 *     EBADF   This is not an fd that can be fcntl'ed.
 *     EDEADLK This will cause a deadlock.
 *     EFAULT  This is not a valid lock.
 *     EINTR   Interupted by signal, try again.
 *     EINVAL  This is not a valid arg.
 *     EMFILE  Too many file descriptors used by process.
 *     ENOLCK  Problem getting a lock.
 *     EPERM   Not allowed to do that.
 *   fsync
 *     EBADF   This is not an fd that is open for writing.
 *     EINVAL, EROFS  This is not an fd that can be fsynced.
 *     EIO     I/O error.
 *
 * How to use it -
 *    int ok = 0;
 *    int result;
 *
 *    E_IF_NO_ERRNO(result, foo(bar), ok)
 *      {
 *         E_IF_NO_ERRNO_NOLOOP(result, foo(bar), ok)
 *            {
 *            }
 *      }
 *
 *   if (!ok)
 *     {
 *        // Something failed, cleanup.
 *     }
 */
      switch (saved_errno)
      {
       case EACCES:
       case EAGAIN:
       case EINTR:
         { /* Not now, try later. */
            ERR("*** Must try again in %s @%u.", file, line);
            result = -1;
            break;
         }

       case EMFILE:
       case ENFILE:
       case ENOLCK:
         { /* Low on resources. */
            ERR("*** Low on resources in %s @%u.", file,
                line);
            result = 0;
            break;
         }

       case EIO:
         { /* I/O error. */
            ERR("*** I/O error in %s @%u.", file, line);
            result = 0;
            break;
         }

       case EFAULT:
       case EBADF:
       case EINVAL:
       case EROFS:
       case EISDIR:
       case EDEADLK:
       case EPERM:
       case EBUSY:
         { /* Programmer fucked up. */
            ERR("*** NAUGHTY PROGRAMMER!!!\n"
                "*** SPANK SPANK SPANK!!!\n"
                "*** Now go fix your code in %s @%u. Tut tut tut!",
                file, line);
            result = 0;
            break;
         }

       default:
         { /* Unsupported errno code, please add this one. */
            ERR("*** NAUGHTY PROGRAMMER!!!\n"
                "*** SPANK SPANK SPANK!!!\n"
                "*** Unsupported errno code %d, please add this one.\n"
                "*** Now go fix your code in %s @%u, from %s @%u. Tut tut tut!",
                saved_errno, __FILE__, __LINE__, file, line);
            result = 0;
            break;
         }
      }
   }
   else /* Everything is fine. */
     result = 1;

   errno = saved_errno;
   return result;
}

static int run_pri = ECORE_EXE_PRIORITY_INHERIT;

void
_impl_ecore_exe_run_priority_set(int pri)
{
   run_pri = pri;
}

int
_impl_ecore_exe_run_priority_get(void)
{
   return run_pri;
}

Eo *
_impl_ecore_exe_eo_base_finalize(Eo *obj, Ecore_Exe_Data *exe)
{
   int statusPipe[2] = { -1, -1 };
   int errorPipe[2] = { -1, -1 };
   int readPipe[2] = { -1, -1 };
   int writePipe[2] = { -1, -1 };
   int n = 0;
   int ok = 1;
   int result;

   if (!exe->cmd) return NULL;

   const char *exe_cmd = exe->cmd;
   Ecore_Exe_Flags flags = exe->flags;

   if ((flags & ECORE_EXE_PIPE_AUTO) && (!(flags & ECORE_EXE_PIPE_ERROR))
       && (!(flags & ECORE_EXE_PIPE_READ)))
     /* We need something to auto pipe. */
     flags |= ECORE_EXE_PIPE_READ | ECORE_EXE_PIPE_ERROR;

   exe->child_fd_error = -1;
   exe->child_fd_read = -1;
   exe->child_fd_write = -1;
   exe->child_fd_error_x = -1;
   exe->child_fd_read_x = -1;
   exe->child_fd_write_x = -1;

   /*  Create some pipes. */
   if (ok)
   {
      E_IF_NO_ERRNO_NOLOOP(result, pipe(statusPipe), ok)
      {
      }
   }
   if (ok && (flags & ECORE_EXE_PIPE_ERROR))
   {
      E_IF_NO_ERRNO_NOLOOP(result, pipe(errorPipe), ok)
      {
         exe->child_fd_error = errorPipe[0];
         exe->child_fd_error_x = errorPipe[1];
      }
   }
   if (ok && (flags & ECORE_EXE_PIPE_READ))
   {
      E_IF_NO_ERRNO_NOLOOP(result, pipe(readPipe), ok)
      {
         exe->child_fd_read = readPipe[0];
         exe->child_fd_read_x = readPipe[1];
      }
   }
   if (ok && (flags & ECORE_EXE_PIPE_WRITE))
   {
      E_IF_NO_ERRNO_NOLOOP(result, pipe(writePipe), ok)
      {
         exe->child_fd_write = writePipe[1];
         exe->child_fd_write_x = writePipe[0];
      }
   }
   if (ok)
   {
      pid_t pid = 0;
      volatile int vfork_exec_errno = 0;

      /* FIXME: I should double check this.  After a quick look around, this is already done, but via a more modern method. */
      /* signal(SIGPIPE, SIG_IGN);    We only want EPIPE on errors */
      pid = fork();

      if (pid == -1)
      {
         ERR("Failed to fork process");
         pid = 0;
      }
      else if (pid == 0) /* child */
      {
#ifdef HAVE_SYSTEMD
         unsetenv("NOTIFY_SOCKET");
#endif
         if (run_pri != ECORE_EXE_PRIORITY_INHERIT)
         {
#ifdef PRIO_PROCESS            
            if ((run_pri >= -20) && (run_pri <= 19))
              setpriority(PRIO_PROCESS, 0, run_pri);
#else
#warning "Your OS/libc does not provide PRIO_PROCESS (and possibly setpriority())"
#warning "This is a POSIX-1.2001 standard and it is highly encouraged that you"
#warning "Have support for this"
#endif
         }
         /* dup2 STDERR, STDIN, and STDOUT.  dup2() allegedly closes the
          * second pipe if it's open. On the other hand, there was the
          * Great FD Leak Scare of '06, so let's be paranoid. */
         if (ok && (flags & ECORE_EXE_PIPE_ERROR))
         {
            E_NO_ERRNO(result, close(STDERR_FILENO), ok);
            E_NO_ERRNO(result, dup2(errorPipe[1], STDERR_FILENO), ok);
         }
         if (ok && (flags & ECORE_EXE_PIPE_READ))
         {
            E_NO_ERRNO(result, close(STDOUT_FILENO), ok);
            E_NO_ERRNO(result, dup2(readPipe[1], STDOUT_FILENO), ok);
         }
         if (ok && (flags & ECORE_EXE_PIPE_WRITE))
         {
            E_NO_ERRNO(result, close(STDIN_FILENO), ok);
            E_NO_ERRNO(result, dup2(writePipe[0], STDIN_FILENO), ok);
         }

         if (ok)
         {
            /* Setup the status pipe. */
             E_NO_ERRNO(result, close(statusPipe[0]), ok);
             E_IF_NO_ERRNO(result, fcntl(statusPipe[1], F_SETFD, FD_CLOEXEC), ok) /* close on exec shows success */
             {
                /* Run the actual command. */
                 _ecore_exe_exec_it(exe_cmd, flags); /* no return */
             }
         }

         /* Something went 'orribly wrong. */
         vfork_exec_errno = errno;

         /* Close the pipes. */
         if (flags & ECORE_EXE_PIPE_ERROR)
           E_NO_ERRNO(result, close(errorPipe[1]), ok);
         if (flags & ECORE_EXE_PIPE_READ)
           E_NO_ERRNO(result, close(readPipe[1]), ok);
         if (flags & ECORE_EXE_PIPE_WRITE)
           E_NO_ERRNO(result, close(writePipe[0]), ok);
         E_NO_ERRNO(result, close(statusPipe[1]), ok);

         _exit(-1);
      }
      else   /* parent */
      {
         /* Close the unused pipes. */
          E_NO_ERRNO(result, close(statusPipe[1]), ok);

          /* FIXME: after having a good look at the current e fd
           * handling, investigate fcntl(dataPipe[x], F_SETSIG, ...) */
          /* FIXME: above F_SETSIG etc. - this is async SIGIO based IO
           * which is also linux specific so we probably don't want to
           * do this as long as select() is working fine. the only time
           * we really want to think of SIGIO async IO is when it all
           * actually works basically everywhere and we can turn all
           * IO into DMA async activities (i.e. you do a read() then
           * the read is complete not on return but when you get a
           * SIGIO - the read() just starts the transfer and it is
           * completed in the background by DMA (or whatever mechanism
           * the kernel choses)) */

          /* Wait for it to start executing. */
          /* FIXME: this doesn't seem very nice - we sit and block
           * waiting on a child process... even though it's just
           * the segment between the fork() and the exec) it just feels
           * wrong */
          for (;; )
          {
             char buf;

             E_NO_ERRNO(result, read(statusPipe[0], &buf, 1), ok);
             if (result == 0)
             {
                if (vfork_exec_errno != 0)
                {
                   n = vfork_exec_errno;
                   ERR("Could not start \"%s\"", exe_cmd);
                   pid = 0;
                }
                break;
             }
          }

          /* Close the status pipe. */
          E_NO_ERRNO(result, close(statusPipe[0]), ok);
      }

      if (pid)
      {
         /* Setup the exe structure. */
          exe->start_bytes = -1;
          exe->end_bytes = -1;
          exe->start_lines = -1;
          exe->end_lines = -1;
          exe->pid = pid;
          exe->flags = flags;
          if (exe->cmd)
          {
             if (flags & ECORE_EXE_PIPE_ERROR) /* Setup the error stuff. */
             {
                E_IF_NO_ERRNO(result,
                              fcntl(exe->child_fd_error, F_SETFL,
                                    O_NONBLOCK), ok) {
                }
                E_IF_NO_ERRNO(result,
                              fcntl(exe->child_fd_error, F_SETFD,
                                    FD_CLOEXEC), ok) {
                }
                E_IF_NO_ERRNO(result,
                              fcntl(exe->child_fd_error_x, F_SETFD,
                                    FD_CLOEXEC), ok) {
                }
                {
                   exe->error_fd_handler =
                     ecore_main_fd_handler_add(exe->child_fd_error,
                                               ECORE_FD_READ,
                                               _ecore_exe_data_error_handler,
                                               obj, NULL, NULL);
                   if (!exe->error_fd_handler)
                     ok = 0;
                }
             }
             if (ok && (flags & ECORE_EXE_PIPE_READ)) /* Setup the read stuff. */
             {
                E_IF_NO_ERRNO(result,
                              fcntl(exe->child_fd_read, F_SETFL,
                                    O_NONBLOCK), ok) {
                }
                E_IF_NO_ERRNO(result,
                              fcntl(exe->child_fd_read, F_SETFD,
                                    FD_CLOEXEC), ok) {
                }
                E_IF_NO_ERRNO(result,
                              fcntl(exe->child_fd_read_x, F_SETFD,
                                    FD_CLOEXEC), ok) {
                }
                {
                   exe->read_fd_handler =
                     ecore_main_fd_handler_add(exe->child_fd_read,
                                               ECORE_FD_READ,
                                               _ecore_exe_data_read_handler,
                                               obj, NULL, NULL);
                   if (!exe->read_fd_handler)
                     ok = 0;
                }
             }
             if (ok && (flags & ECORE_EXE_PIPE_WRITE)) /* Setup the write stuff. */
             {
                E_IF_NO_ERRNO(result,
                              fcntl(exe->child_fd_write, F_SETFL,
                                    O_NONBLOCK), ok) {
                }
                E_IF_NO_ERRNO(result,
                              fcntl(exe->child_fd_write, F_SETFD,
                                    FD_CLOEXEC), ok) {
                }
                E_IF_NO_ERRNO(result,
                              fcntl(exe->child_fd_write_x, F_SETFD,
                                    FD_CLOEXEC), ok) {
                }
                {
                   exe->write_fd_handler =
                     ecore_main_fd_handler_add(exe->child_fd_write,
                                               ECORE_FD_WRITE,
                                               _ecore_exe_data_write_handler,
                                               obj, NULL, NULL);
                   if (exe->write_fd_handler)
                     ecore_main_fd_handler_active_set(exe->write_fd_handler, 0);  /* Nothing to write to start with. */
                   else
                     ok = 0;
                }
             }

             _ecore_exe_exes = eina_list_append(_ecore_exe_exes, obj);
             n = 0;
          }
          else
            ok = 0;
      }
      else
        ok = 0;
   }

   if (!ok) /* Something went wrong, so pull down everything. */
   {
      if (exe->pid) ecore_exe_terminate(obj);
      IF_FN_DEL(ecore_exe_free, obj);
   }
   else
   {
      Ecore_Exe_Event_Add *e;

      e = _ecore_exe_event_add_new();
      if (e)
        {
           e->exe = obj;
           /* Send the event. */
           ecore_event_add(ECORE_EXE_EVENT_ADD, e,
                           _ecore_exe_event_add_free, NULL);
        }
      /* INF("Running as %d for %s.\n", exe->pid, exe->cmd); */
   }

   errno = n;
   return obj;
}

Eina_Bool
_impl_ecore_exe_send(Ecore_Exe  *obj EINA_UNUSED,
               Ecore_Exe_Data *exe,
               const void *data,
               int         size)
{
   void *buf;

   if (exe->child_fd_write == -1)
     {
        ERR("Ecore_Exe %p created without ECORE_EXE_PIPE_WRITE! "
              "Cannot send %d bytes from %p", exe, size, data);
        return EINA_FALSE;
     }

   buf = realloc(exe->write_data_buf, exe->write_data_size + size);
   if (!buf) return EINA_FALSE;

   exe->write_data_buf = buf;
   memcpy((char *)exe->write_data_buf + exe->write_data_size, data, size);
   exe->write_data_size += size;

   if (exe->write_fd_handler)
      ecore_main_fd_handler_active_set(exe->write_fd_handler, ECORE_FD_WRITE);

   return EINA_TRUE;
}

void
_impl_ecore_exe_auto_limits_set(Ecore_Exe *obj EINA_UNUSED,
                          Ecore_Exe_Data *exe,
                          int        start_bytes,
                          int        end_bytes,
                          int        start_lines,
                          int        end_lines)
{
   /* FIXME: sanitize the input. */
   exe->start_bytes = start_bytes;
   exe->end_bytes = end_bytes;
   exe->start_lines = start_lines;
   exe->end_lines = end_lines;

   /* FIXME: get this can of worms working.
    *
    * capture stderr & stdout internally
    *
    * raster and onefang keep moving the goal posts on this one.  It started out as
    * "show users the error output if an exe fails" and is rapidly approaching
    * "alternative method of getting the data, poll vs event driven".  Some serious
    * thinking needs to be applied to this.  Do we really want to go that far?  If
    * so, we should change the names.  The basic design will probably remain the
    * same which ever way we go.  The constant goal post moving is probably due to
    * generic design methods leading to feature creep as we inspired each other to
    * more generic designs.  It does seem like the closer we get to poll driven,
    * the more issues and corner cases there are.
    *
    * Instead of doing the usual register an event handler thing, we are ecore_exe,
    * we can take some short cuts.  Don't send the events, just leave the exe buffers
    * as is until the user asks for them, then return the event.
    *
    * start = 0,  end = 0;   clogged arteries get flushed, everything is ignored.
    * start = -1, end = -1;  clogged arteries get transferred to internal buffers.  Actually, either == -1 means buffer everything.
    * start = X,  end = 0;   buffer first X out of clogged arteries, flush and ignore rest.
    * start = 0,  end = X;   circular buffer X
    * start = X,  end = Y;   buffer first X out of clogged arteries, circular buffer Y from beginning.
    *
    * bytes vs lines, which ever one reaches the limit first.
    * Before we go beyond the start+end limit, leave the end buffer empty, and store both in the start buffer, coz they overlap.
    * After we pass the the start+end limit, insert "\n...\n" at the end of the start buffer, copy the rest to the end buffer, then store in the end buffer.
    *
    * Other issues -
    * Spank programmer for polling data if polling is not turned on.
    * Spank programmer for setting up event callbacks if polling is turned on.
    * Spank programmer for freeing the event data if it came from the event system, as that autofrees.
    * Spank the programmer if they try to set the limits bigger than what has been gathered & ignored already, coz they just lost data.
    * Spank onefang and raster for opening this can of worms.
    * Should we have separate out/err limits?
    * Should we remove from the internal buffer the data that was delivered already?
    * If so, what to do about limits, start, and end?  They could loose their meaning.
    */
}

Ecore_Exe_Event_Data *
_impl_ecore_exe_event_data_get(Ecore_Exe      *obj,
                         Ecore_Exe_Data *exe,
                         Ecore_Exe_Flags flags)
{
   Ecore_Exe_Event_Data *e = NULL;
   int is_buffered = 0;
   unsigned char *inbuf;
   int inbuf_num;

   /* Sort out what sort of event we are. */
   if (flags & ECORE_EXE_PIPE_READ)
   {
      flags = ECORE_EXE_PIPE_READ;
      if (exe->flags & ECORE_EXE_PIPE_READ_LINE_BUFFERED)
        is_buffered = 1;
   }
   else
   {
      flags = ECORE_EXE_PIPE_ERROR;
      if (exe->flags & ECORE_EXE_PIPE_ERROR_LINE_BUFFERED)
        is_buffered = 1;
   }

   /* Get the data. */
   if (flags & ECORE_EXE_PIPE_READ)
   {
      inbuf = exe->read_data_buf;
      inbuf_num = exe->read_data_size;
      exe->read_data_buf = NULL;
      exe->read_data_size = 0;
   }
   else
   {
      inbuf = exe->error_data_buf;
      inbuf_num = exe->error_data_size;
      exe->error_data_buf = NULL;
      exe->error_data_size = 0;
   }

   e = calloc(1, sizeof(Ecore_Exe_Event_Data));
   if (e)
   {
      e->exe = obj;
      e->data = inbuf;
      e->size = inbuf_num;

      if (is_buffered) /* Deal with line buffering. */
      {
         int max = 0;
         int count = 0;
         int i;
         int last = 0;
         char *c;

         c = (char *)inbuf;
         for (i = 0; i < inbuf_num; i++) /* Find the lines. */
         {
            if (inbuf[i] == '\n')
            {
               if (count >= max)
               {
                  /* In testing, the lines seem to arrive in batches of 500 to 1000 lines at most, roughly speaking. */
                   max += 10; /* FIXME: Maybe keep track of the largest number of lines ever sent, and add half that many instead of 10. */
                   e->lines = realloc(e->lines, sizeof(Ecore_Exe_Event_Data_Line) * (max + 1)); /* Allow room for the NULL termination. */
               }
               /* raster said to leave the line endings as line endings, however -
                * This is line buffered mode, we are not dealing with binary here, but lines.
                * If we are not dealing with binary, we must be dealing with ASCII, unicode, or some other text format.
                * Thus the user is most likely gonna deal with this text as strings.
                * Thus the user is most likely gonna pass this data to str functions.
                * rasters way - the endings are always gonna be '\n';  onefangs way - they will always be '\0'
                * We are handing them the string length as a convenience.
                * Thus if they really want it in raw format, they can e->lines[i].line[e->lines[i].size - 1] = '\n'; easily enough.
                * In the default case, we can do this conversion quicker than the user can, as we already have the index and pointer.
                * Let's make it easy on them to use these as standard C strings.
                *
                * onefang is proud to announce that he has just set a new personal record for the
                * most over documentation of a simple assignment statement.  B-)
                */
               inbuf[i] = '\0';
               e->lines[count].line = c;
               e->lines[count].size = i - last;
               last = i + 1;
               c = (char *)&inbuf[last];
               count++;
            }
         }
         if (i > last) /* Partial line left over, save it for next time. */
         {
            if (count != 0) e->size = last;
            if (flags & ECORE_EXE_PIPE_READ)
            {
               exe->read_data_size = i - last;
               exe->read_data_buf = malloc(exe->read_data_size);
               memcpy(exe->read_data_buf, c, exe->read_data_size);
            }
            else
            {
               exe->error_data_size = i - last;
               exe->error_data_buf = malloc(exe->error_data_size);
               memcpy(exe->error_data_buf, c, exe->error_data_size);
            }
         }
         if (count == 0) /* No lines to send, cancel the event. */
         {
            _ecore_exe_event_exe_data_free(NULL, e);
            e = NULL;
         }
         else /* NULL terminate the array, so that people know where the end is. */
         {
            e->lines[count].line = NULL;
            e->lines[count].size = 0;
         }
      }
   }

   return e;
}

void
_impl_ecore_exe_eo_base_destructor(Eo *obj, Ecore_Exe_Data *exe)
{
   void *data;
   int ok = 0;
   int result;

   data = exe->data;

   if (exe->pre_free_cb)
     exe->pre_free_cb(data, obj);

   if (exe->doomsday_clock)
   {
      struct _ecore_exe_dead_exe *dead;

      ecore_timer_del(exe->doomsday_clock);
      exe->doomsday_clock = NULL;
      dead = exe->doomsday_clock_dead;
      if (dead)
      {
         IF_FREE(dead->cmd);
         free(dead);
         exe->doomsday_clock_dead = NULL;
      }
   }
   IF_FN_DEL(ecore_main_fd_handler_del, exe->write_fd_handler);
   IF_FN_DEL(ecore_main_fd_handler_del, exe->read_fd_handler);
   IF_FN_DEL(ecore_main_fd_handler_del, exe->error_fd_handler);
   if (exe->child_fd_write_x != -1)
     E_NO_ERRNO(result, close(exe->child_fd_write_x), ok);
   if (exe->child_fd_read_x != -1)
     E_NO_ERRNO(result, close(exe->child_fd_read_x), ok);
   if (exe->child_fd_error_x != -1)
     E_NO_ERRNO(result, close(exe->child_fd_error_x), ok);
   if (exe->child_fd_write != -1)
     E_NO_ERRNO(result, close(exe->child_fd_write), ok);
   if (exe->child_fd_read != -1)
     E_NO_ERRNO(result, close(exe->child_fd_read), ok);
   if (exe->child_fd_error != -1)
     E_NO_ERRNO(result, close(exe->child_fd_error), ok);
   IF_FREE(exe->write_data_buf);
   IF_FREE(exe->read_data_buf);
   IF_FREE(exe->error_data_buf);
   IF_FREE(exe->cmd);

   _ecore_exe_exes = eina_list_remove(_ecore_exe_exes, obj);
   IF_FREE(exe->tag);
}

void
_impl_ecore_exe_pause(Eo *obj EINA_UNUSED, Ecore_Exe_Data *exe)
{
   kill(exe->pid, SIGSTOP);
}

void
_impl_ecore_exe_continue(Eo *obj EINA_UNUSED, Ecore_Exe_Data *exe)
{
   kill(exe->pid, SIGCONT);
}

void
_impl_ecore_exe_interrupt(Ecore_Exe *obj, Ecore_Exe_Data *exe)
{
   _ecore_exe_dead_attach(obj);
   kill(exe->pid, SIGINT);
}

void
_impl_ecore_exe_quit(Ecore_Exe *obj, Ecore_Exe_Data *exe)
{
   _ecore_exe_dead_attach(obj);
   kill(exe->pid, SIGQUIT);
}

void
_impl_ecore_exe_terminate(Ecore_Exe *obj, Ecore_Exe_Data *exe)
{
   _ecore_exe_dead_attach(obj);
   INF("Sending TERM signal to %s (%d).", exe->cmd, exe->pid);
   kill(exe->pid, SIGTERM);
}

void
_impl_ecore_exe_kill(Ecore_Exe *obj EINA_UNUSED, Ecore_Exe_Data *exe)
{
   struct _ecore_exe_dead_exe *dead;
   dead = calloc(1, sizeof(struct _ecore_exe_dead_exe));
   if (dead)
   {
      dead->pid = exe->pid;
      dead->cmd = strdup(exe->cmd);
      IF_FN_DEL(ecore_timer_del, exe->doomsday_clock);
      exe->doomsday_clock =
        ecore_timer_add(10.0, _ecore_exe_make_sure_its_really_dead, dead);
   }

   INF("Sending KILL signal to %s (%d).", exe->cmd, exe->pid);
   kill(exe->pid, SIGKILL);
}

void
_impl_ecore_exe_signal(Ecore_Exe *obj EINA_UNUSED,
                 Ecore_Exe_Data *exe,
                 int        num)
{
   if (num == 1)
     kill(exe->pid, SIGUSR1);
   else if (num == 2)
     kill(exe->pid, SIGUSR2);
}

void
_impl_ecore_exe_hup(Ecore_Exe *obj EINA_UNUSED, Ecore_Exe_Data *exe)
{
   kill(exe->pid, SIGHUP);
}

static Ecore_Exe *
_ecore_exe_is_it_alive(pid_t pid)
{
   Ecore_Exe *exe = NULL;

   /* FIXME: There is no nice, safe, OS independent way to tell if a
    * particular PID is still alive.  I have written code to do so
    * for my urunlevel busybox applet (http://urunlevel.sourceforge.net/),
    * but it's for linux only, and still not guaranteed.
    *
    * So for now, we just check that a valid Ecore_Exe structure
    * exists for it.  Even that is not a guarantee, as the structure
    * can be freed without killing the process.
    *
    * I think we can safely put exe's into two categories, those users
    * that care about the life of the exe, and the run and forget type.
    * The run and forget type starts up the exe, then free's the
    * Ecore_Exe structure straight away.  They can never call any of
    * the functions that can call this, so we don't worry about them.
    *
    * Those user's that care about the life of exe's will keep the
    * Ecore_Exe structure around, terminate them eventually, or
    * register for exit events.  For these ones the assumption
    * that valid Ecore_Exe struct == live exe is almost valid.
    *
    * I will probably copy my urunlevel code into here someday.
    */
   exe = _ecore_exe_find(pid);

   return exe;
}

static Eina_Bool
_ecore_exe_make_sure_its_dead(void *data)
{
   struct _ecore_exe_dead_exe *dead;

   dead = data;
   if (dead)
   {
      Ecore_Exe *obj = NULL;

      if ((obj = _ecore_exe_is_it_alive(dead->pid)))
      {
         Ecore_Exe_Data *exe = eo_data_scope_get(obj, MY_CLASS);
         if (dead->cmd)
           INF("Sending KILL signal to allegedly dead %s (%d).",
               dead->cmd, dead->pid);
         else
           INF("Sending KILL signal to allegedly dead PID %d.",
               dead->pid);
         exe->doomsday_clock =
           ecore_timer_add(10.0, _ecore_exe_make_sure_its_really_dead,
                           dead);
         kill(dead->pid, SIGKILL);
      }
      else
      {
         IF_FREE(dead->cmd);
         free(dead);
      }
   }
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_ecore_exe_make_sure_its_really_dead(void *data)
{
   struct _ecore_exe_dead_exe *dead;

   dead = data;
   if (dead)
   {
      Ecore_Exe *obj = NULL;

      if ((obj = _ecore_exe_is_it_alive(dead->pid)))
      {
         Ecore_Exe_Data *exe = eo_data_scope_get(obj, MY_CLASS);
         ERR("RUN!  The zombie wants to eat your brains!  And your CPU!");
         if (dead->cmd)
           INF("%s (%d) is not really dead.", dead->cmd, dead->pid);
         else
           INF("PID %d is not really dead.", dead->pid);
         exe->doomsday_clock = NULL;
      }
      IF_FREE(dead->cmd);
      free(dead);
   }
   return ECORE_CALLBACK_CANCEL;
}

Ecore_Timer *
_ecore_exe_doomsday_clock_get(Ecore_Exe *obj)
{
   Ecore_Exe_Data *exe = eo_data_scope_get(obj, MY_CLASS);
   return exe->doomsday_clock;
}

void
_ecore_exe_doomsday_clock_set(Ecore_Exe   *obj,
                              Ecore_Timer *dc)
{
   Ecore_Exe_Data *exe = eo_data_scope_get(obj, MY_CLASS);
   exe->doomsday_clock = dc;
}

static inline void
_ecore_exe_exec_it(const char     *exe_cmd,
                   Ecore_Exe_Flags flags)
{
   char use_sh = 1;
   char *buf = NULL;
   char **args = NULL;
   int save_errno = 0;

   /* So what is this doing?
    *
    * We are trying to avoid wrapping the exe call with /bin/sh -c.
    * We conservatively search for certain shell meta characters,
    * If we don't find them, we can call the exe directly.
    */
   if (!strpbrk(exe_cmd, "|&;<>()$`\\\"'*?#"))
   {
      char *token;
      char pre_command = 1;
      int num_tokens = 0;

      if (!(buf = strdup(exe_cmd)))
        return;

      token = strtok(buf, " \t\n\v");
      while (token)
      {
         if (token[0] == '~')
           break;
         if (pre_command)
         {
            if (token[0] == '[')
              break;
            if (strchr(token, '='))
              break;
            else
              pre_command = 0;
         }
         num_tokens++;
         token = strtok(NULL, " \t\n\v");
      }
      IF_FREE(buf);
      if ((!token) && (num_tokens))
      {
         int i = 0;

         if (!(buf = strdup(exe_cmd)))
           return;

         token = strtok(buf, " \t\n\v");
         use_sh = 0;
         if (!(args = (char **)calloc(num_tokens + 1, sizeof(char *))))
         {
            IF_FREE(buf);
            return;
         }
         for (i = 0; i < num_tokens; i++)
         {
            if (token)
              args[i] = token;
            token = strtok(NULL, " \t\n\v");
         }
         args[num_tokens] = NULL;
      }
   }

#ifdef HAVE_SYS_PRCTL_H
   if ((flags & ECORE_EXE_TERM_WITH_PARENT))
   {
      prctl(PR_SET_PDEATHSIG, SIGTERM);
   }
#endif

   if (!(flags & ECORE_EXE_NOT_LEADER)) setsid();
   if ((flags & ECORE_EXE_USE_SH))
   {
      errno = 0;
      execl("/bin/sh", "/bin/sh", "-c", exe_cmd, (char *)NULL);
   }
   else if (use_sh) /* We have to use a shell to run this. */
   {
      if (!shell) /* Find users preferred shell. */
      {
#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
         if (getuid() == geteuid())
#endif
           shell = getenv("SHELL");
         if (!shell)
           shell = "/bin/sh";
      }
      errno = 0;
      execl(shell, shell, "-c", exe_cmd, (char *)NULL);
   }
   else
   { /* We can run this directly. */
      if (!args)
      {
         IF_FREE(buf);
         IF_FREE(args);
         ERR("arg[0] is NULL!");
         return;
      }
      errno = 0;
      if (args[0]) execvp(args[0], args);
   }

   save_errno = errno;
   IF_FREE(buf);
   IF_FREE(args);
   errno = save_errno;
   return;
}

static Eina_Bool
_ecore_exe_data_generic_handler(void             *data,
                                Ecore_Fd_Handler *fd_handler,
                                Ecore_Exe_Flags   flags)
{
   Ecore_Exe *obj = data;
   int child_fd;
   int event_type;
   const Eo_Event_Description *eo_event = NULL;

   Ecore_Exe_Data *exe = eo_data_scope_get(obj, MY_CLASS);

   /* Sort out what sort of handler we are. */
   if (flags & ECORE_EXE_PIPE_READ)
   {
      flags = ECORE_EXE_PIPE_READ;
      event_type = ECORE_EXE_EVENT_DATA;
      eo_event = ECORE_EXE_EVENT_DATA_GET;
      child_fd = exe->child_fd_read;
   }
   else
   {
      flags = ECORE_EXE_PIPE_ERROR;
      event_type = ECORE_EXE_EVENT_ERROR;
      eo_event = ECORE_EXE_EVENT_DATA_ERROR;
      child_fd = exe->child_fd_error;
   }

   if ((fd_handler)
       && (ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_READ)))
   {
      unsigned char *inbuf;
      int inbuf_num;

      /* Get any left over data from last time. */
      if (flags & ECORE_EXE_PIPE_READ)
      {
         inbuf = exe->read_data_buf;
         inbuf_num = exe->read_data_size;
         exe->read_data_buf = NULL;
         exe->read_data_size = 0;
      }
      else
      {
         inbuf = exe->error_data_buf;
         inbuf_num = exe->error_data_size;
         exe->error_data_buf = NULL;
         exe->error_data_size = 0;
      }

      for (;; )
      {
         int num, lost_exe;
         char buf[READBUFSIZ];

         lost_exe = 0;
         errno = 0;
         if ((num = read(child_fd, buf, READBUFSIZ)) < 1)
         {
            /* FIXME: SPEED/SIZE TRADE OFF - add a smaller READBUFSIZE
             * (currently 64k) to inbuf, use that instead of buf, and
             * save ourselves a memcpy(). */
              lost_exe = ((errno == EIO) ||
                          (errno == EBADF) ||
                          (errno == EPIPE) ||
                          (errno == EINVAL) || (errno == ENOSPC));
              if ((errno != EAGAIN) && (errno != EINTR))
                perror("_ecore_exe_generic_handler() read problem ");
         }
         if (num > 0) /* data got read. */
         {
            inbuf = realloc(inbuf, inbuf_num + num);
            memcpy(inbuf + inbuf_num, buf, num);
            inbuf_num += num;
         }
         else
         { /* No more data to read. */
            if (inbuf)
            {
               Ecore_Exe_Event_Data *e;

               /* Stash the data away for later. */
               if (flags & ECORE_EXE_PIPE_READ)
               {
                  exe->read_data_buf = inbuf;
                  exe->read_data_size = inbuf_num;
               }
               else
               {
                  exe->error_data_buf = inbuf;
                  exe->error_data_size = inbuf_num;
               }

               if (!(exe->flags & ECORE_EXE_PIPE_AUTO))
               {
                  e = ecore_exe_event_data_get(obj, flags);
                  if (e) /* Send the event. */
                    {
                       ecore_event_add(event_type, e,
                             _ecore_exe_event_exe_data_free,
                             NULL);
                       eo_do(obj, eo_event_callback_call(eo_event, e));
                    }
               }
            }
            if (lost_exe)
            {
               if (flags & ECORE_EXE_PIPE_READ)
               {
                  if (exe->read_data_size)
                    INF("There are %d bytes left unsent from the dead exe %s.",
                        exe->read_data_size, exe->cmd);
               }
               else
               {
                  if (exe->error_data_size)
                    INF("There are %d bytes left unsent from the dead exe %s.",
                        exe->error_data_size, exe->cmd);
               }
               /* Thought about this a bit.  If the exe has actually
                * died, this won't do any harm as it must have died
                * recently and the pid has not had a chance to recycle.
                * It is also a paranoid catchall, coz the usual ecore_signal
                * mechenism should kick in.  But let's give it a good
                * kick in the head anyway.
                */
               ecore_exe_terminate(obj);
            }
            break;
         }
      }
   }

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_ecore_exe_data_error_handler(void             *data,
                              Ecore_Fd_Handler *fd_handler)
{
   return _ecore_exe_data_generic_handler(data, fd_handler,
                                          ECORE_EXE_PIPE_ERROR);
}

static Eina_Bool
_ecore_exe_data_read_handler(void             *data,
                             Ecore_Fd_Handler *fd_handler)
{
   return _ecore_exe_data_generic_handler(data, fd_handler,
                                          ECORE_EXE_PIPE_READ);
}

static Eina_Bool
_ecore_exe_data_write_handler(void             *data,
                              Ecore_Fd_Handler *fd_handler EINA_UNUSED)
{
   Ecore_Exe *obj = data;
   Ecore_Exe_Data *exe = eo_data_scope_get(obj, MY_CLASS);

   if ((exe->write_fd_handler) &&
       (ecore_main_fd_handler_active_get
          (exe->write_fd_handler, ECORE_FD_WRITE)))
     _ecore_exe_flush(obj);

   /* If we have sent all there is to send, and we need to close the pipe, then close it. */
   if ((exe->close_stdin == 1)
       && (exe->write_data_size == exe->write_data_offset))
   {
      int ok = 0;
      int result;

      INF("Closing stdin for %s", exe->cmd);
      /* if (exe->child_fd_write != -1)  E_NO_ERRNO(result, fsync(exe->child_fd_write), ok);   This a) doesn't work, and b) isn't needed. */
      IF_FN_DEL(ecore_main_fd_handler_del, exe->write_fd_handler);
      if (exe->child_fd_write != -1)
        E_NO_ERRNO(result, close(exe->child_fd_write), ok);
      exe->child_fd_write = -1;
      IF_FREE(exe->write_data_buf);
   }

   return ECORE_CALLBACK_RENEW;
}

static void
_ecore_exe_flush(Ecore_Exe *obj)
{
   int count;
   Ecore_Exe_Data *exe = eo_data_scope_get(obj, MY_CLASS);

   /* check whether we need to write anything at all. */
   if ((exe->child_fd_write == -1) || (!exe->write_data_buf))
     return;
   if (exe->write_data_size == exe->write_data_offset)
     return;

   count = write(exe->child_fd_write,
                 (char *)exe->write_data_buf + exe->write_data_offset,
                 exe->write_data_size - exe->write_data_offset);
   if (count < 1)
   {
      if (errno == EIO || errno == EBADF || errno == EPIPE || errno == EINVAL || errno == ENOSPC) /* we lost our exe! */
      {
         ecore_exe_terminate(obj);
         if (exe->write_fd_handler)
           ecore_main_fd_handler_active_set(exe->write_fd_handler, 0);
      }
   }
   else
   {
      exe->write_data_offset += count;
      if (exe->write_data_offset >= exe->write_data_size) /* Nothing left to write, clean up. */
      {
         exe->write_data_size = 0;
         exe->write_data_offset = 0;
         IF_FREE(exe->write_data_buf);
         if (exe->write_fd_handler)
           ecore_main_fd_handler_active_set(exe->write_fd_handler, 0);
      }
   }
}

static void
_ecore_exe_dead_attach(Ecore_Exe *obj)
{
   struct _ecore_exe_dead_exe *dead;
   Ecore_Exe_Data *exe = eo_data_scope_get(obj, MY_CLASS);

   if (exe->doomsday_clock_dead) return;
   dead = calloc(1, sizeof(struct _ecore_exe_dead_exe));
   if (dead)
   {
      dead->pid = exe->pid;
      if (exe->cmd) dead->cmd = strdup(exe->cmd);
      IF_FN_DEL(ecore_timer_del, exe->doomsday_clock);
      exe->doomsday_clock =
        ecore_timer_add(10.0, _ecore_exe_make_sure_its_dead, dead);
      exe->doomsday_clock_dead = dead;
   }
}
