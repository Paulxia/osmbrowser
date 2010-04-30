#include "rulecontrol.h"



BEGIN_EVENT_TABLE(RuleControl, wxTextCtrl)
	EVT_TEXT(-1, RuleControl::OnText)
END_EVENT_TABLE()


RuleControl::RuleControl(wxWindow *parent, OsmCanvas *canvas)
	: wxTextCtrl(parent, -1, wxEmptyString, wxDefaultPosition, wxSize(200,200), wxTE_MULTILINE | wxTE_PROCESS_TAB | wxTE_RICH)
{
	m_expr = 0;
	m_canvas = canvas;
}

RuleControl::~RuleControl()
{
	if (m_expr)
	{
		m_expr->DestroyList();
	}
}

void RuleControl::OnText(wxCommandEvent &evt)
{
	char errors[1024];
	unsigned int errorpos = 0;

	LogicalExpression *e = Parse(GetValue().mb_str(wxConvUTF8), errors, 1024, &errorpos);



	if (e || GetValue().Trim().IsEmpty())
	{
		m_expr = e;
		m_canvas->Redraw();
		SetToolTip(wxT("expression ok"));
	}
	else
	{
		SetToolTip(wxString(errors, wxConvUTF8));
	}
}

void RuleControl::SetColor(int from, int to, E_COLORS color)
{
	static wxColour bg(255,255,200);
	static wxTextAttr bracket(wxColour(0,100,0), bg), op(wxColour(50,50,100), bg), str(wxColour(100,100,0), bg), err(wxColour(100,0,0), wxColour(255,200,200));

	wxTextAttr style;

//	printf("setstyle %d fro %d to %d\n", color, from, to);
	switch(color)
	{
		case EC_BRACKET:
			style = bracket;
			break;
		case EC_OPERATOR:
			style = op;
			break;
		case EC_STRING:
			style = str;
			break;
		case EC_ERROR:
			style = err;
			break;
	};
	SetStyle(from, to -1, style);
}

bool RuleControl::Evaluate(IdObjectWithTags *o)
{
	if (!m_expr)
		return true;

		return m_expr->GetValue(o);
}

