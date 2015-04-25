#ifndef __EMOTION_GSTREAMER_H__
#define __EMOTION_GSTREAMER_H__

#include "emotion_modules.h"

typedef void (*Evas_Video_Convert_Cb)(unsigned char *evas_data,
                                      const unsigned char *gst_data,
                                      unsigned int w,
                                      unsigned int h,
                                      unsigned int output_height);

typedef struct _EvasVideoSinkPrivate EvasVideoSinkPrivate;
typedef struct _EvasVideoSink        EvasVideoSink;
typedef struct _EvasVideoSinkClass   EvasVideoSinkClass;
typedef struct _Emotion_Gstreamer_Video Emotion_Gstreamer_Video;
typedef struct _Emotion_Audio_Stream Emotion_Audio_Stream;
typedef struct _Emotion_Gstreamer_Metadata Emotion_Gstreamer_Metadata;
typedef struct _Emotion_Gstreamer_Buffer Emotion_Gstreamer_Buffer;
typedef struct _Emotion_Gstreamer_Message Emotion_Gstreamer_Message;
typedef struct _Emotion_Video_Stream Emotion_Video_Stream;

struct _Emotion_Video_Stream
{
   gdouble     length_time;
   gint        width;
   gint        height;
   gint        fps_num;
   gint        fps_den;
   guint32     fourcc;
   int         index;
};

struct _Emotion_Audio_Stream
{
   gdouble     length_time;
   gint        channels;
   gint        samplerate;
};

struct _Emotion_Gstreamer_Metadata
{
   char *title;
   char *album;
   char *artist;
   char *genre;
   char *comment;
   char *year;
   char *count;
   char *disc_id;
};

struct _Emotion_Gstreamer_Video
{
   const Emotion_Engine *api;

   /* Gstreamer elements */
   GstElement       *pipeline;
   GstElement       *sink;
   GstElement       *esink;
   GstElement       *xvsink;
   GstElement       *tee;
   GstElement       *convert;

   GstPad           *eteepad;
   GstPad           *xvteepad;
   GstPad           *xvpad;
   Eina_List        *threads;

   /* eos */
   GstBus           *eos_bus;

   /* Strams */
   Eina_List        *video_streams;
   Eina_List        *audio_streams;

   int               video_stream_nbr;
   int               audio_stream_nbr;

    /* We need to keep a copy of the last inserted buffer as evas doesn't copy YUV data around */
   GstBuffer        *last_buffer;

   /* Evas object */
   Evas_Object      *obj;

   /* Characteristics of stream */
   double            position;
   double            ratio;
   double            volume;

   volatile int      seek_to;
   volatile int      get_poslen;

   Emotion_Gstreamer_Metadata *metadata;

#ifdef HAVE_ECORE_X
   Ecore_X_Window    win;
#endif

   const char       *uri;

   Emotion_Gstreamer_Buffer *send;

   EvasVideoSinkPrivate *sink_data;

   Emotion_Vis       vis;

   int               in;
   int               out;

   int frames;
   int flapse;
   double rtime;
   double rlapse;

   struct
   {
      double         width;
      double         height;
   } fill;

   Eina_Bool         play         : 1;
   Eina_Bool         play_started : 1;
   Eina_Bool         video_mute   : 1;
   Eina_Bool         audio_mute   : 1;
   Eina_Bool         pipeline_parsed : 1;
   Eina_Bool         delete_me    : 1;
   Eina_Bool         samsung      : 1;
   Eina_Bool         kill_buffer  : 1;
   Eina_Bool         stream       : 1;
   Eina_Bool         priority     : 1;

   int src_width;
   int src_height;
};

struct _EvasVideoSink {
    /*< private >*/
    GstVideoSink parent;
    EvasVideoSinkPrivate *priv;
};

struct _EvasVideoSinkClass {
    /*< private >*/
    GstVideoSinkClass parent_class;
};

struct _EvasVideoSinkPrivate {
   EINA_REFCOUNT;

   Evas_Object *o;

   Emotion_Gstreamer_Video *ev;

   Evas_Video_Convert_Cb func;

   unsigned int width;
   unsigned int height;
   unsigned int source_height;
   Evas_Colorspace eformat;

   Eina_Lock m;
   Eina_Condition c;

   // If this is TRUE all processing should finish ASAP
   // This is necessary because there could be a race between
   // unlock() and render(), where unlock() wins, signals the
   // GCond, then render() tries to render a frame although
   // everything else isn't running anymore. This will lead
   // to deadlocks because render() holds the stream lock.
   //
   // Protected by the buffer mutex
   Eina_Bool unlocked : 1;
   Eina_Bool samsung : 1; /** ST12 will only define a Samsung specific GstBuffer */
};

struct _Emotion_Gstreamer_Buffer
{
   Emotion_Gstreamer_Video *ev;
   EvasVideoSinkPrivate *sink;

   GstBuffer *frame;

   Eina_Bool preroll : 1;
   Eina_Bool force : 1;
};

struct _Emotion_Gstreamer_Message
{
   Emotion_Gstreamer_Video *ev;

   GstMessage *msg;
};

extern Eina_Bool window_manager_video;
extern Eina_Bool debug_fps;
extern int _emotion_gstreamer_log_domain;
extern Eina_Bool _ecore_x_available;

#ifdef DBG
#undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_emotion_gstreamer_log_domain, __VA_ARGS__)

#ifdef INF
#undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_emotion_gstreamer_log_domain, __VA_ARGS__)

#ifdef WRN
#undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_emotion_gstreamer_log_domain, __VA_ARGS__)

#ifdef ERR
#undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_emotion_gstreamer_log_domain, __VA_ARGS__)

#ifdef CRI
#undef CRI
#endif
#define CRI(...) EINA_LOG_DOM_CRIT(_emotion_gstreamer_log_domain, __VA_ARGS__)

#define EVAS_TYPE_VIDEO_SINK evas_video_sink_get_type()

GType fakeeos_bin_get_type(void);

#define EVAS_VIDEO_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    EVAS_TYPE_VIDEO_SINK, EvasVideoSink))

#define EVAS_VIDEO_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), \
    EVAS_TYPE_VIDEO_SINK, EvasVideoSinkClass))

#define EVAS_IS_VIDEO_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
    EVAS_TYPE_VIDEO_SINK))

#define EVAS_IS_VIDEO_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), \
    EVAS_TYPE_VIDEO_SINK))

#define EVAS_VIDEO_SINK_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), \
    EVAS_TYPE_VIDEO_SINK, EvasVideoSinkClass))

#define GST_TYPE_FAKEEOS_BIN fakeeos_bin_get_type()

GstElement *gstreamer_video_sink_new(Emotion_Gstreamer_Video *ev,
                                     Evas_Object *obj,
                                     const char *uri);

gboolean    gstreamer_plugin_init(GstPlugin *plugin);

Emotion_Gstreamer_Buffer *emotion_gstreamer_buffer_alloc(EvasVideoSinkPrivate *sink,
							 GstBuffer *buffer,
                                                         Eina_Bool preroll);
void emotion_gstreamer_buffer_free(Emotion_Gstreamer_Buffer *send);

Emotion_Gstreamer_Message *emotion_gstreamer_message_alloc(Emotion_Gstreamer_Video *ev,
                                                           GstMessage *msg);
void emotion_gstreamer_message_free(Emotion_Gstreamer_Message *send);
Eina_Bool _emotion_gstreamer_video_pipeline_parse(Emotion_Gstreamer_Video *ev,
                                                  Eina_Bool force);

typedef struct _ColorSpace_FourCC_Convertion ColorSpace_FourCC_Convertion;
typedef struct _ColorSpace_Format_Convertion ColorSpace_Format_Convertion;

struct _ColorSpace_FourCC_Convertion
{
   const char *name;
   guint32 fourcc;
   Evas_Colorspace eformat;
   Evas_Video_Convert_Cb func;
   Eina_Bool force_height;
};

struct _ColorSpace_Format_Convertion
{
   const char *name;
   GstVideoFormat format;
   Evas_Colorspace eformat;
   Evas_Video_Convert_Cb func;
};

extern const ColorSpace_FourCC_Convertion colorspace_fourcc_convertion[];
extern const ColorSpace_Format_Convertion colorspace_format_convertion[];

/** Samsung specific infrastructure - do not touch, do not modify */
#define MPLANE_IMGB_MAX_COUNT 4
#define SCMN_IMGB_MAX_PLANE 4

typedef struct _GstMultiPlaneImageBuffer GstMultiPlaneImageBuffer;
typedef struct _SCMN_IMGB SCMN_IMGB;

struct _GstMultiPlaneImageBuffer
{
   GstBuffer buffer;

   /* width of each image plane */
   gint      width[MPLANE_IMGB_MAX_COUNT];
   /* height of each image plane */
   gint      height[MPLANE_IMGB_MAX_COUNT];
   /* stride of each image plane */
   gint      stride[MPLANE_IMGB_MAX_COUNT];
   /* elevation of each image plane */
   gint      elevation[MPLANE_IMGB_MAX_COUNT];
   /* user space address of each image plane */
   guchar   *uaddr[MPLANE_IMGB_MAX_COUNT];
   /* Index of real address of each image plane, if needs */
   guchar   *index[MPLANE_IMGB_MAX_COUNT];
   /* left postion, if needs */
   gint      x;
   /* top position, if needs */
   gint      y;
   /* to align memory */
   gint      __dummy2;
   /* arbitrary data */
   gint      data[16];
};

struct _SCMN_IMGB
{
   /* width of each image plane */
   int      width[SCMN_IMGB_MAX_PLANE];
   /* height of each image plane */
   int      height[SCMN_IMGB_MAX_PLANE];
   /* stride of each image plane */
   int      stride[SCMN_IMGB_MAX_PLANE];
   /* elevation of each image plane */
   int      elevation[SCMN_IMGB_MAX_PLANE];
   /* user space address of each image plane */
   guchar  *uaddr[SCMN_IMGB_MAX_PLANE];
   /* physical address of each image plane, if needs */
   guchar  *p[SCMN_IMGB_MAX_PLANE];
   /* color space type of image */
   int      cs;
   /* left postion, if needs */
   int      x;
   /* top position, if needs */
   int      y;
   /* to align memory */
   int      __dummy2;
   /* arbitrary data */
   int      data[16];
};

void _evas_video_st12_multiplane(unsigned char *evas_data, const unsigned char *gst_data, unsigned int w, unsigned int h, unsigned int output_height EINA_UNUSED);
void _evas_video_st12(unsigned char *evas_data, const unsigned char *gst_data, unsigned int w EINA_UNUSED, unsigned int h, unsigned int output_height EINA_UNUSED);

#endif /* __EMOTION_GSTREAMER_H__ */
