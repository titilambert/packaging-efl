#include "ecore_xcb_private.h"
#ifdef ECORE_XCB_XINPUT
# include <xcb/xinput.h>
# include <xcb/xcb_event.h>
#endif

/* FIXME: this is a guess. can't find defines for touch events in xcb libs
 * online */
/* these are not yet defined in xcb support for xi2 - so manually create */
#ifndef XCB_INPUT_DEVICE_TOUCH_BEGIN
#define XCB_INPUT_DEVICE_TOUCH_BEGIN    18
#endif
#ifndef XCB_INPUT_DEVICE_TOUCH_END
#define XCB_INPUT_DEVICE_TOUCH_END      19
#endif
#ifndef XCB_INPUT_DEVICE_TOUCH_UPDATE
#define XCB_INPUT_DEVICE_TOUCH_UPDATE   21
#endif

#ifndef XCB_INPUT_POINTER_EMULATED_MASK
#define XCB_INPUT_POINTER_EMULATED_MASK (1 << 16)
#endif

/* local variables */
static Eina_Bool _input_avail = EINA_FALSE;

/* external variables */
int _ecore_xcb_event_input = 0;

void
_ecore_xcb_input_init(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

#ifdef ECORE_XCB_XINPUT
   xcb_prefetch_extension_data(_ecore_xcb_conn, &xcb_input_id);
#endif
}

void
_ecore_xcb_input_finalize(void)
{
#ifdef ECORE_XCB_XINPUT
   xcb_input_get_extension_version_cookie_t cookie;
   xcb_input_get_extension_version_reply_t *reply;
   char buff[128];
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

#ifdef ECORE_XCB_XINPUT
   cookie =
     xcb_input_get_extension_version_unchecked(_ecore_xcb_conn, 127, buff);
   reply =
     xcb_input_get_extension_version_reply(_ecore_xcb_conn, cookie, NULL);
   if (reply)
     {
        _input_avail = EINA_TRUE;
        free(reply);
     }

   if (_input_avail)
     {
        const xcb_query_extension_reply_t *ext_reply;

        ext_reply = xcb_get_extension_data(_ecore_xcb_conn, &xcb_input_id);
        if (ext_reply)
          _ecore_xcb_event_input = ext_reply->first_event;
     }
#endif
}

void
_ecore_xcb_input_shutdown(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
}

void
#ifdef ECORE_XCB_XINPUT
_ecore_xcb_input_handle_event(xcb_generic_event_t *event)
#else
_ecore_xcb_input_handle_event(xcb_generic_event_t * event EINA_UNUSED)
#endif
{
#ifdef ECORE_XCB_XINPUT
   xcb_ge_event_t *ev;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

   /* FIXME: look at xlib ecore_x_xi2.c to copy logic in when i can find an
    * xcb-input lib to test with */
#ifdef ECORE_XCB_XINPUT
   ev = (xcb_ge_event_t *)event;
   switch (ev->event_type)
     {
      case XCB_INPUT_DEVICE_MOTION_NOTIFY:
          {
             xcb_input_device_motion_notify_event_t *de;
             unsigned int child_win = 0;

             de = (xcb_input_device_motion_notify_event_t *)ev->pad1;
             child_win = (de->child ? de->child : de->event);
             _ecore_xcb_event_mouse_move(de->time, de->state, de->event_x,
                                         de->event_y, de->root_x, de->root_y,
                                         de->event, child_win, de->root,
                                         de->same_screen, de->device_id,
                                         1, 1, 1.0, 0.0,
                                         de->event_x, de->event_y,
                                         de->root_x, de->root_y);
          }
        break;

      case XCB_INPUT_DEVICE_BUTTON_PRESS:
          {
             xcb_input_device_button_press_event_t *de;
             unsigned int child_win = 0;

             de = (xcb_input_device_button_press_event_t *)ev->pad1;
             child_win = (de->child ? de->child : de->event);
             _ecore_xcb_event_mouse_button(ECORE_EVENT_MOUSE_BUTTON_DOWN,
                                           de->time, de->state, de->detail,
                                           de->event_x, de->event_y,
                                           de->root_x, de->root_y, de->event,
                                           child_win, de->root,
                                           de->same_screen, de->device_id,
                                           1, 1, 1.0, 0.0,
                                           de->event_x, de->event_y,
                                           de->root_x, de->root_y);
          }
        break;

      case XCB_INPUT_DEVICE_BUTTON_RELEASE:
          {
             xcb_input_device_button_release_event_t *de;
             unsigned int child_win = 0;

             de = (xcb_input_device_button_release_event_t *)ev->pad1;
             child_win = (de->child ? de->child : de->event);
             _ecore_xcb_event_mouse_button(ECORE_EVENT_MOUSE_BUTTON_UP,
                                           de->time, de->state, de->detail,
                                           de->event_x, de->event_y,
                                           de->root_x, de->root_y, de->event,
                                           child_win, de->root,
                                           de->same_screen, de->device_id,
                                           1, 1, 1.0, 0.0,
                                           de->event_x, de->event_y,
                                           de->root_x, de->root_y);
          }
        break;

      case XCB_INPUT_DEVICE_TOUCH_UPDATE:
          {
             xcb_input_device_motion_notify_event_t *de;
             unsigned int child_win = 0;

             de = (xcb_input_device_motion_notify_event_t *)ev->pad1;
             child_win = (de->child ? de->child : de->event);
             _ecore_xcb_event_mouse_move(de->time, de->state, de->event_x,
                                         de->event_y, de->root_x, de->root_y,
                                         de->event, child_win, de->root,
                                         de->same_screen, de->device_id,
                                         1, 1, 1.0, 0.0,
                                         de->event_x, de->event_y,
                                         de->root_x, de->root_y);
          }
        break;

      case XCB_INPUT_DEVICE_TOUCH_BEGIN:
          {
             xcb_input_device_button_press_event_t *de;
             unsigned int child_win = 0;

             de = (xcb_input_device_button_press_event_t *)ev->pad1;
             child_win = (de->child ? de->child : de->event);
             _ecore_xcb_event_mouse_button(ECORE_EVENT_MOUSE_BUTTON_DOWN,
                                           de->time, de->state, de->detail,
                                           de->event_x, de->event_y,
                                           de->root_x, de->root_y, de->event,
                                           child_win, de->root,
                                           de->same_screen, de->device_id,
                                           1, 1, 1.0, 0.0,
                                           de->event_x, de->event_y,
                                           de->root_x, de->root_y);
          }
        break;

      case XCB_INPUT_DEVICE_TOUCH_END:
          {
             xcb_input_device_button_release_event_t *de;
             unsigned int child_win = 0;

             de = (xcb_input_device_button_release_event_t *)ev->pad1;
             child_win = (de->child ? de->child : de->event);
             _ecore_xcb_event_mouse_button(ECORE_EVENT_MOUSE_BUTTON_UP,
                                           de->time, de->state, de->detail,
                                           de->event_x, de->event_y,
                                           de->root_x, de->root_y, de->event,
                                           child_win, de->root,
                                           de->same_screen, de->device_id,
                                           1, 1, 1.0, 0.0,
                                           de->event_x, de->event_y,
                                           de->root_x, de->root_y);
          }
        break;

      default:
        break;
     }
#endif
}

EAPI Eina_Bool
ecore_x_input_multi_select(Ecore_X_Window win)
{
   Eina_Bool find = EINA_FALSE;
#ifdef ECORE_XCB_XINPUT
   xcb_input_list_input_devices_cookie_t dcookie;
   xcb_input_list_input_devices_reply_t *dreply;
   xcb_input_device_info_iterator_t diter;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

   if (!_input_avail) return EINA_FALSE;

   /* FIXME: i can't seemingly test this! no xcb input lib so can't look and
    * test and look at types etc. - look at xlib code and copy logic over
    * when we can */
#ifdef ECORE_XCB_XINPUT
   dcookie = xcb_input_list_input_devices_unchecked(_ecore_xcb_conn);
   dreply =
     xcb_input_list_input_devices_reply(_ecore_xcb_conn, dcookie, NULL);
   if (!dreply) return EINA_FALSE;

   diter = xcb_input_list_input_devices_devices_iterator(dreply);
   while (diter.rem)
     {
        xcb_input_device_info_t *dev;
        const xcb_input_event_class_t iclass[] =
          {
             XCB_INPUT_DEVICE_BUTTON_PRESS,
             XCB_INPUT_DEVICE_BUTTON_RELEASE,
             XCB_INPUT_DEVICE_MOTION_NOTIFY,
             XCB_INPUT_DEVICE_TOUCH_BEGIN,
             XCB_INPUT_DEVICE_TOUCH_END,
             XCB_INPUT_DEVICE_TOUCH_UPDATE
          };

        dev = diter.data;
        if (dev->device_use == XCB_INPUT_DEVICE_USE_IS_X_EXTENSION_DEVICE)
          {
             DBG("Device %d", dev->device_id);
             DBG("\tType: %d", dev->device_type);
             DBG("\tNum Classes: %d", dev->num_class_info);
             DBG("\tUse: %d", dev->device_use);

             xcb_input_select_extension_event(_ecore_xcb_conn, win,
                                              sizeof(iclass) / sizeof(xcb_input_event_class_t),
                                              iclass);
             find = EINA_TRUE;
          }
        xcb_input_device_info_next(&diter);
     }
   free(dreply);
#else
   (void)win;
#endif

   return find;
}

EAPI Eina_Bool 
ecore_x_input_raw_select(Ecore_X_Window win EINA_UNUSED)
{
   /* NB: FIXME: This is just a placeholder. XCB does not have XInput2 yet */
   return EINA_FALSE;
}
