/* TODO: List of missing functions
 *
 * ecore_x_randr_crtc_clone_set
 * ecore_x_randr_output_crtc_set
 * ecore_x_randr_edid_version_get
 * ecore_x_randr_edid_info_has_valid_checksum
 * ecore_x_randr_edid_manufacturer_name_get
 * ecore_x_randr_edid_display_ascii_get
 * ecore_x_randr_edid_display_serial_get
 * ecore_x_randr_edid_model_get
 * ecore_x_randr_edid_manufacturer_serial_number_get
 * ecore_x_randr_edid_manufacturer_model_get
 * ecore_x_randr_edid_dpms_available_get
 * ecore_x_randr_edid_dpms_standby_available_get
 * ecore_x_randr_edid_dpms_suspend_available_get
 * ecore_x_randr_edid_dpms_off_available_get
 * ecore_x_randr_edid_display_aspect_ratio_preferred_get
 * ecore_x_randr_edid_display_aspect_ratios_get
 * ecore_x_randr_edid_display_colorscheme_get
 * ecore_x_randr_edid_display_type_digital_get
 * ecore_x_randr_edid_display_interface_type_get
 * ecore_x_randr_screen_backlight_level_set
 * ecore_x_randr_output_subpixel_order_get
 * ecore_x_randr_output_wired_clones_get
 * ecore_x_randr_output_compatibility_list_get
 * ecore_x_randr_output_signal_formats_get
 * ecore_x_randr_output_signal_format_set
 * ecore_x_randr_output_signal_properties_get
 * ecore_x_randr_output_connector_number_get
 * ecore_x_randr_output_connector_type_get
 * ecore_x_randr_crtc_panning_area_get
 * ecore_x_randr_crtc_panning_area_set
 * ecore_x_randr_crtc_tracking_area_get
 * ecore_x_randr_crtc_tracking_area_set
 * ecore_x_randr_crtc_border_area_get
 * ecore_x_randr_crtc_border_area_set
 */

#include "ecore_xcb_private.h"
# ifdef ECORE_XCB_RANDR
#  include <xcb/randr.h>
# endif

#define Ecore_X_Randr_None  0
#define Ecore_X_Randr_Unset -1

#define RANDR_1_1           ((1 << 16) | 1)
#define RANDR_1_2           ((1 << 16) | 2)
#define RANDR_1_3           ((1 << 16) | 3)

#define RANDR_CHECK_1_1_RET(ret) if (_randr_version < RANDR_1_1) return ret
#define RANDR_CHECK_1_2_RET(ret) if (_randr_version < RANDR_1_2) return ret
#define RANDR_CHECK_1_3_RET(ret) if (_randr_version < RANDR_1_3) return ret

#define ECORE_X_RANDR_EDID_VERSION_13 ((1 << 8) | 3)
#define _ECORE_X_RANDR_EDID_OFFSET_VERSION_MAJOR 0x12
#define _ECORE_X_RANDR_EDID_OFFSET_VERSION_MINOR 0x13
#define _ECORE_X_RANDR_EDID_OFFSET_DESCRIPTOR_BLOCK 0x36
#define _ECORE_X_RANDR_EDID_OFFSET_DESCRIPTOR_BLOCK_TYPE 3
#define _ECORE_X_RANDR_EDID_OFFSET_DESCRIPTOR_BLOCK_CONTENT 5
#define _ECORE_X_RANDR_EDID_DISPLAY_DESCRIPTOR_BLOCK_CONTENT_LENGTH_MAX 13

#define _ECORE_X_RANDR_EDID_FOR_EACH_DESCRIPTOR_BLOCK(edid, block) \
  for (block = edid + _ECORE_X_RANDR_EDID_OFFSET_DESCRIPTOR_BLOCK; block <= (edid + _ECORE_X_RANDR_EDID_OFFSET_DESCRIPTOR_BLOCK + (3 * 18)); block += 18)

#define _ECORE_X_RANDR_EDID_FOR_EACH_NON_PIXEL_DESCRIPTOR_BLOCK(edid, block) \
  _ECORE_X_RANDR_EDID_FOR_EACH_DESCRIPTOR_BLOCK(edid, block)                 \
  if ((block[0] == 0) && (block[1] == 0))

/* local function prototypes */
static Eina_Bool                                       _ecore_xcb_randr_output_validate(Ecore_X_Window       root,
                                                                                        Ecore_X_Randr_Output output);
static Eina_Bool                                       _ecore_xcb_randr_crtc_validate(Ecore_X_Window     root,
                                                                                      Ecore_X_Randr_Crtc crtc);
static Eina_Bool                                       _ecore_xcb_randr_root_validate(Ecore_X_Window root);
static int                                             _ecore_xcb_randr_root_to_screen(Ecore_X_Window root);
#ifdef ECORE_XCB_RANDR
static xcb_randr_get_screen_resources_reply_t         *_ecore_xcb_randr_12_get_resources(Ecore_X_Window win);
static xcb_randr_get_screen_resources_current_reply_t *_ecore_xcb_randr_13_get_resources(Ecore_X_Window win);
#endif
static xcb_timestamp_t                                 _ecore_xcb_randr_12_get_resource_timestamp(Ecore_X_Window win);
static xcb_timestamp_t                                 _ecore_xcb_randr_13_get_resource_timestamp(Ecore_X_Window win);

static Ecore_X_Randr_Mode                             *_ecore_xcb_randr_12_output_modes_get(Ecore_X_Window       root,
                                                                                            Ecore_X_Randr_Output output,
                                                                                            int                 *num,
                                                                                            int                 *npreferred);
static Ecore_X_Randr_Mode *_ecore_xcb_randr_13_output_modes_get(Ecore_X_Window       root,
                                                                Ecore_X_Randr_Output output,
                                                                int                 *num,
                                                                int                 *npreferred);
static Ecore_X_Randr_Mode_Info  *_ecore_xcb_randr_12_mode_info_get(Ecore_X_Window     root,
                                                                   Ecore_X_Randr_Mode mode);
static Ecore_X_Randr_Mode_Info  *_ecore_xcb_randr_13_mode_info_get(Ecore_X_Window     root,
                                                                   Ecore_X_Randr_Mode mode);
static Ecore_X_Randr_Mode_Info **_ecore_xcb_randr_12_modes_info_get(Ecore_X_Window root,
                                                                    int           *num);
static Ecore_X_Randr_Mode_Info **_ecore_xcb_randr_13_modes_info_get(Ecore_X_Window root,
                                                                    int           *num);
static void                      _ecore_xcb_randr_12_mode_size_get(Ecore_X_Window     root,
                                                                   Ecore_X_Randr_Mode mode,
                                                                   int               *w,
                                                                   int               *h);
static void _ecore_xcb_randr_13_mode_size_get(Ecore_X_Window     root,
                                              Ecore_X_Randr_Mode mode,
                                              int               *w,
                                              int               *h);
static Ecore_X_Randr_Output *_ecore_xcb_randr_12_output_clones_get(Ecore_X_Window       root,
                                                                   Ecore_X_Randr_Output output,
                                                                   int                 *num);
static Ecore_X_Randr_Output *_ecore_xcb_randr_13_output_clones_get(Ecore_X_Window       root,
                                                                   Ecore_X_Randr_Output output,
                                                                   int                 *num);
static Ecore_X_Randr_Crtc *_ecore_xcb_randr_12_output_possible_crtcs_get(Ecore_X_Window       root,
                                                                         Ecore_X_Randr_Output output,
                                                                         int                 *num);
static Ecore_X_Randr_Crtc *_ecore_xcb_randr_13_output_possible_crtcs_get(Ecore_X_Window       root,
                                                                         Ecore_X_Randr_Output output,
                                                                         int                 *num);
static char *_ecore_xcb_randr_12_output_name_get(Ecore_X_Window       root,
                                                 Ecore_X_Randr_Output output,
                                                 int                 *len);
static char *_ecore_xcb_randr_13_output_name_get(Ecore_X_Window       root,
                                                 Ecore_X_Randr_Output output,
                                                 int                 *len);
static Ecore_X_Randr_Connection_Status _ecore_xcb_randr_12_output_connection_status_get(Ecore_X_Window       root,
                                                                                        Ecore_X_Randr_Output output);
static Ecore_X_Randr_Connection_Status _ecore_xcb_randr_13_output_connection_status_get(Ecore_X_Window       root,
                                                                                        Ecore_X_Randr_Output output);
static Ecore_X_Randr_Output           *_ecore_xcb_randr_12_outputs_get(Ecore_X_Window root,
                                                                       int           *num);
static Ecore_X_Randr_Output           *_ecore_xcb_randr_13_outputs_get(Ecore_X_Window root,
                                                                       int           *num);
static Ecore_X_Randr_Crtc              _ecore_xcb_randr_12_output_crtc_get(Ecore_X_Window       root,
                                                                           Ecore_X_Randr_Output output);
static Ecore_X_Randr_Crtc              _ecore_xcb_randr_13_output_crtc_get(Ecore_X_Window       root,
                                                                           Ecore_X_Randr_Output output);

/* local variables */
static Eina_Bool _randr_avail = EINA_FALSE;
static int _randr_version = -1;

/* external variables */
int _ecore_xcb_event_randr = -1;

void
_ecore_xcb_randr_init(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

#ifdef ECORE_XCB_RANDR
   xcb_prefetch_extension_data(_ecore_xcb_conn, &xcb_randr_id);
#endif
}

void
_ecore_xcb_randr_finalize(void)
{
#ifdef ECORE_XCB_RANDR
   const xcb_query_extension_reply_t *ext_reply;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

#ifdef ECORE_XCB_RANDR
   ext_reply = xcb_get_extension_data(_ecore_xcb_conn, &xcb_randr_id);
   if ((ext_reply) && (ext_reply->present))
     {
        xcb_randr_query_version_cookie_t cookie;
        xcb_randr_query_version_reply_t *reply;

        cookie =
          xcb_randr_query_version_unchecked(_ecore_xcb_conn,
                                            XCB_RANDR_MAJOR_VERSION,
                                            XCB_RANDR_MINOR_VERSION);
        reply = xcb_randr_query_version_reply(_ecore_xcb_conn, cookie, NULL);
        if (reply)
          {
             if ((reply->major_version >= XCB_RANDR_MAJOR_VERSION) &&
                 (reply->minor_version >= XCB_RANDR_MINOR_VERSION))
               _randr_avail = EINA_TRUE;

             _randr_version =
               ((reply->major_version << 16) | reply->minor_version);

             free(reply);
          }

        if (_randr_avail)
          _ecore_xcb_event_randr = ext_reply->first_event;
     }
#endif
}

static Eina_Bool
#ifdef ECORE_XCB_RANDR
_ecore_xcb_randr_root_validate(Ecore_X_Window root)
#else
_ecore_xcb_randr_root_validate(Ecore_X_Window root EINA_UNUSED)
#endif
{
#ifdef ECORE_XCB_RANDR
   Ecore_X_Randr_Screen scr = -1;
# define RANDR_VALIDATE_ROOT(screen, root) \
  ((screen == _ecore_xcb_randr_root_to_screen(root)) != -1)
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

#ifdef ECORE_XCB_RANDR
   if ((root) && RANDR_VALIDATE_ROOT(scr, root))
     return EINA_TRUE;
#endif

   return EINA_FALSE;
}

static int
_ecore_xcb_randr_root_to_screen(Ecore_X_Window root)
{
   int count = 0, num = 0;

   CHECK_XCB_CONN;

   count = xcb_setup_roots_length(xcb_get_setup(_ecore_xcb_conn));
   for (num = 0; num < count; num++)
     if (_ecore_xcb_window_root_of_screen_get(num) == root)
       return num;

   return -1;
}

/* public functions */

/*
 * @brief Query whether RandR is available or not.
 *
 * @return @c EINA_TRUE if extension is available, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_x_randr_query(void)
{
   return _randr_avail;
}

/*
 * @return version of the RandRR extension supported by the server or,
 * in case RandRR extension is not available, Ecore_X_Randr_Unset (=-1).
 * bit version information: 31   MAJOR   16 | 15   MINOR   0
 */
EAPI int
ecore_x_randr_version_get(void)
{
   return _randr_version;
}

/**
 * @brief This function returns the current config timestamp from 
 * XRRScreenConfiguration.
 * 
 * @param root root window to query screen configuration from
 * 
 * @returns The screen configuration timestamp
 * 
 * @since 1.8
 */
EAPI Ecore_X_Time 
ecore_x_randr_config_timestamp_get(Ecore_X_Window root)
{
   Ecore_X_Time timestamp = 0;

#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_info_cookie_t cookie;
   xcb_randr_get_screen_info_reply_t *reply;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   cookie = xcb_randr_get_screen_info_unchecked(_ecore_xcb_conn, root);
   reply = xcb_randr_get_screen_info_reply(_ecore_xcb_conn, cookie, NULL);
   if (reply)
     {
        timestamp = (Ecore_X_Time)reply->config_timestamp;
        free(reply);
     }
#endif

   return timestamp;
}

/*
 * @param root window which's primary output will be queried
 */
EAPI Ecore_X_Randr_Orientation
ecore_x_randr_screen_primary_output_orientations_get(Ecore_X_Window root)
{
   int ret = Ecore_X_Randr_None;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_info_cookie_t cookie;
   xcb_randr_get_screen_info_reply_t *reply;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   cookie = xcb_randr_get_screen_info_unchecked(_ecore_xcb_conn, root);
   reply = xcb_randr_get_screen_info_reply(_ecore_xcb_conn, cookie, NULL);
   if (reply)
     {
        ret = reply->rotations;
        free(reply);
     }
#endif

   return ret;
}

/*
 * @param root window which's primary output will be queried
 * @return the current orientation of the root window's screen primary output
 */
EAPI Ecore_X_Randr_Orientation
ecore_x_randr_screen_primary_output_orientation_get(Ecore_X_Window root)
{
   int ret = Ecore_X_Randr_None;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_info_cookie_t cookie;
   xcb_randr_get_screen_info_reply_t *reply;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   cookie = xcb_randr_get_screen_info_unchecked(_ecore_xcb_conn, root);
   reply = xcb_randr_get_screen_info_reply(_ecore_xcb_conn, cookie, NULL);
   if (reply)
     {
        ret = reply->rotation;
        free(reply);
     }
#endif

   return ret;
}

/*
 * @brief Sets a given screen's primary output's orientation.
 *
 * @param root Window which's screen's primary output will be queried.
 * @param orientation Orientation which should be set for the root window's
 * screen primary output.
 * @return @c EINA_TRUE if the primary output's orientation could be
 * successfully altered.
 */
EAPI Eina_Bool
ecore_x_randr_screen_primary_output_orientation_set(Ecore_X_Window            root,
                                                    Ecore_X_Randr_Orientation orientation)
{
   int ret = EINA_FALSE;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_info_cookie_t cookie;
   xcb_randr_get_screen_info_reply_t *reply;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   cookie = xcb_randr_get_screen_info_unchecked(_ecore_xcb_conn, root);
   reply = xcb_randr_get_screen_info_reply(_ecore_xcb_conn, cookie, NULL);
   if (reply)
     {
        xcb_randr_set_screen_config_cookie_t scookie;
        xcb_randr_set_screen_config_reply_t *sreply;

        scookie =
          xcb_randr_set_screen_config_unchecked(_ecore_xcb_conn, root,
                                                XCB_CURRENT_TIME,
                                                reply->config_timestamp,
                                                reply->sizeID, orientation,
                                                reply->rate);
        sreply =
          xcb_randr_set_screen_config_reply(_ecore_xcb_conn, scookie, NULL);
        if (!sreply)
          ret = EINA_FALSE;
        else
          {
             ret = (sreply->status == XCB_RANDR_SET_CONFIG_SUCCESS) ?
               EINA_TRUE : EINA_FALSE;
             free(sreply);
          }
        free(reply);
     }
#endif

   return ret;
}

/*
 * @brief gets a screen's primary output's possible sizes
 * @param root window which's primary output will be queried
 * @param num number of sizes reported as supported by the screen's primary output
 * @return an array of sizes reported as supported by the screen's primary output or - if query failed - NULL
 */
EAPI Ecore_X_Randr_Screen_Size_MM *
ecore_x_randr_screen_primary_output_sizes_get(Ecore_X_Window root,
                                              int           *num)
{
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_info_cookie_t cookie;
   xcb_randr_get_screen_info_reply_t *reply;
   Ecore_X_Randr_Screen_Size_MM *ret = NULL;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   cookie = xcb_randr_get_screen_info_unchecked(_ecore_xcb_conn, root);
   reply = xcb_randr_get_screen_info_reply(_ecore_xcb_conn, cookie, NULL);
   if (reply)
     {
        int len = 0, i = 0;
        xcb_randr_screen_size_t *sizes;

        len = xcb_randr_get_screen_info_sizes_length(reply);
        sizes = xcb_randr_get_screen_info_sizes(reply);
        if ((!sizes) || (len <= 0))
          {
             free(reply);
             return NULL;
          }
        if (num) *num = len;
        ret = calloc(len, sizeof(Ecore_X_Randr_Screen_Size_MM));
        if (!ret)
          {
             free(reply);
             return NULL;
          }
        for (i = 0; i < len; i++)
          {
             ret[i].width = sizes[i].width;
             ret[i].height = sizes[i].height;
             ret[i].width_mm = sizes[i].mwidth;
             ret[i].height_mm = sizes[i].mheight;
          }

        free(reply);
     }

   return ret;
#else
   return NULL;
#endif
}

/*
 * @brief get the current set size of a given screen's primary output
 * @param root window which's primary output will be queried
 * @param w the current size's width
 * @param h the current size's height
 * @param w_mm the current size's width in mm
 * @param h_mm the current size's height in mm
 * @param size_index of current set size to be used with ecore_x_randr_primary_output_size_set()
 */
EAPI void
ecore_x_randr_screen_primary_output_current_size_get(Ecore_X_Window root,
                                                     int           *w,
                                                     int           *h,
                                                     int           *w_mm,
                                                     int           *h_mm,
                                                     int           *size_index)
{
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_info_cookie_t cookie;
   xcb_randr_get_screen_info_reply_t *reply;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   cookie = xcb_randr_get_screen_info_unchecked(_ecore_xcb_conn, root);
   reply = xcb_randr_get_screen_info_reply(_ecore_xcb_conn, cookie, NULL);
   if (reply)
     {
        int len = 0, idx = 0;
        xcb_randr_screen_size_t *sizes;

        len = xcb_randr_get_screen_info_sizes_length(reply);
        sizes = xcb_randr_get_screen_info_sizes(reply);
        if ((!sizes) || (len <= 0))
          {
             free(reply);
             return;
          }
        idx = reply->sizeID;
        if ((idx < len) && (idx >= 0))
          {
             if (w) *w = sizes[idx].width;
             if (h) *h = sizes[idx].height;
             if (w_mm) *w_mm = sizes[idx].mwidth;
             if (h_mm) *h_mm = sizes[idx].mheight;
             if (size_index) *size_index = idx;
          }

        free(reply);
     }
#endif
}

/*
 * @brief Sets a given screen's primary output size, but disables all other
 * outputs at the same time.
 *
 * @param root Window which's primary output will be queried.
 * @param size_index Within the list of sizes reported as supported by the root
 * window's screen primary output.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure due to e.g.
 * invalid times.
 */
EAPI Eina_Bool
ecore_x_randr_screen_primary_output_size_set(Ecore_X_Window root,
                                             int            size_index)
{
   Eina_Bool ret = EINA_FALSE;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_info_cookie_t cookie;
   xcb_randr_get_screen_info_reply_t *reply;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   if (!((size_index >= 0) && (_ecore_xcb_randr_root_validate(root))))
     return EINA_FALSE;

   cookie = xcb_randr_get_screen_info_unchecked(_ecore_xcb_conn, root);
   reply = xcb_randr_get_screen_info_reply(_ecore_xcb_conn, cookie, NULL);
   if (reply)
     {
        int len = 0;

        len = xcb_randr_get_screen_info_sizes_length(reply);
        if (len <= 0)
          {
             free(reply);
             return EINA_FALSE;
          }
        if ((size_index < len) && (size_index >= 0))
          {
             xcb_randr_set_screen_config_cookie_t scookie;
             xcb_randr_set_screen_config_reply_t *sreply;

             scookie =
               xcb_randr_set_screen_config_unchecked(_ecore_xcb_conn, root,
                                                     XCB_CURRENT_TIME,
                                                     reply->config_timestamp,
                                                     size_index,
                                                     reply->rotation,
                                                     reply->rate);
             sreply =
               xcb_randr_set_screen_config_reply(_ecore_xcb_conn,
                                                 scookie, NULL);
             if (!sreply)
               ret = EINA_FALSE;
             else
               {
                  ret = (sreply->status == XCB_RANDR_SET_CONFIG_SUCCESS) ?
                    EINA_TRUE : EINA_FALSE;
                  free(sreply);
               }
          }

        free(reply);
     }
#endif
   return ret;
}

/*
 * @param root window which's primary output will be queried
 * @return currently used refresh rate or - if request failed or RandRR is not available - 0.0
 */
EAPI Ecore_X_Randr_Refresh_Rate
ecore_x_randr_screen_primary_output_current_refresh_rate_get(Ecore_X_Window root)
{
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_info_cookie_t cookie;
   xcb_randr_get_screen_info_reply_t *reply;
   Ecore_X_Randr_Refresh_Rate ret = 0.0;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   if (!_ecore_xcb_randr_root_validate(root)) return ret;

   cookie = xcb_randr_get_screen_info_unchecked(_ecore_xcb_conn, root);
   reply = xcb_randr_get_screen_info_reply(_ecore_xcb_conn, cookie, NULL);
   if (reply)
     {
        ret = reply->rate;
        free(reply);
     }

   return ret;
#else
   return 0.0;
#endif
}

/*
 * @param root window which's primary output will be queried
 * @param size_index referencing the size to query valid refresh rates for
 * @return currently used refresh rate or - if request failed or RandRR is not available - NULL
 */
EAPI Ecore_X_Randr_Refresh_Rate *
ecore_x_randr_screen_primary_output_refresh_rates_get(Ecore_X_Window root,
                                                      int            size_index,
                                                      int           *num)
{
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_info_cookie_t cookie;
   xcb_randr_get_screen_info_reply_t *reply;
   Ecore_X_Randr_Refresh_Rate *ret = NULL;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   if (!_ecore_xcb_randr_root_validate(root)) return ret;

   cookie = xcb_randr_get_screen_info_unchecked(_ecore_xcb_conn, root);
   reply = xcb_randr_get_screen_info_reply(_ecore_xcb_conn, cookie, NULL);
   if (reply)
     {
        int len = 0;

        len = xcb_randr_get_screen_info_rates_length(reply);
        if (num) *num = len;

        ret = malloc(sizeof(Ecore_X_Randr_Refresh_Rate) * len);
        if (ret)
          {
             xcb_randr_refresh_rates_iterator_t iter;
             int i = 0;

             iter = xcb_randr_get_screen_info_rates_iterator(reply);
             while (i++ < size_index)
               xcb_randr_refresh_rates_next(&iter);

             memcpy(ret, xcb_randr_refresh_rates_rates(iter.data),
                    sizeof(Ecore_X_Randr_Refresh_Rate) * len);
          }
        free(reply);
     }

   return ret;
#else
   return NULL;
#endif
}

/*
 * @brief Sets the current primary output's refresh rate.
 *
 * @param root Window which's primary output will be queried.
 * @param size_index Referencing the size to be set.
 * @param rate The refresh rate to be set.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_x_randr_screen_primary_output_refresh_rate_set(Ecore_X_Window             root,
                                                     int                        size_index,
                                                     Ecore_X_Randr_Refresh_Rate rate)
{
   Eina_Bool ret = EINA_FALSE;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_info_cookie_t cookie;
   xcb_randr_get_screen_info_reply_t *reply;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   if (_randr_version < RANDR_1_1) return EINA_FALSE;

   cookie = xcb_randr_get_screen_info_unchecked(_ecore_xcb_conn, root);
   reply = xcb_randr_get_screen_info_reply(_ecore_xcb_conn, cookie, NULL);
   if (reply)
     {
        xcb_randr_set_screen_config_cookie_t scookie;
        xcb_randr_set_screen_config_reply_t *sreply;

        scookie =
          xcb_randr_set_screen_config_unchecked(_ecore_xcb_conn, root,
                                                XCB_CURRENT_TIME,
                                                reply->config_timestamp,
                                                size_index,
                                                reply->rotation, rate);
        sreply =
          xcb_randr_set_screen_config_reply(_ecore_xcb_conn,
                                            scookie, NULL);
        if (!sreply)
          ret = EINA_FALSE;
        else
          {
             ret = (sreply->status == XCB_RANDR_SET_CONFIG_SUCCESS) ?
               EINA_TRUE : EINA_FALSE;
             free(sreply);
          }
        free(reply);
     }
#endif

   return ret;
}

/*
 * @brief Free detailed mode information. The pointer handed in will be set to
 * @c NULL after freeing the memory.
 *
 * @param mode_info The mode information that should be freed.
 */
EAPI void
ecore_x_randr_mode_info_free(Ecore_X_Randr_Mode_Info *mode_info)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

   RANDR_CHECK_1_2_RET();

   if (!mode_info) return;

   if (mode_info->name) free(mode_info->name);
   free(mode_info);
   mode_info = NULL;
}

/*
 * @param root window which's screen should be queried
 * @return Ecore_X_Randr_Ouptut_Id or - if query failed or none is set - Ecore_X_Randr_None
 */
EAPI Ecore_X_Randr_Output
ecore_x_randr_primary_output_get(Ecore_X_Window root)
{
   Ecore_X_Randr_Output ret = Ecore_X_Randr_None;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_output_primary_cookie_t cookie;
   xcb_randr_get_output_primary_reply_t *reply;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_3_RET(Ecore_X_Randr_None);

   if (!_ecore_xcb_randr_root_validate(root))
     return Ecore_X_Randr_None;

   cookie = xcb_randr_get_output_primary_unchecked(_ecore_xcb_conn, root);
   reply = xcb_randr_get_output_primary_reply(_ecore_xcb_conn, cookie, NULL);
   if (reply)
     {
        ret = reply->output;
        free(reply);
     }
#endif
   return ret;
}

/*
 * @param root window which's screen should be queried
 * @param output that should be set as given root window's screen primary output
 */
EAPI void
ecore_x_randr_primary_output_set(Ecore_X_Window       root,
                                 Ecore_X_Randr_Output output)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_3_RET();

   if ((output) && (_ecore_xcb_randr_root_validate(root)))
     xcb_randr_set_output_primary(_ecore_xcb_conn, root, output);
#endif
}

EAPI Ecore_X_Randr_Mode *
ecore_x_randr_output_modes_get(Ecore_X_Window       root,
                               Ecore_X_Randr_Output output,
                               int                 *num,
                               int                 *npreferred)
{
   Ecore_X_Randr_Mode *modes = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(NULL);

   if (_randr_version >= RANDR_1_3)
     {
        modes =
          _ecore_xcb_randr_13_output_modes_get(root, output, num, npreferred);
     }
   else if (_randr_version == RANDR_1_2)
     {
        modes =
          _ecore_xcb_randr_12_output_modes_get(root, output, num, npreferred);
     }
#endif

   return modes;
}

EAPI Eina_Bool 
ecore_x_randr_output_mode_add(Ecore_X_Randr_Output output, Ecore_X_Randr_Mode mode)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(EINA_FALSE);

   if ((output == Ecore_X_Randr_None) || (mode == Ecore_X_Randr_None))
     return EINA_FALSE;

   xcb_randr_add_output_mode(_ecore_xcb_conn, output, mode);
   return EINA_TRUE;
#endif
   return EINA_FALSE;
}

/*
 * @brief get detailed information for a given mode id
 * @param root window which's screen's ressources are queried
 * @param mode the XID which identifies the mode of interest
 * @return mode's detailed information
 */
EAPI Ecore_X_Randr_Mode_Info *
ecore_x_randr_mode_info_get(Ecore_X_Window     root,
                            Ecore_X_Randr_Mode mode)
{
   Ecore_X_Randr_Mode_Info *ret = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(NULL);

   if (!_ecore_xcb_randr_root_validate(root)) return NULL;

   if (_randr_version >= RANDR_1_3)
     ret = _ecore_xcb_randr_13_mode_info_get(root, mode);
   else if (_randr_version == RANDR_1_2)
     ret = _ecore_xcb_randr_12_mode_info_get(root, mode);
#endif
   return ret;
}

/*
 * @brief add a mode to a display
 * @param root window to which's screen's ressources are added
 * @param mode_info
 * @return Ecore_X_Randr_Mode of the added mode. Ecore_X_Randr_None if mode
 * adding failed.
 * @since 1.2.0
 */
EAPI Ecore_X_Randr_Mode 
ecore_x_randr_mode_info_add(Ecore_X_Window root, Ecore_X_Randr_Mode_Info *mode_info)
{
   Ecore_X_Randr_Mode mode = Ecore_X_Randr_None;
#ifdef ECORE_XCB_RANDR
   xcb_randr_create_mode_cookie_t cookie;
   xcb_randr_create_mode_reply_t *reply;
   xcb_randr_mode_info_t info;
   int namelen = 0;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(EINA_FALSE);

   if (!mode_info) return Ecore_X_Randr_None;
   if (!_ecore_xcb_randr_root_validate(root)) return Ecore_X_Randr_None;

   namelen = strlen(mode_info->name);

   memset(&info, 0, sizeof(info));
   info.width = mode_info->width;
   info.height = mode_info->height;
   info.dot_clock = mode_info->dotClock;
   info.hsync_start = mode_info->hSyncStart;
   info.hsync_end = mode_info->hSyncEnd;
   info.htotal = mode_info->hTotal;
   info.hskew = mode_info->hSkew;
   info.vsync_start = mode_info->vSyncStart;
   info.vsync_end = mode_info->vSyncEnd;
   info.vtotal = mode_info->vTotal;
   info.mode_flags = mode_info->modeFlags;
   info.name_len = namelen;

   cookie = 
     xcb_randr_create_mode_unchecked(_ecore_xcb_conn, root, info, 
                                     namelen, mode_info->name);
   reply = xcb_randr_create_mode_reply(_ecore_xcb_conn, cookie, NULL);
   if (reply)
     {
        mode = mode_info->xid;
        free(reply);
     }
#endif
   return mode;
}

/*
 * @brief get detailed information for all modes related to a root window's screen
 * @param root window which's screen's ressources are queried
 * @param num number of modes returned
 * @return modes' information
 */
EAPI Ecore_X_Randr_Mode_Info **
ecore_x_randr_modes_info_get(Ecore_X_Window root,
                             int           *num)
{
   Ecore_X_Randr_Mode_Info **ret = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

   if (num) *num = 0;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(NULL);

   if (!_ecore_xcb_randr_root_validate(root)) return NULL;

   if (_randr_version >= RANDR_1_3)
     ret = _ecore_xcb_randr_13_modes_info_get(root, num);
   else if (_randr_version == RANDR_1_2)
     ret = _ecore_xcb_randr_12_modes_info_get(root, num);
#endif
   return ret;
}

/**
 * @brief Gets the width and hight of a given mode.
 *
 * @param root Window which's screen's ressources are queried.
 * @param mode The mode which's size is to be looked up.
 * @param w Width of given mode in px.
 * @param h Height of given mode in px.
 */
EAPI void
ecore_x_randr_mode_size_get(Ecore_X_Window     root,
                            Ecore_X_Randr_Mode mode,
                            int               *w,
                            int               *h)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET();

   if (mode == Ecore_X_Randr_None) return;

   if (_randr_version >= RANDR_1_3)
     _ecore_xcb_randr_13_mode_size_get(root, mode, w, h);
   else if (_randr_version == RANDR_1_2)
     _ecore_xcb_randr_12_mode_size_get(root, mode, w, h);
#endif
}

/**
 * @brief Gets the EDID information of an attached output if available.
 * Note that this information is not to be compared using ordinary string
 * comparison functions, since it includes 0-bytes.
 *
 * @param root Window this information should be queried from.
 * @param output The XID of the output.
 * @param length Length of the byte-array. If @c NULL, request will fail.
 * @return EDID information of the output.
 */
EAPI unsigned char *
ecore_x_randr_output_edid_get(Ecore_X_Window       root,
                              Ecore_X_Randr_Output output,
                              unsigned long       *length)
{
   unsigned char *ret = NULL;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_output_property_cookie_t cookie;
   xcb_randr_get_output_property_reply_t *reply;
   Ecore_X_Atom atom;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(NULL);

   if ((!length) || (!_ecore_xcb_randr_output_validate(root, output)))
     return NULL;

   atom = ecore_x_atom_get("EDID");
   cookie =
     xcb_randr_get_output_property_unchecked(_ecore_xcb_conn, output, atom,
                                             XCB_GET_PROPERTY_TYPE_ANY,
                                             0, 100, 0, 0);
   reply =
     xcb_randr_get_output_property_reply(_ecore_xcb_conn, cookie, NULL);
   if (reply)
     {
        if ((reply->type == XCB_ATOM_INTEGER) && (reply->format == 8))
          {
             if (length) *length = reply->num_items;
             if ((ret = malloc(reply->num_items * sizeof(unsigned char))))
               {
                  memcpy(ret, xcb_randr_get_output_property_data(reply),
                         (reply->num_items * sizeof(unsigned char)));
               }
          }
        free(reply);
     }
#endif
   return ret;
}

/**
 * @brief Gets the outputs which might be used simultaneously on the same CRTC.
 *
 * @param root Window that this information should be queried for.
 * @param output The output which's clones we concern.
 * @param num Number of possible clones.
 * @return The existing outputs, @c NULL otherwise.
 */
EAPI Ecore_X_Randr_Output *
ecore_x_randr_output_clones_get(Ecore_X_Window       root,
                                Ecore_X_Randr_Output output,
                                int                 *num)
{
   Ecore_X_Randr_Output *outputs = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(NULL);

   if (output == Ecore_X_Randr_None) return NULL;

   if (_randr_version >= RANDR_1_3)
     outputs = _ecore_xcb_randr_13_output_clones_get(root, output, num);
   else if (_randr_version == RANDR_1_2)
     outputs = _ecore_xcb_randr_12_output_clones_get(root, output, num);
#endif
   return outputs;
}

EAPI Ecore_X_Randr_Crtc *
ecore_x_randr_output_possible_crtcs_get(Ecore_X_Window       root,
                                        Ecore_X_Randr_Output output,
                                        int                 *num)
{
   Ecore_X_Randr_Crtc *crtcs = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(NULL);

   if (output == Ecore_X_Randr_None) return NULL;

   if (_randr_version >= RANDR_1_3)
     crtcs = _ecore_xcb_randr_13_output_possible_crtcs_get(root, output, num);
   else if (_randr_version == RANDR_1_2)
     crtcs = _ecore_xcb_randr_12_output_possible_crtcs_get(root, output, num);
#endif
   return crtcs;
}

/**
 * @brief gets the given output's name as reported by X
 * @param root the window which's screen will be queried
 * @param output The output name given to be reported.
 * @param len length of returned c-string.
 * @return name of the output as reported by X
 */
EAPI char *
ecore_x_randr_output_name_get(Ecore_X_Window       root,
                              Ecore_X_Randr_Output output,
                              int                 *len)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(NULL);

   if (output == Ecore_X_Randr_None) return NULL;

   if (_randr_version >= RANDR_1_3)
     return _ecore_xcb_randr_13_output_name_get(root, output, len);
   else if (_randr_version == RANDR_1_2)
     return _ecore_xcb_randr_12_output_name_get(root, output, len);
#endif

   return NULL;
}

EAPI Ecore_X_Randr_Connection_Status
ecore_x_randr_output_connection_status_get(Ecore_X_Window       root,
                                           Ecore_X_Randr_Output output)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(ECORE_X_RANDR_CONNECTION_STATUS_UNKNOWN);

   if (output == Ecore_X_Randr_None)
     return ECORE_X_RANDR_CONNECTION_STATUS_UNKNOWN;

   if (_randr_version >= RANDR_1_3)
     return _ecore_xcb_randr_13_output_connection_status_get(root, output);
   else if (_randr_version == RANDR_1_2)
     return _ecore_xcb_randr_12_output_connection_status_get(root, output);
#endif

   return ECORE_X_RANDR_CONNECTION_STATUS_UNKNOWN;
}

EAPI Ecore_X_Randr_Output *
ecore_x_randr_outputs_get(Ecore_X_Window root,
                          int           *num)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(NULL);

   if (_randr_version >= RANDR_1_3)
     return _ecore_xcb_randr_13_outputs_get(root, num);
   else if (_randr_version == RANDR_1_2)
     return _ecore_xcb_randr_12_outputs_get(root, num);
#endif

   return NULL;
}

EAPI Ecore_X_Randr_Crtc
ecore_x_randr_output_crtc_get(Ecore_X_Window       root,
                              Ecore_X_Randr_Output output)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(Ecore_X_Randr_None);

   if (output == Ecore_X_Randr_None) return Ecore_X_Randr_None;

   if (_randr_version >= RANDR_1_3)
     return _ecore_xcb_randr_13_output_crtc_get(root, output);
   else if (_randr_version == RANDR_1_2)
     return _ecore_xcb_randr_12_output_crtc_get(root, output);
#endif

   return Ecore_X_Randr_None;
}

EAPI void 
ecore_x_randr_output_size_mm_get(Ecore_X_Window root, Ecore_X_Randr_Output output, int *w_mm, int *h_mm)
{
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_output_info_cookie_t ocookie;
   xcb_randr_get_output_info_reply_t *oreply;
   xcb_timestamp_t timestamp = 0;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

   if (w_mm) *w_mm = 0;
   if (h_mm) *h_mm = 0;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET();

   if ((output != Ecore_X_Randr_None) && (_randr_version >= RANDR_1_3))
     {
        xcb_randr_get_screen_resources_current_reply_t *reply;

        reply = _ecore_xcb_randr_13_get_resources(root);
        timestamp = reply->config_timestamp;
        free(reply);
     }
   else if ((output != Ecore_X_Randr_None) && (_randr_version == RANDR_1_2))
     {
        xcb_randr_get_screen_resources_reply_t *reply;

        reply = _ecore_xcb_randr_12_get_resources(root);
        timestamp = reply->config_timestamp;
        free(reply);
     }

   ocookie =
     xcb_randr_get_output_info_unchecked(_ecore_xcb_conn, output, timestamp);
   oreply = xcb_randr_get_output_info_reply(_ecore_xcb_conn, ocookie, NULL);
   if (oreply)
     {
        if (w_mm) *w_mm = oreply->mm_width;
        if (h_mm) *h_mm = oreply->mm_height;
        free(oreply);
     }
#endif
}

/**
 * @brief Sets the demanded parameters for a given CRTC. Note that the CRTC is
 * auto enabled in it's preferred mode, when it was disabled before.
 *
 * @param root The root window which's default display will be queried.
 * @param crtc The CRTC which's configuration should be altered.
 * @param outputs An array of outputs, that should display this CRTC's content.
 * @param noutputs Number of outputs in the array of outputs. If set to
 * Ecore_X_Randr_Unset, current outputs and number of outputs will be used. If
 * set to Ecore_X_Randr_None, CRTC will be disabled.
 * @param x New x coordinate. If <0 (e.g. Ecore_X_Randr_Unset) the current x
 * coordinate will be assumed.
 * @param y New y coordinate. If <0 (e.g. Ecore_X_Randr_Unset) the current y
 * coordinate will be assumed.
 * @param mode The new mode to be set. If Ecore_X_Randr_None is passed, the
 * CRTC will be disabled. If Ecore_X_Randr_Unset is passed, the current mode is
 * assumed.
 * @param orientation The new orientation to be set. If Ecore_X_Randr_Unset is
 * used, the current mode is assumed.
 * @return @c EINA_TRUE if the configuration alteration was successful,
 * @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_x_randr_crtc_settings_set(Ecore_X_Window            root,
                                Ecore_X_Randr_Crtc        crtc,
                                Ecore_X_Randr_Output     *outputs,
                                int                       noutputs,
                                int                       x,
                                int                       y,
                                Ecore_X_Randr_Mode        mode,
                                Ecore_X_Randr_Orientation orientation)
{
   Eina_Bool ret = EINA_FALSE;
#ifdef ECORE_XCB_RANDR
   xcb_timestamp_t stamp = 0;
   xcb_randr_get_crtc_info_cookie_t ccookie;
   xcb_randr_get_crtc_info_reply_t *creply;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(EINA_FALSE);

   if (!_ecore_xcb_randr_crtc_validate(root, crtc)) return ret;

   if (_randr_version >= RANDR_1_3)
     stamp = _ecore_xcb_randr_13_get_resource_timestamp(root);
   else if (_randr_version == RANDR_1_2)
     stamp = _ecore_xcb_randr_12_get_resource_timestamp(root);

   ccookie =
     xcb_randr_get_crtc_info_unchecked(_ecore_xcb_conn, crtc, stamp);
   creply =
     xcb_randr_get_crtc_info_reply(_ecore_xcb_conn, ccookie, NULL);
   if (creply)
     {
        xcb_randr_set_crtc_config_cookie_t scookie;
        xcb_randr_set_crtc_config_reply_t *sreply;

        if ((mode == Ecore_X_Randr_None) ||
            (noutputs == Ecore_X_Randr_None))
          {
             outputs = NULL;
             noutputs = 0;
          }
        else if (noutputs == (int)Ecore_X_Randr_Unset)
          {
             outputs = xcb_randr_get_crtc_info_outputs(creply);
             noutputs = creply->num_outputs;
          }
        if ((int)mode == Ecore_X_Randr_Unset) mode = creply->mode;
        if (x < 0) x = creply->x;
        if (y < 0) y = creply->y;
        if ((int)orientation == Ecore_X_Randr_Unset)
          orientation = creply->rotation;

        scookie =
          xcb_randr_set_crtc_config_unchecked(_ecore_xcb_conn,
                                              crtc, XCB_CURRENT_TIME, stamp,
                                              x, y, mode, orientation,
                                              noutputs, outputs);
        sreply =
          xcb_randr_set_crtc_config_reply(_ecore_xcb_conn, scookie, NULL);
        if (sreply)
          {
             ret = (sreply->status == XCB_RANDR_SET_CONFIG_SUCCESS) ?
               EINA_TRUE : EINA_FALSE;
             free(sreply);
          }
        free(creply);
     }
#endif

   return ret;
}

/**
 * @brief Sets a mode for a CRTC and the outputs attached to it.
 *
 * @param root The window's screen to be queried
 * @param crtc The CRTC which shall be set
 * @param outputs Array of outputs which have to be compatible with the mode. If
 * @c NULL CRTC will be disabled.
 * @param noutputs Number of outputs in array to be used. Use
 * Ecore_X_Randr_Unset (or @c -1) to use currently used outputs.
 * @param mode XID of the mode to be set. If set to @c 0 the CRTC will be
 * disabled. If set to @c -1 the call will fail.
 * @return @c EINA_TRUE if mode setting was successful, @c EINA_FALSE
 * otherwise.
 */
EAPI Eina_Bool
ecore_x_randr_crtc_mode_set(Ecore_X_Window        root,
                            Ecore_X_Randr_Crtc    crtc,
                            Ecore_X_Randr_Output *outputs,
                            int                   noutputs,
                            Ecore_X_Randr_Mode    mode)
{
   Eina_Bool ret = EINA_FALSE;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(EINA_FALSE);

   if ((int)mode == Ecore_X_Randr_Unset) return ret;
   ret =
     ecore_x_randr_crtc_settings_set(root, crtc, outputs, noutputs,
                                     Ecore_X_Randr_Unset, Ecore_X_Randr_Unset,
                                     mode, Ecore_X_Randr_Unset);
#endif

   return ret;
}

/**
 * @brief Get the current set mode of a given CRTC
 * @param root the window's screen to be queried
 * @param crtc the CRTC which's should be queried
 * @return currently set mode or - in case parameters are invalid -
 * Ecore_X_Randr_Unset
 */
EAPI Ecore_X_Randr_Mode
ecore_x_randr_crtc_mode_get(Ecore_X_Window     root,
                            Ecore_X_Randr_Crtc crtc)
{
   Ecore_X_Randr_Mode ret = Ecore_X_Randr_Unset;
#ifdef ECORE_XCB_RANDR
   xcb_timestamp_t stamp = 0;
   xcb_randr_get_crtc_info_cookie_t ocookie;
   xcb_randr_get_crtc_info_reply_t *oreply;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(Ecore_X_Randr_Unset);

   if (!_ecore_xcb_randr_crtc_validate(root, crtc)) return ret;

   if (_randr_version >= RANDR_1_3)
     stamp = _ecore_xcb_randr_13_get_resource_timestamp(root);
   else if (_randr_version == RANDR_1_2)
     stamp = _ecore_xcb_randr_12_get_resource_timestamp(root);

   ocookie =
     xcb_randr_get_crtc_info_unchecked(_ecore_xcb_conn, crtc, stamp);
   oreply = xcb_randr_get_crtc_info_reply(_ecore_xcb_conn, ocookie, NULL);
   if (oreply)
     {
        ret = oreply->mode;
        free(oreply);
     }
#endif

   return ret;
}

EAPI Ecore_X_Randr_Orientation
ecore_x_randr_crtc_orientation_get(Ecore_X_Window     root,
                                   Ecore_X_Randr_Crtc crtc)
{
   Ecore_X_Randr_Orientation ret = Ecore_X_Randr_None;
#ifdef ECORE_XCB_RANDR
   xcb_timestamp_t stamp = 0;
   xcb_randr_get_crtc_info_cookie_t ocookie;
   xcb_randr_get_crtc_info_reply_t *oreply;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(Ecore_X_Randr_None);

   if (!_ecore_xcb_randr_crtc_validate(root, crtc)) return ret;

   if (_randr_version >= RANDR_1_3)
     stamp = _ecore_xcb_randr_13_get_resource_timestamp(root);
   else if (_randr_version == RANDR_1_2)
     stamp = _ecore_xcb_randr_12_get_resource_timestamp(root);

   ocookie =
     xcb_randr_get_crtc_info_unchecked(_ecore_xcb_conn, crtc, stamp);
   oreply = xcb_randr_get_crtc_info_reply(_ecore_xcb_conn, ocookie, NULL);
   if (oreply)
     {
        ret = oreply->rotation;
        free(oreply);
     }
#endif

   return ret;
}

EAPI Eina_Bool
ecore_x_randr_crtc_orientation_set(Ecore_X_Window            root,
                                   Ecore_X_Randr_Crtc        crtc,
                                   Ecore_X_Randr_Orientation orientation)
{
   Eina_Bool ret = EINA_FALSE;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(EINA_FALSE);

   if (orientation != Ecore_X_Randr_None)
     {
        ret =
          ecore_x_randr_crtc_settings_set(root, crtc, NULL,
                                          Ecore_X_Randr_Unset, Ecore_X_Randr_Unset,
                                          Ecore_X_Randr_Unset, Ecore_X_Randr_Unset,
                                          orientation);
     }
#endif
   return ret;
}

EAPI Ecore_X_Randr_Orientation
ecore_x_randr_crtc_orientations_get(Ecore_X_Window     root,
                                    Ecore_X_Randr_Crtc crtc)
{
   Ecore_X_Randr_Orientation ret = Ecore_X_Randr_None;
#ifdef ECORE_XCB_RANDR
   xcb_timestamp_t stamp = 0;
   xcb_randr_get_crtc_info_cookie_t ocookie;
   xcb_randr_get_crtc_info_reply_t *oreply;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(Ecore_X_Randr_None);

   if (!_ecore_xcb_randr_crtc_validate(root, crtc)) return ret;

   if (_randr_version >= RANDR_1_3)
     stamp = _ecore_xcb_randr_13_get_resource_timestamp(root);
   else if (_randr_version == RANDR_1_2)
     stamp = _ecore_xcb_randr_12_get_resource_timestamp(root);

   ocookie =
     xcb_randr_get_crtc_info_unchecked(_ecore_xcb_conn, crtc, stamp);
   oreply =
     xcb_randr_get_crtc_info_reply(_ecore_xcb_conn, ocookie, NULL);
   if (oreply)
     {
        ret = oreply->rotations;
        free(oreply);
     }
#endif

   return ret;
}

/*
 * @brief get a CRTC's possible outputs.
 * @param root the root window which's screen will be queried
 * @param num number of possible outputs referenced by given CRTC
 */
EAPI Ecore_X_Randr_Output *
ecore_x_randr_crtc_possible_outputs_get(Ecore_X_Window     root,
                                        Ecore_X_Randr_Crtc crtc,
                                        int               *num)
{
   Ecore_X_Randr_Output *ret = NULL;
#ifdef ECORE_XCB_RANDR
   xcb_timestamp_t stamp = 0;
   xcb_randr_get_crtc_info_cookie_t ocookie;
   xcb_randr_get_crtc_info_reply_t *oreply;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(NULL);

   if (!_ecore_xcb_randr_crtc_validate(root, crtc)) return ret;

   if (_randr_version >= RANDR_1_3)
     stamp = _ecore_xcb_randr_13_get_resource_timestamp(root);
   else if (_randr_version == RANDR_1_2)
     stamp = _ecore_xcb_randr_12_get_resource_timestamp(root);

   ocookie =
     xcb_randr_get_crtc_info_unchecked(_ecore_xcb_conn, crtc, stamp);
   oreply = xcb_randr_get_crtc_info_reply(_ecore_xcb_conn, ocookie, NULL);
   if (oreply)
     {
        if (num) *num = oreply->num_possible_outputs;
        ret = malloc(sizeof(Ecore_X_Randr_Output) *
                     oreply->num_possible_outputs);
        if (ret)
          {
             memcpy(ret, xcb_randr_get_crtc_info_possible(oreply),
                    sizeof(Ecore_X_Randr_Output) *
                    oreply->num_possible_outputs);
          }
        free(oreply);
     }
#endif

   return ret;
}

/*
 * @brief get all known CRTCs related to a root window's screen
 * @param root window which's screen's ressources are queried
 * @param num number of CRTCs returned
 * @return CRTC IDs
 */
EAPI Ecore_X_Randr_Crtc *
ecore_x_randr_crtcs_get(Ecore_X_Window root,
                        int           *num)
{
   Ecore_X_Randr_Crtc *ret = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(NULL);

   if (_randr_version >= RANDR_1_3)
     {
        xcb_randr_get_screen_resources_current_reply_t *reply;

        reply = _ecore_xcb_randr_13_get_resources(root);
        if (reply)
          {
             if (num) *num = reply->num_crtcs;
             ret = malloc(sizeof(Ecore_X_Randr_Crtc) * reply->num_crtcs);
             if (ret)
               memcpy(ret, xcb_randr_get_screen_resources_current_crtcs(reply),
                      sizeof(Ecore_X_Randr_Crtc) * reply->num_crtcs);
             free(reply);
          }
     }
   else if (_randr_version == RANDR_1_2)
     {
        xcb_randr_get_screen_resources_reply_t *reply;

        reply = _ecore_xcb_randr_12_get_resources(root);
        if (reply)
          {
             if (num) *num = reply->num_crtcs;
             ret = malloc(sizeof(Ecore_X_Randr_Crtc) * reply->num_crtcs);
             if (ret)
               memcpy(ret, xcb_randr_get_screen_resources_crtcs(reply),
                      sizeof(Ecore_X_Randr_Crtc) * reply->num_crtcs);
             free(reply);
          }
     }
#endif

   return ret;
}

/*
 * @deprecated bad naming. Use ecore_x_randr_window_crtcs_get instead.
 * @brief Get the CRTCs, which display a certain window.
 *
 * @param window Window the displaying CRTCs shall be found for.
 * @param num The number of CRTCs displaying the window.
 * @return Array of CRTCs that display a certain window. @c NULL if no CRTCs
 * was found that displays the specified window.
 */
EAPI Ecore_X_Randr_Crtc *
ecore_x_randr_current_crtc_get(Ecore_X_Window window,
                                 int           *num)
{
   return ecore_x_randr_window_crtcs_get(window, num);
}

/*
 * @brief Get the CRTCs, which display a certain window.
 *
 * @param window Window the displaying crtcs shall be found for.
 * @param num The number of crtcs displaying the window.
 * @return Array of crtcs that display a certain window. @c NULL if no crtcs
 * was found that displays the specified window.
 * @since 1.2.0
 */
EAPI Ecore_X_Randr_Crtc *
ecore_x_randr_window_crtcs_get(Ecore_X_Window window,
                               int *num)
{
#ifdef ECORE_XCB_RANDR
   Ecore_X_Window root;
   Eina_Rectangle w_geo, c_geo;
   Ecore_X_Randr_Crtc *crtcs, *ret = NULL;
   Ecore_X_Randr_Mode mode;
   int ncrtcs, i, nret = 0;
   xcb_translate_coordinates_cookie_t cookie;
   xcb_translate_coordinates_reply_t *trans;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(NULL);

   ecore_x_window_geometry_get(window, &w_geo.x, &w_geo.y, &w_geo.w, &w_geo.h);

   root = ecore_x_window_root_get(window);
   crtcs = ecore_x_randr_crtcs_get(root, &ncrtcs);
   if (!crtcs) goto _ecore_x_randr_window_crtcs_get_fail;

   /* now get window RELATIVE to root window - thats what matters. */
   cookie = xcb_translate_coordinates(_ecore_xcb_conn, window, root, 0, 0);
   trans = xcb_translate_coordinates_reply(_ecore_xcb_conn, cookie, NULL);
   w_geo.x = trans->dst_x;
   w_geo.y = trans->dst_y;
   free(trans);

   ret = calloc(1, ncrtcs * sizeof(Ecore_X_Randr_Crtc));
   if (!ret)
     {
        free(crtcs);
        goto _ecore_x_randr_window_crtcs_get_fail;
     }
   for (i = 0, nret = 0; i < ncrtcs; i++)
     {
        /* if crtc is not enabled, don't bother about it any further */
         mode = ecore_x_randr_crtc_mode_get(root, crtcs[i]);
         if (mode == Ecore_X_Randr_None) continue;

         ecore_x_randr_crtc_geometry_get(root, crtcs[i], &c_geo.x, &c_geo.y,
                                         &c_geo.w, &c_geo.h);
         if (eina_rectangles_intersect(&w_geo, &c_geo))
           {
              ret[nret] = crtcs[i];
              nret++;
           }
     }
   free(crtcs);

   if (num) *num = nret;
   return ret;

_ecore_x_randr_window_crtcs_get_fail:
#endif
   if (num) *num = 0;
   return NULL;
}

/*
 * @brief get a CRTC's outputs.
 * @param root the root window which's screen will be queried
 * @param num number of outputs referenced by given CRTC
 */
EAPI Ecore_X_Randr_Output *
ecore_x_randr_crtc_outputs_get(Ecore_X_Window     root,
                               Ecore_X_Randr_Crtc crtc,
                               int               *num)
{
   Ecore_X_Randr_Output *ret = NULL;
#ifdef ECORE_XCB_RANDR
   xcb_timestamp_t stamp = 0;
   xcb_randr_get_crtc_info_cookie_t ocookie;
   xcb_randr_get_crtc_info_reply_t *oreply;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(NULL);

   if (!_ecore_xcb_randr_crtc_validate(root, crtc)) return ret;

   if (_randr_version >= RANDR_1_3)
     stamp = _ecore_xcb_randr_13_get_resource_timestamp(root);
   else if (_randr_version == RANDR_1_2)
     stamp = _ecore_xcb_randr_12_get_resource_timestamp(root);

   ocookie =
     xcb_randr_get_crtc_info_unchecked(_ecore_xcb_conn, crtc, stamp);
   oreply = xcb_randr_get_crtc_info_reply(_ecore_xcb_conn, ocookie, NULL);
   if (oreply)
     {
        if (num) *num = oreply->num_outputs;
        ret = malloc(sizeof(Ecore_X_Randr_Output) * oreply->num_outputs);
        if (ret)
          memcpy(ret, xcb_randr_get_crtc_info_outputs(oreply),
                 sizeof(Ecore_X_Randr_Output) * oreply->num_outputs);
        free(oreply);
     }
#endif

   return ret;
}

EAPI void
ecore_x_randr_crtc_geometry_get(Ecore_X_Window     root,
                                Ecore_X_Randr_Crtc crtc,
                                int               *x,
                                int               *y,
                                int               *w,
                                int               *h)
{
#ifdef ECORE_XCB_RANDR
   xcb_timestamp_t stamp = 0;
   xcb_randr_get_crtc_info_cookie_t ocookie;
   xcb_randr_get_crtc_info_reply_t *oreply;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET();

   if (!_ecore_xcb_randr_crtc_validate(root, crtc)) return;

   if (_randr_version >= RANDR_1_3)
     stamp = _ecore_xcb_randr_13_get_resource_timestamp(root);
   else if (_randr_version == RANDR_1_2)
     stamp = _ecore_xcb_randr_12_get_resource_timestamp(root);

   ocookie =
     xcb_randr_get_crtc_info_unchecked(_ecore_xcb_conn, crtc, stamp);
   oreply = xcb_randr_get_crtc_info_reply(_ecore_xcb_conn, ocookie, NULL);
   if (oreply)
     {
        if (x) *x = oreply->x;
        if (y) *y = oreply->y;
        if (w) *w = oreply->width;
        if (h) *h = oreply->height;
        free(oreply);
     }
#endif
}

/**
 * @brief Sets a CRTC relative to another one.
 *
 * @param root The window on which CRTC's position will be set.
 * @param crtc_r1 The CRTC to be positioned.
 * @param crtc_r2 The CRTC the position should be relative to.
 * @param policy The relation between the crtcs.
 * @param alignment In case CRTCs size differ, aligns CRTC1 accordingly at
 * CRTC2's borders.
 * @return @c EINA_TRUE if crtc could be successfully positioned, @c EINA_FALSE
 * if repositioning failed or if position of new crtc would be out of given
 * screen's min/max bounds.
 */
EAPI Eina_Bool
ecore_x_randr_crtc_pos_relative_set(Ecore_X_Window                   root,
                                    Ecore_X_Randr_Crtc               crtc_r1,
                                    Ecore_X_Randr_Crtc               crtc_r2,
                                    Ecore_X_Randr_Output_Policy      policy,
                                    Ecore_X_Randr_Relative_Alignment alignment)
{
#ifdef ECORE_XCB_RANDR
   Eina_Rectangle r1, r2;
   int w_max = 0, h_max = 0, cw = 0, ch = 0, xn = -1, yn = -1;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(EINA_FALSE);

   if ((ecore_x_randr_crtc_mode_get(root, crtc_r1) == 0) ||
       (ecore_x_randr_crtc_mode_get(root, crtc_r2) == 0))
     return EINA_FALSE;

   if ((!_ecore_xcb_randr_crtc_validate(root, crtc_r1) ||
        (!(crtc_r1 != crtc_r2) && (!_ecore_xcb_randr_crtc_validate(root, crtc_r2)))))
     return EINA_FALSE;

   ecore_x_randr_crtc_geometry_get(root, crtc_r1, &r1.x, &r1.y, &r1.w, &r1.h);
   ecore_x_randr_crtc_geometry_get(root, crtc_r2, &r2.x, &r2.y, &r2.w, &r2.h);
   ecore_x_randr_screen_size_range_get(root, NULL, NULL, &w_max, &h_max);
   ecore_x_randr_screen_current_size_get(root, &cw, &ch, NULL, NULL);

   switch (policy)
     {
      case ECORE_X_RANDR_OUTPUT_POLICY_RIGHT:
        xn = (r2.x + r2.w);
        if (alignment == ECORE_X_RANDR_RELATIVE_ALIGNMENT_NONE)
          yn = -1;
        else if (alignment == ECORE_X_RANDR_RELATIVE_ALIGNMENT_CENTER_REL)
          yn = ((int)(((double)r2.h / 2.0) + (double)r2.y - ((double)r1.h / 2.0)));
        else if (alignment == ECORE_X_RANDR_RELATIVE_ALIGNMENT_CENTER_SCR)
          yn = ((int)((double)ch / 2.0) - ((double)r1.h / 2.0));
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_LEFT:
        xn = (r2.x - r1.w);
        if (alignment == ECORE_X_RANDR_RELATIVE_ALIGNMENT_NONE)
          yn = -1;
        else if (alignment == ECORE_X_RANDR_RELATIVE_ALIGNMENT_CENTER_REL)
          yn = ((int)(((double)r2.h / 2.0) + (double)r2.y - ((double)r1.h / 2.0)));
        else if (alignment == ECORE_X_RANDR_RELATIVE_ALIGNMENT_CENTER_SCR)
          yn = ((int)((double)ch / 2.0) - ((double)r1.h / 2.0));
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_BELOW:
        yn = (r2.y + r2.h);
        if (alignment == ECORE_X_RANDR_RELATIVE_ALIGNMENT_NONE)
          xn = -1;
        else if (alignment == ECORE_X_RANDR_RELATIVE_ALIGNMENT_CENTER_REL)
          xn = ((int)((((double)r2.x + (double)r2.w) / 2.0) - ((double)r1.w / 2.0)));
        else if (alignment == ECORE_X_RANDR_RELATIVE_ALIGNMENT_CENTER_SCR)
          xn = ((int)((double)cw / 2.0));
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_ABOVE:
        yn = (r2.y - r1.h);
        if (alignment == ECORE_X_RANDR_RELATIVE_ALIGNMENT_NONE)
          xn = -1;
        else if (alignment == ECORE_X_RANDR_RELATIVE_ALIGNMENT_CENTER_REL)
          xn = ((int)((((double)r2.x + (double)r2.w) / 2.0) - ((double)r1.w / 2.0)));
        else if (alignment == ECORE_X_RANDR_RELATIVE_ALIGNMENT_CENTER_SCR)
          xn = ((int)((double)cw / 2.0));
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_CLONE:
        return ecore_x_randr_crtc_pos_set(root, crtc_r1, r2.x, r2.y);
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_NONE:
        break;
      default:
        return EINA_FALSE;
     }

   if ((xn == r1.x) && (yn == r1.x)) return EINA_TRUE;
   if (((yn + r1.h) > h_max) || ((xn + r1.w) > w_max))
     return EINA_FALSE;

   return ecore_x_randr_crtc_pos_set(root, crtc_r1, xn, yn);
#endif

   return EINA_FALSE;
}

EAPI Eina_Bool
ecore_x_randr_move_all_crtcs_but(Ecore_X_Window            root,
                                 const Ecore_X_Randr_Crtc *not_moved,
                                 int                       num,
                                 int                       dx,
                                 int                       dy)
{
   Eina_Bool ret = EINA_FALSE;
#ifdef ECORE_XCB_RANDR
   Ecore_X_Randr_Crtc *crtcs = NULL, *move = NULL;
   int i = 0, j = 0, k = 0, n = 0, total = 0;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   if ((num <= 0) || (!not_moved) || (!_ecore_xcb_randr_root_validate(root)))
     return EINA_FALSE;

   crtcs = ecore_x_randr_crtcs_get(root, &total);
   n = (total - num);
   move = malloc(sizeof(Ecore_X_Randr_Crtc) * n);
   if (move)
     {
        for (i = 0, k = 0; (i < total) && (k < n); i++)
          {
             for (j = 0; j < num; j++)
               if (crtcs[i] == not_moved[j]) break;
             if (j == num)
               move[k++] = crtcs[i];
          }
        ret = ecore_x_randr_move_crtcs(root, move, n, dx, dy);
        free(move);
        free(crtcs);
     }
#endif

   return ret;
}

EAPI void
ecore_x_randr_crtc_pos_get(Ecore_X_Window     root,
                           Ecore_X_Randr_Crtc crtc,
                           int               *x,
                           int               *y)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET();

   ecore_x_randr_crtc_geometry_get(root, crtc, x, y, NULL, NULL);
#endif
}

/*
 * @brief Sets the position of given CRTC within root window's screen.
 *
 * @param root The window's screen to be queried.
 * @param crtc The CRTC which's position within the mentioned screen is to be
 * altered.
 * @param x Position on the x-axis (0 == left) of the screen. if x < 0 current
 * value will be kept.
 * @param y Position on the y-ayis (0 == top) of the screen. if y < 0, current
 * value will be kept.
 * @return @c EINA_TRUE if position could be successfully be altered.
 */
EAPI Eina_Bool
ecore_x_randr_crtc_pos_set(Ecore_X_Window     root,
                           Ecore_X_Randr_Crtc crtc,
                           int                x,
                           int                y)
{
   Eina_Bool ret = EINA_FALSE;
#ifdef ECORE_XCB_RANDR
   int w = 0, h = 0, nw = 0, nh = 0;
   Eina_Rectangle rect;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(EINA_FALSE);

   ecore_x_randr_crtc_geometry_get(root, crtc,
                                   &rect.x, &rect.y, &rect.w, &rect.h);
   ecore_x_randr_screen_current_size_get(root, &w, &h, NULL, NULL);
   if (x < 0) x = rect.x;
   if (y < 0) y = rect.y;
   if ((x + rect.w) > w)
     nw = (x + rect.w);
   if ((y + rect.h) > h)
     nh = (y + rect.h);

   if ((nw != 0) || (nh != 0))
     {
        if (!ecore_x_randr_screen_current_size_set(root, nw, nh, 0, 0))
          return EINA_FALSE;
     }

   ret = ecore_x_randr_crtc_settings_set(root, crtc, NULL, -1, x, y, -1, -1);
#endif

   return ret;
}

EAPI void
ecore_x_randr_crtc_size_get(Ecore_X_Window     root,
                            Ecore_X_Randr_Crtc crtc,
                            int               *w,
                            int               *h)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET();
   ecore_x_randr_crtc_geometry_get(root, crtc, NULL, NULL, w, h);
#endif
}

EAPI Eina_Bool 
ecore_x_randr_crtc_clone_set(Ecore_X_Window root, Ecore_X_Randr_Crtc original, Ecore_X_Randr_Crtc cln)
{
   Eina_Bool ret = EINA_FALSE;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(EINA_FALSE);

   if (_randr_version >= RANDR_1_3)
     {
        xcb_randr_get_screen_resources_current_reply_t *reply;
        xcb_timestamp_t stamp = 0;

        reply = _ecore_xcb_randr_13_get_resources(root);
        if (reply)
          {
             xcb_randr_get_crtc_info_cookie_t rcookie;
             xcb_randr_get_crtc_info_reply_t *rreply;

             if (_randr_version >= RANDR_1_3)
               stamp = _ecore_xcb_randr_13_get_resource_timestamp(root);
             else if (_randr_version == RANDR_1_2)
               stamp = _ecore_xcb_randr_12_get_resource_timestamp(root);

             rcookie =
               xcb_randr_get_crtc_info_unchecked(_ecore_xcb_conn, original,
                                                 stamp);

             rreply = xcb_randr_get_crtc_info_reply(_ecore_xcb_conn,
                                                    rcookie, NULL);
             if (rreply)
               {
                  int ox = 0, oy = 0;
                  Ecore_X_Randr_Orientation orient = 0;
                  Ecore_X_Randr_Mode mode = -1;

                  ox = rreply->x;
                  oy = rreply->y;
                  orient = rreply->rotation;
                  mode = rreply->mode;

                  free(rreply);

                  ret = ecore_x_randr_crtc_settings_set(root, cln, NULL, -1, 
                                                        ox, oy, mode, orient);
               }

             free(reply);
          }
     }
#endif

   return ret;
}

EAPI Ecore_X_Randr_Crtc_Info *
ecore_x_randr_crtc_info_get(Ecore_X_Window root, const Ecore_X_Randr_Crtc crtc)
{
   Ecore_X_Randr_Crtc_Info *ret = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(NULL);

   if (!_ecore_xcb_randr_crtc_validate(root, crtc)) return NULL;

   if (_randr_version >= RANDR_1_3)
     {
        xcb_randr_get_screen_resources_current_reply_t *reply;
        xcb_timestamp_t stamp = 0;

        reply = _ecore_xcb_randr_13_get_resources(root);
        if (reply)
          {
             xcb_randr_get_crtc_info_cookie_t rcookie;
             xcb_randr_get_crtc_info_reply_t *rreply;

             if (_randr_version >= RANDR_1_3)
               stamp = _ecore_xcb_randr_13_get_resource_timestamp(root);
             else if (_randr_version == RANDR_1_2)
               stamp = _ecore_xcb_randr_12_get_resource_timestamp(root);

             rcookie =
               xcb_randr_get_crtc_info_unchecked(_ecore_xcb_conn, crtc,
                                                 stamp);

             rreply = xcb_randr_get_crtc_info_reply(_ecore_xcb_conn,
                                                    rcookie, NULL);
             if (rreply)
               {
                  if ((ret = malloc(sizeof(Ecore_X_Randr_Crtc_Info))))
                    {
                       ret->timestamp = rreply->timestamp;
                       ret->x = rreply->x;
                       ret->y = rreply->y;
                       ret->width = rreply->width;
                       ret->height = rreply->height;
                       ret->mode = rreply->mode;
                       ret->rotation = rreply->rotation;
                       ret->noutput = 
                         xcb_randr_get_crtc_info_outputs_length(rreply);
                       ret->npossible = 
                         xcb_randr_get_crtc_info_possible_length(rreply);

                       if ((ret->outputs = 
                            malloc(ret->noutput * sizeof(Ecore_X_Randr_Output))))
                         {
                            xcb_randr_output_t *outs;
                            int i = 0;

                            outs = xcb_randr_get_crtc_info_outputs(rreply);
                            for (i = 0; i < ret->noutput; i++)
                              ret->outputs[i] = outs[i];
                         }

                       if ((ret->possible = 
                            malloc(ret->npossible * sizeof(Ecore_X_Randr_Output))))
                         {
                            xcb_randr_output_t *outs;
                            int i = 0;

                            outs = xcb_randr_get_crtc_info_possible(rreply);
                            for (i = 0; i < ret->npossible; i++)
                              ret->possible[i] = outs[i];
                         }
                    }
                  free(rreply);
               }

             free(reply);
          }
     }
#endif

   return ret;
}

EAPI void 
ecore_x_randr_crtc_info_free(Ecore_X_Randr_Crtc_Info *info)
{
#ifdef ECORE_XCB_RANDR
   if (_randr_version >= RANDR_1_2)
     {
        if (info)
          {
             if (info->outputs) free(info->outputs);
             if (info->possible) free(info->possible);
             free(info);
             info = NULL;
          }
     }
#endif
}

EAPI Ecore_X_Randr_Refresh_Rate
ecore_x_randr_crtc_refresh_rate_get(Ecore_X_Window     root,
                                    Ecore_X_Randr_Crtc crtc,
                                    Ecore_X_Randr_Mode mode)
{
   Ecore_X_Randr_Refresh_Rate ret = 0.0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(0.0);

   if (!_ecore_xcb_randr_crtc_validate(root, crtc)) return 0.0;

   if (_randr_version >= RANDR_1_3)
     {
        xcb_randr_get_screen_resources_current_reply_t *reply;

        reply = _ecore_xcb_randr_13_get_resources(root);
        if (reply)
          {
             xcb_randr_mode_info_iterator_t miter;

             miter =
               xcb_randr_get_screen_resources_current_modes_iterator(reply);
             while (miter.rem)
               {
                  xcb_randr_mode_info_t *minfo;

                  minfo = miter.data;
                  if (minfo->id == mode)
                    {
                       if ((minfo->htotal) && (minfo->vtotal))
                         {
                            ret = ((double)minfo->dot_clock /
                                   ((double)minfo->htotal *
                                    (double)minfo->vtotal));
                         }
                       break;
                    }
                  xcb_randr_mode_info_next(&miter);
               }
             free(reply);
          }
     }
   else if (_randr_version == RANDR_1_2)
     {
        xcb_randr_get_screen_resources_reply_t *reply;

        reply = _ecore_xcb_randr_12_get_resources(root);
        if (reply)
          {
             xcb_randr_mode_info_iterator_t miter;

             miter = xcb_randr_get_screen_resources_modes_iterator(reply);
             while (miter.rem)
               {
                  xcb_randr_mode_info_t *minfo;

                  minfo = miter.data;
                  if (minfo->id == mode)
                    {
                       if ((minfo->htotal) && (minfo->vtotal))
                         {
                            ret = ((double)minfo->dot_clock /
                                   ((double)minfo->htotal *
                                    (double)minfo->vtotal));
                         }
                       break;
                    }
                  xcb_randr_mode_info_next(&miter);
               }
             free(reply);
          }
     }
#endif
   return ret;
}

/*
 * @brief Move given CRTCs belonging to the given root window's screen dx/dy
 * pixels relative to their current position. The screen size will be
 * automatically adjusted if necessary and possible.
 *
 * @param root Window which's screen's resources are used.
 * @param crtcs List of CRTCs to be moved.
 * @param ncrtc Number of CRTCs in array.
 * @param dx Amount of pixels the CRTCs should be moved in x direction.
 * @param dy Amount of pixels the CRTCs should be moved in y direction.
 * @return @c EINA_TRUE if all crtcs could be moved successfully.
 */
EAPI Eina_Bool
ecore_x_randr_move_crtcs(Ecore_X_Window            root,
                         const Ecore_X_Randr_Crtc *crtcs,
                         int                       num,
                         int                       dx,
                         int                       dy)
{
   Eina_Bool ret = EINA_TRUE;
#ifdef ECORE_XCB_RANDR
   xcb_timestamp_t stamp = 0;
   xcb_randr_get_crtc_info_reply_t *oreply[num];
   int i = 0, cw = 0, ch = 0;
   int mw = 0, mh = 0, nw = 0, nh = 0;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(EINA_FALSE);

   if (!_ecore_xcb_randr_root_validate(root)) return EINA_FALSE;

   if (_randr_version >= RANDR_1_3)
     stamp = _ecore_xcb_randr_13_get_resource_timestamp(root);
   else if (_randr_version == RANDR_1_2)
     stamp = _ecore_xcb_randr_12_get_resource_timestamp(root);

   ecore_x_randr_screen_size_range_get(root, NULL, NULL, &mw, &mh);
   ecore_x_randr_screen_current_size_get(root, &cw, &ch, NULL, NULL);
   nw = cw;
   nh = ch;

   for (i = 0; i < num; i++)
     {
        xcb_randr_get_crtc_info_cookie_t ocookie;

        ocookie =
          xcb_randr_get_crtc_info_unchecked(_ecore_xcb_conn, crtcs[i],
                                            stamp);
        oreply[i] = xcb_randr_get_crtc_info_reply(_ecore_xcb_conn,
                                                  ocookie, NULL);
        if (oreply[i])
          {
             if (((oreply[i]->x + dx) < 0) ||
                 ((oreply[i]->y + dy) < 0) ||
                 ((oreply[i]->x + oreply[i]->width + dx) > mw) ||
                 ((oreply[i]->y + oreply[i]->height + dy) > mh))
               {
                  continue;
               }
             nw = MAX((int)(oreply[i]->x + oreply[i]->width + dx), nw);
             nh = MAX((int)(oreply[i]->y + oreply[i]->height + dy), nh);
          }
     }

   if ((nw > cw) || (nh > ch))
     {
        if (!ecore_x_randr_screen_current_size_set(root, nw, nh, -1, -1))
          {
             for (i = 0; i < num; i++)
               if (oreply[i]) free(oreply[i]);

             return EINA_FALSE;
          }
     }

   for (i = 0; ((i < num) && (oreply[i])); i++)
     {
        if (!oreply[i]) continue;
        if (!ecore_x_randr_crtc_settings_set(root, crtcs[i], NULL, -1,
                                             (oreply[i]->x + dx),
                                             (oreply[i]->y + dy),
                                             oreply[i]->mode,
                                             oreply[i]->rotation))
          {
             ret = EINA_FALSE;
             break;
          }
     }

   if (i < num)
     {
        while (i-- >= 0)
          {
             if (oreply[i])
               ecore_x_randr_crtc_settings_set(root, crtcs[i], NULL, -1,
                                               (oreply[i]->x - dx),
                                               (oreply[i]->y - dy),
                                               oreply[i]->mode,
                                               oreply[i]->rotation);
          }
     }

   for (i = 0; i < num; i++)
     if (oreply[i]) free(oreply[i]);
#endif

   return ret;
}

/**
 * @brief enable event selection. This enables basic interaction with
 * output/crtc events and requires RRandR >= 1.2.
 * @param win select this window's properties for RandRR events
 * @param on enable/disable selecting
 */
EAPI void
ecore_x_randr_events_select(Ecore_X_Window win,
                            Eina_Bool      on)
{
#ifdef ECORE_XCB_RANDR
   uint16_t mask = 0;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   if (on)
     {
        mask = XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE;
        if (_randr_version >= ((1 << 16) | 2))
          {
             mask |= (XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE |
                      XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE |
                      XCB_RANDR_NOTIFY_MASK_OUTPUT_PROPERTY);
          }
     }

   xcb_randr_select_input(_ecore_xcb_conn, win, mask);
#endif
}

/**
 * @brief removes unused screen space. The most upper left CRTC is set to 0x0
 * and all other CRTCs dx,dy respectively.
 * @param root the window's screen which will be reset.
 */
EAPI void
ecore_x_randr_screen_reset(Ecore_X_Window root)
{
#ifdef ECORE_XCB_RANDR
   xcb_timestamp_t stamp = 0;
   Ecore_X_Randr_Crtc *crtcs = NULL;
   int total = 0, i = 0, w = 0, h = 0;
   int dx = 100000, dy = 100000, num = 0;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   if (!_ecore_xcb_randr_root_validate(root)) return;
   crtcs = ecore_x_randr_crtcs_get(root, &total);

   if (_randr_version >= RANDR_1_3)
     stamp = _ecore_xcb_randr_13_get_resource_timestamp(root);
   else if (_randr_version == RANDR_1_2)
     stamp = _ecore_xcb_randr_12_get_resource_timestamp(root);

   /* I hate declaring variables inside code like this, but we need the
    * value of 'total' before we can */
   Ecore_X_Randr_Crtc enabled[total];

   for (i = 0; i < total; i++)
     {
        xcb_randr_get_crtc_info_cookie_t ocookie;
        xcb_randr_get_crtc_info_reply_t *oreply;

        ocookie =
          xcb_randr_get_crtc_info_unchecked(_ecore_xcb_conn, crtcs[i], stamp);
        oreply = xcb_randr_get_crtc_info_reply(_ecore_xcb_conn,
                                               ocookie, NULL);
        if (!oreply) continue;
        if ((oreply->mode <= 0) || (oreply->num_outputs == 0))
          {
             free(oreply);
             continue;
          }

        enabled[num++] = crtcs[i];
        if ((int)(oreply->x + oreply->width) > w)
          w = (oreply->x + oreply->width);
        if ((int)(oreply->y + oreply->height) > h)
          h = (oreply->y + oreply->height);

        if (oreply->x < dx) dx = oreply->x;
        if (oreply->y < dy) dy = oreply->y;

        free(oreply);
     }
   free(crtcs);

   if ((dx > 0) || (dy > 0))
     {
        if (ecore_x_randr_move_crtcs(root, enabled, num, -dx, -dy))
          {
             w -= dx;
             h -= dy;
          }
     }

   ecore_x_randr_screen_current_size_set(root, w, h, -1, -1);
#endif
}

/*
 * @param root window which's screen will be queried
 * @param wmin minimum width the screen can be set to
 * @param hmin minimum height the screen can be set to
 * @param wmax maximum width the screen can be set to
 * @param hmax maximum height the screen can be set to
 */
EAPI void
ecore_x_randr_screen_size_range_get(Ecore_X_Window root,
                                    int           *minw,
                                    int           *minh,
                                    int           *maxw,
                                    int           *maxh)
{
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_size_range_cookie_t cookie;
   xcb_randr_get_screen_size_range_reply_t *reply;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET();

   cookie = xcb_randr_get_screen_size_range_unchecked(_ecore_xcb_conn, root);
   reply = xcb_randr_get_screen_size_range_reply(_ecore_xcb_conn, cookie, NULL);
   if (reply)
     {
        if (minw) *minw = reply->min_width;
        if (minh) *minh = reply->min_height;
        if (maxw) *maxw = reply->max_width;
        if (maxh) *maxh = reply->max_height;
        free(reply);
     }
#endif
}

/*
 * @param w width of screen in px
 * @param h height of screen in px
 */
EAPI void
ecore_x_randr_screen_current_size_get(Ecore_X_Window root,
                                      int           *w,
                                      int           *h,
                                      int           *w_mm,
                                      int           *h_mm)
{
#ifdef ECORE_XCB_RANDR
   Ecore_X_Randr_Screen scr = 0;
   xcb_screen_t *s;
# define RANDR_VALIDATE_ROOT(screen, root) \
  ((screen == _ecore_xcb_randr_root_to_screen(root)) != -1)
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET();

   if (!RANDR_VALIDATE_ROOT(scr, root)) return;

   s = ecore_x_screen_get(scr);
   if (w) *w = s->width_in_pixels;
   if (h) *h = s->height_in_pixels;
   if (w_mm) *w_mm = s->width_in_millimeters;
   if (h_mm) *h_mm = s->height_in_millimeters;
#endif
}

/*
 * @param root Window which's screen's size should be set. If invalid (e.g. 
 * @c NULL) no action is taken.
 * @param w Width in px the screen should be set to. If out of valid
 * boundaries, current value is assumed.
 * @param h Height in px the screen should be set to. If out of valid
 * boundaries, current value is assumed.
 * @param w_mm Width in mm the screen should be set to. If @c 0, current
 * aspect is assumed.
 * @param h_mm Height in mm the screen should be set to. If @c 0, current
 * aspect is assumed.
 * @return @c EINA_TRUE if request was successfully sent or screen is already
 * in requested size, @c EINA_FALSE if parameters are invalid.
 */
EAPI Eina_Bool
ecore_x_randr_screen_current_size_set(Ecore_X_Window root,
                                      int            w,
                                      int            h,
                                      int            w_mm,
                                      int            h_mm)
{
   Eina_Bool ret = EINA_TRUE;
#ifdef ECORE_XCB_RANDR
   Ecore_X_Randr_Screen scr;
   int wc = 0, hc = 0, w_mm_c = 0, h_mm_c = 0;
   int mw = 0, mh = 0, xw = 0, xh = 0;
# define RANDR_VALIDATE_ROOT(screen, root) \
  ((screen == _ecore_xcb_randr_root_to_screen(root)) != -1)
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(EINA_FALSE);

   if (!RANDR_VALIDATE_ROOT(scr, root)) return EINA_FALSE;
   ecore_x_randr_screen_current_size_get(root, &wc, &hc, &w_mm_c, &h_mm_c);
   if ((w == wc) && (h == hc) && (w_mm == w_mm_c) && (h_mm == h_mm_c))
     return EINA_TRUE;
   ecore_x_randr_screen_size_range_get(root, &mw, &mh, &xw, &xh);
   if (((w != 1) && ((w < mw) || (w > xw))) ||
       ((h != -1) && ((h < mh) || (h > xh)))) return EINA_FALSE;

   if (w <= 0)
     w = ((xcb_screen_t *)_ecore_xcb_screen)->width_in_pixels;
   if (h <= 0)
     h = ((xcb_screen_t *)_ecore_xcb_screen)->height_in_pixels;

   /* NB: Hmmmm, xlib version divides w_mm by width ... that seems wrong */
   if (w_mm <= 0)
     w_mm = ((xcb_screen_t *)_ecore_xcb_screen)->width_in_millimeters;
   if (h_mm <= 0)
     h_mm = ((xcb_screen_t *)_ecore_xcb_screen)->height_in_millimeters;

   xcb_randr_set_screen_size(_ecore_xcb_conn, root, w, h, w_mm, h_mm);
#endif

   return ret;
}

/*
 * @deprecated bad naming. Use ecore_x_randr_window_outputs_get instead.
 * @brief Get the outputs, which display a certain window.
 *
 * @param window Window the displaying outputs shall be found for.
 * @param num The number of outputs displaying the window.
 * @return Array of outputs that display a certain window. @c NULL if no
 * outputs was found that displays the specified window.
 */

Ecore_X_Randr_Output *
ecore_x_randr_current_output_get(Ecore_X_Window window,
                                 int *num)
{
   return ecore_x_randr_window_outputs_get(window, num);
}

/*
 * @brief Get the outputs, which display a certain window.
 *
 * @param window Window the displaying outputs shall be found for.
 * @param num The number of outputs displaying the window.
 * @return Array of outputs that display a certain window. @c NULL if no
 * outputs was found that displays the specified window.
 */
EAPI Ecore_X_Randr_Output *
ecore_x_randr_window_outputs_get(Ecore_X_Window window,
                                 int           *num)
{
#ifdef ECORE_XCB_RANDR
   Ecore_X_Window root;
   Ecore_X_Randr_Crtc *crtcs;
   Ecore_X_Randr_Output *outputs, *ret = NULL, *tret;
   int ncrtcs, noutputs, i, nret = 0;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

   if (num) *num = 0;

#ifdef ECORE_XCB_RANDR
   if (_randr_version < RANDR_1_2) goto _ecore_x_randr_current_output_get_fail;

   root = ecore_x_window_root_get(window);
   if (!(crtcs = ecore_x_randr_window_crtcs_get(window, &ncrtcs)))
     goto _ecore_x_randr_current_output_get_fail;

   for (i = 0, nret = 0; i < ncrtcs; i++)
     {

        outputs = ecore_x_randr_crtc_outputs_get(root, crtcs[i],
              &noutputs);
        if (outputs)
          {
             if (noutputs > 0)
               {
                  tret = realloc(ret, ((nret + noutputs) * sizeof(Ecore_X_Randr_Output)));
                  if (!tret) goto _ecore_x_randr_current_output_get_fail_free;
                  ret = tret;
                  memcpy(&ret[nret], outputs, (noutputs * sizeof(Ecore_X_Randr_Output)));
                  nret += noutputs;
               }
             free(outputs);
             outputs = NULL;
          }
     }
   free(crtcs);

   if (num)
     *num = nret;

   return ret;

_ecore_x_randr_current_output_get_fail_free:
   free(outputs);
   free(crtcs);
   free(ret);
_ecore_x_randr_current_output_get_fail:
#endif
   if (num) *num = 0;
   return NULL;
}

/*
 * @brief get the backlight level of the given output
 * @param root window which's screen should be queried
 * @param output from which the backlight level should be retrieved
 * @return the backlight level
 */
EAPI double
ecore_x_randr_output_backlight_level_get(Ecore_X_Window       root,
                                         Ecore_X_Randr_Output output)
{
#ifdef ECORE_XCB_RANDR
   Ecore_X_Atom _backlight;
   xcb_intern_atom_cookie_t acookie;
   xcb_intern_atom_reply_t *areply;
   xcb_randr_get_output_property_cookie_t cookie;
   xcb_randr_get_output_property_reply_t *reply;
   xcb_randr_query_output_property_cookie_t qcookie;
   xcb_randr_query_output_property_reply_t *qreply;
   double dvalue;
   long value, max, min;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(-1);

   acookie =
     xcb_intern_atom_unchecked(_ecore_xcb_conn, 1,
                               strlen("Backlight"), "Backlight");
   areply = xcb_intern_atom_reply(_ecore_xcb_conn, acookie, NULL);

   if (!areply)
     {
        ERR("Backlight property is not suppported on this server or driver");
        return -1;
     }
   else
     {
        _backlight = areply->atom;
        free(areply);
     }

   if (!_ecore_xcb_randr_output_validate(root, output))
     {
        ERR("Invalid output");
        return -1;
     }

   cookie =
     xcb_randr_get_output_property_unchecked(_ecore_xcb_conn,
                                             output, _backlight,
                                             XCB_ATOM_NONE, 0, 4, 0, 0);
   reply =
     xcb_randr_get_output_property_reply(_ecore_xcb_conn, cookie, NULL);
   if (!reply)
     {
        WRN("Backlight not supported on this output");
        return -1;
     }

   if ((reply->format != 32) || (reply->num_items != 1) ||
       (reply->type != XCB_ATOM_INTEGER))
     {
        free(reply);
        return -1;
     }

   value = *((long *)xcb_randr_get_output_property_data(reply));
   free (reply);

   /* I have the current value of the backlight */
   /* Now retrieve the min and max intensities of the output */
   qcookie =
     xcb_randr_query_output_property_unchecked(_ecore_xcb_conn,
                                               output, _backlight);
   qreply =
     xcb_randr_query_output_property_reply(_ecore_xcb_conn, qcookie, NULL);
   if (qreply)
     {
        dvalue = -1;
        if ((qreply->range) &&
            (xcb_randr_query_output_property_valid_values_length(qreply) == 2))
          {
             int32_t *vals;

             vals = xcb_randr_query_output_property_valid_values(qreply);
             /* finally convert the current value in the interval [0..1] */
             min = vals[0];
             max = vals[1];
             dvalue = ((double)(value - min)) / ((double)(max - min));
          }
        free(qreply);
        return dvalue;
     }
#endif
   return -1;
}

/*
 * @brief Set the backlight level of a given output.
 *
 * @param root Window which's screen should be queried.
 * @param output That should be set.
 * @param level For which the backlight should be set.
 * @return @c EINA_TRUE in case of success.
 */
EAPI Eina_Bool
ecore_x_randr_output_backlight_level_set(Ecore_X_Window       root,
                                         Ecore_X_Randr_Output output,
                                         double               level)
{
#ifdef ECORE_XCB_RANDR
   Ecore_X_Atom _backlight;
   xcb_intern_atom_cookie_t acookie;
   xcb_intern_atom_reply_t *areply;
   xcb_randr_query_output_property_cookie_t qcookie;
   xcb_randr_query_output_property_reply_t *qreply;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(EINA_FALSE);

   if ((level < 0) || (level > 1))
     {
        ERR("Backlight level should be between 0 and 1");
        return EINA_FALSE;
     }

   if (!_ecore_xcb_randr_output_validate(root, output))
     {
        ERR("Wrong output value");
        return EINA_FALSE;
     }

   acookie =
     xcb_intern_atom_unchecked(_ecore_xcb_conn, 1,
                               strlen("Backlight"), "Backlight");
   areply = xcb_intern_atom_reply(_ecore_xcb_conn, acookie, NULL);
   if (!areply)
     {
        WRN("Backlight property is not suppported on this server or driver");
        return EINA_FALSE;
     }
   else
     {
        _backlight = areply->atom;
        free(areply);
     }

   qcookie =
     xcb_randr_query_output_property_unchecked(_ecore_xcb_conn,
                                               output, _backlight);
   qreply =
     xcb_randr_query_output_property_reply(_ecore_xcb_conn, qcookie, NULL);
   if (qreply)
     {
        if ((qreply->range) && (qreply->length == 2))
          {
             int32_t *vals;
             double min, max, tmp;
             long n;

             vals = xcb_randr_query_output_property_valid_values(qreply);
             min = vals[0];
             max = vals[1];
             tmp = (level * (max - min)) + min;
             n = tmp;
             if (n > max) n = max;
             if (n < min) n = min;
             xcb_randr_change_output_property(_ecore_xcb_conn, output,
                                              _backlight, XCB_ATOM_INTEGER,
                                              32, XCB_PROP_MODE_REPLACE,
                                              1, (unsigned char *)&n);
             ecore_x_flush(); // needed
          }

        free(qreply);
        return EINA_TRUE;
     }
#endif
   return EINA_FALSE;
}

/*
 * @brief Check if a backlight is available.
 *
 * @return Whether a backlight is available.
 */
EAPI Eina_Bool 
ecore_x_randr_output_backlight_available(void) 
{
#ifdef ECORE_XCB_RANDR
   xcb_intern_atom_cookie_t acookie;
   xcb_intern_atom_reply_t *areply;
#endif

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(EINA_FALSE);

   acookie =
     xcb_intern_atom_unchecked(_ecore_xcb_conn, 1,
                               strlen("Backlight"), "Backlight");
   areply = xcb_intern_atom_reply(_ecore_xcb_conn, acookie, NULL);

   if (!areply)
     {
        ERR("Backlight property is not suppported on this server or driver");
        return EINA_FALSE;
     }
   else
     {
        free(areply);
        return EINA_TRUE;
     }
#endif
   return EINA_FALSE;
}

EAPI int
ecore_x_randr_edid_version_get(unsigned char *edid, unsigned long edid_length)
{
   if ((edid_length > _ECORE_X_RANDR_EDID_OFFSET_VERSION_MINOR) &&
       (ecore_x_randr_edid_has_valid_header(edid, edid_length)))
     return (edid[_ECORE_X_RANDR_EDID_OFFSET_VERSION_MAJOR] << 8) |
            edid[_ECORE_X_RANDR_EDID_OFFSET_VERSION_MINOR];
   return ECORE_X_RANDR_EDID_UNKNOWN_VALUE;
}

EAPI char *
ecore_x_randr_edid_display_name_get(unsigned char *edid, unsigned long edid_length) 
{
   unsigned char *block = NULL;
   int version = 0;

   version = ecore_x_randr_edid_version_get(edid, edid_length);
   if (version < ECORE_X_RANDR_EDID_VERSION_13) return NULL;

   _ECORE_X_RANDR_EDID_FOR_EACH_NON_PIXEL_DESCRIPTOR_BLOCK(edid, block)
     {
        if (block[_ECORE_X_RANDR_EDID_OFFSET_DESCRIPTOR_BLOCK_TYPE] == 0xfc)
          {
             char *name, *p;
             const char *edid_name;

             edid_name = (const char *)block + 
               _ECORE_X_RANDR_EDID_OFFSET_DESCRIPTOR_BLOCK_CONTENT;
             name = 
               malloc(_ECORE_X_RANDR_EDID_DISPLAY_DESCRIPTOR_BLOCK_CONTENT_LENGTH_MAX + 1);
             if (!name) return NULL;

             strncpy(name, edid_name, 
                     _ECORE_X_RANDR_EDID_DISPLAY_DESCRIPTOR_BLOCK_CONTENT_LENGTH_MAX);
             name[_ECORE_X_RANDR_EDID_DISPLAY_DESCRIPTOR_BLOCK_CONTENT_LENGTH_MAX] = 0;
             for (p = name; *p; p++)
               if ((*p < ' ') || (*p > '~')) *p = 0;

             return name;
          }
     }
   return NULL;
}

EAPI Eina_Bool
ecore_x_randr_edid_has_valid_header(unsigned char *edid, unsigned long edid_length)
{
   const unsigned char header[] =
     { 
        0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 
     };

   if ((!edid) || (edid_length < 8)) return EINA_FALSE;
   if (!memcmp(edid, header, 8)) return EINA_TRUE;
   return EINA_FALSE;
}

/* local functions */
static Eina_Bool
_ecore_xcb_randr_output_validate(Ecore_X_Window       root,
                                 Ecore_X_Randr_Output output)
{
   Eina_Bool ret = EINA_FALSE;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(EINA_FALSE);

   if ((output) && (_ecore_xcb_randr_root_validate(root)))
     {
        if (_randr_version >= RANDR_1_3)
          {
             xcb_randr_get_screen_resources_current_reply_t *reply;

             reply = _ecore_xcb_randr_13_get_resources(root);
             if (reply)
               {
                  int len = 0, i = 0;
                  xcb_randr_output_t *outputs;

                  len =
                    xcb_randr_get_screen_resources_current_outputs_length(reply);
                  outputs =
                    xcb_randr_get_screen_resources_current_outputs(reply);
                  for (i = 0; i < len; i++)
                    {
                       if (outputs[i] == output)
                         {
                            ret = EINA_TRUE;
                            break;
                         }
                    }
                  free(reply);
               }
          }
        else if (_randr_version == RANDR_1_2)
          {
             xcb_randr_get_screen_resources_reply_t *reply;

             reply = _ecore_xcb_randr_12_get_resources(root);
             if (reply)
               {
                  int len = 0, i = 0;
                  xcb_randr_output_t *outputs;

                  len = xcb_randr_get_screen_resources_outputs_length(reply);
                  outputs = xcb_randr_get_screen_resources_outputs(reply);
                  for (i = 0; i < len; i++)
                    {
                       if (outputs[i] == output)
                         {
                            ret = EINA_TRUE;
                            break;
                         }
                    }
                  free(reply);
               }
          }
     }
#endif
   return ret;
}

/**
 * @brief Validates a CRTC for a given root window's screen.
 *
 * @param root The window which's default display will be queried.
 * @param crtc The CRTC to be validated.
 * @return In case it is found @c EINA_TRUE will be returned, else
 * @c EINA_FALSE is returned.
 */
static Eina_Bool
_ecore_xcb_randr_crtc_validate(Ecore_X_Window     root,
                               Ecore_X_Randr_Crtc crtc)
{
   Eina_Bool ret = EINA_FALSE;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);
   CHECK_XCB_CONN;

#ifdef ECORE_XCB_RANDR
   RANDR_CHECK_1_2_RET(EINA_FALSE);

   if (((int)crtc == Ecore_X_Randr_None) || ((int)crtc == Ecore_X_Randr_Unset))
     return ret;

   if ((crtc) && (_ecore_xcb_randr_root_validate(root)))
     {
        if (_randr_version >= RANDR_1_3)
          {
             xcb_randr_get_screen_resources_current_reply_t *reply;

             reply = _ecore_xcb_randr_13_get_resources(root);
             if (reply)
               {
                  int i = 0;
                  xcb_randr_crtc_t *crtcs;

                  crtcs = xcb_randr_get_screen_resources_current_crtcs(reply);
                  for (i = 0; i < reply->num_crtcs; i++)
                    {
                       if (crtcs[i] == crtc)
                         {
                            ret = EINA_TRUE;
                            break;
                         }
                    }
                  free(reply);
               }
          }
        else if (_randr_version == RANDR_1_2)
          {
             xcb_randr_get_screen_resources_reply_t *reply;

             reply = _ecore_xcb_randr_12_get_resources(root);
             if (reply)
               {
                  int i = 0;
                  xcb_randr_crtc_t *crtcs;

                  crtcs = xcb_randr_get_screen_resources_crtcs(reply);
                  for (i = 0; i < reply->num_crtcs; i++)
                    {
                       if (crtcs[i] == crtc)
                         {
                            ret = EINA_TRUE;
                            break;
                         }
                    }
                  free(reply);
               }
          }
     }
#endif

   return ret;
}

static Ecore_X_Randr_Mode *
_ecore_xcb_randr_12_output_modes_get(Ecore_X_Window       root,
                                     Ecore_X_Randr_Output output,
                                     int                 *num,
                                     int                 *npreferred)
{
   Ecore_X_Randr_Mode *modes = NULL;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_reply_t *reply;

   reply = _ecore_xcb_randr_12_get_resources(root);
   if (reply)
     {
        xcb_randr_get_output_info_cookie_t ocookie;
        xcb_randr_get_output_info_reply_t *oreply;

        ocookie =
          xcb_randr_get_output_info_unchecked(_ecore_xcb_conn, output,
                                              reply->config_timestamp);
        oreply = xcb_randr_get_output_info_reply(_ecore_xcb_conn,
                                                 ocookie, NULL);
        if (oreply)
          {
             if (num) *num = oreply->num_modes;
             if (npreferred) *npreferred = oreply->num_preferred;

             modes = malloc(sizeof(Ecore_X_Randr_Mode) *
                            oreply->num_modes);
             if (modes)
               {
                  xcb_randr_mode_t *rmodes;
                  int len = 0;

                  len = xcb_randr_get_output_info_modes_length(oreply);
                  rmodes = xcb_randr_get_output_info_modes(oreply);
                  memcpy(modes, rmodes, sizeof(Ecore_X_Randr_Mode) * len);
               }
             free(oreply);
          }
        free(reply);
     }
#endif
   return modes;
}

static Ecore_X_Randr_Mode *
_ecore_xcb_randr_13_output_modes_get(Ecore_X_Window       root,
                                     Ecore_X_Randr_Output output,
                                     int                 *num,
                                     int                 *npreferred)
{
   Ecore_X_Randr_Mode *modes = NULL;
#ifdef ECORE_XCB_RANDR
   xcb_timestamp_t stamp = 0;
   xcb_randr_get_output_info_cookie_t ocookie;
   xcb_randr_get_output_info_reply_t *oreply;

   stamp = _ecore_xcb_randr_13_get_resource_timestamp(root);

   ocookie =
     xcb_randr_get_output_info_unchecked(_ecore_xcb_conn, output, stamp);
   oreply = xcb_randr_get_output_info_reply(_ecore_xcb_conn, ocookie, NULL);
   if (oreply)
     {
        if (num) *num = oreply->num_modes;
        if (npreferred) *npreferred = oreply->num_preferred;

        modes = malloc(sizeof(Ecore_X_Randr_Mode) * oreply->num_modes);
        if (modes)
          {
             xcb_randr_mode_t *rmodes;
             int len = 0;

             len = xcb_randr_get_output_info_modes_length(oreply);
             rmodes = xcb_randr_get_output_info_modes(oreply);
             memcpy(modes, rmodes, sizeof(Ecore_X_Randr_Mode) * len);
          }
        free(oreply);
     }
#endif
   return modes;
}

static Ecore_X_Randr_Mode_Info *
_ecore_xcb_randr_12_mode_info_get(Ecore_X_Window     root,
                                  Ecore_X_Randr_Mode mode)
{
   Ecore_X_Randr_Mode_Info *ret = NULL;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_reply_t *reply;

   reply = _ecore_xcb_randr_12_get_resources(root);
   if (reply)
     {
        if ((ret = malloc(sizeof(Ecore_X_Randr_Mode_Info))))
          {
             uint8_t *nbuf;
             xcb_randr_mode_info_iterator_t miter;

             nbuf = xcb_randr_get_screen_resources_names(reply);
             miter = xcb_randr_get_screen_resources_modes_iterator(reply);
             while (miter.rem)
               {
                  xcb_randr_mode_info_t *minfo;

                  minfo = miter.data;
                  nbuf += minfo->name_len;

                  if (minfo->id == mode)
                    {
                       ret->xid = minfo->id;
                       ret->width = minfo->width;
                       ret->height = minfo->height;
                       ret->dotClock = minfo->dot_clock;
                       ret->hSyncStart = minfo->hsync_start;
                       ret->hSyncEnd = minfo->hsync_end;
                       ret->hTotal = minfo->htotal;
                       ret->vSyncStart = minfo->vsync_start;
                       ret->vSyncEnd = minfo->vsync_end;
                       ret->vTotal = minfo->vtotal;
                       ret->modeFlags = minfo->mode_flags;

                       ret->name = NULL;
                       ret->nameLength = minfo->name_len;
                       if (ret->nameLength > 0)
                         {
                            ret->name = malloc(ret->nameLength + 1);
                            if (ret->name)
                              memcpy(ret->name, nbuf, ret->nameLength + 1);
                         }

                       break;
                    }
                  xcb_randr_mode_info_next(&miter);
               }
          }

        free(reply);
     }
#endif
   return ret;
}

static Ecore_X_Randr_Mode_Info *
_ecore_xcb_randr_13_mode_info_get(Ecore_X_Window     root,
                                  Ecore_X_Randr_Mode mode)
{
   Ecore_X_Randr_Mode_Info *ret = NULL;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_current_reply_t *reply;

   reply = _ecore_xcb_randr_13_get_resources(root);
   if (reply)
     {
        if ((ret = malloc(sizeof(Ecore_X_Randr_Mode_Info))))
          {
             uint8_t *nbuf;
             xcb_randr_mode_info_iterator_t miter;

             nbuf = xcb_randr_get_screen_resources_current_names(reply);
             miter =
               xcb_randr_get_screen_resources_current_modes_iterator(reply);
             while (miter.rem)
               {
                  xcb_randr_mode_info_t *minfo;

                  minfo = miter.data;
                  nbuf += minfo->name_len;

                  if (minfo->id == mode)
                    {
                       ret->xid = minfo->id;
                       ret->width = minfo->width;
                       ret->height = minfo->height;
                       ret->dotClock = minfo->dot_clock;
                       ret->hSyncStart = minfo->hsync_start;
                       ret->hSyncEnd = minfo->hsync_end;
                       ret->hTotal = minfo->htotal;
                       ret->vSyncStart = minfo->vsync_start;
                       ret->vSyncEnd = minfo->vsync_end;
                       ret->vTotal = minfo->vtotal;
                       ret->modeFlags = minfo->mode_flags;

                       ret->name = NULL;
                       ret->nameLength = minfo->name_len;
                       if (ret->nameLength > 0)
                         {
                            ret->name = malloc(ret->nameLength + 1);
                            if (ret->name)
                              memcpy(ret->name, nbuf, ret->nameLength + 1);
                         }

                       break;
                    }
                  xcb_randr_mode_info_next(&miter);
               }
          }

        free(reply);
     }
#endif
   return ret;
}

static Ecore_X_Randr_Mode_Info **
_ecore_xcb_randr_12_modes_info_get(Ecore_X_Window root,
                                   int           *num)
{
   Ecore_X_Randr_Mode_Info **ret = NULL;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_reply_t *reply;

   reply = _ecore_xcb_randr_12_get_resources(root);
   if (reply)
     {
        if (num) *num = reply->num_modes;
        ret = malloc(sizeof(Ecore_X_Randr_Mode_Info *) * reply->num_modes);
        if (ret)
          {
             xcb_randr_mode_info_iterator_t miter;
             int i = 0;
             uint8_t *nbuf;

             nbuf = xcb_randr_get_screen_resources_names(reply);
             miter = xcb_randr_get_screen_resources_modes_iterator(reply);
             while (miter.rem)
               {
                  xcb_randr_mode_info_t *minfo;

                  minfo = miter.data;
                  nbuf += minfo->name_len;
                  if ((ret[i] = malloc(sizeof(Ecore_X_Randr_Mode_Info))))
                    {
                       ret[i]->xid = minfo->id;
                       ret[i]->width = minfo->width;
                       ret[i]->height = minfo->height;
                       ret[i]->dotClock = minfo->dot_clock;
                       ret[i]->hSyncStart = minfo->hsync_start;
                       ret[i]->hSyncEnd = minfo->hsync_end;
                       ret[i]->hTotal = minfo->htotal;
                       ret[i]->vSyncStart = minfo->vsync_start;
                       ret[i]->vSyncEnd = minfo->vsync_end;
                       ret[i]->vTotal = minfo->vtotal;
                       ret[i]->modeFlags = minfo->mode_flags;

                       ret[i]->name = NULL;
                       ret[i]->nameLength = minfo->name_len;
                       if (ret[i]->nameLength > 0)
                         {
                            ret[i]->name = malloc(ret[i]->nameLength + 1);
                            if (ret[i]->name)
                              memcpy(ret[i]->name, nbuf,
                                     ret[i]->nameLength + 1);
                         }
                    }
                  else
                    {
                       while (i > 0)
                         free(ret[--i]);
                       free(ret);
                       ret = NULL;
                       break;
                    }
                  i++;
                  xcb_randr_mode_info_next(&miter);
               }
          }
        free(reply);
     }
#endif
   return ret;
}

static Ecore_X_Randr_Mode_Info **
_ecore_xcb_randr_13_modes_info_get(Ecore_X_Window root,
                                   int           *num)
{
   Ecore_X_Randr_Mode_Info **ret = NULL;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_current_reply_t *reply;

   reply = _ecore_xcb_randr_13_get_resources(root);
   if (reply)
     {
        if (num) *num = reply->num_modes;
        ret = malloc(sizeof(Ecore_X_Randr_Mode_Info *) * reply->num_modes);
        if (ret)
          {
             xcb_randr_mode_info_iterator_t miter;
             int i = 0;
             uint8_t *nbuf;

             nbuf = xcb_randr_get_screen_resources_current_names(reply);
             miter =
               xcb_randr_get_screen_resources_current_modes_iterator(reply);
             while (miter.rem)
               {
                  xcb_randr_mode_info_t *minfo;

                  minfo = miter.data;
                  nbuf += minfo->name_len;
                  if ((ret[i] = malloc(sizeof(Ecore_X_Randr_Mode_Info))))
                    {
                       ret[i]->xid = minfo->id;
                       ret[i]->width = minfo->width;
                       ret[i]->height = minfo->height;
                       ret[i]->dotClock = minfo->dot_clock;
                       ret[i]->hSyncStart = minfo->hsync_start;
                       ret[i]->hSyncEnd = minfo->hsync_end;
                       ret[i]->hTotal = minfo->htotal;
                       ret[i]->vSyncStart = minfo->vsync_start;
                       ret[i]->vSyncEnd = minfo->vsync_end;
                       ret[i]->vTotal = minfo->vtotal;
                       ret[i]->modeFlags = minfo->mode_flags;

                       ret[i]->name = NULL;
                       ret[i]->nameLength = minfo->name_len;
                       if (ret[i]->nameLength > 0)
                         {
                            ret[i]->name = malloc(ret[i]->nameLength + 1);
                            if (ret[i]->name)
                              memcpy(ret[i]->name, nbuf,
                                     ret[i]->nameLength + 1);
                         }
                    }
                  else
                    {
                       while (i > 0)
                         free(ret[--i]);
                       free(ret);
                       ret = NULL;
                       break;
                    }
                  i++;
                  xcb_randr_mode_info_next(&miter);
               }
          }
        free(reply);
     }
#endif
   return ret;
}

static void
_ecore_xcb_randr_12_mode_size_get(Ecore_X_Window     root,
                                  Ecore_X_Randr_Mode mode,
                                  int               *w,
                                  int               *h)
{
   if (w) *w = 0;
   if (h) *h = 0;

#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_reply_t *reply;

   reply = _ecore_xcb_randr_12_get_resources(root);
   if (reply)
     {
        xcb_randr_mode_info_iterator_t miter;

        miter = xcb_randr_get_screen_resources_modes_iterator(reply);
        while (miter.rem)
          {
             xcb_randr_mode_info_t *minfo;

             minfo = miter.data;
             if (minfo->id == mode)
               {
                  if (w) *w = minfo->width;
                  if (h) *h = minfo->height;
                  break;
               }
             xcb_randr_mode_info_next(&miter);
          }
        free(reply);
     }
#endif
}

static void
_ecore_xcb_randr_13_mode_size_get(Ecore_X_Window     root,
                                  Ecore_X_Randr_Mode mode,
                                  int               *w,
                                  int               *h)
{
   if (w) *w = 0;
   if (h) *h = 0;

#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_current_reply_t *reply;

   reply = _ecore_xcb_randr_13_get_resources(root);
   if (reply)
     {
        xcb_randr_mode_info_iterator_t miter;

        miter = xcb_randr_get_screen_resources_current_modes_iterator(reply);
        while (miter.rem)
          {
             xcb_randr_mode_info_t *minfo;

             minfo = miter.data;
             if (minfo->id == mode)
               {
                  if (w) *w = minfo->width;
                  if (h) *h = minfo->height;
                  break;
               }
             xcb_randr_mode_info_next(&miter);
          }
        free(reply);
     }
#endif
}

static Ecore_X_Randr_Output *
_ecore_xcb_randr_12_output_clones_get(Ecore_X_Window       root,
                                      Ecore_X_Randr_Output output,
                                      int                 *num)
{
   Ecore_X_Randr_Output *outputs = NULL;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_reply_t *reply;

   reply = _ecore_xcb_randr_12_get_resources(root);
   if (reply)
     {
        xcb_randr_get_output_info_cookie_t ocookie;
        xcb_randr_get_output_info_reply_t *oreply;

        ocookie =
          xcb_randr_get_output_info_unchecked(_ecore_xcb_conn, output,
                                              reply->config_timestamp);
        oreply = xcb_randr_get_output_info_reply(_ecore_xcb_conn,
                                                 ocookie, NULL);
        if (oreply)
          {
             if (num) *num = oreply->num_clones;

             outputs =
               malloc(sizeof(Ecore_X_Randr_Output) * oreply->num_clones);
             if (outputs)
               {
                  memcpy(outputs, xcb_randr_get_output_info_clones(oreply),
                         sizeof(Ecore_X_Randr_Output) * oreply->num_clones);
               }
             free(oreply);
          }
        free(reply);
     }
#endif
   return outputs;
}

static Ecore_X_Randr_Output *
_ecore_xcb_randr_13_output_clones_get(Ecore_X_Window       root,
                                      Ecore_X_Randr_Output output,
                                      int                 *num)
{
   Ecore_X_Randr_Output *outputs = NULL;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_current_reply_t *reply;

   reply = _ecore_xcb_randr_13_get_resources(root);
   if (reply)
     {
        xcb_randr_get_output_info_cookie_t ocookie;
        xcb_randr_get_output_info_reply_t *oreply;

        ocookie =
          xcb_randr_get_output_info_unchecked(_ecore_xcb_conn, output,
                                              reply->config_timestamp);
        oreply = xcb_randr_get_output_info_reply(_ecore_xcb_conn,
                                                 ocookie, NULL);
        if (oreply)
          {
             if (num) *num = oreply->num_clones;

             outputs =
               malloc(sizeof(Ecore_X_Randr_Output) * oreply->num_clones);
             if (outputs)
               {
                  memcpy(outputs, xcb_randr_get_output_info_clones(oreply),
                         sizeof(Ecore_X_Randr_Output) * oreply->num_clones);
               }
             free(oreply);
          }
        free(reply);
     }
#endif
   return outputs;
}

static Ecore_X_Randr_Crtc *
_ecore_xcb_randr_12_output_possible_crtcs_get(Ecore_X_Window       root,
                                              Ecore_X_Randr_Output output,
                                              int                 *num)
{
   Ecore_X_Randr_Crtc *crtcs = NULL;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_reply_t *reply;

   reply = _ecore_xcb_randr_12_get_resources(root);
   if (reply)
     {
        xcb_randr_get_output_info_cookie_t ocookie;
        xcb_randr_get_output_info_reply_t *oreply;

        ocookie =
          xcb_randr_get_output_info_unchecked(_ecore_xcb_conn, output,
                                              reply->config_timestamp);
        oreply = xcb_randr_get_output_info_reply(_ecore_xcb_conn,
                                                 ocookie, NULL);
        if (oreply)
          {
             if (num) *num = oreply->num_crtcs;

             crtcs = malloc(sizeof(Ecore_X_Randr_Crtc) * oreply->num_crtcs);
             if (crtcs)
               {
                  memcpy(crtcs, xcb_randr_get_output_info_crtcs(oreply),
                         sizeof(Ecore_X_Randr_Crtc) * oreply->num_crtcs);
               }
             free(oreply);
          }
        free(reply);
     }
#endif
   return crtcs;
}

static Ecore_X_Randr_Crtc *
_ecore_xcb_randr_13_output_possible_crtcs_get(Ecore_X_Window       root,
                                              Ecore_X_Randr_Output output,
                                              int                 *num)
{
   Ecore_X_Randr_Crtc *crtcs = NULL;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_current_reply_t *reply;

   reply = _ecore_xcb_randr_13_get_resources(root);
   if (reply)
     {
        xcb_randr_get_output_info_cookie_t ocookie;
        xcb_randr_get_output_info_reply_t *oreply;

        ocookie =
          xcb_randr_get_output_info_unchecked(_ecore_xcb_conn, output,
                                              reply->config_timestamp);
        oreply = xcb_randr_get_output_info_reply(_ecore_xcb_conn,
                                                 ocookie, NULL);
        if (oreply)
          {
             if (num) *num = oreply->num_crtcs;

             crtcs = malloc(sizeof(Ecore_X_Randr_Crtc) * oreply->num_crtcs);
             if (crtcs)
               {
                  memcpy(crtcs, xcb_randr_get_output_info_crtcs(oreply),
                         sizeof(Ecore_X_Randr_Crtc) * oreply->num_crtcs);
               }
             free(oreply);
          }
        free(reply);
     }
#endif
   return crtcs;
}

static char *
_ecore_xcb_randr_12_output_name_get(Ecore_X_Window       root,
                                    Ecore_X_Randr_Output output,
                                    int                 *len)
{
   char *ret = NULL;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_reply_t *reply;

   reply = _ecore_xcb_randr_12_get_resources(root);
   if (reply)
     {
        xcb_randr_get_output_info_cookie_t ocookie;
        xcb_randr_get_output_info_reply_t *oreply;

        ocookie =
          xcb_randr_get_output_info_unchecked(_ecore_xcb_conn, output,
                                              reply->config_timestamp);
        oreply = xcb_randr_get_output_info_reply(_ecore_xcb_conn,
                                                 ocookie, NULL);
        if (oreply)
          {
             uint8_t *nbuf;

             nbuf = xcb_randr_get_output_info_name(oreply);
             nbuf += oreply->name_len;

             if (len) *len = oreply->name_len;
             if (oreply->name_len > 0)
               {
                  ret = malloc(oreply->name_len + 1);
                  if (ret)
                    memcpy(ret, nbuf, oreply->name_len + 1);
               }

             free(oreply);
          }
        free(reply);
     }
#endif
   return ret;
}

static char *
_ecore_xcb_randr_13_output_name_get(Ecore_X_Window       root,
                                    Ecore_X_Randr_Output output,
                                    int                 *len)
{
   char *ret = NULL;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_current_reply_t *reply;

   reply = _ecore_xcb_randr_13_get_resources(root);
   if (reply)
     {
        xcb_randr_get_output_info_cookie_t ocookie;
        xcb_randr_get_output_info_reply_t *oreply;

        ocookie =
          xcb_randr_get_output_info_unchecked(_ecore_xcb_conn, output,
                                              reply->config_timestamp);
        oreply = xcb_randr_get_output_info_reply(_ecore_xcb_conn,
                                                 ocookie, NULL);
        if (oreply)
          {
             uint8_t *nbuf;

             nbuf = xcb_randr_get_output_info_name(oreply);
             nbuf += oreply->name_len;

             if (len) *len = oreply->name_len;
             if (oreply->name_len > 0)
               {
                  ret = malloc(oreply->name_len + 1);
                  if (ret)
                    memcpy(ret, nbuf, oreply->name_len + 1);
               }

             free(oreply);
          }
        free(reply);
     }
#endif
   return ret;
}

static Ecore_X_Randr_Connection_Status
_ecore_xcb_randr_12_output_connection_status_get(Ecore_X_Window       root,
                                                 Ecore_X_Randr_Output output)
{
   Ecore_X_Randr_Connection_Status ret = ECORE_X_RANDR_CONNECTION_STATUS_UNKNOWN;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_reply_t *reply;

   reply = _ecore_xcb_randr_12_get_resources(root);
   if (reply)
     {
        xcb_randr_get_output_info_cookie_t ocookie;
        xcb_randr_get_output_info_reply_t *oreply;

        ocookie =
          xcb_randr_get_output_info_unchecked(_ecore_xcb_conn, output,
                                              reply->config_timestamp);
        oreply = xcb_randr_get_output_info_reply(_ecore_xcb_conn,
                                                 ocookie, NULL);
        if (oreply)
          {
             ret = oreply->connection;
             free(oreply);
          }
        free(reply);
     }
#endif
   return ret;
}

static Ecore_X_Randr_Connection_Status
_ecore_xcb_randr_13_output_connection_status_get(Ecore_X_Window       root,
                                                 Ecore_X_Randr_Output output)
{
   Ecore_X_Randr_Connection_Status ret = ECORE_X_RANDR_CONNECTION_STATUS_UNKNOWN;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_current_reply_t *reply;

   reply = _ecore_xcb_randr_13_get_resources(root);
   if (reply)
     {
        xcb_randr_get_output_info_cookie_t ocookie;
        xcb_randr_get_output_info_reply_t *oreply;

        ocookie =
          xcb_randr_get_output_info_unchecked(_ecore_xcb_conn, output,
                                              reply->config_timestamp);
        oreply = xcb_randr_get_output_info_reply(_ecore_xcb_conn,
                                                 ocookie, NULL);
        if (oreply)
          {
             ret = oreply->connection;
             free(oreply);
          }
        free(reply);
     }
#endif
   return ret;
}

static Ecore_X_Randr_Output *
_ecore_xcb_randr_12_outputs_get(Ecore_X_Window root,
                                int           *num)
{
   Ecore_X_Randr_Output *ret = NULL;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_reply_t *reply;

   reply = _ecore_xcb_randr_12_get_resources(root);
   if (reply)
     {
        if (num) *num = reply->num_outputs;
        ret = malloc(sizeof(Ecore_X_Randr_Output) * reply->num_outputs);
        if (ret)
          memcpy(ret, xcb_randr_get_screen_resources_outputs(reply),
                 sizeof(Ecore_X_Randr_Output) * reply->num_outputs);
        free(reply);
     }
#endif
   return ret;
}

static Ecore_X_Randr_Output *
_ecore_xcb_randr_13_outputs_get(Ecore_X_Window root,
                                int           *num)
{
   Ecore_X_Randr_Output *ret = NULL;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_current_reply_t *reply;

   reply = _ecore_xcb_randr_13_get_resources(root);
   if (reply)
     {
        if (num) *num = reply->num_outputs;
        ret = malloc(sizeof(Ecore_X_Randr_Output) * reply->num_outputs);
        if (ret)
          memcpy(ret, xcb_randr_get_screen_resources_current_outputs(reply),
                 sizeof(Ecore_X_Randr_Output) * reply->num_outputs);
        free(reply);
     }
#endif
   return ret;
}

static Ecore_X_Randr_Crtc
_ecore_xcb_randr_12_output_crtc_get(Ecore_X_Window       root,
                                    Ecore_X_Randr_Output output)
{
   Ecore_X_Randr_Crtc ret = Ecore_X_Randr_None;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_reply_t *reply;

   reply = _ecore_xcb_randr_12_get_resources(root);
   if (reply)
     {
        xcb_randr_get_output_info_cookie_t ocookie;
        xcb_randr_get_output_info_reply_t *oreply;

        ocookie =
          xcb_randr_get_output_info_unchecked(_ecore_xcb_conn, output,
                                              reply->config_timestamp);
        oreply = xcb_randr_get_output_info_reply(_ecore_xcb_conn,
                                                 ocookie, NULL);
        if (oreply)
          {
             ret = oreply->crtc;
             free(oreply);
          }
        free(reply);
     }
#endif
   return ret;
}

static Ecore_X_Randr_Crtc
_ecore_xcb_randr_13_output_crtc_get(Ecore_X_Window       root,
                                    Ecore_X_Randr_Output output)
{
   Ecore_X_Randr_Crtc ret = Ecore_X_Randr_None;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_current_reply_t *reply;

   reply = _ecore_xcb_randr_13_get_resources(root);
   if (reply)
     {
        xcb_randr_get_output_info_cookie_t ocookie;
        xcb_randr_get_output_info_reply_t *oreply;

        ocookie =
          xcb_randr_get_output_info_unchecked(_ecore_xcb_conn, output,
                                              reply->config_timestamp);
        oreply = xcb_randr_get_output_info_reply(_ecore_xcb_conn,
                                                 ocookie, NULL);
        if (oreply)
          {
             ret = oreply->crtc;
             free(oreply);
          }
        free(reply);
     }
#endif
   return ret;
}

static xcb_randr_get_screen_resources_reply_t *
_ecore_xcb_randr_12_get_resources(Ecore_X_Window win)
{
   xcb_randr_get_screen_resources_cookie_t cookie;
   xcb_randr_get_screen_resources_reply_t *reply;

   cookie = xcb_randr_get_screen_resources_unchecked(_ecore_xcb_conn, win);
   reply = xcb_randr_get_screen_resources_reply(_ecore_xcb_conn, cookie, NULL);
   return reply;
}

static xcb_randr_get_screen_resources_current_reply_t *
_ecore_xcb_randr_13_get_resources(Ecore_X_Window win)
{
   xcb_randr_get_screen_resources_current_cookie_t cookie;
   xcb_randr_get_screen_resources_current_reply_t *reply;

   cookie =
     xcb_randr_get_screen_resources_current_unchecked(_ecore_xcb_conn, win);
   reply =
     xcb_randr_get_screen_resources_current_reply(_ecore_xcb_conn,
                                                  cookie, NULL);
   return reply;
}

static xcb_timestamp_t
_ecore_xcb_randr_12_get_resource_timestamp(Ecore_X_Window win)
{
   xcb_timestamp_t stamp = 0;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_reply_t *reply;

   reply = _ecore_xcb_randr_12_get_resources(win);
   stamp = reply->config_timestamp;
   free(reply);
#endif
   return stamp;
}

static xcb_timestamp_t
_ecore_xcb_randr_13_get_resource_timestamp(Ecore_X_Window win)
{
   xcb_timestamp_t stamp = 0;
#ifdef ECORE_XCB_RANDR
   xcb_randr_get_screen_resources_current_reply_t *reply;

   reply = _ecore_xcb_randr_13_get_resources(win);
   stamp = reply->config_timestamp;
   free(reply);
#endif
   return stamp;
}

