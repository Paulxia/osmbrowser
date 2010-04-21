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
        Render();
        Draw(NULL);
}

void Canvas::OnPaint(wxPaintEvent &WXUNUSED(event))
{
//    int i,j;
        Render();
        wxPaintDC dc(this);
        dc.Clear();

        Draw(&dc);
        
}

BEGIN_EVENT_TABLE(Canvas, wxWindow)
        EVT_PAINT(Canvas::OnPaint)
        EVT_SIZE(Canvas::OnSize)
END_EVENT_TABLE()
