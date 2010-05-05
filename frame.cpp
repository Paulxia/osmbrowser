#include "frame.h"

#include <wx/menu.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include <wx/wx.h>

#include "rulecontrol.h"

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(Minimal_Quit,  MainFrame::OnQuit)
    EVT_MENU(Minimal_About, MainFrame::OnAbout)
    EVT_CLOSE(MainFrame::OnClose)
END_EVENT_TABLE()


// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

// frame constructor
MainFrame::MainFrame(wxApp *app, const wxString& title, wxString const &fileName)
       : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(1024,768))
{

#if wxUSE_MENUS
    // create a menu bar
    wxMenu *fileMenu = new wxMenu;

    // the "About" item should be in the help menu
    wxMenu *helpMenu = new wxMenu;
    helpMenu->Append(Minimal_About, _T("&About...\tF1"), _T("Show about dialog"));

    fileMenu->Append(Minimal_Quit, _T("E&xit\tAlt-X"), _T("Quit this program"));

    // now append the freshly created menu to the menu bar...
    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, _T("&File"));
    menuBar->Append(helpMenu, _T("&Help"));

    // ... and attach this menu bar to the frame
    SetMenuBar(menuBar);
#endif // wxUSE_MENUS

	wxSplitterWindow *splitter = new wxSplitterWindow(this, -1, wxDefaultPosition, wxDefaultSize, wxSP_3D);

	wxSizer *leftSizer = new wxBoxSizer(wxVERTICAL);

	wxScrolledWindow *leftPanel = new wxScrolledWindow(splitter);
	leftPanel->SetSizer(leftSizer);

	leftPanel->SetScrollRate(0, 10);
        
	m_canvas = new OsmCanvas(app, splitter, fileName);

	splitter->SplitVertically(leftPanel, m_canvas, 250);

	wxStaticText *text = new wxStaticText(leftPanel, -1 , wxT("Drawing Rule:"));

	leftSizer->Add(text, 0, wxEXPAND);

	m_drawRule = new RuleControl(leftPanel, m_canvas, wxSize(200,200));

	leftSizer->Add(m_drawRule, 0, wxEXPAND);

	m_colorRules = new ColorRules(leftPanel, leftSizer, m_canvas);

	leftSizer->Add(new AddButton(leftPanel, m_colorRules), 0, wxEXPAND);

	m_canvas->SetDrawRuleControl(m_drawRule);
	m_canvas->SetColorRules(m_colorRules);

	RulesComboBox *rulesComboBox = new RulesComboBox(leftPanel, m_drawRule, m_colorRules);

	leftSizer->Insert(0, rulesComboBox, 0, wxEXPAND);

	Load(wxT("lastused"));
#if wxUSE_STATUSBAR
    // create a status bar just for fun (by default with 1 pane only)
    CreateStatusBar(2);
    SetStatusText(_T("Welcome to wxWidgets!"));
#endif // wxUSE_STATUSBAR
}


// event handlers

void MainFrame::OnClose(wxCloseEvent & WXUNUSED(evt))
{
	printf("close\n");
	Save(wxT("lastused"));
	Destroy();

}


void MainFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    // true is to force the frame to close
    Close(true);
}

void MainFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox(wxString::Format(
                    _T("Welcome to %s!\n")
                    _T("\n")
                    _T("This is the minimal wxWidgets sample\n")
                    _T("running under %s."),
                    wxVERSION_STRING,
                    wxGetOsDescription().c_str()
                 ),
                 _T("About wxWidgets minimal sample"),
                 wxOK | wxICON_INFORMATION,
                 this);
}



void MainFrame::Save(wxString const &name)
{
	m_drawRule->Save(wxString(wxT("rules/")) + name + wxT("/"));
	m_colorRules->Save(name);
}


void MainFrame::Load(wxString const &name)
{
	m_drawRule->Load(wxString(wxT("rules/")) + name + wxT("/"));
	m_colorRules->Load(name);
	m_canvas->Redraw();
}


