// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#include "osmcanvas.h"
#include "parse.h"
#include "rulecontrol.h"
#include "tiledrawer.h"
#include "info.h"
#include "frame.h"

BEGIN_EVENT_TABLE(OsmCanvas, Canvas)
	EVT_MOUSEWHEEL(OsmCanvas::OnMouseWheel)
	EVT_LEFT_DOWN(OsmCanvas::OnLeftDown)
//        EVT_MIDDLE_DOWN(OsmCanvas::OnMiddleDown)
//        EVT_RIGHT_DOWN(OsmCanvas::OnRightDown)
	EVT_LEFT_UP(OsmCanvas::OnLeftUp)
//        EVT_MIDDLE_UP(OsmCanvas::OnMiddleUp)
//        EVT_RIGHT_UP(OsmCanvas::OnRightUp)
	EVT_MOTION(OsmCanvas::OnMouseMove)
	EVT_TIMER(-1, OsmCanvas::OnTimer)
END_EVENT_TABLE()


OsmCanvas::OsmCanvas(wxApp * app, MainFrame *mainFrame, wxWindow *parent, wxString const &fileName, int numLayers)
	: Canvas(parent)
{
	m_timer.SetOwner(this);
	m_done = false;
	m_restart = true;
	m_app = app;
	m_mainFrame = mainFrame;
	m_dragging = false;
	m_locked = true;
	m_info = NULL;
	m_cursorLocked = false;
	m_firstDragStep = false;
	wxString binFile = fileName;
	m_renderer = NULL;
	m_renderJob = NULL;

	binFile.Append(wxT(".cache"));

	FILE *infile;
	if (fileName.IsSameAs(wxT("-")))
	{
		binFile = wxString(wxT("stdin.cache"));
	}

	infile = fopen(binFile.mb_str(wxConvUTF8), "r");
	
	if (infile)
	{
		printf("found preprocessed file %s, opening that instead.\n", (char const *)(binFile.mb_str(wxConvUTF8)) );
		m_data = parse_binary(infile, true);
	}
	else
	{
		if (fileName.IsSameAs(wxT("-")))
		{
			infile = stdin;
		}
		else
		{
			infile = fopen(fileName.mb_str(wxConvUTF8), "r");
		}

		if (!infile)
		{
			puts("could not open file:");
			puts(fileName.mb_str(wxConvUTF8));
			abort();
		}
	
		if (fileName.EndsWith(wxT(".cache")))
		{
			m_data = parse_binary(infile, true);
		}
		else
		{
			m_data = parse_osm(infile, true);
	
			FILE *outFile = fopen(binFile.mb_str(wxConvUTF8) , "wb");
	
			if (outFile)
			{
			
				printf("writing cache\n");
				write_binary(m_data, outFile);
				fclose(outFile);
			}
		}
	}
	fclose(infile);

	double xscale = 1200.0 / (m_data->m_maxlon - m_data->m_minlon);
	double yscale = 1200.0 / (m_data->m_maxlon - m_data->m_minlon);
	m_scale = xscale < yscale ? xscale : yscale;
	m_xOffset = m_data->m_minlon;
	m_yOffset = m_data->m_minlat;

	m_lastX = m_lastY = 0;

	m_tileDrawer = new TileDrawer(m_data->m_minlon, m_data->m_minlat, m_data->m_maxlon, m_data->m_maxlat, .05, .04);

	m_tileDrawer->AddWays(static_cast<OsmWay *>(m_data->m_ways.m_content));

	m_tileDrawer->SetSelectionColor(255,100,100);

	m_timer.Start(100);
}

void OsmCanvas::Render(bool force)
{

	if (force)
	{
		m_restart = true;
	}
	
	if (!(m_backBuffer.IsOk()))
	{
		m_restart = true;
		return;
	}

	if (!IsShownOnScreen() || m_locked)
	{
		return;
	}
	
	if (!m_restart && m_done)
	{
		return;
	}

	SetupRenderer();
	
	if (m_restart)
	{
		m_renderer->Clear();
		delete m_renderJob;
		m_renderJob = NULL;
		m_restart = false;
	}

	if (!m_renderJob)
	{
		m_renderJob = new CanvasJob(m_app, m_mainFrame, m_renderer);
	}

	m_done = m_tileDrawer->RenderTiles(m_renderJob, 10);

	m_tileDrawer->DrawOverlay(m_renderer);
	
	m_renderer->Commit();
	Draw(NULL);

	if (m_done)
	{
		m_mainFrame->SetProgress(-1);
	}
	
	return;
}

OsmCanvas::~OsmCanvas()
{
	delete m_tileDrawer;
	delete m_renderer;
	delete m_data;
}

void OsmCanvas::OnMouseWheel(wxMouseEvent &evt)
{
	double scaleCorrection = cos(m_yOffset * M_PI / 180);
	double w = evt.GetWheelRotation() / 1200.0;
	int h = m_backBuffer.GetHeight();

	double xm = evt.m_x / (m_scale * scaleCorrection);
	double ym = (h - evt.m_y) / m_scale;

	m_xOffset += xm;
	m_yOffset += ym;

	m_scale = m_scale * (1.0 + w);

	xm = evt.m_x / (m_scale * scaleCorrection);
	ym = (h - evt.m_y) / m_scale;
	m_xOffset -= xm;
	m_yOffset -= ym;

	SetupRenderer();
	Redraw();
}

void OsmCanvas::OnMouseMove(wxMouseEvent &evt)
{
	double scaleCorrection = cos(m_yOffset * M_PI / 180);

	if (m_dragging)
	{
		int idx = evt.m_x - m_lastX;
		int idy = evt.m_y - m_lastY;

		if (!m_firstDragStep || idx * idx + idy *idy > 25)
		{
			m_firstDragStep = false;
			m_lastX = evt.m_x;
			m_lastY = evt.m_y;
			double dx = idx / (m_scale * scaleCorrection);
			double dy = idy / m_scale;

			m_xOffset -= dx;
			m_yOffset += dy;

			SetupRenderer();
			Redraw();
		}
	}
	else
	{
		if (!m_cursorLocked)
		{
			double lon = m_xOffset + evt.m_x / (m_scale * scaleCorrection);
			double lat = m_yOffset + (m_backBuffer.GetHeight() - evt.m_y) / m_scale;
			if (m_tileDrawer->SetSelection(lon, lat))
			{
				SetupRenderer();
				m_tileDrawer->DrawOverlay(m_renderer, true);
				m_renderer->Commit();
				Draw();
	
				if (m_info)
				{
					TileWay *list = m_tileDrawer->GetSelection();
	
	
					m_info->SetInfo(list);
	
					if (list)
						list->DestroyList();
				}
			}
		}
	}
}


void OsmCanvas::OnLeftDown(wxMouseEvent &evt)
{
	if (!HasCapture())
		CaptureMouse();

	m_lastX = evt.m_x;
	m_lastY = evt.m_y;

	m_dragging = true;
	m_firstDragStep = true;

}


void OsmCanvas::OnLeftUp(wxMouseEvent &evt)
{
	if (HasCapture())
		ReleaseMouse();
	m_dragging = false;

	// we haven't dragged, so change the cursor locking
	if (m_firstDragStep)
	{
		m_cursorLocked = !m_cursorLocked;
		if (!m_cursorLocked)
		{
			m_tileDrawer->SetSelectionColor(255,100,100);
			double scaleCorrection = cos(m_yOffset * M_PI / 180);
			double lon = m_xOffset + evt.m_x / (m_scale * scaleCorrection);
			double lat = m_yOffset + (m_backBuffer.GetHeight() - evt.m_y) / m_scale;
			if (m_tileDrawer->SetSelection(lon, lat))
			{
				if (m_info)
				{
					TileWay *list = m_tileDrawer->GetSelection();

					m_info->SetInfo(list);

					if (list)
						list->DestroyList();
				}
			}
		}
		else
		{
			m_tileDrawer->SetSelectionColor(255,0,0);
		}
		SetupRenderer();
		m_tileDrawer->DrawOverlay(m_renderer, true);
		m_renderer->Commit();
		Draw();
	}

}


void OsmCanvas::SetupRenderer()
{
	if (m_renderer)
	{
		if (static_cast<int>(m_renderer->GetWidth()) != m_backBuffer.GetWidth()
			|| static_cast<int>(m_renderer->GetHeight()) != m_backBuffer.GetHeight())
		{
			delete m_renderer;
			m_renderer = NULL;
		}
	}
	
	if (!m_renderer)
	{
		m_renderer = new CairoRenderer(&m_backBuffer, NUMLAYERS + 1);
	}

	double scaleCorrection = cos(m_yOffset * M_PI / 180);

	int renderW = m_backBuffer.GetWidth();
	int renderH = m_backBuffer.GetHeight();
	double sw = renderW / (scaleCorrection * m_scale);
	double sh = renderH / m_scale;

	m_renderer->SetupViewport(DRect(m_xOffset, m_yOffset, sw, sh));
}

void OsmCanvas::SetRuleControls(RuleControl *rules, ColorRules *colors)
{
	m_tileDrawer->SetDrawRuleControl(rules);
	m_tileDrawer->SetColorRules(colors);
}

void OsmCanvas::SetInfoDisplay(InfoTreeCtrl *info)
{
	m_info = info;
}

void OsmCanvas::SelectWay(OsmWay *way)
{
	if (m_tileDrawer->SetSelectedWay(way))
	{
		m_tileDrawer->DrawOverlay(m_renderer, true);
		m_renderer->Commit();
		Draw();
	}
}

void OsmCanvas::SaveView(wxString const &fileName, MainFrame *mainFrame)
{
	int w = m_backBuffer.GetWidth();
	int h = m_backBuffer.GetHeight();

	double xScale = cos(m_yOffset * M_PI / 180) * m_scale;

	Renderer *r = new CairoPdfRenderer(fileName, w*10, h*10);

	r->SetupViewport(DRect(m_xOffset, m_yOffset, w /  xScale, h / m_scale));

//	double progress;
//!todo
//	bool done = m_tileDrawer->RenderTiles(m_app, r, m_xOffset, m_yOffset, w / xScale, h / m_scale, true, 10, &progress);
//	mainFrame->SetProgress(progress);

//	while (!done)
//	{
//		done = m_tileDrawer->RenderTiles(m_app, r, m_xOffset, m_yOffset, w / xScale, h / m_scale, false, 10, &progress);
//		mainFrame->SetProgress(progress);
//	}

	mainFrame->SetProgress(-1);

	delete r;
}


CanvasJob::CanvasJob(wxApp *app, MainFrame *mainFrame, Renderer *r)
	: RenderJob(r)
{
	m_app = app;
	m_mainFrame = mainFrame;
}

bool CanvasJob::MustCancel(double progress)
{
	m_mainFrame->SetProgress(progress);
	return m_app->Pending();
}

