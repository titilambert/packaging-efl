#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include "ecore_private.h"
#include <Ecore_Input.h>
#include <Ecore_Input_Evas.h>

#include <Ecore_Cocoa.h>
#include <Evas_Engine_GL_Cocoa.h>

#include "Ecore_Evas.h"
#include "ecore_evas_private.h"


// FIXME: this engine has lots of problems. only 1 window at a time, drawRect looks wrong, doesnt handle resizes and more

static int                      _ecore_evas_init_count = 0;
static Eina_List                *ecore_evases = NULL;
static Ecore_Event_Handler      *ecore_evas_event_handlers[4] = {
  NULL, NULL, NULL, NULL
};
static Ecore_Idle_Enterer       *ecore_evas_idle_enterer = NULL;

//static const char               *ecore_evas_cocoa_default = "EFL Cocoa";



static int
_ecore_evas_cocoa_render(Ecore_Evas *ee)
{
   int rend = 0;
   Eina_List *updates = NULL;
   Eina_List *ll;
   Ecore_Evas *ee2;

   DBG("Render");

   EINA_LIST_FOREACH(ee->sub_ecore_evas, ll, ee2)
     {
        if (ee2->func.fn_pre_render) ee2->func.fn_pre_render(ee2);
        if (ee2->engine.func->fn_render)
          rend |= ee2->engine.func->fn_render(ee2);
        if (ee2->func.fn_post_render) ee2->func.fn_post_render(ee2);
     }

   if (ee->func.fn_pre_render) ee->func.fn_pre_render(ee);
   updates = evas_render_updates(ee->evas);
   if (ee->prop.avoid_damage)
     {
        updates = evas_render_updates(ee->evas);
        if (updates) evas_render_updates_free(updates);
     }
   else if ((ee->visible) ||
            ((ee->should_be_visible) && (ee->prop.fullscreen)) ||
            ((ee->should_be_visible) && (ee->prop.override)))
     {
        if (ee->shaped)
          {
             updates = evas_render_updates(ee->evas);
             if (updates) evas_render_updates_free(updates);
          }
        else
          {
             updates = evas_render_updates(ee->evas);
             if (updates) evas_render_updates_free(updates);
          }
     }
   else
     evas_norender(ee->evas);
   if (updates) rend = 1;
   if (ee->func.fn_post_render) ee->func.fn_post_render(ee);

   if (rend)
     {
        static int frames = 0;
        static double t0 = 0.0;
        double t, td;

        t = ecore_time_get();
        frames++;
        if ((t - t0) > 1.0)
          {
             td = t - t0;
             printf("FPS: %3.3f\n", (double)frames / td);
             frames = 0;
             t0 = t;
          }
     }

   return rend;
}


static Ecore_Evas *
_ecore_evas_cocoa_match(void)
{
  DBG("Match");
  return eina_list_nth(ecore_evases, 0);
}

static Eina_Bool
_ecore_evas_cocoa_event_got_focus(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
  Ecore_Evas                   *ee;

  DBG("Got Focus");

  ee = _ecore_evas_cocoa_match();

  if (!ee) return ECORE_CALLBACK_PASS_ON;
  ee->prop.focused = EINA_TRUE;
  evas_focus_in(ee->evas);
  if (ee->func.fn_focus_in) ee->func.fn_focus_in(ee);

  return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_evas_cocoa_event_lost_focus(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   // TODO
  Ecore_Evas                   *ee;

  DBG("Lost Focus");

  ee = _ecore_evas_cocoa_match();

  if (!ee) return ECORE_CALLBACK_PASS_ON;
  evas_focus_out(ee->evas);
  ee->prop.focused = EINA_FALSE;
  if (ee->func.fn_focus_out) ee->func.fn_focus_out(ee);

  return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_evas_cocoa_event_video_resize(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Cocoa_Event_Video_Resize *e = event;
   Ecore_Evas                   *ee;

   ee = _ecore_evas_cocoa_match();
   if (!ee)
     {
        printf("Ecore_Evas %p was not registered\n", ee);
        return ECORE_CALLBACK_PASS_ON;
     }

   ee->req.w = ee->w = e->w;
   ee->req.h = ee->h = e->h;

   evas_output_size_set(ee->evas, e->w, e->h);
   evas_output_viewport_set(ee->evas, 0, 0, e->w, e->h);
   evas_damage_rectangle_add(ee->evas, 0, 0, e->w, e->h);

  if (ee->func.fn_resize) ee->func.fn_resize(ee);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ecore_evas_cocoa_event_video_expose(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
  Ecore_Evas                   *ee;
  int                          w;
  int                          h;

  DBG("Video Expose");

  ee = _ecore_evas_cocoa_match();

  if (!ee) return ECORE_CALLBACK_PASS_ON;
  evas_output_size_get(ee->evas, &w, &h);
  evas_damage_rectangle_add(ee->evas, 0, 0, w, h);

  return ECORE_CALLBACK_PASS_ON;
}

//static int
//_ecore_evas_idle_enter(void *data EINA_UNUSED)
//{
//  Ecore_Evas  *ee;
//  Eina_List *l;
//  
//  DBG("Idle enter");
//
//  EINA_LIST_FOREACH(ecore_evases, l, ee)
//    {
//      if (ee->visible)
//	evas_render(ee->evas);
//      else
//	evas_norender(ee->evas);
//    }
//
//  return EINA_TRUE;
//}


static int
_ecore_evas_cocoa_init(void)
{
  DBG("Cocoa Init");
  _ecore_evas_init_count++;
  if (_ecore_evas_init_count > 1)
    return _ecore_evas_init_count;

  ecore_event_evas_init();

  ecore_evas_event_handlers[0] = ecore_event_handler_add(ECORE_COCOA_EVENT_GOT_FOCUS, _ecore_evas_cocoa_event_got_focus, NULL);
  ecore_evas_event_handlers[1] = ecore_event_handler_add(ECORE_COCOA_EVENT_LOST_FOCUS, _ecore_evas_cocoa_event_lost_focus, NULL);
  ecore_evas_event_handlers[2] = ecore_event_handler_add(ECORE_COCOA_EVENT_RESIZE, _ecore_evas_cocoa_event_video_resize, NULL);
  ecore_evas_event_handlers[3] = ecore_event_handler_add(ECORE_COCOA_EVENT_EXPOSE, _ecore_evas_cocoa_event_video_expose, NULL);

  return _ecore_evas_init_count;
}

static int
_ecore_evas_cocoa_shutdown(void)
{
    Ecore_Evas *ee;
  DBG("Cocoa SHutodwn");
  _ecore_evas_init_count--;
  if (_ecore_evas_init_count == 0)
    {
      unsigned int i;

      EINA_LIST_FREE(ecore_evases, ee)
         _ecore_evas_free(ee);

      for (i = 0; i < sizeof (ecore_evas_event_handlers) / sizeof (Ecore_Event_Handler*); i++)
	ecore_event_handler_del(ecore_evas_event_handlers[i]);
      ecore_event_evas_shutdown();
      ecore_idle_enterer_del(ecore_evas_idle_enterer);
      ecore_evas_idle_enterer = NULL;

      ecore_event_evas_shutdown();
    }
  if (_ecore_evas_init_count < 0) _ecore_evas_init_count = 0;
  return _ecore_evas_init_count;
}

static void
_ecore_evas_cocoa_free(Ecore_Evas *ee)
{
  DBG("Cocoa Free");
  ecore_evases = eina_list_remove(ecore_evases, ee);
  ecore_event_window_unregister(0);
  _ecore_evas_cocoa_shutdown();
  ecore_cocoa_shutdown();
}

static void
_ecore_evas_resize(Ecore_Evas *ee, int w, int h)
{
  DBG("Resize");
  if ((w == ee->w) && (h == ee->h)) return;
  ee->req.w = w;
  ee->req.h = h;
  ee->w = w;
  ee->h = h;

  printf("Ecore_Evas Resize %d %d\n", w, h);

  ecore_cocoa_window_resize((Ecore_Cocoa_Window *)ee->prop.window, w, h);

  evas_output_size_set(ee->evas, ee->w, ee->h);
  evas_output_viewport_set(ee->evas, 0, 0, ee->w, ee->h);
  evas_damage_rectangle_add(ee->evas, 0, 0, ee->w, ee->h);

  if (ee->func.fn_resize) ee->func.fn_resize(ee);
}

static void
_ecore_evas_move_resize(Ecore_Evas *ee, int x, int y, int w, int h)
{
  DBG("Move Resize");
  if ((ee->w == w) && (ee->h == h) && (x == ee->x) && (y == ee->y))
    return;

  ee->req.x = x;
  ee->req.y = y;
  ee->req.w = w;
  ee->req.h = h;
  ee->w = w;
  ee->h = h;
  ee->x = x;
  ee->y = y;

  ecore_cocoa_window_move_resize((Ecore_Cocoa_Window *)ee->prop.window, x, y, w, h);

  evas_output_size_set(ee->evas, ee->w, ee->h);
  evas_output_viewport_set(ee->evas, 0, 0, ee->w, ee->h);
  evas_damage_rectangle_add(ee->evas, 0, 0, ee->w, ee->h);

  if (ee->func.fn_resize) ee->func.fn_resize(ee);
}


static void
_ecore_evas_show(Ecore_Evas *ee)
{
  DBG("Show");
   ee->should_be_visible = 1;
   if (ee->prop.avoid_damage)
     _ecore_evas_cocoa_render(ee);

   ecore_cocoa_window_show((Ecore_Cocoa_Window *)ee->prop.window);
   evas_damage_rectangle_add(ee->evas, 0, 0, ee->w, ee->h);
}


static void
_ecore_evas_hide(Ecore_Evas *ee)
{
  DBG("Hide");

   ecore_cocoa_window_hide((Ecore_Cocoa_Window *)ee->prop.window);
   ee->should_be_visible = 0;
}

static void
_ecore_evas_title_set(Ecore_Evas *ee, const char *title)
{
   INF("ecore evas title set");

   if (eina_streq(ee->prop.title, title)) return;
   if (ee->prop.title) free(ee->prop.title);
   ee->prop.title = NULL;
   if (title) ee->prop.title = strdup(title);
   ecore_cocoa_window_title_set((Ecore_Cocoa_Window *)ee->prop.window,
                                ee->prop.title);
}

static void
_ecore_evas_object_cursor_del(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
  Ecore_Evas *ee;

  DBG("Cursor DEL");

  ee = data;
  if (ee)
    ee->prop.cursor.object = NULL;
}

static void
_ecore_evas_object_cursor_unset(Ecore_Evas *ee)
{
   evas_object_event_callback_del_full(ee->prop.cursor.object, EVAS_CALLBACK_DEL, _ecore_evas_object_cursor_del, ee);
}

static void
_ecore_evas_object_cursor_set(Ecore_Evas *ee, Evas_Object *obj, int layer, int hot_x, int hot_y)
{
  int x, y;
  DBG("Cursor Set");
  if (ee->prop.cursor.object) evas_object_del(ee->prop.cursor.object);

  if (obj == NULL)
    {
      ee->prop.cursor.object = NULL;
      ee->prop.cursor.layer = 0;
      ee->prop.cursor.hot.x = 0;
      ee->prop.cursor.hot.y = 0;
      return;
    }

  ee->prop.cursor.object = obj;
  ee->prop.cursor.layer = layer;
  ee->prop.cursor.hot.x = hot_x;
  ee->prop.cursor.hot.y = hot_y;

  evas_pointer_output_xy_get(ee->evas, &x, &y);
  evas_object_layer_set(ee->prop.cursor.object, ee->prop.cursor.layer);
  evas_object_move(ee->prop.cursor.object,
		   x - ee->prop.cursor.hot.x,
		   y - ee->prop.cursor.hot.y);

  evas_object_pass_events_set(ee->prop.cursor.object, 1);

  if (evas_pointer_inside_get(ee->evas))
    evas_object_show(ee->prop.cursor.object);

  evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL, _ecore_evas_object_cursor_del, ee);
}

static int
_ecore_evas_engine_cocoa_init(Ecore_Evas *ee)
{
   Evas_Engine_Info_GL_Cocoa *einfo;
   const char                *driver;
   int                        rmethod;

   DBG("Cocoa Init");

   driver = "gl_cocoa";

   rmethod = evas_render_method_lookup(driver);
   if (!rmethod)
     return 0;

   ee->driver = driver;
   evas_output_method_set(ee->evas, rmethod);

   einfo = (Evas_Engine_Info_GL_Cocoa *)evas_engine_info_get(ee->evas);
   if (einfo)
     {
        /* FIXME: REDRAW_DEBUG missing for now */
        einfo->window = (void *)ee->prop.window;
        //einfo->info.depth = ecore_win32_screen_depth_get();
        //einfo->info.rotation = 0;
        if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
          {
             ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
             return 0;
          }
	ecore_cocoa_window_view_set(einfo->window, einfo->view);
     }
   else
     {
        ERR("evas_engine_info_set() init engine '%s' failed.", ee->driver);
        return 0;
     }

   return 1;
}

static void
_ecore_evas_screen_geometry_get(const Ecore_Evas *ee EINA_UNUSED, int *x, int *y, int *w, int *h)
{
   if (x) *x = 0;
   if (y) *y = 0;
   ecore_cocoa_screen_size_get(NULL, w, h);
   printf("screen geometry_get  %dx%d\n", *w, *h);
}

static Ecore_Evas_Engine_Func _ecore_cocoa_engine_func =
  {
    _ecore_evas_cocoa_free,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, //move
    NULL,
    _ecore_evas_resize,
    _ecore_evas_move_resize,
    NULL, //rotation
    NULL, //shaped
    _ecore_evas_show,
    _ecore_evas_hide,
    NULL, //raise
    NULL, //lower
    NULL, //activate
    _ecore_evas_title_set,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    _ecore_evas_object_cursor_set,
    _ecore_evas_object_cursor_unset,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, //transparent
    NULL, // profiles_set
    NULL, // profile_set

     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,

     NULL, // render
     _ecore_evas_screen_geometry_get,
     NULL, // screen_dpi_get
     NULL,
     NULL  // msg_send
  };

EAPI Ecore_Evas *
ecore_evas_cocoa_new_internal(Ecore_Cocoa_Window *parent EINA_UNUSED, int x, int y, int w, int h)
{
//  Evas_Engine_Info_GL_Cocoa *einfo;
  Ecore_Evas           *ee;

  DBG("Cocoa new");

  if (!ecore_cocoa_init())
    return NULL;

  ee = calloc(1, sizeof(Ecore_Evas));
  if (!ee)
    goto shutdown_ecore_cocoa;

  ECORE_MAGIC_SET(ee, ECORE_MAGIC_EVAS);

  _ecore_evas_cocoa_init();

  ee->engine.func = (Ecore_Evas_Engine_Func *)&_ecore_cocoa_engine_func;

  if (w < 1) w = 1;
  if (h < 1) h = 1;
  ee->visible = 1;
  ee->x = x;
  ee->y = y;
  ee->w = w;
  ee->h = h;
  ee->req.x = ee->x;
  ee->req.y = ee->y;
  ee->req.w = ee->w - ee->y;
  ee->req.h = ee->h;

  ee->semi_sync = 1;


  ee->prop.max.w = 32767;
  ee->prop.max.h = 32767;
  ee->prop.layer = 4;
  ee->prop.request_pos = EINA_FALSE;
  ee->prop.sticky = EINA_FALSE;
  ee->prop.window = 0;
  ee->prop.withdrawn = EINA_TRUE;

  printf("Create New Evas\n");

  ee->evas = evas_new();

  if (!ee->evas)
    goto free_name;

  evas_data_attach_set(ee->evas, ee);
  evas_output_size_set(ee->evas, w, h);
  evas_output_viewport_set(ee->evas, 0, 0, w, h);

  printf("Create New Cocoa Window\n");
  ee->prop.window = (Ecore_Window)ecore_cocoa_window_new(x, y, w, h);
  printf("Window Created %p\n", (Ecore_Cocoa_Window *)ee->prop.window);
  if (!ee->prop.window)
    {
      _ecore_evas_cocoa_shutdown();
      free(ee);
      return NULL;
    }

  printf("Init Evas engine cocoa\n");
  if (!_ecore_evas_engine_cocoa_init(ee))
    {
      _ecore_evas_cocoa_shutdown();
      free(ee);
      return NULL;
    }


  ee->engine.func->fn_render = _ecore_evas_cocoa_render;
  _ecore_evas_register(ee);
  ecore_event_window_register(ee->prop.window, ee, ee->evas,
                              (Ecore_Event_Mouse_Move_Cb)_ecore_evas_mouse_move_process,
                              (Ecore_Event_Multi_Move_Cb)_ecore_evas_mouse_multi_move_process,
                              (Ecore_Event_Multi_Down_Cb)_ecore_evas_mouse_multi_down_process,
                              (Ecore_Event_Multi_Up_Cb)_ecore_evas_mouse_multi_up_process);

  evas_event_feed_mouse_in(ee->evas, (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) & 0xffffffff), NULL);
  printf("Ecore Evas returned : %p\n", ee);

  ecore_evases = eina_list_append(ecore_evases, ee);

  return ee;

 //free_window:
  /* FIXME: free window here */
 //free_evas:
 // free(ee->evas);
free_name:
  free(ee->name);
 //free_ee:
  _ecore_evas_cocoa_shutdown();
  free(ee);
 shutdown_ecore_cocoa:
  ecore_cocoa_shutdown();

  return NULL;
}
