// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#ifndef __INFO_H__
#define __INFO_H__


#include <wx/treectrl.h>

#include "tiledrawer.h"

class OsmCanvas;

class InfoTreeCtrl
	: public wxTreeCtrl
{
	public:
		InfoTreeCtrl(wxWindow *parent);

		void SetInfo(TileWay *ways);

		void SetCanvas(OsmCanvas *c);

	private:
		void AddWay(wxTreeItemId const &root, OsmWay *way);

		OsmCanvas *m_canvas;

		DECLARE_EVENT_TABLE();

		void OnSelection(wxTreeEvent &evt);
};

#endif
