#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Evas.h>
#include <Ecore.h>

#define HTTP_STREAM 0
#define RTSP_STREAM 1
#include <glib.h>
#include <gst/gst.h>
#include <glib-object.h>
#include <gst/video/gstvideosink.h>
#include <gst/video/video.h>

// forcibly disable x overlay window.. broken badly.
#undef HAVE_ECORE_X

#ifdef HAVE_ECORE_X
# include <Ecore_X.h>
# include <Ecore_Evas.h>
# ifdef HAVE_XOVERLAY_H
#  include <gst/interfaces/xoverlay.h>
# endif
#endif

#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
# include <unistd.h>
# include <sys/types.h>
#endif

#include "emotion_modules.h"
#include "emotion_gstreamer.h"

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE("sink",
                                                                   GST_PAD_SINK, GST_PAD_ALWAYS,
                                                                   GST_STATIC_CAPS(GST_VIDEO_CAPS_YUV("{ I420, YV12, YUY2, NV12, ST12, TM12 }") ";"
                                                                                   GST_VIDEO_CAPS_BGRx ";" GST_VIDEO_CAPS_BGR ";" GST_VIDEO_CAPS_BGRA));

GST_DEBUG_CATEGORY_STATIC(evas_video_sink_debug);
#define GST_CAT_DEFAULT evas_video_sink_debug

enum {
  REPAINT_REQUESTED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_EVAS_OBJECT,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_EV,
  PROP_LAST
};

static guint evas_video_sink_signals[LAST_SIGNAL] = { 0, };

#define _do_init(bla)                                   \
  GST_DEBUG_CATEGORY_INIT(evas_video_sink_debug,        \
                          "emotion-sink",		\
                          0,                            \
                          "emotion video sink")

GST_BOILERPLATE_FULL(EvasVideoSink,
                     evas_video_sink,
                     GstVideoSink,
                     GST_TYPE_VIDEO_SINK,
                     _do_init);


static void unlock_buffer_mutex(EvasVideoSinkPrivate* priv);
static void evas_video_sink_main_render(void *data);
static void evas_video_sink_samsung_main_render(void *data);

static void
evas_video_sink_base_init(gpointer g_class)
{
   GstElementClass* element_class;

   element_class = GST_ELEMENT_CLASS(g_class);
   gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&sinktemplate));
   gst_element_class_set_details_simple(element_class, "Evas video sink",
                                        "Sink/Video", "Sends video data from a GStreamer pipeline to an Evas object",
                                        "Vincent Torri <vtorri@univ-evry.fr>");
}

static void
evas_video_sink_init(EvasVideoSink* sink, EvasVideoSinkClass* klass EINA_UNUSED)
{
   EvasVideoSinkPrivate* priv;

   INF("sink init");
   sink->priv = priv = G_TYPE_INSTANCE_GET_PRIVATE(sink, EVAS_TYPE_VIDEO_SINK, EvasVideoSinkPrivate);
   priv->o = NULL;
   priv->width = 0;
   priv->height = 0;
   priv->func = NULL;
   priv->eformat = EVAS_COLORSPACE_ARGB8888;
   priv->samsung = EINA_FALSE;
   eina_lock_new(&priv->m);
   eina_condition_new(&priv->c, &priv->m);
   priv->unlocked = EINA_FALSE;
}

/**** Object methods ****/
static void
_cleanup_priv(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   EvasVideoSinkPrivate* priv;

   priv = data;

   eina_lock_take(&priv->m);
   if (priv->o == obj)
     priv->o = NULL;
   eina_lock_release(&priv->m);
}

static void
evas_video_sink_set_property(GObject * object, guint prop_id,
                             const GValue * value, GParamSpec * pspec)
{
   EvasVideoSink* sink;
   EvasVideoSinkPrivate* priv;

   sink = EVAS_VIDEO_SINK (object);
   priv = sink->priv;

   switch (prop_id) {
    case PROP_EVAS_OBJECT:
       eina_lock_take(&priv->m);
       if (priv->o)
         evas_object_event_callback_del(priv->o, EVAS_CALLBACK_DEL, _cleanup_priv);
       priv->o = g_value_get_pointer (value);
       INF("sink set Evas_Object %p.", priv->o);
       if (priv->o)
         evas_object_event_callback_add(priv->o, EVAS_CALLBACK_DEL, _cleanup_priv, priv);
       eina_lock_release(&priv->m);
       break;
    case PROP_EV:
       INF("sink set ev.");
       eina_lock_take(&priv->m);
       priv->ev = g_value_get_pointer (value);
       if (priv->ev)
         priv->ev->samsung = EINA_TRUE;
       eina_lock_release(&priv->m);
       break;
    default:
       G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
       ERR("invalid property");
       break;
   }
}

static void
evas_video_sink_get_property(GObject * object, guint prop_id,
                             GValue * value, GParamSpec * pspec)
{
   EvasVideoSink* sink;
   EvasVideoSinkPrivate* priv;

   sink = EVAS_VIDEO_SINK (object);
   priv = sink->priv;

   switch (prop_id) {
    case PROP_EVAS_OBJECT:
       INF("sink get property.");
       eina_lock_take(&priv->m);
       g_value_set_pointer(value, priv->o);
       eina_lock_release(&priv->m);
       break;
    case PROP_WIDTH:
       INF("sink get width.");
       eina_lock_take(&priv->m);
       g_value_set_int(value, priv->width);
       eina_lock_release(&priv->m);
       break;
    case PROP_HEIGHT:
       INF("sink get height.");
       eina_lock_take(&priv->m);
       g_value_set_int (value, priv->height);
       eina_lock_release(&priv->m);
       break;
    case PROP_EV:
       INF("sink get ev.");
       eina_lock_take(&priv->m);
       g_value_set_pointer (value, priv->ev);
       eina_lock_release(&priv->m);
       break;
    default:
       G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
       ERR("invalide property");
       break;
   }
}

static void
evas_video_sink_dispose(GObject* object)
{
   EvasVideoSink* sink;
   EvasVideoSinkPrivate* priv;

   INF("dispose.");

   sink = EVAS_VIDEO_SINK(object);
   priv = sink->priv;

   eina_lock_free(&priv->m);
   eina_condition_free(&priv->c);

   G_OBJECT_CLASS(parent_class)->dispose(object);
}


/**** BaseSink methods ****/

gboolean evas_video_sink_set_caps(GstBaseSink *bsink, GstCaps *caps)
{
   EvasVideoSink* sink;
   EvasVideoSinkPrivate* priv;
   GstStructure *structure;
   GstVideoFormat format;
   guint32 fourcc;
   unsigned int i;

   sink = EVAS_VIDEO_SINK(bsink);
   priv = sink->priv;

   structure = gst_caps_get_structure(caps, 0);

   if (gst_structure_get_int(structure, "width", (int*) &priv->width)
       && gst_structure_get_int(structure, "height", (int*) &priv->height)
       && gst_structure_get_fourcc(structure, "format", &fourcc))
     {
        priv->source_height = priv->height;

        for (i = 0; colorspace_fourcc_convertion[i].name != NULL; ++i)
          if (fourcc == colorspace_fourcc_convertion[i].fourcc)
            {
               DBG("Found '%s'", colorspace_fourcc_convertion[i].name);
               priv->eformat = colorspace_fourcc_convertion[i].eformat;
               priv->func = colorspace_fourcc_convertion[i].func;
              if (colorspace_fourcc_convertion[i].force_height)
                 {
                    priv->height = (priv->height >> 1) << 1;
                 }
               if (priv->ev)
                 priv->ev->kill_buffer = EINA_TRUE;
               return TRUE;
            }

        if (fourcc == GST_MAKE_FOURCC('S', 'T', '1', '2'))
          {
             DBG("Found '%s'", "ST12");
             priv->eformat = EVAS_COLORSPACE_YCBCR420TM12601_PL;
             priv->samsung = EINA_TRUE;
             priv->func = NULL;
             if (priv->ev)
               {
                  priv->ev->samsung = EINA_TRUE;
                  priv->ev->kill_buffer = EINA_TRUE;
               }
	     return TRUE;
          }
     }

   INF("fallback code !");
   if (!gst_video_format_parse_caps(caps, &format, (int*) &priv->width, (int*) &priv->height))
     {
        ERR("Unable to parse caps.");
        return FALSE;
     }

   priv->source_height = priv->height;

   for (i = 0; colorspace_format_convertion[i].name != NULL; ++i)
     if (format == colorspace_format_convertion[i].format)
       {
          DBG("Found '%s'", colorspace_format_convertion[i].name);
          priv->eformat = colorspace_format_convertion[i].eformat;
          priv->func = colorspace_format_convertion[i].func;
          if (priv->ev)
            priv->ev->kill_buffer = EINA_FALSE;
          return TRUE;
       }

   ERR("unsupported : %d\n", format);
   return FALSE;
}

static gboolean
evas_video_sink_start(GstBaseSink* base_sink)
{
   EvasVideoSinkPrivate* priv;
   gboolean res = TRUE;

   INF("sink start");

   priv = EVAS_VIDEO_SINK(base_sink)->priv;
   eina_lock_take(&priv->m);
   if (!priv->o)
     res = FALSE;
   else
     priv->unlocked = EINA_FALSE;
   eina_lock_release(&priv->m);
   return res;
}

static gboolean
evas_video_sink_stop(GstBaseSink* base_sink)
{
   EvasVideoSinkPrivate* priv = EVAS_VIDEO_SINK(base_sink)->priv;

   INF("sink stop");

   unlock_buffer_mutex(priv);
   return TRUE;
}

static gboolean
evas_video_sink_unlock(GstBaseSink* object)
{
   EvasVideoSink* sink;

   INF("sink unlock");

   sink = EVAS_VIDEO_SINK(object);

   unlock_buffer_mutex(sink->priv);

   return GST_CALL_PARENT_WITH_DEFAULT(GST_BASE_SINK_CLASS, unlock,
                                       (object), TRUE);
}

static gboolean
evas_video_sink_unlock_stop(GstBaseSink* object)
{
   EvasVideoSink* sink;
   EvasVideoSinkPrivate* priv;

   sink = EVAS_VIDEO_SINK(object);
   priv = sink->priv;

   INF("sink unlock stop");

   eina_lock_take(&priv->m);
   priv->unlocked = FALSE;
   eina_lock_release(&priv->m);

   return GST_CALL_PARENT_WITH_DEFAULT(GST_BASE_SINK_CLASS, unlock_stop,
                                       (object), TRUE);
}

static GstFlowReturn
evas_video_sink_preroll(GstBaseSink* bsink, GstBuffer* buffer)
{
   Emotion_Gstreamer_Buffer *send;
   EvasVideoSinkPrivate *priv;
   EvasVideoSink *sink;

   INF("sink preroll %p [%i]", GST_BUFFER_DATA(buffer), GST_BUFFER_SIZE(buffer));

   sink = EVAS_VIDEO_SINK(bsink);
   priv = sink->priv;

   if (GST_BUFFER_SIZE(buffer) <= 0 && !priv->samsung)
     {
        WRN("empty buffer");
        return GST_FLOW_OK;
     }

   send = emotion_gstreamer_buffer_alloc(priv, buffer, EINA_TRUE);

   if (send)
     {
        if (priv->samsung)
          {
             if (!priv->func)
               {
                  GstStructure *structure;
                  GstCaps *caps;
                  gboolean is_multiplane = FALSE;

                  caps = GST_BUFFER_CAPS(buffer);
                  structure = gst_caps_get_structure (caps, 0);
                  gst_structure_get_boolean(structure, "multiplane", &is_multiplane);
		  gst_caps_unref(caps);

                  if (is_multiplane)
                    priv->func = _evas_video_st12_multiplane;
                  else
                    priv->func = _evas_video_st12;
               }
             _emotion_pending_ecore_begin();
             ecore_main_loop_thread_safe_call_async(evas_video_sink_samsung_main_render, send);
          }
        else
          {
             _emotion_pending_ecore_begin();
             ecore_main_loop_thread_safe_call_async(evas_video_sink_main_render, send);
          }
     }

   return GST_FLOW_OK;
}

static GstFlowReturn
evas_video_sink_render(GstBaseSink* bsink, GstBuffer* buffer)
{
   Emotion_Gstreamer_Buffer *send;
   EvasVideoSinkPrivate *priv;
   EvasVideoSink *sink;

   INF("sink render %p", buffer);

   sink = EVAS_VIDEO_SINK(bsink);
   priv = sink->priv;

   eina_lock_take(&priv->m);

   if (priv->unlocked) {
      ERR("LOCKED");
      eina_lock_release(&priv->m);
      return GST_FLOW_OK;
   }

   send = emotion_gstreamer_buffer_alloc(priv, buffer, EINA_FALSE);
   if (!send) {
      eina_lock_release(&priv->m);
      return GST_FLOW_ERROR;
   }

   if (priv->samsung)
     {
        if (!priv->func)
          {
             GstStructure *structure;
             GstCaps *caps;
             gboolean is_multiplane = FALSE;

             caps = GST_BUFFER_CAPS(buffer);
             structure = gst_caps_get_structure (caps, 0);
             gst_structure_get_boolean(structure, "multiplane", &is_multiplane);
	     gst_caps_unref(caps);

             if (is_multiplane)
               priv->func = _evas_video_st12_multiplane;
             else
               priv->func = _evas_video_st12;
          }
        _emotion_pending_ecore_begin();
        ecore_main_loop_thread_safe_call_async(evas_video_sink_samsung_main_render, send);
     }
   else
     {
        _emotion_pending_ecore_begin();
        ecore_main_loop_thread_safe_call_async(evas_video_sink_main_render, send);
     }

   eina_condition_wait(&priv->c);
   eina_lock_release(&priv->m);

   return GST_FLOW_OK;
}

static void
_update_emotion_fps(Emotion_Gstreamer_Video *ev)
{
   double tim;

   if (!debug_fps) return;

   tim = ecore_time_get();
   ev->frames++;

   if (ev->rlapse == 0.0)
     {
        ev->rlapse = tim;
        ev->flapse = ev->frames;
     }
   else if ((tim - ev->rlapse) >= 0.5)
     {
        printf("FRAME: %i, FPS: %3.1f\n",
               ev->frames,
               (ev->frames - ev->flapse) / (tim - ev->rlapse));
        ev->rlapse = tim;
        ev->flapse = ev->frames;
     }
}

static void
evas_video_sink_samsung_main_render(void *data)
{
   Emotion_Gstreamer_Buffer *send;
   Emotion_Video_Stream *vstream;
   EvasVideoSinkPrivate *priv = NULL;
   GstBuffer* buffer;
   unsigned char *evas_data;
   const guint8 *gst_data;
   GstFormat fmt = GST_FORMAT_TIME;
   gint64 pos;
   Eina_Bool preroll = EINA_FALSE;
   int stride, elevation;
   Evas_Coord w, h;

   send = data;

   if (!send) goto exit_point;

   priv = send->sink;
   buffer = send->frame;
   preroll = send->preroll;

   /* frame after cleanup */
   if (!preroll && !send->ev->last_buffer)
     {
        priv = NULL;
        goto exit_point;
     }

   if (!priv || !priv->o || priv->unlocked)
     goto exit_point;

   if (send->ev->send)
     {
        emotion_gstreamer_buffer_free(send->ev->send);
        send->ev->send = NULL;
     }

   if (!send->ev->stream && !send->force)
     {
        send->ev->send = send;
        _emotion_frame_new(send->ev->obj);
        goto exit_stream;
     }

   _emotion_gstreamer_video_pipeline_parse(send->ev, EINA_TRUE);

   /* Getting stride to compute the right size and then fill the object properly */
   /* Y => [0] and UV in [1] */
   if (priv->func == _evas_video_st12_multiplane)
     {
        const GstMultiPlaneImageBuffer *mp_buf = (const GstMultiPlaneImageBuffer *) buffer;

        stride = mp_buf->stride[0];
        elevation = mp_buf->elevation[0];
        priv->width = mp_buf->width[0];
        priv->height = mp_buf->height[0];

        gst_data = (const guint8 *) mp_buf;
     }
   else
     {
        const SCMN_IMGB *imgb = (const SCMN_IMGB *) GST_BUFFER_MALLOCDATA(buffer);

        stride = imgb->stride[0];
        elevation = imgb->elevation[0];
        priv->width = imgb->width[0];
        priv->height = imgb->height[0];

        gst_data = (const guint8 *) imgb;
     }

   evas_object_geometry_get(priv->o, NULL, NULL, &w, &h);

   send->ev->fill.width = (double) stride / priv->width;
   send->ev->fill.height = (double) elevation / priv->height;

   evas_object_image_alpha_set(priv->o, 0);
   evas_object_image_colorspace_set(priv->o, priv->eformat);
   evas_object_image_size_set(priv->o, stride, elevation);

   _update_emotion_fps(send->ev);

   evas_data = evas_object_image_data_get(priv->o, 1);

   if (priv->func)
     priv->func(evas_data, gst_data, stride, elevation, elevation);
   else
     WRN("No way to decode %x colorspace !", priv->eformat);

   evas_object_image_data_set(priv->o, evas_data);
   evas_object_image_data_update_add(priv->o, 0, 0, priv->width, priv->height);
   evas_object_image_pixels_dirty_set(priv->o, 0);

   if (!preroll && send->ev->play_started)
     {
        _emotion_playback_started(send->ev->obj);
        send->ev->play_started = 0;
     }

   if (!send->force)
     {
        _emotion_frame_new(send->ev->obj);
     }

   vstream = eina_list_nth(send->ev->video_streams, send->ev->video_stream_nbr - 1);

   gst_element_query_position(send->ev->pipeline, &fmt, &pos);
   send->ev->position = (double)pos / (double)GST_SECOND;

   if (vstream)
     {
        vstream->width = priv->width;
        vstream->height = priv->height;

        _emotion_video_pos_update(send->ev->obj, send->ev->position, vstream->length_time);
     }

   send->ev->ratio = (double) priv->width / (double) priv->height;
   _emotion_frame_refill(send->ev->obj, send->ev->fill.width, send->ev->fill.height);
   _emotion_frame_resize(send->ev->obj, priv->width, priv->height, send->ev->ratio);

   buffer = gst_buffer_ref(buffer);
   if (send->ev->last_buffer) gst_buffer_unref(send->ev->last_buffer);
   send->ev->last_buffer = buffer;

 exit_point:
   if (send) emotion_gstreamer_buffer_free(send);

 exit_stream:
   if (priv)
     {
        if (preroll || !priv->o)
          {
             _emotion_pending_ecore_end();
             return;
          }
        
        if (!priv->unlocked)
          eina_condition_signal(&priv->c);
     }
   _emotion_pending_ecore_end();
}

static void
evas_video_sink_main_render(void *data)
{
   Emotion_Gstreamer_Buffer *send;
   Emotion_Gstreamer_Video *ev = NULL;
   Emotion_Video_Stream *vstream;
   EvasVideoSinkPrivate *priv = NULL;
   GstBuffer *buffer;
   unsigned char *evas_data;
   GstFormat fmt = GST_FORMAT_TIME;
   gint64 pos;
   Eina_Bool preroll = EINA_FALSE;

   send = data;

   if (!send) goto exit_point;

   priv = send->sink;
   buffer = send->frame;
   preroll = send->preroll;
   ev = send->ev;

   /* frame after cleanup */
   if (!preroll && !ev->last_buffer)
     {
        priv = NULL;
        goto exit_point;
     }

   if (!priv || !priv->o || priv->unlocked)
     goto exit_point;

   if (ev->send && send != ev->send)
     {
        emotion_gstreamer_buffer_free(ev->send);
        ev->send = NULL;
     }

   if (!ev->stream && !send->force)
     {
        ev->send = send;
        _emotion_frame_new(ev->obj);
        evas_object_image_data_update_add(priv->o, 0, 0, priv->width, priv->height);
        goto exit_stream;
     }

   _emotion_gstreamer_video_pipeline_parse(ev, EINA_TRUE);

   INF("sink main render [%i, %i] (source height: %i)", priv->width, priv->height, priv->source_height);

   evas_object_image_alpha_set(priv->o, 0);
   evas_object_image_colorspace_set(priv->o, priv->eformat);
   evas_object_image_size_set(priv->o, priv->width, priv->height);

   evas_data = evas_object_image_data_get(priv->o, 1);

   if (priv->func)
     priv->func(evas_data, GST_BUFFER_DATA(buffer), priv->width, priv->source_height, priv->height);
   else
     WRN("No way to decode %x colorspace !", priv->eformat);

   evas_object_image_data_set(priv->o, evas_data);
   evas_object_image_data_update_add(priv->o, 0, 0, priv->width, priv->height);
   evas_object_image_pixels_dirty_set(priv->o, 0);

   _update_emotion_fps(ev);

   if (!preroll && ev->play_started)
     {
        _emotion_playback_started(ev->obj);
        ev->play_started = 0;
     }

   if (!send->force)
     {
        _emotion_frame_new(ev->obj);
     }

   gst_element_query_position(ev->pipeline, &fmt, &pos);
   ev->position = (double)pos / (double)GST_SECOND;

   vstream = eina_list_nth(ev->video_streams, ev->video_stream_nbr - 1);

   if (vstream)
     {
       vstream->width = priv->width;
       vstream->height = priv->height;
       _emotion_video_pos_update(ev->obj, ev->position, vstream->length_time);
     }

   ev->ratio = (double) priv->width / (double) priv->height;

   _emotion_frame_resize(ev->obj, priv->width, priv->height, ev->ratio);

   buffer = gst_buffer_ref(buffer);
   if (ev->last_buffer) gst_buffer_unref(ev->last_buffer);
   ev->last_buffer = buffer;

 exit_point:
   if (send) emotion_gstreamer_buffer_free(send);

 exit_stream:
   if (priv)
     {
        if (preroll || !priv->o)
          {
             _emotion_pending_ecore_end();
             return;
          }
        
        if (!priv->unlocked)
          eina_condition_signal(&priv->c);
     }
   _emotion_pending_ecore_end();
}

static void
unlock_buffer_mutex(EvasVideoSinkPrivate* priv)
{
   priv->unlocked = EINA_TRUE;

   eina_condition_signal(&priv->c);
}

static void
marshal_VOID__MINIOBJECT(GClosure * closure, GValue * return_value EINA_UNUSED,
                         guint n_param_values, const GValue * param_values,
                         gpointer invocation_hint EINA_UNUSED, gpointer marshal_data)
{
   typedef void (*marshalfunc_VOID__MINIOBJECT) (gpointer obj, gpointer arg1, gpointer data2);
   marshalfunc_VOID__MINIOBJECT callback;
   GCClosure *cc;
   gpointer data1, data2;

   cc = (GCClosure *) closure;

   g_return_if_fail(n_param_values == 2);

   if (G_CCLOSURE_SWAP_DATA(closure)) {
      data1 = closure->data;
      data2 = g_value_peek_pointer(param_values + 0);
   } else {
      data1 = g_value_peek_pointer(param_values + 0);
      data2 = closure->data;
   }
   callback = (marshalfunc_VOID__MINIOBJECT) (marshal_data ? marshal_data : cc->callback);

   callback(data1, gst_value_get_mini_object(param_values + 1), data2);
}

static void
evas_video_sink_class_init(EvasVideoSinkClass* klass)
{
   GObjectClass* gobject_class;
   GstBaseSinkClass* gstbase_sink_class;

   gobject_class = G_OBJECT_CLASS(klass);
   gstbase_sink_class = GST_BASE_SINK_CLASS(klass);

   g_type_class_add_private(klass, sizeof(EvasVideoSinkPrivate));

   gobject_class->set_property = evas_video_sink_set_property;
   gobject_class->get_property = evas_video_sink_get_property;

   g_object_class_install_property (gobject_class, PROP_EVAS_OBJECT,
                                    g_param_spec_pointer ("evas-object", "Evas Object",
                                                          "The Evas object where the display of the video will be done",
                                                          G_PARAM_READWRITE));

   g_object_class_install_property (gobject_class, PROP_WIDTH,
                                    g_param_spec_int ("width", "Width",
                                                      "The width of the video",
                                                      0, 65536, 0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

   g_object_class_install_property (gobject_class, PROP_HEIGHT,
                                    g_param_spec_int ("height", "Height",
                                                      "The height of the video",
                                                      0, 65536, 0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
   g_object_class_install_property (gobject_class, PROP_EV,
                                    g_param_spec_pointer ("ev", "Emotion_Gstreamer_Video",
                                                          "THe internal data of the emotion object",
                                                          G_PARAM_READWRITE));

   gobject_class->dispose = evas_video_sink_dispose;

   gstbase_sink_class->set_caps = evas_video_sink_set_caps;
   gstbase_sink_class->stop = evas_video_sink_stop;
   gstbase_sink_class->start = evas_video_sink_start;
   gstbase_sink_class->unlock = evas_video_sink_unlock;
   gstbase_sink_class->unlock_stop = evas_video_sink_unlock_stop;
   gstbase_sink_class->render = evas_video_sink_render;
   gstbase_sink_class->preroll = evas_video_sink_preroll;

   evas_video_sink_signals[REPAINT_REQUESTED] = g_signal_new("repaint-requested",
                                                             G_TYPE_FROM_CLASS(klass),
                                                             (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
                                                             0,
                                                             0,
                                                             0,
                                                             marshal_VOID__MINIOBJECT,
                                                             G_TYPE_NONE, 1, GST_TYPE_BUFFER);
}

gboolean
gstreamer_plugin_init (GstPlugin * plugin)
{
   return gst_element_register (plugin,
                                "emotion-sink",
                                GST_RANK_NONE,
                                EVAS_TYPE_VIDEO_SINK);
}

static void
_emotion_gstreamer_pause(void *data, Ecore_Thread *thread)
{
   Emotion_Gstreamer_Video *ev = data;
   gboolean res;

   if (ecore_thread_check(thread) || !ev->pipeline) return;

   gst_element_set_state(ev->pipeline, GST_STATE_PAUSED);
   res = gst_element_get_state(ev->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
   if (res == GST_STATE_CHANGE_NO_PREROLL)
     {
        gst_element_set_state(ev->pipeline, GST_STATE_PLAYING);
	gst_element_get_state(ev->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
     }
}

static void
_emotion_gstreamer_cancel(void *data, Ecore_Thread *thread)
{
   Emotion_Gstreamer_Video *ev = data;

   ev->threads = eina_list_remove(ev->threads, thread);

#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
   if (getuid() == geteuid())
#endif
     {
        if (getenv("EMOTION_GSTREAMER_DOT")) GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(ev->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, getenv("EMOTION_GSTREAMER_DOT"));
     }

   if (ev->in == ev->out && ev->delete_me)
     ev->api->del(ev);
}

static void
_emotion_gstreamer_end(void *data, Ecore_Thread *thread)
{
   Emotion_Gstreamer_Video *ev = data;

   ev->threads = eina_list_remove(ev->threads, thread);

   if (ev->play)
     {
        gst_element_set_state(ev->pipeline, GST_STATE_PLAYING);
        ev->play_started = 1;
     }

#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
   if (getuid() == geteuid())
#endif
     {
        if (getenv("EMOTION_GSTREAMER_DOT")) GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(ev->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, getenv("EMOTION_GSTREAMER_DOT"));
     }

   if (ev->in == ev->out && ev->delete_me)
     ev->api->del(ev);
   else
     _emotion_gstreamer_video_pipeline_parse(data, EINA_TRUE);
}

static void
_video_resize(void *data, Evas_Object *obj EINA_UNUSED, const Evas_Video_Surface *surface EINA_UNUSED,
              Evas_Coord w, Evas_Coord h)
{
#ifdef HAVE_ECORE_X
   Emotion_Gstreamer_Video *ev = data;

   ecore_x_window_resize(ev->win, w, h);
   DBG("resize: %i, %i", w, h);
#else   
   if (data)
     {
        DBG("resize: %i, %i (fake)", w, h);
     }
#endif
}

static void
_video_move(void *data, Evas_Object *obj EINA_UNUSED, const Evas_Video_Surface *surface EINA_UNUSED,
            Evas_Coord x, Evas_Coord y)
{
#ifdef HAVE_ECORE_X
   Emotion_Gstreamer_Video *ev = data;
   unsigned int pos[2];

   DBG("move: %i, %i", x, y);
   pos[0] = x; pos[1] = y;
   ecore_x_window_prop_card32_set(ev->win, ECORE_X_ATOM_E_VIDEO_POSITION, pos, 2);
#else   
   if (data)
     {
        DBG("move: %i, %i (fake)", x, y);
     }
#endif
}

#if 0
/* Much better idea to always feed the XvImageSink and let him handle optimizing the rendering as we do */
static void
_block_pad_unlink_cb(GstPad *pad, gboolean blocked, gpointer user_data)
{
   if (blocked)
     {
        Emotion_Gstreamer_Video *ev = user_data;
        GstEvent *gev;

        gst_pad_unlink(ev->xvteepad, ev->xvpad);
        gev = gst_event_new_eos();
        gst_pad_send_event(ev->xvpad, gev);
        gst_pad_set_blocked_async(pad, FALSE, _block_pad_unlink_cb, NULL);
     }
}

static void
_block_pad_link_cb(GstPad *pad, gboolean blocked, gpointer user_data)
{
   if (blocked)
     {
        Emotion_Gstreamer_Video *ev = user_data;

        gst_pad_link(ev->xvteepad, ev->xvpad);
        if (ev->play)
          gst_element_set_state(ev->xvsink, GST_STATE_PLAYING);
        else
          gst_element_set_state(ev->xvsink, GST_STATE_PAUSED);
        gst_pad_set_blocked_async(pad, FALSE, _block_pad_link_cb, NULL);
     }
}
#endif

static void
_video_show(void *data, Evas_Object *obj EINA_UNUSED, const Evas_Video_Surface *surface EINA_UNUSED)
{
#ifdef HAVE_ECORE_X
   Emotion_Gstreamer_Video *ev = data;

   DBG("show xv");
   ecore_x_window_show(ev->win);
#else
   if (data)
     {
        DBG("show xv (fake)");
     }
#endif
   /* gst_pad_set_blocked_async(ev->xvteepad, TRUE, _block_pad_link_cb, ev); */
}

static void
_video_hide(void *data, Evas_Object *obj EINA_UNUSED, const Evas_Video_Surface *surface EINA_UNUSED)
{
#ifdef HAVE_ECORE_X
   Emotion_Gstreamer_Video *ev = data;

   DBG("hide xv");
   ecore_x_window_hide(ev->win);
#else
   if (data)
     {
        DBG("hide xv (fake)");
     }
#endif
   /* gst_pad_set_blocked_async(ev->xvteepad, TRUE, _block_pad_unlink_cb, ev); */
}

static void
_video_update_pixels(void *data, Evas_Object *obj EINA_UNUSED, const Evas_Video_Surface *surface EINA_UNUSED)
{
   Emotion_Gstreamer_Video *ev = data;
   Emotion_Gstreamer_Buffer *send;
   EvasVideoSinkPrivate *priv = NULL;

   if (!ev->send) return;

   send = ev->send;
   priv = send->sink;
   send->force = EINA_TRUE;
   ev->send = NULL;

   if (priv->samsung)
     {
        _emotion_pending_ecore_begin();
        evas_video_sink_samsung_main_render(send);
     }
   else
     {
        _emotion_pending_ecore_begin();
        evas_video_sink_main_render(send);
     }
}

static void
_image_resize(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Emotion_Gstreamer_Video *ev = data;
   Evas_Coord width, height;
   int image_area, src_area;
   double ratio;

   GstElementFactory *cfactory = NULL;
   GstElement *convert = NULL, *filter = NULL, *queue = NULL;
   GstPad *pad = NULL, *teepad = NULL;
   GstCaps *caps = NULL;
   Eina_List *l, *engines;
   const char *ename, *engine = NULL;

   evas_object_geometry_get(obj, NULL, NULL, &width, &height);
   image_area = width * height;
   src_area = ev->src_width * ev->src_height;
   ratio = (double)image_area / (double)src_area;

   // when an image is much smaller than original video size,
   // add fimcconvert element to the pipeline
   if (ratio < 0.8 && ev->stream && !ev->convert)
     {
        cfactory = gst_element_factory_find("fimcconvert");
        if (!cfactory) return;

        convert = gst_element_factory_create(cfactory, NULL);
        if (!convert) return;

        // add capsfilter to limit size and formats based on the backend
        filter = gst_element_factory_make("capsfilter", "fimccapsfilter");
        if (!filter)
          {
             gst_object_unref(convert);
             return;
          }

        engines = evas_render_method_list();
        EINA_LIST_FOREACH(engines, l, ename)
          {
             if (evas_render_method_lookup(ename) ==
                 evas_output_method_get(evas_object_evas_get(obj)))
               {
                  engine = ename;
                  break;
               }
          }

        if (!engine) return;

        if (strstr(engine, "software") != NULL)
          {
             caps = gst_caps_new_simple("video/x-raw-rgb",
                                        "width", G_TYPE_INT, width,
                                        "height", G_TYPE_INT, height,
                                        NULL);
          }
        else if (strstr(engine, "gl") != NULL)
          {
             caps = gst_caps_new_simple("video/x-raw-yuv",
                                        "width", G_TYPE_INT, width,
                                        "height", G_TYPE_INT, height,
                                        NULL);
          }
        g_object_set(G_OBJECT(filter), "caps", caps, NULL);
        gst_caps_unref(caps);

        // add new elements to the pipeline
        queue = gst_bin_get_by_name(GST_BIN(ev->sink), "equeue");
        gst_element_unlink(ev->tee, queue);
        gst_element_release_request_pad(ev->tee, ev->eteepad);
        gst_object_unref(ev->eteepad);

        gst_bin_add_many(GST_BIN(ev->sink), convert, filter, NULL);
        gst_element_link_many(ev->tee, convert, filter, queue, NULL);

        pad = gst_element_get_pad(convert, "sink");
        teepad = gst_element_get_request_pad(ev->tee, "src%d");
        gst_pad_link(teepad, pad);
        gst_object_unref(pad);

        gst_element_sync_state_with_parent(convert);
        gst_element_sync_state_with_parent(filter);

        ev->eteepad = teepad;
        ev->convert = convert;
        evas_render_method_list_free(engines);

        INF("add fimcconvert element. video size: %dx%d. emotion object size: %dx%d",
            ev->src_width, ev->src_height, width, height);
     }
   // set size again to the capsfilter when the image is resized
   else if (ev->convert)
     {
        filter = gst_bin_get_by_name(GST_BIN(ev->sink), "fimccapsfilter");

        engines = evas_render_method_list();
        EINA_LIST_FOREACH(engines, l, ename)
          {
             if (evas_render_method_lookup(ename) ==
                 evas_output_method_get(evas_object_evas_get(obj)))
               {
                  engine = ename;
                  break;
               }
          }

        if (!engine) return;

        if (strstr(engine, "software") != NULL)
          {
             caps = gst_caps_new_simple("video/x-raw-rgb",
                                        "width", G_TYPE_INT, width,
                                        "height", G_TYPE_INT, height,
                                        NULL);
          }
        else if (strstr(engine, "gl") != NULL)
          {
             caps = gst_caps_new_simple("video/x-raw-yuv",
                                        "width", G_TYPE_INT, width,
                                        "height", G_TYPE_INT, height,
                                        NULL);
          }

        g_object_set(G_OBJECT(filter), "caps", caps, NULL);
        gst_caps_unref(caps);
        evas_render_method_list_free(engines);

        INF("set capsfilter size again:. video size: %dx%d. emotion object size: %dx%d",
            ev->src_width, ev->src_height, width, height);
     }
}

GstElement *
gstreamer_video_sink_new(Emotion_Gstreamer_Video *ev,
			 Evas_Object *o,
			 const char *uri)
{
   GstElement *playbin;
   GstElement *bin = NULL;
   GstElement *esink = NULL;
   GstElement *xvsink = NULL;
   GstElement *tee = NULL;
   GstElement *queue = NULL;
   Evas_Object *obj;
   GstPad *pad;
   GstPad *teepad;
   int flags;
   const char *launch;
#if defined HAVE_ECORE_X && defined HAVE_XOVERLAY_H
   const char *engine = NULL;
   Eina_List *engines;
#endif

   obj = emotion_object_image_get(o);
   if (!obj)
     {
//        ERR("Not Evas_Object specified");
        return NULL;
     }

   if (!uri)
     return NULL;

   launch = emotion_webcam_custom_get(uri);
   if (launch)
     {
        GError *error = NULL;

        playbin = gst_parse_bin_from_description(launch, 1, &error);
        if (!playbin)
          {
             ERR("Unable to setup command : '%s' got error '%s'.", launch, error->message);
             g_error_free(error);
             return NULL;
          }
        if (error)
          {
             WRN("got recoverable error '%s' for command : '%s'.", error->message, launch);
             g_error_free(error);
          }
     }
   else
     {
        playbin = gst_element_factory_make("playbin2", "playbin");
        if (!playbin)
          {
             ERR("Unable to create 'playbin' GstElement.");
             return NULL;
          }
     }

   bin = gst_bin_new(NULL);
   if (!bin)
     {
       ERR("Unable to create GstBin !");
       goto unref_pipeline;
     }

   tee = gst_element_factory_make("tee", NULL);
   if (!tee)
     {
       ERR("Unable to create 'tee' GstElement.");
       goto unref_pipeline;
     }

#if defined HAVE_ECORE_X && defined HAVE_XOVERLAY_H
   if (window_manager_video)
     {
        Eina_List *l;
        const char *ename;
        
        engines = evas_render_method_list();

        EINA_LIST_FOREACH(engines, l, ename)
          {
             if (evas_render_method_lookup(ename) == 
                 evas_output_method_get(evas_object_evas_get(obj)))
               {
                  engine = ename;
                  break;
               }
          }

       if (ev->priority && engine && strstr(engine, "_x11") != NULL)
	 {
	   Ecore_Evas *ee;
	   Evas_Coord x, y, w, h;
	   Ecore_X_Window win;
	   Ecore_X_Window parent;

	   evas_object_geometry_get(obj, &x, &y, &w, &h);

	   ee = ecore_evas_ecore_evas_get(evas_object_evas_get(obj));

	   if (w < 4) w = 4;
	   if (h < 2) h = 2;

	   /* Here we really need to have the help of the window manager, this code will change when we update E17. */
	   parent = (Ecore_X_Window) ecore_evas_window_get(ee);
	   DBG("parent: %x", parent);

	   win = ecore_x_window_new(0, x, y, w, h);
	   DBG("creating window: %x [%i, %i, %i, %i]", win, x, y, w, h);
	   if (win)
	     {
	       Ecore_X_Window_State state[] = { ECORE_X_WINDOW_STATE_SKIP_TASKBAR, ECORE_X_WINDOW_STATE_SKIP_PAGER };

	       ecore_x_netwm_window_state_set(win, state, 2);
	       ecore_x_window_hide(win);
	       xvsink = gst_element_factory_make("xvimagesink", NULL);
	       if (xvsink)
		 {
		   unsigned int pos[2];

#ifdef HAVE_X_OVERLAY_SET
		   gst_x_overlay_set_window_handle(GST_X_OVERLAY(xvsink), win);
#else
		   gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(xvsink), win);
#endif
		   ev->win = win;

		   ecore_x_window_prop_card32_set(win, ECORE_X_ATOM_E_VIDEO_PARENT, &parent, 1);

		   pos[0] = x; pos[1] = y;
		   ecore_x_window_prop_card32_set(win, ECORE_X_ATOM_E_VIDEO_POSITION, pos, 2);
		 }
	       else
		 {
		   DBG("destroying win: %x", win);
		   ecore_x_window_free(win);
		 }
	     }
	 }
       evas_render_method_list_free(engines);
     }
#else
//# warning "missing: ecore_x OR xoverlay"
#endif

   esink = gst_element_factory_make("emotion-sink", "sink");
   if (!esink)
     {
        ERR("Unable to create 'emotion-sink' GstElement.");
        goto unref_pipeline;
     }

   g_object_set(G_OBJECT(esink), "evas-object", obj, NULL);
   g_object_set(G_OBJECT(esink), "ev", ev, NULL);

   evas_object_image_pixels_get_callback_set(obj, NULL, NULL);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _image_resize, ev);

   /* We need queue to force each video sink to be in its own thread */
   queue = gst_element_factory_make("queue", "equeue");
   if (!queue)
     {
        ERR("Unable to create 'queue' GstElement.");
        goto unref_pipeline;
     }

   gst_bin_add_many(GST_BIN(bin), tee, queue, esink, NULL);
   gst_element_link_many(queue, esink, NULL);

   /* link both sink to GstTee */
   pad = gst_element_get_pad(queue, "sink");
   teepad = gst_element_get_request_pad(tee, "src%d");
   gst_pad_link(teepad, pad);
   gst_object_unref(pad);

   ev->eteepad = teepad;

   if (xvsink)
     {
        GstElement *fakeeos;

        queue = gst_element_factory_make("queue", "xvqueue");
        fakeeos = GST_ELEMENT(GST_BIN(g_object_new(GST_TYPE_FAKEEOS_BIN, "name", "eosbin", NULL)));
        if (queue && fakeeos)
          {
             GstPad *queue_pad;

             gst_bin_add_many(GST_BIN(bin), fakeeos, NULL);

             gst_bin_add_many(GST_BIN(fakeeos), queue, xvsink, NULL);
             gst_element_link_many(queue, xvsink, NULL);
             queue_pad = gst_element_get_pad(queue, "sink");
             gst_element_add_pad(fakeeos, gst_ghost_pad_new("sink", queue_pad));

             pad = gst_element_get_pad(fakeeos, "sink");
             teepad = gst_element_get_request_pad(tee, "src%d");
             gst_pad_link(teepad, pad);

             xvsink = fakeeos;

             ev->xvteepad = teepad;
             ev->xvpad = pad;
	  }
	else
	  {
             if (fakeeos) gst_object_unref(fakeeos);
             if (queue) gst_object_unref(queue);
             gst_object_unref(xvsink);
             xvsink = NULL;
	  }
     }

   teepad = gst_element_get_pad(tee, "sink");
   gst_element_add_pad(bin, gst_ghost_pad_new("sink", teepad));
   gst_object_unref(teepad);

#define GST_PLAY_FLAG_NATIVE_VIDEO  (1 << 6)
#define GST_PLAY_FLAG_DOWNLOAD      (1 << 7)
#define GST_PLAY_FLAG_AUDIO         (1 << 1)
#define GST_PLAY_FLAG_NATIVE_AUDIO  (1 << 5)

   if (launch)
     {
        g_object_set(G_OBJECT(playbin), "sink", bin, NULL);
     }
   else
     {
        g_object_get(G_OBJECT(playbin), "flags", &flags, NULL);
        g_object_set(G_OBJECT(playbin), "flags", flags | GST_PLAY_FLAG_NATIVE_VIDEO | GST_PLAY_FLAG_DOWNLOAD | GST_PLAY_FLAG_NATIVE_AUDIO, NULL);
        g_object_set(G_OBJECT(playbin), "video-sink", bin, NULL);
        g_object_set(G_OBJECT(playbin), "uri", uri, NULL);
     }

   evas_object_image_pixels_get_callback_set(obj, NULL, NULL);

   ev->stream = EINA_TRUE;

   if (xvsink)
     {
        Evas_Video_Surface video;

        video.version = EVAS_VIDEO_SURFACE_VERSION;
        video.data = ev;
        video.parent = NULL;
        video.move = _video_move;
        video.resize = _video_resize;
        video.show = _video_show;
        video.hide = _video_hide;
        video.update_pixels = _video_update_pixels;

        evas_object_image_video_surface_set(obj, &video);
        ev->stream = EINA_FALSE;
     }

   eina_stringshare_replace(&ev->uri, uri);
   ev->pipeline = playbin;
   ev->sink = bin;
   ev->esink = esink;
   ev->xvsink = xvsink;
   ev->tee = tee;
   ev->threads = eina_list_append(ev->threads,
                                  ecore_thread_run(_emotion_gstreamer_pause,
                                                   _emotion_gstreamer_end,
                                                   _emotion_gstreamer_cancel,
                                                   ev));

   /** NOTE: you need to set: GST_DEBUG_DUMP_DOT_DIR=/tmp EMOTION_ENGINE=gstreamer to save the $EMOTION_GSTREAMER_DOT file in '/tmp' */
   /** then call dot -Tpng -oemotion_pipeline.png /tmp/$TIMESTAMP-$EMOTION_GSTREAMER_DOT.dot */
#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
   if (getuid() == geteuid())
#endif
     {
        if (getenv("EMOTION_GSTREAMER_DOT")) GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(playbin), GST_DEBUG_GRAPH_SHOW_ALL, getenv("EMOTION_GSTREAMER_DOT"));
     }

   return playbin;

 unref_pipeline:
   gst_object_unref(xvsink);
   gst_object_unref(esink);
   gst_object_unref(tee);
   gst_object_unref(bin);
   gst_object_unref(playbin);
   return NULL;
}
