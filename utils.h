// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#ifndef __UTILS_H__
#define __UTILS_H__

#include <wx/image.h>
#include <wx/dcmemory.h>
#include <wx/bitmap.h>

inline void ClearToWhite(wxImage *image)
{

	size_t size = image->GetWidth() * image->GetHeight() *3;

	memset(image->GetData(), 255, size);
}

inline void ClearToWhite(wxBitmap *image)
{
	wxMemoryDC dc;
	dc.SelectObject(*image);

	dc.SetBackground(*wxWHITE_BRUSH);
	dc.Clear();
}




#endif

