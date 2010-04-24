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
        OsmData *m_data;
        DECLARE_EVENT_TABLE();

        void OnMouseWheel(wxMouseEvent &evt);
        void OnLeftDown(wxMouseEvent &evt);
        void OnLeftUp(wxMouseEvent &evt);
        void OnMouseMove(wxMouseEvent &evt);

        double m_scale;
        double m_xOffset, m_yOffset;

        int m_lastX, m_lastY;
        bool m_dragging;
};


#endif
