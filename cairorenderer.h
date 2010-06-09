#ifndef __CAIRORENDERER__H__
#define __CAIRORENDERER__H__

#include <cairo.h>
#include "renderer.h"

class CairoRendererBase
	: public Renderer
{
	public:
	CairoRendererBase(int numLayers)
		: Renderer(numLayers)
	{
			m_width = m_height = -1;
	}

	void SetLineColor(int r, int g, int b, int a = 0)
	{
		m_lineA = (255 - a) / 255.0;
		m_lineR = (r / 255.0);
		m_lineG = (g / 255.0);
		m_lineB = (b / 255.0);
	}

	void SetFillColor(int r, int g, int b, int a = 0)
	{
		m_fillA = (255 - a) / 255.0;
		m_fillR = (r / 255.0);
		m_fillG = (g / 255.0);
		m_fillB = (b / 255.0);
	}

	void SetLineWidth(int width)
	{
		m_lineWidth = width;
	}

	protected:
		double m_fillR, m_fillG, m_fillB, m_fillA;
		double m_lineR, m_lineG, m_lineB, m_lineA;
		double m_lineWidth;
		int m_width, m_height;

} ;

class CairoRenderer
	: public CairoRendererBase
{
	public:
		CairoRenderer(int numLayers)
			: CairoRendererBase(numLayers)
		{
			layerBuffers = new cairo_surface_t *[m_numLayers];
			layers = new cairo_t *[m_numLayers];

			for (int i = 0; i < m_numLayers; i++)
			{
				layerBuffers[i] = NULL;
				layers[i] = NULL;
			}
		}

		void Setup(wxBitmap *output, DRect const &viewport)
		{
			m_outputBitmap = output;
			m_offX = viewport.m_x;
			m_offY = viewport.m_y;
			m_scaleX = output->GetWidth() / viewport.m_w;
			m_scaleY = output->GetHeight() / viewport.m_h;
			
			for (int i = 0; i < m_numLayers; i++)
			{
				if (layers[i] && (m_width != output->GetWidth() || m_height != output->GetHeight()))
				{
					cairo_destroy(layers[i]);
					cairo_surface_destroy(layerBuffers[i]);
					layerBuffers[i] = NULL;
					layers[i] = NULL;
				}
				
				if (!layers[i])
				{
					layerBuffers[i] = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, output->GetWidth(), output->GetHeight());
					layers[i] = cairo_create(layerBuffers[i]);
				}
				
			}

			m_width = output->GetWidth();
			m_height = output->GetHeight();

		}


		void Begin(Renderer::TYPE type, int layer)
		{
			m_type = type;
			m_curLayer = layer;

			cairo_new_path(layers[layer]);
		}

		void AddPoint(double x, double y, double xshift = 0, double yshift = 0)
		{
			cairo_line_to(layers[m_curLayer], (x - m_offX) * m_scaleX + xshift, m_height - (y - m_offY) * m_scaleY + yshift);
		}

		void End()
		{
			switch(m_type)
			{
				case R_POLYGON:
					cairo_set_source_rgba(layers[m_curLayer], m_fillR, m_fillG, m_fillB, m_fillA);
					cairo_fill_preserve(layers[m_curLayer]);
					// fall through;
				case R_LINE:
					cairo_set_line_width(layers[m_curLayer], m_lineWidth);
					cairo_set_source_rgba(layers[m_curLayer], m_lineR, m_lineG, m_lineB, m_lineA);
					cairo_stroke(layers[m_curLayer]);
					break;
			}
		}

		bool SupportsLayers() { return true; }

		void Clear(int layer = -1)
		{
			for (int i = 0; i < m_numLayers; i++)
			{
				if (layer < 0 || layer == i)
				{
					cairo_set_operator(layers[i], CAIRO_OPERATOR_SOURCE);
					cairo_set_source_rgba(layers[i], 0,0,0,0);
					cairo_paint(layers[i]);
					cairo_set_operator(layers[i], CAIRO_OPERATOR_OVER);
				}
			}
		}

		void Commit();

		virtual void DrawCenteredText(char const *text, double x, double y, double angle, int r, int g, int b, int a, int layer)
		{
			// not implemented
		}


	private:
		Renderer::TYPE  m_type;
		int m_curLayer;
		double m_scaleX, m_scaleY, m_offX, m_offY;
		cairo_t **layers;
		cairo_surface_t **layerBuffers;

		wxBitmap *m_outputBitmap;
};


#endif