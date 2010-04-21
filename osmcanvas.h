#ifndef __OSMCANVAS_H__
#define __OSMCANVAS_H__

#include "wxcanvas.h"
#include "osm.h"

class OsmCanvas
    : public Canvas
{
    public:
        OsmCanvas(wxWindow *parent, wxString const &fileName);
        void Render();
        ~OsmCanvas();
    private:
        OSMDATA *m_data;
        DECLARE_EVENT_TABLE();

        void OnMouseWheel(wxMouseEvent &evt);
};


#endif
