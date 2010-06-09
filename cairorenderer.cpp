#include "cairorenderer.h"
#include "utils.h"
#include "wxcairo.h"

void CairoRenderer::Commit()
{
	wxImage tmp(m_outputBitmap->GetWidth(), m_outputBitmap->GetHeight());

	ClearToWhite(&tmp);

	for (int i = 0; i < m_numLayers; i++)
	{
		OverlayImageSurface(layerBuffers[i], &tmp);
	}

	wxBitmap tmpBitmap(tmp);

	wxMemoryDC to;
	to.SelectObject(*m_outputBitmap);

	to.DrawBitmap(tmpBitmap, 0, 0);

}



void CairoPdfRenderer::Setup(wxBitmap *output, DRect const &viewport)
{
			m_offX = viewport.m_x;
			m_offY = viewport.m_y;
			m_scaleX = m_width / viewport.m_w;
			m_scaleY = m_height / viewport.m_h;
}

void CairoPdfRenderer::Begin(Renderer::TYPE type, int layer)
{
	m_type = type;
	cairo_new_path(m_context);
}


void CairoPdfRenderer::AddPoint(double x, double y, double xshift, double yshift)
{
	cairo_line_to(m_context, (x - m_offX) * m_scaleX + xshift, m_height - (y - m_offY) * m_scaleY + yshift);
}

void CairoPdfRenderer::End()
{
	switch(m_type)
	{
		case R_POLYGON:
			cairo_set_source_rgba(m_context, m_fillR, m_fillG, m_fillB, m_fillA);
			cairo_fill_preserve(m_context);
			// fall through;
		case R_LINE:
			cairo_set_line_width(m_context, m_lineWidth);
			cairo_set_source_rgba(m_context, m_lineR, m_lineG, m_lineB, m_lineA);
			cairo_stroke(m_context);
			break;
	}
}

void CairoPdfRenderer::DrawCenteredText(char const *text, double x, double y, double angle, int r, int g, int b, int a, int layer){ }

