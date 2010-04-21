#include "osmcanvas.h"
#include "parse.h"

BEGIN_EVENT_TABLE(OsmCanvas, Canvas)
        EVT_MOUSEWHEEL(OsmCanvas::OnMouseWheel)
END_EVENT_TABLE()


OsmCanvas::OsmCanvas(wxWindow *parent, wxString const &fileName)
    : Canvas(parent)
{

    FILE *infile = fopen(fileName.mb_str(wxConvUTF8), "r");

    if (!infile)
    {
        puts("could not open file:");
        puts(fileName.mb_str(wxConvUTF8));
        abort();
    }

    m_data = parse_osm(infile);

    fclose(infile);
    
}

void OsmCanvas::Render()
{
    int w = m_backBuffer.GetWidth();
    int h = m_backBuffer.GetHeight();

    double xscale = w / (m_data->m_maxlon - m_data->m_minlon);
    double yscale = h / (m_data->m_maxlat - m_data->m_minlat);
    double xoff = m_data->m_minlon;
    double yoff = m_data->m_minlat;

    wxMemoryDC dc;
    dc.SelectObject(m_backBuffer);
    dc.SetPen(*wxBLACK_PEN);
    dc.Clear();

    for (OsmWay *w = static_cast<OsmWay *>(m_data->m_ways.m_content); w ; w = static_cast<OsmWay *>(w->m_next))
    {
        for (unsigned j = 0; j < w->m_numResolvedNodes - 1; j++)
        {
            OsmNode *node1 = w->m_resolvedNodes[j];
            OsmNode *node2 = w->m_resolvedNodes[j+1];
            if (node1 && node2)
            {
                int x1 = (node1->m_lon - xoff) * xscale;
                int y1 = (node1->m_lat - yoff) * yscale;
                int x2 = (node2->m_lon - xoff) * xscale;
                int y2 = (node2->m_lat - yoff) * yscale;
                y1 = h - y1;
                y2 = h - y2;
                dc.DrawLine(x1,y1, x2,y2);
            }
            
        }
    }
    
}

OsmCanvas::~OsmCanvas()
{
    delete m_data;
}

void OsmCanvas::OnMouseWheel(wxMouseEvent &evt)
{

}

