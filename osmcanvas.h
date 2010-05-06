#ifndef __OSMCANVAS_H__
#define __OSMCANVAS_H__

#include <wx/timer.h>
#include <wx/app.h>
#include "wxcanvas.h"
#include "osm.h"
#include "renderer.h"

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

class TileSorter
{
	public:
		TileSorter(Renderer *renderer, double minLon,double minLat, double maxLon, double maxLat, double dLon, double dLat)
		{
			m_renderer = renderer;
			m_tiles = NULL;

			m_drawRule = NULL;
			m_colorRules = NULL;
			
			m_xNum = static_cast<int>((maxLon - minLon) / dLon) + 1;
			m_yNum = static_cast<int>((maxLat - minLat) / dLat) + 1;
			m_minLon = minLon;
			m_w = m_xNum * dLon;
			m_minLat = minLat;
			m_h = m_yNum *dLat;
			m_dLon = dLon;
			m_dLat = dLat;
			m_renderedTiles = m_visibleTiles = NULL;
			
			unsigned id = 0;
			// build a list of empty tiles;
			m_tileArray = new OsmTile **[m_xNum];
			for (unsigned x = 0; x < m_xNum; x++)
			{
				m_tileArray[x] = new OsmTile *[m_yNum];
				for (unsigned y = 0; y < m_yNum; y++)
				{
					m_tiles = new OsmTile(id++, m_minLon + x * dLon , m_minLat + y * dLat, m_minLon + (x + 1) * dLon, m_minLat + (y+1) * dLat, m_tiles);
					m_tileArray[x][y] = m_tiles;
				}
			}
		 }

		~TileSorter()
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
		TileList *GetTiles(double minLon, double minLat, double maxLon, double maxLat)
		{
//            printf("gettiles (%g %g)-(%g-%g):\n", minLon, minLat, maxLon, maxLat);
			int xMin = static_cast<int>((minLon - m_minLon) / m_dLon);
			int xMax = static_cast<int>((maxLon - m_minLon) / m_dLon) + 1;
			int yMin = static_cast<int>((minLat - m_minLat) / m_dLat);
			int yMax = static_cast<int>((maxLat - m_minLat) / m_dLat) + 1;

			if (xMin < 0) xMin = 0;
			if (xMin > static_cast<int>(m_xNum - 1)) xMin = static_cast<int>(m_xNum - 1);
			if (yMin < 0) yMin = 0;
			if (yMin > static_cast<int>(m_yNum - 1)) yMin = static_cast<int>(m_yNum - 1);

			if (xMax < 0) xMax = 0;
			if (xMax > static_cast<int>(m_xNum)) xMax = static_cast<int>(m_xNum);
			if (yMax < 0) yMax = 0;
			if (yMax > static_cast<int>(m_yNum)) yMax = static_cast<int>(m_yNum);

			TileList *ret = NULL;

//            printf("(%d,%d)-(%d, %d) (%d tiles)\n", xMin, yMin, xMax, yMax, (xMax - xMin)*(yMax - yMin));
			int xMiddle = (xMax + xMin)/2;
			int yMiddle = (yMax + yMin)/2;

			for (int x = xMin; x < xMax; x++)
			{
				for (int y = yMin; y < yMax; y++)
				{
					if (x != xMiddle || y != yMiddle)
					{
						ret = new TileList(m_tileArray[x][y], ret);
					}
				}
			}

			ret = new TileList(m_tileArray[xMiddle][yMiddle], ret);
			
			if (ret)
				ret->Ref();

			return ret;

			
		}

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

		void SetRuleControls(RuleControl *drawRule, ColorRules *colorRules)
		{
		
		}

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

class OsmCanvas
	: public Canvas
{
	public:
		OsmCanvas(wxApp *app, wxWindow *parent, wxString const &fileName);
		void Render(bool force = false);

		~OsmCanvas();

		void Lock()
		{
			m_locked = true;
		}

		void Unlock(bool refresh = true)
		{
			m_locked = false;
			if (refresh)
			{
				Redraw();
			}
		}

		void Redraw()
		{
			m_restart = true;
		}

		void SetRuleControls(RuleControl *rules, ColorRules *colors)
		{
			m_tileSorter->SetDrawRuleControl(rules);
			m_tileSorter->SetColorRules(colors);
		}
	private:
		OsmData *m_data;
		DECLARE_EVENT_TABLE();

		void OnMouseWheel(wxMouseEvent &evt);
		void OnLeftDown(wxMouseEvent &evt);
		void OnLeftUp(wxMouseEvent &evt);
		void OnMouseMove(wxMouseEvent &evt);
		void OnTimer(wxTimerEvent &evt)
		{
			m_timer.Stop();
			if (m_restart || !m_done)
			{
				Render();
			}

			if (m_done)
				Draw(NULL);

			m_timer.Start(100);
		}

		double m_scale;
		double m_xOffset, m_yOffset;

		int m_lastX, m_lastY;
		bool m_dragging;

		TileSorter *m_tileSorter;

		wxApp *m_app;
		bool m_done;
		bool m_restart;
		bool m_locked;
		wxTimer m_timer;

		void SetupRenderer();
		RendererWxBitmap m_renderer;

};


#endif
