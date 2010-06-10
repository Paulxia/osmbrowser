// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#include "cairorenderer.h"
#include "utils.h"
#include "wxcairo.h"

void CairoRenderer::Commit()
{
	wxImage tmp(m_outputBitmap->GetWidth(), m_outputBitmap->GetHeight());

	ClearToWhite(&tmp);

	for (int i = 0; i < m_numLayers; i++)
	{
		cairo_surface_flush(layerBuffers[i]);
		OverlayImageSurface(layerBuffers[i], &tmp);
	}

	wxBitmap tmpBitmap(tmp);

	wxMemoryDC to;
	to.SelectObject(*m_outputBitmap);

	to.DrawBitmap(tmpBitmap, 0, 0);

}

void CairoPdfRenderer::Begin(Renderer::TYPE type, int layer)
{
	m_type = type;
	cairo_new_path(m_context);
}


void CairoPdfRenderer::AddPoint(double x, double y, double xshift, double yshift)
{
	cairo_line_to(m_context, (x - m_offX) * m_scaleX + xshift, m_outputHeight - (y - m_offY) * m_scaleY + yshift);
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





