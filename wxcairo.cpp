// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#include "wxcairo.h"
#include <wx/bitmap.h>
#include <wx/rawbmp.h>
#include <wx/dcmemory.h>
#include <wx/log.h>


bool HaveRawBitmap()
{
	typedef wxPixelData<wxBitmap, wxNativePixelFormat> PixelData;

	wxBitmap bmp(10,10);
	PixelData data(bmp);

	return data;
}

void OverlayImageSurface(cairo_surface_t *src, wxBitmap *dest)
{
	typedef wxPixelData<wxBitmap, wxNativePixelFormat> PixelData;
	static bool warned = false;

	PixelData data(*dest);
	if ( !data )
	{
		if (!warned)
		{
			wxLogDebug("warning, using slow fallback via wximage\n");
			warned = true;
		}
		wxImage tmp = dest->ConvertToImage();
		OverlayImageSurface(src, &tmp);
		wxBitmap tmpB(tmp);

		wxMemoryDC dc;
		dc.SelectObject(*dest);
		dc.DrawBitmap(tmpB, 0, 0);
		return;
	}


	PixelData::Iterator p(data);

	int w = cairo_image_surface_get_width(src);
	int srcStride = cairo_image_surface_get_stride(src);
	int h = cairo_image_surface_get_height(src);

	if (dest->GetWidth() < w)
		w = dest->GetWidth();

	if (dest->GetHeight() < h)
		h = dest->GetHeight();

	unsigned char const *srcData  = cairo_image_surface_get_data(src);

	for (int y = 0; y < h; y++)
	{
		PixelData::Iterator rowStart = p;
		wxUint32 const *srcScanline = reinterpret_cast<wxUint32 const *>(srcData + y * srcStride);

		for ( int x = 0; x < w; ++x, ++p )
		{

			int dstR = p.Red();
			int dstG = p.Green();
			int dstB = p.Blue();

			int srcA = (srcScanline[x] >> 24) & 0xFF;
			int srcR = (srcScanline[x] >> 16) & 0xFF;
			int srcG = (srcScanline[x] >> 8) & 0xFF;
			int srcB = (srcScanline[x] >> 0) & 0xFF;

//			printf("src %d %d %d %d\n", srcA, srcR, srcG, srcB);

			dstR = dstR  - ((dstR * srcA)/0xFF) + srcR;
			dstG = dstG  - ((dstG * srcA)/0xFF) + srcG;
			dstB = dstB  - ((dstB * srcA)/0xFF) + srcB;

			p.Red() = dstR;
			p.Green() = dstG;
			p.Blue() = dstB;
		}

		p = rowStart;
		p.OffsetY(data, 1);

	}

}



void OverlayImageSurface(cairo_surface_t *src, wxImage *dest)
{
	int w = cairo_image_surface_get_width(src);
	int srcStride = cairo_image_surface_get_stride(src);
	int h = cairo_image_surface_get_height(src);
	int dstStride = dest->GetWidth() * 3;

	if (dest->GetWidth() < w)
		w = dest->GetWidth();

	if (dest->GetHeight() < h)
		h = dest->GetHeight();

	unsigned char const *srcData  = cairo_image_surface_get_data(src);

	unsigned char *dstData = dest->GetData();

	for (int y = 0; y < h; y++)
	{
		wxUint32 const *srcScanline = reinterpret_cast<wxUint32 const *>(srcData + y * srcStride);
		unsigned char *dstScanline = dstData + y * dstStride;
		for (int x = 0; x < w ; x++)
		{
			int dstR = dstScanline[3*x];
			int dstG = dstScanline[3*x+1];
			int dstB = dstScanline[3*x+2];

			int srcA = (srcScanline[x] >> 24) & 0xFF;
			int srcR = (srcScanline[x] >> 16) & 0xFF;
			int srcG = (srcScanline[x] >> 8) & 0xFF;
			int srcB = (srcScanline[x] >> 0) & 0xFF;

//			printf("src %d %d %d %d\n", srcA, srcR, srcG, srcB);

			dstR = dstR  - ((dstR * srcA)/0xFF) + srcR;
			dstG = dstG  - ((dstG * srcA)/0xFF) + srcG;
			dstB = dstB  - ((dstB * srcA)/0xFF) + srcB;

			dstScanline[3*x] = static_cast<unsigned char>(dstR);
			dstScanline[3*x+1] = static_cast<unsigned char>(dstG);
			dstScanline[3*x+2] = static_cast<unsigned char>(dstB);
		}
	}
}

