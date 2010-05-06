#ifndef __TILEDRAWER_H__
#define __TILEDRAWER_H__

#include "osm.h"
#include "renderer.h"
#include <wx/app.h>

class TileList;
class RuleControl;
class ColorRules;


class TileWay
	: public ListObject
{
	public:
		TileWay(OsmWay *way, TileList *allTiles, TileWay *next);

		~TileWay();
		
		OsmWay *m_way; // the way to render
		TileList *m_tiles; // all tiles containing this way, to prevent multiple redraws
		
};

class OsmTile
	: public IdObject, public DRect
{
	public:
		OsmTile(unsigned id, double minLon, double minLat, double maxLon, double maxLat, OsmTile *next)
			: IdObject(id, next), DRect(minLon, minLat, maxLon - minLon, maxLat - minLat)
		{
			m_ways = NULL;
//            printf("created tile %u %g,%g  %g-%g\n", id, minLon, minLat, maxLon, maxLat);
		}

		~OsmTile()
		{
			m_ways->DestroyList();
		}

		void AddWay(OsmWay *way, TileList *allTiles)
		{
//            printf("tile %u add way %u\n", m_id, way->m_id);
			m_ways = new TileWay(way, allTiles, m_ways);
		}

		TileWay *m_ways;

};

class TileList
	: public ListObject
{
	public:
		TileList(OsmTile *t, TileList *m_next)
		: ListObject(m_next)
		{
		   m_tile = t;
		   m_refCount = 0;
		}

		void Ref()
		{
			m_refCount++;
		}

		void UnRef()
		{
			m_refCount--;
			if (m_refCount <=0)
			{
				DestroyList();
			}
		}
	   
		OsmTile *m_tile;
		int m_refCount;
};

class OsmCanvas;

class TileDrawer
{
	public:
		TileDrawer(Renderer *renderer, double minLon,double minLat, double maxLon, double maxLat, double dLon, double dLat);

		~TileDrawer()
		{
			m_tiles->DestroyList();
			for (unsigned x = 0; x < m_xNum; x++)
			{
				delete [] m_tileArray[x];
			}

			delete [] m_tileArray;
		}

		void AddWays(OsmWay *ways)
		{
			unsigned count = 0;
			for (OsmWay *w = ways; w ; w = static_cast<OsmWay *>(w->m_next))
			{
				count++;
				AddWay(w);
				if (!(count % 10000))
				{
					printf("sorted %uK ways\n", count / 1000);
				}
			}
		}

		void AddWay(OsmWay *way)
		{
			DRect bb = way->GetBB();
			
			TileList *allTiles = GetTiles(bb);
			assert(allTiles);
			for (TileList *l = allTiles; l; l = static_cast<TileList *>(l->m_next))
			{
				l->m_tile->AddWay(way, allTiles->m_next ? allTiles : NULL);
			}
			allTiles->UnRef();
			
		}

		TileList *GetTiles(DRect box)
		{
			return GetTiles(box.m_x, box.m_y, box.m_x + box.m_w, box.m_y + box.m_h);
		}

		// you should UnRef the list when done, which will destroy it if not used anymore
		TileList *GetTiles(double minLon, double minLat, double maxLon, double maxLat);

		bool RenderTiles(wxApp *app, OsmCanvas *canvas, double lon, double lat, double w, double h, bool restart);


		void SetDrawRuleControl(RuleControl *r)
		{
			m_drawRule = r;
		}
		
		void SetColorRules(ColorRules *r)
		{
			m_colorRules = r;
		}

		// with explicit colours
		void RenderWay(OsmWay *w, wxColour lineColour, bool polygon = false, wxColour fillColour = wxColour(255,255,55));

		// with default colours
		void RenderWay(OsmWay *w, int curlayer);
		void Rect(wxString const &text, DRect const &re, int border, int r, int g, int b)
		{
			Rect(text, re.m_x, re.m_y, re.m_x + re.m_w, re.m_y + re.m_h, border, r, g, b);
		}
		
		void Rect(wxString const &text, double lon1, double lat1, double lon2, double lat2, double border, int r, int g, int b);

	private:

		OsmTile *m_tiles;
		OsmTile ***m_tileArray;
		unsigned m_xNum, m_yNum;
		double m_minLon, m_minLat, m_w, m_h, m_dLon, m_dLat;

		TileList *m_visibleTiles, *m_renderedTiles, *m_curTile;
		int m_curLayer;

		Renderer *m_renderer;

		RuleControl *m_drawRule;
		ColorRules *m_colorRules;
};

#endif
