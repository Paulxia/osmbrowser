#ifndef __WXCAIRO_H__
#define __WXCAIRO_H__

#include <cairo.h>
#include <wx/image.h>

void OverlayImageSurface(cairo_surface_t *src, wxImage *dest);

#endif
