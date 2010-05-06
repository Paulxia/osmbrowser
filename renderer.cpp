#include "renderer.h"


void RendererWxBitmap::DrawCenteredText(char const *s, double x, double y, double angle, int r, int g, int b)
{
	// not implemented yet
}

void RendererWxBitmap::Setup(wxBitmap *bitmap, DRect const &viewport)
{
	m_offX = viewport.m_x;
	m_offY = viewport.m_y;
	m_bitmap = bitmap;

	m_scaleX = bitmap->GetWidth() / viewport.m_w;
	m_scaleY = bitmap->GetHeight() / viewport.m_h;
}
void RendererWxBitmap::ScalePoints()
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

void RendererWxBitmap::DrawPolygon()
{
	ScalePoints();
	m_dc.SelectObject(*m_bitmap);
	m_dc.SetPen(m_pen);
	m_dc.SetBrush(m_brush);
	m_dc.DrawPolygon(m_numPoints, m_wxPoints);
}

void RendererWxBitmap::DrawLine()
{
	ScalePoints();
	m_dc.SelectObject(*m_bitmap);
	m_dc.SetPen(m_pen);
	m_dc.DrawLines(m_numPoints, m_wxPoints);
}

