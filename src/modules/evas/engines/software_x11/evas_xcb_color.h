#ifndef EVAS_XCB_COLOR_H
# define EVAS_XCB_COLOR_H

# include "evas_engine.h"

void evas_software_xcb_color_init(void);
Convert_Pal *evas_software_xcb_color_allocate(xcb_connection_t *conn, xcb_colormap_t cmap, xcb_visualtype_t *vis, Convert_Pal_Mode colors);
void evas_software_xcb_color_deallocate(xcb_connection_t *conn, xcb_colormap_t cmap, xcb_visualtype_t *vis, Convert_Pal *pal);

#endif
