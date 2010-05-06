#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "osm.h"
#include <wx/dcmemory.h>

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
		virtual void DrawCenteredText(char const *text, double x, double y, double angle, int r, int g, int b) = 0;

		void Rect(DRect const &re, double border, int r, int g, int b)
		{
			Rect(re.m_x, re.m_y, re.m_w, re.m_h, border, r, g, b);
		}

		void Rect(double x, double y, double w, double h, double border, int r, int g, int b)
		{
			Begin(R_LINE);
			AddPoint(x - border, y - border);
			AddPoint(x + w + 2 * border, y - border);
			AddPoint(x + w + 2 * border, y + h + 2 * border);
			AddPoint(x - border, y + h + 2 * border);
			AddPoint(x - border, y - border);
			End();
		}

		virtual void SetLineColor(int r, int g, int b, int a = 0) = 0;
		virtual void SetFillColor(int r, int g, int b, int a = 0) = 0;

};

class RendererSimple
	: public Renderer
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
			m_numPoints = 0;
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
		
};

class RendererWxBitmap
	: public RendererSimple
{
	public:
		RendererWxBitmap()
		{
			m_wxPoints = NULL;
			m_numWxPoints  = 0;
			m_bitmap = NULL;
			m_brush.SetColour(0,0,0);
			m_pen.SetColour(0,0,0);
		}

		~RendererWxBitmap()
		{
			delete [] m_wxPoints;
			//m_dc.SelectObject(NULL);
		}

		void DrawCenteredText(char const *s, double x, double y, double angle, int r, int g, int b);

		void SetLineColor(int r, int g, int b, int a = 0)
		{
			m_pen.SetColour(wxColour(r,g,b));
		}

		void SetFillColor(int r, int g, int b, int a = 0)
		{
			m_brush.SetColour(wxColour(r,g,b));
		}

		void Setup(wxBitmap *bitmap, DRect const &viewport);
	private:
		double m_offX, m_offY, m_scaleX, m_scaleY;
		wxBitmap *m_bitmap;
		wxMemoryDC m_dc;
		wxPoint *m_wxPoints;
		wxPen m_pen;
		wxBrush m_brush;
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
				m_wxPoints[i].y = m_bitmap->GetHeight() - static_cast<int>((m_points[i].y - m_offY) * m_scaleY);
			}
		}

		void DrawPolygon();

		void DrawLine();
};

#endif
