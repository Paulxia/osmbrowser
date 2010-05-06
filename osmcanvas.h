#ifndef __OSMCANVAS_H__
#define __OSMCANVAS_H__

#include <wx/timer.h>
#include <wx/app.h>
#include "wxcanvas.h"
#include "osm.h"

class TileList;
class RuleControl;
class ColorRules;

class Renderer
{
	public:
		enum TYPE
		{
			R_POLYGON,
			R_LINE
		};
		virtual void Begin(Renderer::TYPE type) = 0;
		virtual void AddPoint(double x, double y) = 0;
		virtual void End() = 0;
		virtual void DrawTextCentered(char const *text, double x, double y, double angle) = 0;

		void Rect(DRect const &re, double border, int r, int g, int b)
		{
			Rect(re.m_x, re.m_y, re.m_w, re.m_h, border, r, g, b);
		}

		void Rect(double x, double y, double w, double h, double border, int r, int g, int b)
		{
			Begin(R_LINE);
			AddPoint(x - border, y - border);
			AddPoint(x + w + 2 * border, y - border);
			AddPoint(x + w + 2 * border, y + 2 * border);
			AddPoint(x - border, y + 2 * border);
			AddPoint(x - border, y - border);
			End();
		}

		virtual void SetLineColor(int r, int g, int b) = 0;
		virtual void SetFillColor(int r, int g, int b) = 0;

};

class RendererSimple
{
	public:
		RendererSimple()
		{
			m_maxPoints = 1024;
			m_points = new RendererSimple::Point[m_maxPoints];
			m_numPoints = 0;
		}

		~RendererSimple()
		{
			delete [] m_points;
		}

		void Begin(Renderer::TYPE type)
		{
			m_type = type;
		}

		void AddPoint(double x, double y)
		{
			if (m_numPoints >= m_maxPoints)
			{
				Grow();
			}

			m_points[m_numPoints].x = x;
			m_points[m_numPoints].y = y;
			m_numPoints++;
		}

		void End()
		{
			switch(m_type)
			{
				case Renderer::R_LINE:
				DrawLine();
				break;
				case Renderer::R_POLYGON:
				DrawPolygon();
				break;
			};
		}

	protected:
		virtual void DrawPolygon() = 0;
		virtual void DrawLine() = 0;
	private:
		struct Point
		{
			double x, y;
		};

		void Grow()
		{
			m_maxPoints *=2;
			RendererSimple::Point *n = new  RendererSimple::Point[m_maxPoints];
			for (unsigned i = 0; i < m_numPoints; i++)
			{
				n[i] = m_points[i];
			}

			delete [] m_points;
			m_points = n;
		}
	protected:
		Renderer::TYPE m_type;
		RendererSimple::Point *m_points;
		unsigned m_maxPoints;
		unsigned m_numPoints;
		
};

class RendererWxBitmap
	: public RendererSimple
{
	public:
		RendererWxBitmap()
		{
			m_wxPoints = NULL;
			m_numWxPoints  = 0;
			m_h = 0;
		}

		~RendererWxBitmap()
		{
			delete [] m_wxPoints;
			//m_dc.SelectObject(NULL);
		}

		void DrawTextCentered(char const *s, double x, double y, double angle)
		{
			// not implemented yet
		}

		void SetLineColor(int r, int g, int b)
		{
			wxPen pen(wxColour(r,g,b));
			m_dc.SetPen(pen);
		}

		void SetFillColor(int r, int g, int b)
		{
			wxBrush brush(wxColour(r,g,b));
			m_dc.SetBrush(brush);
		}

		void Setup(wxBitmap *bitmap, DRect const &viewport)
		{
			m_offX = viewport.m_x;
			m_offY = viewport.m_y;
			m_h = bitmap->GetHeight();

			m_scaleX = bitmap->GetWidth() / viewport.m_w;
			m_scaleY = bitmap->GetHeight() / viewport.m_h;
			m_dc.SelectObject(*bitmap);
		}
	private:
		double m_offX, m_offY, m_scaleX, m_scaleY;
		int m_h;
		wxMemoryDC m_dc;
		wxPoint *m_wxPoints;
		unsigned m_numWxPoints;
	protected:
		void ScalePoints()
		{
			if (m_numWxPoints < m_maxPoints)
			{
				delete [] m_wxPoints;
				m_wxPoints = new wxPoint[m_maxPoints];
				m_numWxPoints = m_maxPoints;
			}

			for (unsigned i = 0; i < m_numPoints; i++)
			{
				m_wxPoints[i].x = static_cast<int>((m_points[i].x - m_offX) * m_scaleX);
				m_wxPoints[i].y = m_h - static_cast<int>((m_points[i].y - m_offY) * m_scaleY);
			}
		}
		
		void DrawPolygon()
		{
			ScalePoints();
			m_dc.DrawPolygon(m_numPoints, m_wxPoints);
		}

		void DrawLine()
		{
			ScalePoints();
			m_dc.DrawLines(m_numPoints, m_wxPoints);
		}
};

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
			: IdObject(id, next), DRect(minLon, minLat, maxLon, maxLat)
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
		TileSorter(double minLon,double minLat, double maxLon, double maxLat, double dLon, double dLat)
		{
			m_tiles = NULL;
			unsigned id = 0;
			// build a list of empty tiles;

			m_xNum = static_cast<int>((maxLon - minLon) / dLon) + 1;
			m_yNum = static_cast<int>((maxLat - minLat) / dLat) + 1;
			m_minLon = minLon;
			m_w = m_xNum * dLon;
			m_minLat = minLat;
			m_h = m_yNum *dLat;
			m_dLon = dLon;
			m_dLat = dLat;
			m_renderedTiles = m_visibleTiles = NULL;
			
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

	private:
		OsmTile *m_tiles;
		OsmTile ***m_tileArray;
		unsigned m_xNum, m_yNum;
		double m_minLon, m_minLat, m_w, m_h, m_dLon, m_dLat;

		TileList *m_visibleTiles, *m_renderedTiles, *m_curTile;
		int m_curLayer;
		
};

class OsmCanvas
	: public Canvas
{
	public:
		OsmCanvas(wxApp *app, wxWindow *parent, wxString const &fileName);
		void Render(bool force = false);

		void SetDrawRuleControl(RuleControl *r);
		void SetColorRules(ColorRules *r);

		// with explicit colours
		void RenderWay(OsmWay *w, wxColour lineColour, bool polygon = false, wxColour fillColour = wxColour(255,255,55));

		// with default colours
		void RenderWay(OsmWay *w, bool fast, int curlayer);
		void Rect(wxString const &text, DRect const &re, int border, int r, int g, int b)
		{
			Rect(text, re.m_x, re.m_y, re.m_x + re.m_w, re.m_y + re.m_h, border, r, g, b);
		}
		void Rect(wxString const &text, double lon1, double lat1, double lon2, double lat2, int border, int r, int g, int b);

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
//			Render();
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


		#define MAXPOLYCOUNT (1024*16)
		void StartPolygon()
		{
			m_polygonCount = 0;
			m_polygonVisible = true;
		}
		
		void AddPoint(int x, int y)
		{
			int w = m_backBuffer.GetWidth();
			int h = m_backBuffer.GetHeight();

		
			if (m_polygonCount < MAXPOLYCOUNT)
			{
				m_polygonPoints[m_polygonCount].x = x;
				m_polygonPoints[m_polygonCount].y = y;

				m_polygonCount++;

				if (x > 0 && x < w && y > 0 && y < h)
				{
					m_polygonVisible = true;
				}
			}
			
		}
		
		void EndPolygon(wxDC *dc)
		{
			if (m_polygonCount)
				dc->DrawPolygon(m_polygonCount, m_polygonPoints);
		}

		wxPoint m_polygonPoints[MAXPOLYCOUNT];
		int m_polygonCount;
		bool m_polygonVisible;

		TileSorter *m_tileSorter;

		OsmTag *m_fastTags;

		RuleControl *m_drawRuleControl;
		ColorRules *m_colorRules;

		wxApp *m_app;
		bool m_done;
		bool m_restart;
		bool m_locked;
		wxTimer m_timer;
};


#endif
