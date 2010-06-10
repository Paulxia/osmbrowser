// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#ifndef __WXCANVAS_H__
#define __WXCANVAS_H__

#include <wx/window.h>
#include <wx/dcmemory.h>

class Canvas
	: public wxWindow
{
	public:
		Canvas(wxWindow *parent);
		~Canvas();


		wxBitmap m_backBuffer;
		// will be called whenever the backbuffer is destroyed
		// override to redraw backbuffer
		virtual void Render(bool force = false)
		{
		};

	protected:
		void OnPaint(wxPaintEvent &event);
		void OnSize(wxSizeEvent &event);
                // blits backbuffer to dc
		void Draw(wxDC *onto = NULL);

		DECLARE_EVENT_TABLE();
};





#endif
