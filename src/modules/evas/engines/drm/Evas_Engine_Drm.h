#ifndef _EVAS_ENGINE_DRM_H
# define _EVAS_ENGINE_DRM_H

# include <Ecore_Drm.h>

typedef struct _Evas_Engine_Info_Drm Evas_Engine_Info_Drm;

struct _Evas_Engine_Info_Drm
{
   /* PRIVATE - don't mess with this baby or evas will poke its tongue out */
   /* at you and make nasty noises */
   Evas_Engine_Info magic;

   struct
     {
        unsigned int rotation, depth;
        Eina_Bool destination_alpha : 1;
        Eina_Bool vsync : 1;

        unsigned int crtc_id, conn_id, buffer_id;

        Eina_Bool use_hw_accel : 1;
        Ecore_Drm_Device *dev;
     } info;

   /* non-blocking or blocking mode */
   Evas_Engine_Render_Mode render_mode;
};

#endif
