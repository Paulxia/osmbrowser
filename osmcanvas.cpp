#include "osmcanvas.h"
#include "parse.h"
#include "rulecontrol.h"

BEGIN_EVENT_TABLE(OsmCanvas, Canvas)
	EVT_MOUSEWHEEL(OsmCanvas::OnMouseWheel)
	EVT_LEFT_DOWN(OsmCanvas::OnLeftDown)
//        EVT_MIDDLE_DOWN(OsmCanvas::OnMiddleDown)
//        EVT_RIGHT_DOWN(OsmCanvas::OnRightDown)
	EVT_LEFT_UP(OsmCanvas::OnLeftUp)
//        EVT_MIDDLE_UP(OsmCanvas::OnMiddleUp)
//        EVT_RIGHT_UP(OsmCanvas::OnRightUp)
	EVT_MOTION(OsmCanvas::OnMouseMove)
	EVT_TIMER(-1, OsmCanvas::OnTimer)
END_EVENT_TABLE()


TileWay::TileWay(OsmWay *way, TileList *allTiles, TileWay *next)
	: ListObject(next)
{
	m_way = way;
	m_tiles = allTiles;
	if (m_tiles)
		m_tiles->Ref();
}

TileWay::~TileWay()
{
	m_tiles->UnRef();
}

bool TileRenderer::RenderTiles(wxApp *app, OsmCanvas *canvas, double lon, double lat, double w, double h, bool restart)
{
	bool mustCancel = false;

	DRect bb(lon, lat);
	bb.SetSize(w, h);

	if (restart || !m_visibleTiles)
	{
		if (m_visibleTiles)
			m_visibleTiles->DestroyList();

		if (m_renderedTiles)
			m_renderedTiles->DestroyList();

		m_renderedTiles = NULL;
		m_visibleTiles = GetTiles(bb);

		m_curTile = m_visibleTiles;
		m_curLayer = 0;
	}

	if (!m_visibleTiles)
	{
		return false;
	}

	bool fast = false;//m_visibleTiles->GetSize() > 16;

	while (!mustCancel && m_curLayer < NUMLAYERS)
	{
		while (m_curTile && !mustCancel)
		{
			OsmTile *t = m_curTile->m_tile;
			if (t->OverLaps(bb))
			{
				for (TileWay *w = t->m_ways; w && !mustCancel; w = static_cast<TileWay *>(w->m_next))
				{

					bool already = false;
					// loop over all tiles that contaoin this way
					for (TileList *a = w->m_tiles; a; a = static_cast<TileList *>(a->m_next))
					{
						// loop over all tiles already drawn
						for (TileList *o = m_renderedTiles; o ; o = static_cast<TileList *>(o->m_next))
						{
							 if (o->m_tile->m_id == a->m_tile->m_id)
							 {
								already = true;
								break;
							 }
						}
	
						if (already)
						{
							break;
						}
					}
	
					if (!already)
					{
						canvas->RenderWay(w->m_way, fast, m_curLayer);
					}
				}	// for way
			}  // if overlaps
			mustCancel = app->Pending();
			m_renderedTiles = new TileList(t, m_renderedTiles);
			m_curTile = static_cast<TileList *>(m_curTile->m_next);
		}	 // while curTile
		
		if (!m_curTile)
		{
			m_curLayer++;
			m_curTile = m_visibleTiles;
			m_renderedTiles->DestroyList();
			m_renderedTiles = NULL;
		}
	}  // while curLayer

	return !mustCancel;
}


OsmCanvas::OsmCanvas(wxApp * app, wxWindow *parent, wxString const &fileName)
	: Canvas(parent)
{
	m_timer.SetOwner(this);
	m_done = false;
	m_restart = true;
	m_app = app;
	m_drawRuleControl = NULL;
	m_colorRules = NULL;
	m_dragging = false;
	m_locked = true;
	wxString binFile = fileName;

	binFile.Append(wxT(".cache"));

	FILE *infile;
	if (fileName.IsSameAs(wxT("-")))
	{
		binFile = wxString(wxT("stdin.cache"));
	}

	infile = fopen(binFile.mb_str(wxConvUTF8), "r");
	
	if (infile)
	{
		printf("found preprocessed file %s, opening that instead.\n", (char const *)(binFile.mb_str(wxConvUTF8)) );
		m_data = parse_binary(infile);
	}
	else
	{
		if (fileName.IsSameAs(wxT("-")))
		{
			infile = stdin;
		}
		else
		{
			infile = fopen(fileName.mb_str(wxConvUTF8), "r");
		}

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
//    m_fastTags = new OsmTag("natural", "water", m_fastTags);
	m_fastTags = new OsmTag("railway", "rail", m_fastTags);

	m_timer.Start(100);
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
void OsmCanvas::RenderWay(OsmWay *w, bool fast, int curLayer)
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

	if ((!m_drawRuleControl) || m_drawRuleControl->Evaluate(w))
	{
		wxColour c = wxColour(150,150,150);
		bool poly = false;
		int layer = 1;
		if (m_colorRules)
		{
			for (int i = 0; i < m_colorRules->m_num; i++)
			{
				if (m_colorRules->m_rules[i]->Evaluate(w))
				{
					c = m_colorRules->m_pickers[i]->GetColour();
					poly = m_colorRules->m_checkBoxes[i]->IsChecked();
					layer = m_colorRules->m_layers[i]->GetSelection();
					break; // stop after first match
				}
			}
		}
		if (curLayer == layer)
		{
			RenderWay(w, c, poly, c);
		}
	}
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



void OsmCanvas::Render(bool force)
{

	if (force)
	{
		m_restart = true;
	}
	
	if (!(m_backBuffer.IsOk()))
	{
		m_restart = true;
		return;
	}

	if (!IsShownOnScreen() || m_locked)
	{
		return;
	}
	
	if (!m_restart && m_done)
	{
		return;
	}
	int w = m_backBuffer.GetWidth();
	int h = m_backBuffer.GetHeight();
	double xScale = cos(m_yOffset * M_PI / 180) * m_scale;

	if (m_restart)
	{
		wxMemoryDC dc;
		dc.SelectObject(m_backBuffer);
		dc.Clear();
	}
	Rect(wxEmptyString, m_data->m_minlon, m_data->m_minlat, m_data->m_maxlon, m_data->m_maxlat, 0, 0,255,0);

	m_done = m_tileRenderer->RenderTiles(m_app, this, m_xOffset, m_yOffset, w / xScale, h / m_scale, m_restart);
	m_restart = false;

	Draw(NULL);
	return;
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

	Redraw();
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

		Redraw();
		
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


void OsmCanvas::SetDrawRuleControl(RuleControl *r)
{
	m_drawRuleControl = r;
}


void OsmCanvas::SetColorRules(ColorRules *r)
{
	m_colorRules = r;
}
