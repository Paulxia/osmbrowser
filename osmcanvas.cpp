#include "osmcanvas.h"
#include "parse.h"

BEGIN_EVENT_TABLE(OsmCanvas, Canvas)
        EVT_MOUSEWHEEL(OsmCanvas::OnMouseWheel)
        EVT_LEFT_DOWN(OsmCanvas::OnLeftDown)
//        EVT_MIDDLE_DOWN(OsmCanvas::OnMiddleDown)
//        EVT_RIGHT_DOWN(OsmCanvas::OnRightDown)
        EVT_LEFT_UP(OsmCanvas::OnLeftUp)
//        EVT_MIDDLE_UP(OsmCanvas::OnMiddleUp)
//        EVT_RIGHT_UP(OsmCanvas::OnRightUp)
        EVT_MOTION(OsmCanvas::OnMouseMove)
END_EVENT_TABLE()


OsmCanvas::OsmCanvas(wxWindow *parent, wxString const &fileName)
    : Canvas(parent)
{

    m_dragging = false;
    wxString binFile = fileName;
    binFile.Append(wxT(".cache"));

    FILE *infile = fopen(binFile.mb_str(wxConvUTF8), "r");

    if (infile)
    {
        printf("found preprocessed file %s, opening that instead.\n", (char const *)(binFile.mb_str(wxConvUTF8)) );
        m_data = parse_binary(infile);
    }
    else
    {
        infile = fopen(fileName.mb_str(wxConvUTF8), "r");
    
        if (!infile)
        {
            puts("could not open file:");
            puts(fileName.mb_str(wxConvUTF8));
            abort();
        }
    
        if (fileName.EndsWith(wxT(".cache")))
        {
            m_data = parse_binary(infile);
        }
        else
        {
            m_data = parse_osm(infile);
    
            FILE *outFile = fopen(binFile.mb_str(wxConvUTF8) , "wb");
    
            if (outFile)
            {
            
                printf("writing cache\n");
                write_binary(m_data, outFile);
                fclose(outFile);
            }
        }
    }
    fclose(infile);

    double xscale = 1200.0 / (m_data->m_maxlon - m_data->m_minlon);
    double yscale = 1200.0 / (m_data->m_maxlon - m_data->m_minlon);
    m_scale = xscale < yscale ? xscale : yscale;
    m_xOffset = m_data->m_minlon;
    m_yOffset = m_data->m_minlat;

    m_lastX = m_lastY = 0;
    
}

void OsmCanvas::RenderWay(OsmWay *w, wxColour lineColour, bool poly, wxColour fillColour)
{
    int width = m_backBuffer.GetWidth();
    int height = m_backBuffer.GetHeight();
    wxMemoryDC dc;
    dc.SelectObject(m_backBuffer);
    double xScale = cos(m_yOffset * M_PI / 180) * m_scale;

    double sxMax = m_xOffset + width / xScale;
    double syMax = m_yOffset + height / xScale;

    wxPen pen;
    wxBrush brush;

    pen.SetColour(lineColour);
    brush.SetColour(fillColour);

    dc.SetPen(pen);
    dc.SetBrush(brush);
    if (!poly)
    {
        for (unsigned j = 0; j < w->m_numResolvedNodes - 1; j++)
        {
            OsmNode *node1 = w->m_resolvedNodes[j];
            OsmNode *node2 = w->m_resolvedNodes[j+1];
        
            if (node1 && node2)
            {
                double lon1 = node1->m_lon;
                double lon2 = node2->m_lon;
                double lat1 = node1->m_lat;
                double lat2 = node2->m_lat;
        
        
                if ( (lon1 > m_xOffset && lon1 < sxMax && lat1 > m_yOffset && lat1 < syMax)
                     || (lon2 > m_xOffset && lon2 < sxMax && lat2 > m_yOffset && lat2 < syMax))
                {
                        int x1 = (lon1 - m_xOffset) * xScale;
                        int y1 = (lat1 - m_yOffset) * m_scale;
                        int x2 = (lon2 - m_xOffset) * xScale;
                        int y2 = (lat2 - m_yOffset) * m_scale;
                        y1 = height - y1;
                        y2 = height - y2;
        
        //                printf("drawing %g %g %g %g s%g %d %d\n",node1->m_lon, node1->m_lat, m_xOffset, m_yOffset, m_scale, x1,y1);
        
                        dc.DrawLine(x1,y1, x2,y2);
                }
            }
        }
    }
    else
    {
        StartPolygon();
        for (unsigned j = 0; j < w->m_numResolvedNodes; j++)
        {
            OsmNode *node1 = w->m_resolvedNodes[j];
        
            if (node1)
            {
                double lon1 = node1->m_lon;
                double lat1 = node1->m_lat;
        
        
                int x1 = (lon1 - m_xOffset) * xScale;
                int y1 = (lat1 - m_yOffset) * m_scale;
                y1 = height - y1;
        
                AddPoint(x1,y1);
            }
            
        }
        EndPolygon(&dc);
    }
}



void OsmCanvas::Render()
{
    int w = m_backBuffer.GetWidth();
    int h = m_backBuffer.GetHeight();

    wxMemoryDC dc;
    dc.SelectObject(m_backBuffer);
    dc.Clear();

    double xScale = cos(m_yOffset * M_PI / 180) * m_scale;

    double sxMax = m_xOffset + w / xScale;
    double syMax = m_yOffset + h / xScale;

    wxColour lineC(0,0,0), fillC(255,255,255);

    OsmTag boundary("boundary"), border("border"), water("natural", "water"), wood("natural", "wood"), park("leisure","park"), building("building"), highway("highway"), cycleway("highway", "cycleway"), coastline("natural", "coastline");
    OsmTag natural("natural"), snelweg("highway", "motorway"), trein("railway", "rail"), polygon("type", "multipolygon");

    bool poly = false;
    for (OsmWay *w = static_cast<OsmWay *>(m_data->m_ways.m_content); w ; w = static_cast<OsmWay *>(w->m_next))
    {
        if (!( w->HasTag(coastline) || (w->HasTag(boundary) && w->HasTag("admin_level", "2")) || w->HasTag(snelweg)  || w->HasTag(natural) || w->HasTag(trein) ))
        {
//            continue;
        }
    
        if (w->HasTag(water))
        {
            lineC.Set(0,0,255);
            fillC.Set(0,0,255);
            poly = true;
        } else if (w->HasTag(wood) || w->HasTag(park))
        {
            lineC.Set(100,255,100);
            fillC.Set(180, 255, 180);
            poly = true;
        }
        else if (w->HasTag(building))
        {
            lineC.Set(100,0,0);
            fillC.Set(100,0,0);
            poly = true;
        }
        else if (w->HasTag(cycleway))
        {
            lineC.Set(255,100,50);
            poly = false;
        }
        else if (w->HasTag(snelweg))
        {
            lineC.Set(200,0,0);
            poly = false;
        }
        else if (w->HasTag(trein))
        {
            lineC.Set(155,155,0);
            poly = false;
        }
        else if (w->HasTag(highway))
        {
            lineC.Set(0,0,0);
            poly = false;
        }
        else
        {
            lineC.Set(180,180,150);
            fillC.Set(180,180,150);
            poly = w->HasTag(polygon);
        }

        RenderWay(w, lineC, poly, fillC);


    }
}

OsmCanvas::~OsmCanvas()
{
    delete m_data;
}

void OsmCanvas::OnMouseWheel(wxMouseEvent &evt)
{
    double scaleCorrection = cos(m_yOffset * M_PI / 180);
    double w = evt.GetWheelRotation() / 1200.0;
    int h = m_backBuffer.GetHeight();

    double xm = evt.m_x / (m_scale * scaleCorrection);
    double ym = (h - evt.m_y) / m_scale;

    m_xOffset += xm;
    m_yOffset += ym;

    m_scale = m_scale * (1.0 + w);

    xm = evt.m_x / (m_scale * scaleCorrection);
    ym = (h - evt.m_y) / m_scale;
    m_xOffset -= xm;
    m_yOffset -= ym;


    Render();
    Draw();
}

void OsmCanvas::OnMouseMove(wxMouseEvent &evt)
{
    double scaleCorrection = cos(m_yOffset * M_PI / 180);

    if (m_dragging)
    {
        int idx = evt.m_x - m_lastX;
        int idy = evt.m_y - m_lastY;


        m_lastX = evt.m_x;
        m_lastY = evt.m_y;
        double dx = idx / (m_scale * scaleCorrection);
        double dy = idy / m_scale;

        m_xOffset -= dx;
        m_yOffset += dy;


        Render();
        Draw();
        
    }
}


void OsmCanvas::OnLeftDown(wxMouseEvent &evt)
{
    if (!HasCapture())
        CaptureMouse();

    m_lastX = evt.m_x;
    m_lastY = evt.m_y;

    m_dragging = true;
}


void OsmCanvas::OnLeftUp(wxMouseEvent &evt)
{
    if (HasCapture())
        ReleaseMouse();
    m_dragging = false;

}

