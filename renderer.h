#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "osm.h"
#include <wx/dcmemory.h>

class Renderer
{
	public:
		Renderer(int numLayers)
		{
			m_numLayers = numLayers;
		}
		
		enum TYPE
		{
			R_POLYGON,
			R_LINE
		};
		virtual void Begin(Renderer::TYPE type, int layer) = 0;
		virtual void AddPoint(double x, double y) = 0;
		virtual void End() = 0;
		virtual void DrawCenteredText(char const *text, double x, double y, double angle, int r, int g, int b, int layer) = 0;

		void Rect(DRect const &re, double border, int r, int g, int b, int layer)
		{
			Rect(re.m_x, re.m_y, re.m_w, re.m_h, border, r, g, b, layer);
		}

		void Rect(double x, double y, double w, double h, double border, int r, int g, int b, int layer)
		{
			Begin(R_LINE, layer);
			AddPoint(x - border, y - border);
			AddPoint(x + w + 2 * border, y - border);
			AddPoint(x + w + 2 * border, y + h + 2 * border);
			AddPoint(x - border, y + h + 2 * border);
			AddPoint(x - border, y - border);
			End();
		}

		virtual void SetLineColor(int r, int g, int b, int a = 0) = 0;
		virtual void SetFillColor(int r, int g, int b, int a = 0) = 0;

		virtual void Clear(int layer = -1) = 0;

		// merge all layers and output to screen
		virtual void Commit() = 0;

	protected:
		int m_numLayers;
};

class RendererSimple
	: public Renderer
{
	public:
		RendererSimple(int numLayers)
			: Renderer(numLayers)
		{
			m_maxPoints = 1024;
			m_points = new RendererSimple::Point[m_maxPoints];
			m_numPoints = 0;
		}

		~RendererSimple()
		{
			delete [] m_points;
		}

		void Begin(Renderer::TYPE type, int layer)
		{
			m_numPoints = 0;
			m_type = type;
			m_curLayer = layer;
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
			if (!m_numPoints)
			{
				return;
			}
			
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
		int m_curLayer;
};

class RendererWxBitmap
	: public RendererSimple
{
	public:
		RendererWxBitmap(int numLayers)
		:	RendererSimple(numLayers), m_size(-1,-1)
		{
			m_wxPoints = NULL;
			m_numWxPoints  = 0;
			m_maskColor = wxColour(255,0,255);
			m_brush.SetColour(0,0,0);
			m_pen.SetColour(0,0,0);
			m_dc = new wxMemoryDC[m_numLayers];
			m_layer = new wxBitmap[m_numLayers];
		}

		~RendererWxBitmap()
		{
			delete [] m_wxPoints;
			//m_dc.SelectObject(NULL);
		}

		void DrawCenteredText(char const *s, double x, double y, double angle, int r, int g, int b, int layer);

		void SetLineColor(int r, int g, int b, int a = 0)
		{
			m_pen.SetColour(wxColour(r,g,b));
		}

		void SetFillColor(int r, int g, int b, int a = 0)
		{
			m_brush.SetColour(wxColour(r,g,b));
		}

		void Clear(int layer = -1)
		{
			if (layer >=0)
			{
				m_dc[layer].Clear();
			}
			else
			{
				for (int i = 0; i < m_numLayers; i++)
				{
					m_dc[i].Clear();
				}
			}
		}

		void Setup(wxBitmap *output, DRect const &viewport);

		void Commit();
	private:
		double m_offX, m_offY, m_scaleX, m_scaleY;
		wxBitmap *m_layer;
		wxBitmap *m_output;
		wxMemoryDC *m_dc;
		wxPoint *m_wxPoints;
		wxPen m_pen;
		wxBrush m_brush;
		wxColour m_maskColor;
		unsigned m_numWxPoints;
		wxSize m_size;
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
				m_wxPoints[i].y = m_layer[0].GetHeight() - static_cast<int>((m_points[i].y - m_offY) * m_scaleY);
			}
		}
		void BlitWithTransparency(wxBitmap *from, wxImage  *to);

		void DrawPolygon();

		void DrawLine();
};

#endif
