// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#ifndef __CAIRORENDERER__H__
#define __CAIRORENDERER__H__

#include <cairo.h>
#include <cairo-pdf.h>
#include <cairo-ps.h>
#include "renderer.h"

class CairoRendererBase
	: public Renderer
{
	public:
	CairoRendererBase(int numLayers)
		: Renderer(numLayers)
	{
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

} ;

class CairoRenderer
	: public CairoRendererBase
{
	public:
		CairoRenderer(wxBitmap *output, int numLayers)
			: CairoRendererBase(numLayers)
		{
			layerBuffers = new cairo_surface_t *[m_numLayers];
			layers = new cairo_t *[m_numLayers];

			Setup(output);
		}

		~CairoRenderer()
		{
			for (int i = 0; i < m_numLayers; i++)
			{
				cairo_surface_destroy(layerBuffers[i]);
				cairo_destroy(layers[i]);
			}

			delete [] layerBuffers;
			delete [] layers;
		}

		void Begin(Renderer::TYPE type, int layer)
		{
			m_type = type;
			m_curLayer = layer;

			cairo_new_path(layers[layer]);
		}

		void AddPoint(double x, double y, double xshift = 0, double yshift = 0)
		{
			cairo_line_to(layers[m_curLayer], (x - m_offX) * m_scaleX + xshift, m_outputHeight - (y - m_offY) * m_scaleY + yshift);
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
		void Setup(wxBitmap *output)
		{
			m_outputBitmap = output;
			
			for (int i = 0; i < m_numLayers; i++)
			{
				layerBuffers[i] = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, output->GetWidth(), output->GetHeight());
				layers[i] = cairo_create(layerBuffers[i]);

			}

			m_outputWidth = output->GetWidth();
			m_outputHeight = output->GetHeight();

		}

		Renderer::TYPE  m_type;
		int m_curLayer;
		cairo_t **layers;
		cairo_surface_t **layerBuffers;

		wxBitmap *m_outputBitmap;
};


class CairoPdfRenderer
	: public CairoRendererBase
{
	public:
		CairoPdfRenderer(wxString const &fileName, int w, int h)
			: CairoRendererBase(1)
		{
			m_surface = cairo_pdf_surface_create(fileName.mb_str(wxConvUTF8), w, h);
			m_context = cairo_create(m_surface);
			m_outputWidth = w;
			m_outputHeight = h;
		}

		~CairoPdfRenderer()
		{
			cairo_show_page(m_context);
			cairo_destroy(m_context);
			cairo_surface_flush(m_surface);
			cairo_surface_destroy(m_surface);
		}

		void Begin(Renderer::TYPE type, int layer);
		void AddPoint(double x, double y, double xshift = 0, double yshift = 0);
		void End();
		virtual void DrawCenteredText(char const *text, double x, double y, double angle, int r, int g, int b, int a, int layer);

		bool SupportsLayers() { return false; }

		void Clear(int layer = -1) { /* pdf doesn't support clearing */ }

		void Commit() { /* nop, we don't support layers */ }


		
	private:
		Renderer::TYPE  m_type;
		cairo_surface_t *m_surface;
		cairo_t *m_context;
};


#endif