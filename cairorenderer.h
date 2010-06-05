#ifndef __CAIRORENDERER__H__
#define __CAIRORENDERER__H__

#include <cairo.h>
#include "renderer.h"

class CairoRenderer
	: public Renderer
{
	public:
		CairoRenderer(int numLayers)
			: Renderer(numLayers)
		{
			layerBuffers = new cairo_surface_t *[m_numLayers];
			layers = new cairo_t *[m_numLayers];

			for (int i = 0; i < m_numLayers; i++)
			{
				layerBuffers[i] = NULL; // cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
				layers[i] = NULL; //cairo_create(layerBuffers[i]);
			}

		}

		void Setup(wxBitmap *output, DRect const &viewport)
		{
			for (int i = 0; i < m_numLayers; i++)
			{
				if (layers[i])
				{
					cairo_destroy(layers[i]);
				}

				if (layerBuffers[i])
				{
					cairo_surface_destroy(layerBuffers[i]);
				}
				
			
				layerBuffers[i] = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, output->GetWidth(), output->GetHeight());
				layers[i] = cairo_create(layerBuffers[i]);
			}
		}
		

	private:
		cairo_t **layers;
		cairo_surface_t **layerBuffers;
};


#endif