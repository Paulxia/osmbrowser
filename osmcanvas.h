#ifndef __OSMCANVAS_H__
#define __OSMCANVAS_H__

#include <wx/timer.h>
#include <wx/app.h>
#include "wxcanvas.h"
#include "osm.h"
#include "renderer.h"
class TileDrawer;
class RuleControl;
class ColorRules;
class InfoTreeCtrl;

class OsmCanvas
	: public Canvas
{
	public:
		OsmCanvas(wxApp *app, wxWindow *parent, wxString const &fileName, int numLayers);
		void Render(bool force = false);

		~OsmCanvas();

		void Lock()
		{
			m_locked = true;
		}

		void Unlock(bool refresh = true)
		{
			m_locked = false;
			if (refresh)
			{
				Redraw();
			}
		}

		void Redraw()
		{
			m_restart = true;
		}

		void SetRuleControls(RuleControl *rules, ColorRules *colors);

		void SetInfoDisplay(InfoTreeCtrl *info);
	private:
		OsmData *m_data;
		InfoTreeCtrl *m_info;
		DECLARE_EVENT_TABLE();

		void OnMouseWheel(wxMouseEvent &evt);
		void OnLeftDown(wxMouseEvent &evt);
		void OnLeftUp(wxMouseEvent &evt);
		void OnMouseMove(wxMouseEvent &evt);
		void OnTimer(wxTimerEvent &evt)
		{
			m_timer.Stop();
			if (m_restart || !m_done)
			{
				Render();
			}

			if (m_done)
				Draw(NULL);

			m_timer.Start(100);
		}

		double m_scale;
		double m_xOffset, m_yOffset;

		int m_lastX, m_lastY;
		bool m_dragging;

		TileDrawer *m_tileDrawer;

		wxApp *m_app;
		bool m_done;
		bool m_restart;
		bool m_locked;
		wxTimer m_timer;

		void SetupRenderer();
		RendererWxBitmap m_renderer;

		bool m_cursorLocked;
		bool m_firstDragStep;
};


#endif
