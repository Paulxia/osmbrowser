// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#include "wxcanvas.h"
#include <wx/dcmemory.h>
#include <stdio.h>


Canvas::Canvas(wxWindow *parent)
	: wxWindow(parent, -1)
{
	wxSize s = GetClientSize();
	m_backBuffer.Create(s.x, s.y);
}


Canvas::~Canvas()
{
}

void Canvas::Draw(wxDC *onto)
{
	if (!(m_backBuffer.IsOk()))
		return;

	bool mustDeleteDC = false;

	if (!onto)
	{
		onto = new wxClientDC(this);
		mustDeleteDC =true;
	}

	onto->DrawBitmap(wxBitmap(m_backBuffer),0,0,false);
	if (mustDeleteDC)
	{
		delete onto;
	}

}


void Canvas::OnSize(wxSizeEvent &WXUNUSED(event))
{
	wxSize s = GetClientSize();
	m_backBuffer.Create(s.x, s.y);
	Render(true);
	Draw(NULL);
}

void Canvas::OnPaint(wxPaintEvent &WXUNUSED(event))
{
//    int i,j;
	wxPaintDC dc(this);
	dc.Clear();

	if (!IsShownOnScreen())
	    return;

	Render();

	Draw(&dc);
	
}

BEGIN_EVENT_TABLE(Canvas, wxWindow)
	EVT_PAINT(Canvas::OnPaint)
	EVT_SIZE(Canvas::OnSize)
END_EVENT_TABLE()
