#include "rulecontrol.h"



BEGIN_EVENT_TABLE(RuleControl, wxTextCtrl)
	EVT_TEXT(-1, RuleControl::OnText)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(ColorPicker, wxColourPickerCtrl)
	EVT_COLOURPICKER_CHANGED(-1, ColorPicker::OnChanged)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(PolyCheckBox, wxCheckBox)
	EVT_CHECKBOX(-1, PolyCheckBox::OnChanged)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(AddButton, wxButton)
	EVT_BUTTON(-1, AddButton::OnClick)
END_EVENT_TABLE()


RuleControl::RuleControl(wxWindow *parent, OsmCanvas *canvas, wxSize const &size)
	: wxTextCtrl(parent, -1, wxEmptyString, wxDefaultPosition, size == wxDefaultSize ? wxSize(200,40) : size, wxTE_MULTILINE | wxTE_RICH | wxTE_PROCESS_TAB)
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
	static wxColour bg(155,255,155);
	static wxTextAttr bracket(wxColour(0,200,0), bg), op(wxColour(50,50,250), bg), str(wxColour(150,150,0), bg), err(wxColour(100,0,0), wxColour(255,150,150)), disabled(wxColour(200,200,200),bg);

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
		case EC_DISABLED:
			style = disabled;
			break;
		case EC_SPACE:
			style = bracket;
			break;
	};
	SetStyle(from, to, style);
}

bool RuleControl::Evaluate(IdObjectWithTags *o)
{
	if (!m_expr)
		return true;

		return m_expr->GetValue(o);
}



void ColorRules::Add()
{
	wxStaticBoxSizer *s = new wxStaticBoxSizer(wxVERTICAL, m_parent);

	m_sizer->Add(s, 0, wxEXPAND);

	m_pickers[m_num] = new ColorPicker(m_parent, m_canvas);
	m_checkBoxes[m_num] = new PolyCheckBox(m_parent, m_canvas);
	m_rules[m_num] = new RuleControl(m_parent, m_canvas);

	s->Add(m_pickers[m_num]);
	s->Add(m_checkBoxes[m_num]);
	s->Add(m_rules[m_num]);

	s->Layout();

	m_num++;
	m_parent->Layout();
}
