#ifndef __MAINFRAME_H__
#define __MAINFRAME_H__

#include <wx/frame.h>
#include <wx/string.h>
#include <wx/app.h>

#include "osmcanvas.h"
class RuleControl;

// Define a new frame type: this is going to be our main frame
class MainFrame : public wxFrame
{
public:
    // ctor(s)
    MainFrame(wxApp *app, const wxString& title, wxString const &fileName);

    // event handlers (these functions should _not_ be virtual)
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
	void OnClose(wxCloseEvent &close);

    OsmCanvas *m_canvas;
	RuleControl *m_drawRule;
	ColorRules *m_colorRules;

	void Save(wxString const &name);
	void Load(wxString const &name);

private:
    // any class wishing to process wxWidgets events must use this macro
    DECLARE_EVENT_TABLE()

};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum
{
    // menu items
    Minimal_Quit = wxID_EXIT,

    // it is important for the id corresponding to the "About" command to have
    // this standard value as otherwise it won't be handled properly under Mac
    // (where it is special and put into the "Apple" menu)
    Minimal_About = wxID_ABOUT
};



#endif
