#ifndef __INFO_H__
#define __INFO_H__


#include <wx/treectrl.h>

#include "tiledrawer.h"

class InfoTreeCtrl
	: public wxTreeCtrl
{
	public:
		InfoTreeCtrl(wxWindow *parent);

		void SetInfo(TileWay *ways);

	private:
		void AddWay(wxTreeItemId const &root, OsmWay *way);
};

#endif
