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

    double xscale = w / (m_data->maxlon - m_data->minlon);
    double yscale = h / (m_data->maxlat - m_data->minlat);
    double xoff = m_data->minlon;
    double yoff = m_data->minlat;

    wxMemoryDC dc;
    dc.SelectObject(m_backBuffer);
    dc.SetPen(*wxBLACK_PEN);
    dc.Clear();

    for (unsigned i = 0; i < m_data->numWays; i++)
    {
        for (unsigned j = 0; j < m_data->ways[i].numResolvedNodes - 1; j++)
        {
            OSMNODE *node1 = m_data->ways[i].resolvedNodes[j];
            OSMNODE *node2 = m_data->ways[i].resolvedNodes[j+1];
            if (node1 && node2)
            {
                int x1 = (node1->lon - xoff) * xscale;
                int y1 = (node1->lat - yoff) * yscale;
                int x2 = (node2->lon - xoff) * xscale;
                int y2 = (node2->lat - yoff) * yscale;
                y1 = h - y1;
                y2 = h - y2;
                dc.DrawLine(x1,y1, x2,y2);
            }
            
        }
    }
    
}

OsmCanvas::~OsmCanvas()
{
    osmdata_destroy(m_data);
}

void OsmCanvas::OnMouseWheel(wxMouseEvent &evt)
{

}

