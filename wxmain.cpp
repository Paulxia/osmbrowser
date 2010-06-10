// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
 
#ifdef __BORLANDC__
	#pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
	#include "wx/wx.h"
#endif

#include <wx/cmdline.h>
#include <wx/config.h>
#include "osmcanvas.h"
#include "rulecontrol.h"
#include "frame.h"

class MyApp : public wxApp
{
public:
	virtual bool OnInit();
	void OnInitCmdLine(wxCmdLineParser& parser);
};


IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
	// call the base class initialization method, currently it only parses a
	// few common command-line options but it could be do more in the future
	if ( !wxApp::OnInit() )
		return false;


	wxConfig *cfg = new wxConfig(wxT("OsmBrowser"));

	wxConfig::Set(cfg);

	if (argc !=2)
	{
		printf("usage: osmparse <osmfile>\n");
		return false;
	}

	// create the main application window
	MainFrame *frame = new MainFrame(this, _T("Osm Browser"), argv[1]);

	// and show it (the frames, unlike simple controls, are not shown when
	// created initially)
	frame->Show(true);

	frame->m_canvas->Unlock(true);

	// success: wxApp::OnRun() will be called which will enter the main message
	// loop and the application will run. If we returned false here, the
	// application would exit immediately.
	return true;
}

static const wxCmdLineEntryDesc gCmdLineDesc[] =
{
	{ wxCMD_LINE_SWITCH, wxT("v"), wxT("verbose"), wxT("verbose logging"), wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL },
	{ wxCMD_LINE_SWITCH, wxT("h"), wxT("help"), wxT("Display usage info"), wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
	{ wxCMD_LINE_PARAM, NULL, NULL, wxT("File to open"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL},
	{ wxCMD_LINE_NONE, NULL, NULL, NULL, wxCMD_LINE_VAL_NONE, 0},
}; 

void MyApp::OnInitCmdLine(wxCmdLineParser& parser)
{
		parser.SetDesc(gCmdLineDesc);
		parser.SetSwitchChars(wxT("-"));
}

