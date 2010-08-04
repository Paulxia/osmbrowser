// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#include "frame.h"

#include <wx/menu.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include <wx/wx.h>

#include "rulecontrol.h"
#include "info.h"

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
	EVT_MENU(Menu_Quit,  MainFrame::OnQuit)
	EVT_MENU(Menu_About, MainFrame::OnAbout)
	EVT_MENU(Menu_Save_Pdf, MainFrame::OnSavePdf)
	EVT_CLOSE(MainFrame::OnClose)
	EVT_SIZE(MainFrame::OnSize)
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
    helpMenu->Append(Menu_About, _T("&About...\tF1"), _T("Show about dialog"));

    fileMenu->Append(Menu_Save_Pdf, _T("Save P&df\tAlt-P"), _T("save current view to pdf"));
    fileMenu->Append(Menu_Quit, _T("E&xit\tAlt-X"), _T("Quit this program"));

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

	wxSplitterWindow *subSplitter = new wxSplitterWindow(splitter, -1, wxDefaultPosition, wxDefaultSize, wxSP_3D);

	m_canvas = new OsmCanvas(app, this, subSplitter, fileName, NUMLAYERS + 1);

	wxPanel *rightPanel = new wxScrolledWindow(subSplitter);

	wxSizer *rightSizer = new wxBoxSizer(wxVERTICAL);

	rightPanel->SetSizer(rightSizer);
	
	m_info = new InfoTreeCtrl(rightPanel);
	rightSizer->Add(m_info, 1, wxEXPAND);

	rightPanel->FitInside();
	
	splitter->SetMinimumPaneSize(50);
	subSplitter->SetMinimumPaneSize(50);

	subSplitter->SetSashGravity(1.0);


	splitter->SplitVertically(leftPanel, subSplitter, 250);
	subSplitter->SplitVertically(m_canvas, rightPanel, -200);


	wxStaticText *text = new wxStaticText(leftPanel, -1 , wxT("Drawing Rule:"));

	leftSizer->Add(text, 0, wxEXPAND);

	m_drawRule = new RuleControl(leftPanel, m_canvas, wxSize(200,200));

	leftSizer->Add(m_drawRule, 0, wxEXPAND);

	m_colorRules = new ColorRules(leftPanel, leftSizer, m_canvas);

	leftSizer->Add(new AddButton(leftPanel, m_colorRules), 0, wxEXPAND);

	m_info->SetCanvas(m_canvas);
	m_canvas->SetRuleControls(m_drawRule, m_colorRules);
	m_canvas->SetInfoDisplay(m_info);

	RulesComboBox *rulesComboBox = new RulesComboBox(leftPanel, this);

	leftSizer->Insert(0, rulesComboBox, 0, wxEXPAND);

	Load(wxT("lastused"));
    // create a status bar just for fun (by default with 1 pane only)

	m_statusBar = CreateStatusBar(2);
	SetStatusText(_T("Welcome to osmbrowser!"));

	wxRect rect;

	m_statusBar->GetFieldRect(1, rect);
	m_progress = new wxGauge(m_statusBar, -1, 1000, rect.GetPosition(), rect.GetSize(), wxGA_SMOOTH);
	m_progress->Show(false);

}


// event handlers

void MainFrame::OnClose(wxCloseEvent & WXUNUSED(evt))
{
	printf("close\n");
	Save(wxT("lastused"));
	Destroy();

}


MainFrame::~MainFrame()
{
	delete m_colorRules;
}

void MainFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    // true is to force the frame to close
    Close(true);
}

void MainFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox(wxString::Format(
                    _T("Welcome to osmbrowser!\n")
                 ),
                 _T("About osmbrowser"),
                 wxOK | wxICON_INFORMATION,
                 this);
}



void MainFrame::Save(wxString const &name)
{
	m_drawRule->Save(wxString(wxT("rules/")) + name + wxT("/"));
	m_colorRules->Save(name);
	wxConfig::Get()->Flush();
}


void MainFrame::Load(wxString const &name)
{
	m_drawRule->Load(wxString(wxT("rules/")) + name + wxT("/"));
	m_colorRules->Load(name);
	m_canvas->Redraw();
}

void MainFrame::SetProgress(double progress, wxString const &text)
{
	if (progress >= 0)
	{
		m_progress->Show();
	}
	else
	{
		m_progress->Show(false);
	}

	if (1000 * progress >= 0)
	{
		m_progress->SetValue(static_cast<int>(progress * 1000));
	}

	if (text.Length())
	{
		SetStatusText(text);
	}
	else
	{
		SetStatusText(wxT("Welcome to osmbrowser!"));
	}
}

void MainFrame::OnSize(wxSizeEvent &evt)
{
	evt.Skip();
	wxRect rect;
	m_statusBar->GetFieldRect(1, rect);
	m_progress->SetSize(rect);
}

void MainFrame::OnSavePdf(wxCommandEvent& WXUNUSED(event))
{
	m_canvas->SaveView(wxT("out.pdf"), this);
}

