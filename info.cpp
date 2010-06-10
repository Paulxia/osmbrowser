// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#include "info.h"
#include "osmcanvas.h"

BEGIN_EVENT_TABLE(InfoTreeCtrl, wxTreeCtrl)
	EVT_TREE_SEL_CHANGED(-1, InfoTreeCtrl::OnSelection)
END_EVENT_TABLE()


class InfoData
	: public wxTreeItemData
{
	public:
		InfoData(OsmWay *way)
		{
			m_way = way;
		}

		OsmWay *m_way;

};


InfoTreeCtrl::InfoTreeCtrl(wxWindow *parent)
	: wxTreeCtrl(parent, -1)
{
	m_canvas = NULL;
}

void InfoTreeCtrl::SetInfo(TileWay *ways)
{
	if (m_canvas)
	{
		m_canvas->SelectWay(NULL);
	}

	DeleteAllItems();

	wxTreeItemId root = AddRoot(wxT("this node is a member of:"));

	for (TileWay *w = ways; w ; w = static_cast<TileWay *>(w->m_next))
	{
		AddWay(root, w->m_way);
	}

	ExpandAll();
}



void InfoTreeCtrl::AddWay(wxTreeItemId const &root, OsmWay *way)
{
	InfoData *data = new InfoData(way);
	wxTreeItemId w = AppendItem(root, wxString::Format(wxT("%ud"), way->m_id), -1, -1, data);

	for (OsmTag *t = way->m_tags; t; t = static_cast<OsmTag *>(t->m_next))
	{
		char const *k = t->GetKey();
		char const *v = t->GetValue();


		wxString tag(k, wxConvUTF8);

		if (v)
		{
			tag += wxT("=");
			tag += wxString(v, wxConvUTF8);
		}

		
		AppendItem(w, tag);
	}
}

void InfoTreeCtrl::SetCanvas(OsmCanvas *canvas)
{
	m_canvas = canvas;
}


void InfoTreeCtrl::OnSelection(wxTreeEvent &evt)
{
	wxTreeItemId id = evt.GetItem();


	InfoData *data = static_cast<InfoData *>(GetItemData(id));


	if (!m_canvas)
	{
		return;
	}
	
	if (data)
	{
		m_canvas->SelectWay(data->m_way);
	}
	else
	{
		m_canvas->SelectWay(NULL);
	}
}

