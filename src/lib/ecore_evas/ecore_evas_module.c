#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include "ecore_private.h"

#include "Ecore_Evas.h"
#include "ecore_evas_private.h"
#include <unistd.h>

static Eina_Hash *_registered_engines = NULL;
static Eina_List *_engines_paths = NULL;
static Eina_List *_engines_available = NULL;

#ifdef _WIN32
# define ECORE_EVAS_ENGINE_NAME "module.dll"
#else
# define ECORE_EVAS_ENGINE_NAME "module.so"
#endif


Eina_Module *
_ecore_evas_engine_load(const char *engine)
{
   const char *path;
   Eina_List *l;
   Eina_Module *em = NULL;
   Eina_Bool run_in_tree;

   EINA_SAFETY_ON_NULL_RETURN_VAL(engine, NULL);

   em =  (Eina_Module *)eina_hash_find(_registered_engines, engine);
   if (em) return em;

   run_in_tree = !!getenv("EFL_RUN_IN_TREE");

   EINA_LIST_FOREACH(_engines_paths, l, path)
     {
        char tmp[PATH_MAX] = "";

#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
        if (getuid() == geteuid())
#endif
          {
             if (run_in_tree)
               {
                  struct stat st;
                  snprintf(tmp, sizeof(tmp), "%s/%s/.libs/%s",
                           path, engine, ECORE_EVAS_ENGINE_NAME);
                  if (stat(tmp, &st) != 0)
                  tmp[0] = '\0';
               }
          }

        if (tmp[0] == '\0')
          snprintf(tmp, sizeof(tmp), "%s/%s/%s/%s",
                   path, engine, MODULE_ARCH, ECORE_EVAS_ENGINE_NAME);

        em = eina_module_new(tmp);
        if (!em) continue;

        if (!eina_module_load(em))
          {
             eina_module_free(em);
             continue;
          }
        if (eina_hash_add(_registered_engines, engine, em))
          return em;
     }

   return NULL;
}

void
_ecore_evas_engine_init(void)
{
   char *paths[2] = { NULL, NULL };
   unsigned int i;
   unsigned int j;

/* avoid freeing modules ever to avoid deferred cb symbol problems */
//   _registered_engines = eina_hash_string_small_new(EINA_FREE_CB(eina_module_free));
   _registered_engines = eina_hash_string_small_new(NULL);

#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
   if (getuid() == geteuid())
#endif
     {
        if (getenv("EFL_RUN_IN_TREE"))
          {
             struct stat st;
             const char mp[] = PACKAGE_BUILD_DIR"/src/modules/ecore_evas/engines/";
             if (stat(mp, &st) == 0)
               {
                  _engines_paths = eina_list_append(_engines_paths, strdup(mp));
                  return;
               }
          }
     }

   /* 1. libecore_evas.so/../ecore_evas/engines/ */
   paths[0] = eina_module_symbol_path_get(_ecore_evas_engine_init, "/ecore_evas/engines");
#ifndef _WIN32
   /* 3. PREFIX/ecore_evas/engines/ */
   paths[1] = strdup(PACKAGE_LIB_DIR "/ecore_evas/engines");
#else
   paths[1] = eina_module_symbol_path_get(_ecore_evas_engine_init, "/../lib/ecore_evas/engines");
#endif

   for (j = 0; j < ((sizeof (paths) / sizeof (char*)) - 1); ++j)
     for (i = j + 1; i < sizeof (paths) / sizeof (char*); ++i)
       if (paths[i] && paths[j] && !strcmp(paths[i], paths[j]))
         {
            free(paths[i]);
            paths[i] = NULL;
         }

   for (i = 0; i < sizeof (paths) / sizeof (char*); ++i)
     if (paths[i])
       _engines_paths = eina_list_append(_engines_paths, paths[i]);
}

void
_ecore_evas_engine_shutdown(void)
{
   char *path;

/* don't free modules to avoid fn callback deferred symbol problems
   if (_registered_engines)
     {
       eina_hash_free(_registered_engines);
       _registered_engines = NULL;
     }
 */
   
   EINA_LIST_FREE(_engines_paths, path)
     free(path);

   EINA_LIST_FREE(_engines_available, path)
     eina_stringshare_del(path);
}

static Eina_Bool
_file_exists(const char *file)
{
   struct stat st;
   if (!file) return EINA_FALSE;

   if (stat(file, &st) < 0) return EINA_FALSE;
   return EINA_TRUE;
}

const Eina_List *
_ecore_evas_available_engines_get(void)
{
   Eina_File_Direct_Info *info;
   Eina_Iterator *it;
   Eina_List *l = NULL, *result = NULL;
   const char *path;

   if (_engines_available) return _engines_available;

   EINA_LIST_FOREACH(_engines_paths, l, path)
     {
        it = eina_file_direct_ls(path);

        if (it)
          {
             EINA_ITERATOR_FOREACH(it, info)
               {
                  char tmp[PATH_MAX];
                  snprintf(tmp, sizeof (tmp), "%s/%s/" ECORE_EVAS_ENGINE_NAME,
                           info->path, MODULE_ARCH);

                  if (_file_exists(tmp))
                    {
                       const char *name;

#ifdef _WIN32
                       EVIL_PATH_SEP_WIN32_TO_UNIX(info->path);
#endif
                       name = strrchr(info->path, '/');
                       if (name) name++;
                       else name = info->path;
#define ADDENG(x) result = eina_list_append(result, eina_stringshare_add(x))
                       if (!strcmp(name, "fb"))
                         {
#ifdef BUILD_ECORE_EVAS_FB
                            ADDENG("fb");
#endif
                         }
                       else if (!strcmp(name, "x"))
                         {
#ifdef BUILD_ECORE_EVAS_OPENGL_X11
                            ADDENG("opengl_x11");
#endif
#ifdef BUILD_ECORE_EVAS_SOFTWARE_XLIB
                            ADDENG("software_x11");
#else
# ifdef BUILD_ECORE_EVAS_SOFTWARE_XCB
                            ADDENG("software_x11");
# endif
#endif
                         }
                       else if (!strcmp(name, "buffer"))
                         {
#ifdef BUILD_ECORE_EVAS_BUFFER
                            ADDENG("buffer");
#endif
#ifdef BUILD_ECORE_EVAS_EWS
                            ADDENG("ews");
#endif
                         }
                       else if (!strcmp(name, "cocoa"))
                         {
#ifdef BUILD_ECORE_EVAS_OPENGL_COCOA
                            ADDENG("opengl_cocoa");
#endif
                         }
                       else if (!strcmp(name, "psl1ght"))
                         {
#ifdef BUILD_ECORE_EVAS_PSL1GHT
                            ADDENG("psl1ght");
#endif
                         }
                       else if (!strcmp(name, "sdl"))
                         {
#ifdef BUILD_ECORE_EVAS_OPENGL_SDL
                            ADDENG("opengl_sdl");
#endif
#ifdef BUILD_ECORE_EVAS_SOFTWARE_SDL
                            ADDENG("sdl");
#endif
                         }
                       else if (!strcmp(name, "wayland"))
                         {
#ifdef BUILD_ECORE_EVAS_WAYLAND_SHM
                            ADDENG("wayland_shm");
#endif
#ifdef BUILD_ECORE_EVAS_WAYLAND_EGL
                            ADDENG("wayland_egl");
#endif
                         }
                       else if (!strcmp(name, "win32"))
                         {
#ifdef BUILD_ECORE_EVAS_SOFTWARE_GDI
                            ADDENG("software_gdi");
#endif
#ifdef BUILD_ECORE_EVAS_SOFTWARE_DDRAW
                            ADDENG("software_ddraw");
#endif
#ifdef BUILD_ECORE_EVAS_DIRECT3D
                            ADDENG("direct3d");
#endif
#ifdef BUILD_ECORE_EVAS_OPENGL_GLEW
                            ADDENG("opengl_glew");
#endif
                         }
                       else if (!strcmp(name, "drm"))
                         {
#ifdef BUILD_ECORE_EVAS_DRM
                            ADDENG("drm");
#endif
#ifdef BUILD_ECORE_EVAS_GL_DRM
                            ADDENG("opengl_drm");
#endif
                         }
                    }
               }
             eina_iterator_free(it);
          }
     }

   _engines_available = result;
   return result;
}
