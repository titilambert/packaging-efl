#include "evas_common_private.h" /* Also includes international specific stuff */
#include "evas_private.h"
#include "evas_engine.h"

#include <dlfcn.h>

#include <SDL2/SDL_opengl.h>

Evas_GL_Common_Context_New glsym_evas_gl_common_context_new = NULL;
Evas_GL_Common_Context_Call glsym_evas_gl_common_context_free = NULL;
Evas_GL_Common_Context_Call glsym_evas_gl_common_context_use = NULL;
Evas_GL_Common_Context_Call glsym_evas_gl_common_context_flush = NULL;
Evas_GL_Common_Context_Call glsym_evas_gl_common_image_all_unload = NULL;
Evas_GL_Common_Context_Resize_Call glsym_evas_gl_common_context_resize = NULL;
Evas_GL_Preload_Render_Call glsym_evas_gl_preload_render_lock = NULL;
Evas_Gl_Symbols glsym_evas_gl_symbols = NULL;

static Outbuf *_sdl_output_setup(int w, int h, int fullscreen, int noframe, Evas_Engine_Info_GL_SDL *info);

int _evas_engine_GL_SDL_log_dom = -1;
/* function tables - filled in later (func and parent func) */
static Evas_Func func, pfunc;

static void
_outbuf_reconfigure(Outbuf *ob EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED, int rot EINA_UNUSED, Outbuf_Depth depth EINA_UNUSED)
{
}

static Eina_Bool
_outbuf_region_first_rect(Outbuf *ob EINA_UNUSED)
{
   return EINA_FALSE;
}

static void *
_outbuf_new_region_for_update(Outbuf *ob,
                              int x EINA_UNUSED, int y EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED,
                              int *cx EINA_UNUSED, int *cy EINA_UNUSED, int *cw EINA_UNUSED, int *ch EINA_UNUSED)
{
   return ob->gl_context->def_surface;
}

static void
_outbuf_push_updated_region(Outbuf *ob EINA_UNUSED,
                            RGBA_Image *update EINA_UNUSED,
                            int x EINA_UNUSED, int y EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED)
{
}

static void
_outbuf_free_region_for_update(Outbuf *ob EINA_UNUSED, RGBA_Image *update EINA_UNUSED)
{
}

static void
_outbuf_free(Outbuf *ob)
{
   glsym_evas_gl_common_context_free(ob->gl_context);
}

static int
_outbuf_get_rot(Outbuf *ob EINA_UNUSED)
{
   return 0;
}

static void
_outbuf_flush(Outbuf *ob, Tilebuf_Rect *rects EINA_UNUSED, Evas_Render_Mode render_mode EINA_UNUSED)
{
   SDL_GL_SwapWindow(ob->window);
}

static Eina_Bool
eng_window_make_current(void *data, void *doit EINA_UNUSED)
{
   Outbuf *ob = data;

   SDL_GL_MakeCurrent(ob->window, ob->context);
   return EINA_TRUE;
}

static void
_window_use(Outbuf *ob)
{
   /* With SDL 1.x, only one window, no issue here so only flush evas context */
   glsym_evas_gl_preload_render_lock(eng_window_make_current, ob);

   if (ob)
     {
        glsym_evas_gl_common_context_use(ob->gl_context);
        glsym_evas_gl_common_context_flush(ob->gl_context);
     }
}

static Evas_Engine_GL_Context *
_window_gl_context_get(Outbuf *ob)
{
   return ob->gl_context;
}

static void *
_window_egl_display_get(Outbuf *ob)
{
#ifdef GL_GLES
   return ob->egl_disp;
#else
   (void) ob;
   return NULL;
#endif
}

struct _Context_3D
{
   Outbuf *ob;
   SDL_GLContext sdl_context;
};

static Context_3D *
_window_gl_context_new(Outbuf *ob)
{
   Context_3D *ctx;

   ctx = calloc(1, sizeof (Context_3D));
   if (!ctx) return NULL;

   ctx->ob = ob;
   ctx->sdl_context = SDL_GL_CreateContext(ob->window);

   return ctx;
}

static void
_window_gl_context_use(Context_3D *ctx)
{
   SDL_GL_MakeCurrent(ctx->ob->window, ctx->sdl_context);
}

/* FIXME: noway to destroy Context_3D */

static void *
evgl_eng_display_get(void *data)
{
   Render_Engine *re = data;

   if (!re->generic.software.ob) return NULL;
#ifdef GL_GLES
   return re->generic.software.ob->egl_disp;
#else
   return NULL; /* FIXME: what should we do here ? */
#endif
}

static void *
evgl_eng_evas_surface_get(void *data)
{
   Render_Engine *re = data;

   return re->generic.software.ob->window;
}

static int
evgl_eng_make_current(void *data EINA_UNUSED,
                      void *surface, void *context,
                      int flush)
{
   if (flush) _window_use(NULL);
   SDL_GL_MakeCurrent(surface, context);
   return EINA_TRUE;
}

static void *
evgl_eng_native_window_create(void *data EINA_UNUSED)
{
   /* FIXME: Need to understand how to implement that with SDL */
   return NULL;
   /* return SDL_CreateWindow(NULL, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, */
   /*                         2, 2, SDL_WINDOW_OPENGL); */
}

static int
evgl_eng_native_window_destroy(void *data EINA_UNUSED, void *native_window)
{
   /* SDL_DestroyWindow(native_window); */
   return 1;
}

static void *
evgl_eng_window_surface_create(void *data EINA_UNUSED, void *native_window)
{
   return native_window;
}

static int
evgl_eng_window_surface_destroy(void *data EINA_UNUSED,
                                void *surface EINA_UNUSED)
{
   return 1;
}

static void *
evgl_eng_context_create(void *data, void *share_ctx EINA_UNUSED, int version)
{
   Render_Engine *re = data;

   if (version != EVAS_GL_GLES_2_X)
     {
        ERR("This engine only supports OpenGL-ES 2.0 contexts for now!");
        return NULL;
     }

   SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
   return SDL_GL_CreateContext(re->generic.software.ob->window);
}

static int
evgl_eng_context_destroy(void *data EINA_UNUSED, void *context)
{
   SDL_GL_DeleteContext(context);
   return 1;
}

static const char *
evgl_eng_string_get(void *data EINA_UNUSED)
{
   const char *(*glGetString)(GLenum n);

   glGetString = SDL_GL_GetProcAddress("glGetString");
   if (glGetString) return glGetString(GL_EXTENSIONS);
   return NULL;
}

static void *
evgl_eng_proc_address_get(const char *name)
{
   return SDL_GL_GetProcAddress(name);
}

static int
evgl_eng_rotation_angle_get(void *data EINA_UNUSED)
{
   return 0;
}

static const EVGL_Interface evgl_funcs =
{
   evgl_eng_display_get,
   evgl_eng_evas_surface_get,
   evgl_eng_native_window_create,
   evgl_eng_native_window_destroy,
   evgl_eng_window_surface_create,
   evgl_eng_window_surface_destroy,
   evgl_eng_context_create,
   evgl_eng_context_destroy,
   evgl_eng_make_current,
   evgl_eng_proc_address_get,
   evgl_eng_string_get,
   evgl_eng_rotation_angle_get,
   NULL, // PBuffer
   NULL, // PBuffer
   NULL, // OpenGL-ES 1
   NULL, // OpenGL-ES 1
};


static void *
eng_info(Evas *e EINA_UNUSED)
{
   Evas_Engine_Info_GL_SDL *info;

   info = calloc(1, sizeof(Evas_Engine_Info_GL_SDL));
   if (!info) return NULL;
   info->magic.magic = rand();
   return info;
}

static void
eng_info_free(Evas *e EINA_UNUSED, void *info)
{
   Evas_Engine_Info_GL_SDL *in;
   in = (Evas_Engine_Info_GL_SDL *)info;
   free(in);
}

static int
eng_setup(Evas *eo_e, void *in)
{
   Evas_Public_Data *e = eo_data_scope_get(eo_e, EVAS_CANVAS_CLASS);
   Render_Engine *re = NULL;
   Outbuf *ob = NULL;
   Evas_Engine_Info_GL_SDL *info;

   info = (Evas_Engine_Info_GL_SDL *)in;

   ob = _sdl_output_setup(e->output.w, e->output.h,
                          info->flags.fullscreen,
                          info->flags.noframe,
                          info);
   if (!ob) goto on_error;

   re = calloc(1, sizeof (Render_Engine));
   if (!re) goto on_error;


   if (!evas_render_engine_gl_generic_init(&re->generic, ob, NULL,
                                           _outbuf_get_rot,
                                           _outbuf_reconfigure,
                                           _outbuf_region_first_rect,
                                           _outbuf_new_region_for_update,
                                           _outbuf_push_updated_region,
                                           _outbuf_free_region_for_update,
                                           NULL,
                                           _outbuf_flush,
                                           _outbuf_free,
                                           _window_use,
                                           _window_gl_context_get,
                                           _window_egl_display_get,
                                           _window_gl_context_new,
                                           _window_gl_context_use,
                                           &evgl_funcs,
                                           e->output.w, e->output.h))
     goto on_error;

   e->engine.data.output = re;
   if (!e->engine.data.output)
     return 0;
   e->engine.func = &func;
   e->engine.data.context = e->engine.func->context_new(e->engine.data.output);

   /* if we haven't initialized - init (automatic abort if already done) */
   evas_common_cpu_init();
   evas_common_blend_init();
   evas_common_image_init();
   evas_common_convert_init();
   evas_common_scale_init();
   evas_common_rectangle_init();
   evas_common_polygon_init();
   evas_common_line_init();
   evas_common_font_init();
   evas_common_draw_init();
   evas_common_tilebuf_init();

   return 1;

 on_error:
   if (ob) _outbuf_free(ob);
   free(ob);
   free(re);
   return 0;
}

static void
eng_output_free(void *data)
{
   Render_Engine *re = data;

   evas_render_engine_software_generic_clean(&re->generic.software);
}

static void
eng_output_dump(void *data)
{
   Render_Engine *re;

   re = (Render_Engine *)data;
   evas_common_image_image_all_unload();
   evas_common_font_font_all_unload();
   glsym_evas_gl_common_image_all_unload(re->generic.software.ob->gl_context);
}

static Eina_Bool
eng_canvas_alpha_get(void *data EINA_UNUSED, void *info EINA_UNUSED)
{
   return 0;
}

static void
gl_symbols(void)
{
#define LINK2GENERIC(sym)                       \
   glsym_##sym = dlsym(RTLD_DEFAULT, #sym);

   LINK2GENERIC(evas_gl_symbols);
   LINK2GENERIC(evas_gl_common_context_new);
   LINK2GENERIC(evas_gl_common_context_free);
   LINK2GENERIC(evas_gl_common_context_use);
   LINK2GENERIC(evas_gl_common_context_flush);
   LINK2GENERIC(evas_gl_common_image_all_unload);
   LINK2GENERIC(evas_gl_common_context_resize);
   LINK2GENERIC(evas_gl_preload_render_lock);

   glsym_evas_gl_symbols((void*)SDL_GL_GetProcAddress);
}

static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   /* get whatever engine module we inherit from */
   if (!_evas_module_engine_inherit(&pfunc, "gl_generic")) return 0;
   if (_evas_engine_GL_SDL_log_dom < 0)
     _evas_engine_GL_SDL_log_dom = eina_log_domain_register
       ("evas-gl_sdl", EVAS_DEFAULT_LOG_COLOR);
   if (_evas_engine_GL_SDL_log_dom < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
     }
   /* store it for later use */
   func = pfunc;
   /* now to override methods */
   #define ORD(f) EVAS_API_OVERRIDE(f, &func, eng_)
   ORD(info);
   ORD(info_free);
   ORD(setup);
   ORD(canvas_alpha_get);
   ORD(output_free);
   ORD(output_dump);

   gl_symbols();

   /* now advertise out own api */
   em->functions = (void *)(&func);
   return 1;
}

static void
module_close(Evas_Module *em EINA_UNUSED)
{
    eina_log_domain_unregister(_evas_engine_GL_SDL_log_dom);
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "gl_sdl",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_ENGINE, engine, gl_sdl);

#ifndef EVAS_STATIC_BUILD_GL_SDL
EVAS_EINA_MODULE_DEFINE(engine, gl_sdl);
#endif

static Outbuf *
_sdl_output_setup(int w, int h, int fullscreen EINA_UNUSED, int noframe EINA_UNUSED, Evas_Engine_Info_GL_SDL *info)
{
   Outbuf *ob = NULL;
   const char *(*glGetString)(GLenum n);

   if (!info->window) return NULL;
   if (w <= 0) w = 640;
   if (h <= 0) h = 480;

   /* GL Initialization */
#ifdef HAVE_SDL_GL_CONTEXT_VERSION
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
   SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

   ob = calloc(1, sizeof(Outbuf));
   if (!ob) return NULL;

   ob->window = info->window;
   ob->w = w;
   ob->h = h;
   ob->info = info;
   ob->context = SDL_GL_CreateContext(ob->window);
   if (!ob->context)
     {
        ERR("Impossible to create a context for : %p", info->window);
        goto on_error;
     }

   glGetString = SDL_GL_GetProcAddress("glGetString");

   INF("Vendor: '%s', Renderer: '%s', Version: '%s'",
     glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION));

   ob->gl_context = glsym_evas_gl_common_context_new();
   if (!ob->gl_context) goto on_error;

   glsym_evas_gl_common_context_use(ob->gl_context);
   glsym_evas_gl_common_context_resize(ob->gl_context, w, h, ob->gl_context->rot);

   /* End GL Initialization */
   return ob;

 on_error:
   if (ob && ob->window) SDL_DestroyWindow(ob->window);
   free(ob);
   return NULL;
}
