#ifndef _EVAS_IMAGE_H
#define _EVAS_IMAGE_H


EAPI void              evas_common_image_init                      (void);
EAPI void              evas_common_image_shutdown                  (void);

EAPI void              evas_common_image_image_all_unload          (void);
    
EAPI void              evas_common_rgba_image_free                 (Image_Entry *ie);
EAPI void              evas_common_rgba_image_unload               (Image_Entry *ie);
EAPI void              evas_common_image_colorspace_normalize      (RGBA_Image *im);
EAPI void              evas_common_image_colorspace_dirty          (RGBA_Image *im);
EAPI void              evas_common_image_cache_free                (void); /*2*/
EAPI void              evas_common_image_premul                    (Image_Entry *ie); /*2*/
EAPI void              evas_common_image_set_alpha_sparse          (Image_Entry *ie); /*2*/
/* EAPI RGBA_Image   *evas_common_image_alpha_create      (int w, int h); */
/* EAPI RGBA_Image   *evas_common_image_create            (int w, int h); */
EAPI RGBA_Image       *evas_common_image_new                       (unsigned int w, unsigned int h, unsigned int alpha);
EAPI Evas_Cache_Image *evas_common_image_cache_get                 (void);
#ifdef EVAS_CSERVE2
EAPI Evas_Cache2      *evas_common_image_cache2_get                 (void);
#endif

EAPI void              evas_common_image_set_cache                 (unsigned int size);
EAPI int               evas_common_image_get_cache                 (void);

EAPI RGBA_Image       *evas_common_image_line_buffer_obtain        (int len);
EAPI void              evas_common_image_line_buffer_release       (RGBA_Image *im);

EAPI RGBA_Image       *evas_common_image_alpha_line_buffer_obtain  (int len);
EAPI void              evas_common_image_alpha_line_buffer_release (RGBA_Image *im);
EAPI void              evas_common_image_alpha_line_buffer_free    (RGBA_Image *im);

EAPI RGBA_Image       *evas_common_load_image_from_file            (const char *file, const char *key, Evas_Image_Load_Opts *lo, int *error);
EAPI RGBA_Image       *evas_common_load_image_from_mmap            (Eina_File *f, const char *key, Evas_Image_Load_Opts *lo, int *error);
EAPI int               evas_common_save_image_to_file              (RGBA_Image *im, const char *file, const char *key, int quality, int compress, const char *encoding);

EAPI void evas_common_rgba_image_scalecache_init(Image_Entry *ie);
EAPI void evas_common_rgba_image_scalecache_shutdown(Image_Entry *ie);
EAPI void evas_common_rgba_image_scalecache_size_set(unsigned int size);
EAPI unsigned int evas_common_rgba_image_scalecache_size_get(void);
EAPI void evas_common_rgba_image_scalecache_flush(void);
EAPI void evas_common_rgba_image_scalecache_dump(void);
EAPI void evas_common_rgba_image_scalecache_prune(void);
EAPI void
  evas_common_rgba_image_scalecache_prepare(Image_Entry *ie, RGBA_Image *dst,
                                            RGBA_Draw_Context *dc, int smooth,
                                            int src_region_x, int src_region_y,
                                            int src_region_w, int src_region_h,
                                            int dst_region_x, int dst_region_y,
                                            int dst_region_w, int dst_region_h);
EAPI void
  evas_common_rgba_image_scalecache_do(Image_Entry *ie, RGBA_Image *dst,
                                       RGBA_Draw_Context *dc, int smooth,
                                       int src_region_x, int src_region_y,
                                       int src_region_w, int src_region_h,
                                       int dst_region_x, int dst_region_y,
                                       int dst_region_w, int dst_region_h);
EAPI Eina_Bool
  evas_common_rgba_image_scalecache_do_cbs(Image_Entry *ie, RGBA_Image *dst,
                                           RGBA_Draw_Context *dc, int smooth,
                                           int src_region_x, int src_region_y,
                                           int src_region_w, int src_region_h,
                                           int dst_region_x, int dst_region_y,
                                           int dst_region_w, int dst_region_h,
                                           Evas_Common_Scale_In_To_Out_Clip_Cb cb_sample,
                                           Evas_Common_Scale_In_To_Out_Clip_Cb cb_smooth);


EAPI int evas_common_load_rgba_image_module_from_file (Image_Entry *im);
EAPI int evas_common_load_rgba_image_data_from_file   (Image_Entry *im);
EAPI double evas_common_load_rgba_image_frame_duration_from_file(Image_Entry *im, int start_frame, int frame_num);

void _evas_common_rgba_image_post_surface(Image_Entry *ie);

EAPI Eina_Bool evas_common_extension_can_load_get(const char *file);

#endif /* _EVAS_IMAGE_H */
