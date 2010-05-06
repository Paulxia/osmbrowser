#include "renderer.h"


void RendererWxBitmap::DrawCenteredText(char const *s, double x, double y, double angle, int r, int g, int b)
{
	// not implemented yet
}

void RendererWxBitmap::Setup(wxBitmap *bitmap, DRect const &viewport)
{
	m_offX = viewport.m_x;
	m_offY = viewport.m_y;

	m_scaleX = bitmap->GetWidth() / viewport.m_w;
	m_scaleY = bitmap->GetHeight() / viewport.m_h;

	m_bitmap = bitmap;
	m_dc.SelectObject(*m_bitmap);
}

void RendererWxBitmap::DrawPolygon()
{
	ScalePoints();
	m_dc.SetPen(m_pen);
	m_dc.SetBrush(m_brush);
	m_dc.DrawPolygon(m_numPoints, m_wxPoints);
}

void RendererWxBitmap::DrawLine()
{
	ScalePoints();
	m_dc.SetPen(m_pen);
	m_dc.DrawLines(m_numPoints, m_wxPoints);
}

