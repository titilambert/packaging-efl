#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Evas.h>
#include <Ecore.h>

#include <glib.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideosink.h>

#ifdef HAVE_ECORE_X
# include <Ecore_X.h>
# ifdef HAVE_XOVERLAY_H
#  include <gst/interfaces/xoverlay.h>
# endif
#endif

#include "Emotion.h"
#include "emotion_gstreamer.h"

Emotion_Gstreamer_Buffer *
emotion_gstreamer_buffer_alloc(EvasVideoSinkPrivate *sink,
			       GstBuffer *buffer,
			       Eina_Bool preroll)
{
   Emotion_Gstreamer_Buffer *send;

   if (!sink->ev) return NULL;

   send = malloc(sizeof (Emotion_Gstreamer_Buffer));
   if (!send) return NULL;

   send->sink = sink;
   send->frame = gst_buffer_ref(buffer);
   send->preroll = preroll;
   send->force = EINA_FALSE;
   sink->ev->out++;
   send->ev = sink->ev;

   return send;
}

void
emotion_gstreamer_buffer_free(Emotion_Gstreamer_Buffer *send)
{
   send->ev->in++;

   if (send->ev->in == send->ev->out
       && send->ev->threads == NULL
       && send->ev->delete_me)
     send->ev->api->del(send->ev);

   gst_buffer_unref(send->frame);
   free(send);
}

Emotion_Gstreamer_Message *
emotion_gstreamer_message_alloc(Emotion_Gstreamer_Video *ev,
				GstMessage *msg)
{
   Emotion_Gstreamer_Message *send;

   if (!ev) return NULL;

   send = malloc(sizeof (Emotion_Gstreamer_Message));
   if (!send) return NULL;

   ev->out++;
   send->ev = ev;
   send->msg = gst_message_ref(msg);

   return send;
}

void
emotion_gstreamer_message_free(Emotion_Gstreamer_Message *send)
{
   send->ev->in++;

   if (send->ev->in == send->ev->out
       && send->ev->threads == NULL
       && send->ev->delete_me)
     send->ev->api->del(send->ev);

   gst_message_unref(send->msg);
   free(send);
}
