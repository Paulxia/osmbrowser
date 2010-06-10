// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#ifndef __WXCAIRO_H__
#define __WXCAIRO_H__

#include <cairo.h>
#include <wx/image.h>

void OverlayImageSurface(cairo_surface_t *src, wxImage *dest);

#endif
