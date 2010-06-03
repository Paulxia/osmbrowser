#include "info.h"




InfoTreeCtrl::InfoTreeCtrl(wxWindow *parent)
	: wxTreeCtrl(parent, -1)
{
}

void InfoTreeCtrl::SetInfo(TileWay *ways)
{
	DeleteAllItems();

	wxTreeItemId root = AddRoot("this node is a member of:");

	for (TileWay *w = ways; w ; w = static_cast<TileWay *>(w->m_next))
	{
		AddWay(root, w->m_way);
	}

	ExpandAll();
}



void InfoTreeCtrl::AddWay(wxTreeItemId const &root, OsmWay *way)
{
	wxTreeItemId w = AppendItem(root, wxString::Format("%ud", way->m_id));

	for (OsmTag *t = way->m_tags; t; t = static_cast<OsmTag *>(t->m_next))
	{
		char const *k = t->GetKey();
		char const *v = t->GetValue();
		
		AppendItem(w, v ? wxString::Format("%s=%s", k, v) : wxString::Format("%s", k));
	}
}

