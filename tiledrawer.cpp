#include "tiledrawer.h"
#include "rulecontrol.h"

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

bool TileDrawer::RenderTiles(wxApp *app, OsmCanvas *canvas, double lon, double lat, double w, double h, bool restart)
{
	bool mustCancel = false;

	DRect bb(lon, lat, w, h);

//	canvas->Rect(wxEmptyString, bb, 0, 255,0,0);

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

	while (!mustCancel && m_curLayer < NUMLAYERS)
	{
		while (m_curTile && !mustCancel)
		{
			OsmTile *t = m_curTile->m_tile;
//			canvas->Rect(wxEmptyString, *t, -1, 0,255,0);
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
						RenderWay(w->m_way, m_curLayer);
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

void TileDrawer::Rect(wxString const &text, double lon1, double lat1, double lon2, double lat2, double border, int r, int g, int b)
{
	m_renderer->SetLineColor(r,g,b);
	m_renderer->Rect(lon1, lat1, lon2 - lon1, lat2 - lat1, border , r, g, b);
	m_renderer->DrawCenteredText(text.mb_str(wxConvUTF8), (lon1 + lon2)/2, (lat1 + lat2)/2, 0, r, g, b);
}

// render using default colours. should plug in rule engine here
void TileDrawer::RenderWay(OsmWay *w, int curLayer)
{

	if ((!m_drawRule) || m_drawRule->Evaluate(w))
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


void TileDrawer::RenderWay(OsmWay *w, wxColour lineColour, bool poly, wxColour fillColour)
{


	m_renderer->SetLineColor(lineColour.Red(), lineColour.Green(), lineColour.Blue());
	m_renderer->SetFillColor(fillColour.Red(), fillColour.Green(), fillColour.Blue());

	if (!poly)
	{
		m_renderer->Begin(Renderer::R_LINE);
		for (unsigned j = 0; j < w->m_numResolvedNodes; j++)
		{
			OsmNode *node = w->m_resolvedNodes[j];
		
			if (node)
			{
				m_renderer->AddPoint(node->m_lon, node->m_lat);
			}
			else
			{
				m_renderer->End();
				m_renderer->Begin(Renderer::R_LINE);
			}
		
		}
		m_renderer->End();
	}
	else
	{
		m_renderer->Begin(Renderer::R_POLYGON);
		for (unsigned j = 0; j < w->m_numResolvedNodes; j++)
		{
			OsmNode *node = w->m_resolvedNodes[j];
		
			if (node)
			{
				m_renderer->AddPoint(node->m_lon, node->m_lat);
			}
			
		}
		m_renderer->End();
	}
}


