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


TileWay::TileWay(OsmWay *way, TileList *allTiles, TileWay *next)
    : ListObject(next)
{
    m_way = way;
    m_tiles = allTiles;
    m_tiles->Ref();
}

TileWay::~TileWay()
{
    m_tiles->UnRef();
}


void TileRenderer::RenderTiles(OsmCanvas *canvas, double lon, double lat, double w, double h)
{

    DRect bb(lon, lat);
    bb.SetSize(w, h);

    TileList *renderedTiles = NULL;

    TileList *visibleTiles = GetTiles(bb);

    if (!visibleTiles)
    {
        return;
    }

    bool fast = visibleTiles->GetSize() > 16;
    
    for (TileList *tl = visibleTiles; tl; tl = static_cast<TileList *>(tl->m_next))
    {
        OsmTile *t = tl->m_tile;
//        printf("render tile %u  (%g,%g)-(%g, %g) bb=(%g, %g)-(%g, %g)\n", t->m_id, t->m_x, t->m_y, t->m_x + t->m_w, t->m_y + t->m_h, lon, lat, lon+w, lat+h);
//        canvas->DrawTileOutline(t, 200,200,200);
        if (t->OverLaps(bb))
        {
//            canvas->DrawTileOutline(t, 200,0,0);
//            printf("    is visible\n");
//            printf("rendering tile %u\n", t->m_id);
            for (TileWay *w = t->m_ways; w; w = static_cast<TileWay *>(w->m_next))
            {
//                printf("    render way %u\n",   w->m_way->m_id);
                bool already = false;
                // loop over all tiles that contaoin this way
                for (TileList *a = w->m_tiles; a; a = static_cast<TileList *>(a->m_next))
                {
                    // loop over all tiles already drawn
                    for (TileList *o = renderedTiles; o ; o = static_cast<TileList *>(o->m_next))
                    {
                         if (o->m_tile->m_id == a->m_tile->m_id)
                         {
                            already = true;
                            break;
                         }
                    }

                    if (already)
                    {
//                        printf("    already drawn %u\n",   w->m_way->m_id);
                        break;
                    }
                }

                if (!already)
                {
                    canvas->RenderWay(w->m_way, fast);
                }
                
            }
            renderedTiles = new TileList(t, renderedTiles);

        }
        
    }

    if (visibleTiles)
        visibleTiles->DestroyList();

    if (renderedTiles)
        renderedTiles->DestroyList();
}


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

    m_tileRenderer = new TileRenderer(m_data->m_minlon, m_data->m_minlat, m_data->m_maxlon, m_data->m_maxlat, .05, .04);

    m_tileRenderer->AddWays(static_cast<OsmWay *>(m_data->m_ways.m_content));

    m_fastTags = NULL;
//    m_fastTags = new OsmTag("boundary");
    m_fastTags = new OsmTag("highway", "motorway", m_fastTags);
    m_fastTags = new OsmTag("natural", "coastline", m_fastTags);
    m_fastTags = new OsmTag("natural", "water", m_fastTags);
    m_fastTags = new OsmTag("railway", "rail", m_fastTags);
    
}


void OsmCanvas::Rect(wxString const &text, double lon1, double lat1, double lon2, double lat2, int border, int r, int g, int b)
{
    int width = m_backBuffer.GetWidth();
    int height = m_backBuffer.GetHeight();
    wxMemoryDC dc;
    dc.SelectObject(m_backBuffer);
    double xScale = cos(m_yOffset * M_PI / 180) * m_scale;

    double sxMax = m_xOffset + width / xScale;
    double syMax = m_yOffset + height / xScale;

    wxPen pen;

    pen.SetColour(r,g,b);

    dc.SetPen(pen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);

    int x1 = (lon1 - m_xOffset) * xScale;
    int y1 = (lat1 - m_yOffset) * m_scale;

    int x2 = (lon2 - m_xOffset) * xScale;
    int y2 = (lat2 - m_yOffset) * m_scale;

    y1 = height - y1;
    y2 = height - y2;

    int xd = x1 < x2 ? x1 : x2;
    int yd = y1 < y2 ? y1 : y2;

    int wd = x2 - x1;
    int hd = y1 - y2;

    if (wd < 0)
        wd = -wd;

    if (hd < 0)
        hd = -hd;
    
    

    dc.DrawRectangle(xd+border, yd-border, wd - 2*border, hd - 2*border);

    dc.DrawText(text, xd + wd /2, yd + hd /2);

}

void OsmCanvas::DrawTileOutline(OsmTile *t, int r, int g, int b)
{


    wxString id;
    
    id.Printf(wxT("tile: %u"), t->m_id);

    Rect(id, t->m_x, t->m_y, t->m_x + t->m_w, t->m_y + t->m_h, 3, r,g,b);

    
}


// render using default colours. should plug in rule engine here
void OsmCanvas::RenderWay(OsmWay *w, bool fast)
{

    if (fast)
    {
        bool draw = false;
        for (OsmTag *t = m_fastTags; t; t = static_cast<OsmTag *>(t->m_next))
        {
            if (w->HasTag(*t))
                draw = true;
        }

        if (!draw)
        {
            return;
        }
    }
    
    RenderWay(w, wxColour(0,0,0), false, wxColour(255,255,255));
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
    double xScale = cos(m_yOffset * M_PI / 180) * m_scale;
    wxMemoryDC dc;
    dc.SelectObject(m_backBuffer);
    dc.Clear();

    Rect(wxEmptyString, m_data->m_minlon, m_data->m_minlat, m_data->m_maxlon, m_data->m_maxlat, 0, 0,255,0);

    if (m_tileRenderer)
    {
        m_tileRenderer->RenderTiles(this, m_xOffset, m_yOffset, w / xScale, h / m_scale);
    }
    else
    {
        double sxMax = m_xOffset + w / xScale;
        double syMax = m_yOffset + h / m_scale;
    
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

