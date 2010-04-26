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


        #define MAXPOLYCOUNT (1024*16)
        void StartPolygon()
        {
            m_polygonCount = 0;
            m_polygonVisible = true;
        }
        
        void AddPoint(int x, int y)
        {
            int w = m_backBuffer.GetWidth();
            int h = m_backBuffer.GetHeight();

        
            if (m_polygonCount < MAXPOLYCOUNT)
            {
                m_polygonPoints[m_polygonCount].x = x;
                m_polygonPoints[m_polygonCount].y = y;

                m_polygonCount++;

                if (x > 0 && x < w && y > 0 && y < h)
                {
                    m_polygonVisible = true;
                }
            }
            
        }
        
        void EndPolygon(wxDC *dc)
        {
            if (m_polygonCount)
                dc->DrawPolygon(m_polygonCount, m_polygonPoints);
        }

        wxPoint m_polygonPoints[MAXPOLYCOUNT];
        int m_polygonCount;
        bool m_polygonVisible;
        
};


#endif
