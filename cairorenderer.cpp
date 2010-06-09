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

