// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#include "rulecontrol.h"
#include <wx/config.h>


BEGIN_EVENT_TABLE(RuleControl, wxTextCtrl)
	EVT_TEXT(-1, RuleControl::OnText)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(ColorPicker, wxColourPickerCtrl)
	EVT_COLOURPICKER_CHANGED(-1, ColorPicker::OnChanged)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(LayerPicker, wxChoice)
	EVT_CHOICE(-1, LayerPicker::OnSelect)
END_EVENT_TABLE()


BEGIN_EVENT_TABLE(PolyCheckBox, wxCheckBox)
	EVT_CHECKBOX(-1, PolyCheckBox::OnChanged)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(AddButton, wxButton)
	EVT_BUTTON(-1, AddButton::OnClick)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(RulesComboBox, wxComboBox)
	EVT_TEXT_ENTER(-1, RulesComboBox::OnEnter)
	EVT_COMBOBOX(-1, RulesComboBox::OnSelected)
END_EVENT_TABLE()



RuleControl::RuleControl(wxWindow *parent, OsmCanvas *canvas, wxSize const &size)
	: wxTextCtrl(parent, -1, wxEmptyString, wxDefaultPosition, size == wxDefaultSize ? wxSize(200,40) : size, wxTE_MULTILINE | wxTE_RICH | wxTE_PROCESS_TAB)
{
	m_expr = 0;
	m_canvas = canvas;
	m_valueOnEmpty = true;
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
		return m_valueOnEmpty;

    LogicalExpression::STATE s = m_expr->GetValue(o);

	if (s == LogicalExpression::S_TRUE)
	{
		return true;
	}
	else if (s == LogicalExpression::S_FALSE)
	{
		return false;
	}

	return m_valueOnEmpty;
}



void ColorRules::Add()
{
	wxStaticBoxSizer *s = new wxStaticBoxSizer(wxVERTICAL, m_parent);

	m_sizer->Add(s, 0, wxEXPAND);

	m_pickers[m_num] = new ColorPicker(m_parent, m_canvas);
	m_checkBoxes[m_num] = new PolyCheckBox(m_parent, m_canvas);
	m_rules[m_num] = new RuleControl(m_parent, m_canvas);
	m_layers[m_num] = new LayerPicker(m_parent, m_canvas);

	wxSizer *top = new wxBoxSizer(wxHORIZONTAL);
	s->Add(top, 0, wxEXPAND);
	top->Add(m_pickers[m_num], 0);
	top->Add(m_layers[m_num], 1);
	s->Add(m_checkBoxes[m_num]);
	s->Add(m_rules[m_num], 0,wxEXPAND);

	m_rules[m_num]->SetValueOnEmpty(false);

	m_parent->FitInside();
	m_num++;
}

void ColorRules::Remove(int number)
{
	assert(number < m_num);
	assert(number >=0 );

	wxSizerItem *item = m_sizer->GetItem(number);

	wxSizer *s = item->GetSizer();

	assert(s);

	wxSizer *top = s->GetItem(static_cast<size_t>(0))->GetSizer();

	top->Detach(m_pickers[number]);
	top->Detach(m_layers[number]);
	s->Detach(m_checkBoxes[number]);
	s->Detach(m_rules[number]);

	m_sizer->Remove(number);

	delete m_pickers[number];
	delete m_checkBoxes[number];
	delete m_rules[number];
	delete m_layers[number];

	m_num--;
	for (int i = number; i < m_num; i++)
	{
		m_pickers[i] = m_pickers[i+1];
		m_checkBoxes[i] = m_checkBoxes[i+1];
		m_rules[i] = m_rules[i+1];
		m_layers[i] = m_layers[i+1];
	}
}

void ColorRules::Save(wxString const &name)
{
	wxConfigBase *config = wxConfig::Get();

	wxString baseGroup = wxString(wxT("rules/") + name);
	wxString numKey = baseGroup + wxT("/numRules");
	config->Write(numKey, m_num);

	for (int i = 0; i < m_num; i++)
	{
		wxString ruleGroup = wxString::Format(wxT("/colorrule_%d/"), i);

		m_rules[i]->Save(baseGroup + ruleGroup);
		m_checkBoxes[i]->Save(baseGroup + ruleGroup);
		m_pickers[i]->Save(baseGroup + ruleGroup);
		m_layers[i]->Save(baseGroup + ruleGroup);
		
	}
}



void ColorRules::Load(wxString const &name)
{
	Clear();
	
	wxConfigBase *config = wxConfig::Get();

	wxString baseGroup = wxString(wxT("rules/") + name);
	wxString numKey = baseGroup + wxT("/numRules");
	int num = config->Read(numKey, 0l);

	for (int i = 0; i < num; i++)
	{
		wxString ruleGroup = wxString::Format(wxT("/colorrule_%d/"), i);
		Add();

		m_rules[i]->Load(baseGroup + ruleGroup);
		m_checkBoxes[i]->Load(baseGroup + ruleGroup);
		m_pickers[i]->Load(baseGroup + ruleGroup);
		m_layers[i]->Load(baseGroup + ruleGroup);
		
	}
}


void RuleControl::Save(wxString const &group)
{
	wxString key = group + wxT("rule");

	wxConfigBase *config = wxConfig::Get();

	config->Write(key, GetValue());
}

void RuleControl::Load(wxString const &group)
{
	wxString key = group + wxT("rule");

	wxConfigBase *config = wxConfig::Get();

	SetValue(config->Read(key, wxEmptyString));

}



void PolyCheckBox::Save(wxString const &group)
{
	wxString key = group + wxT("polygon");

	wxConfigBase *config = wxConfig::Get();

	config->Write(key, static_cast<long>(IsChecked()));

}

void PolyCheckBox::Load(wxString const &group)
{
	wxString key = group + wxT("polygon");

	wxConfigBase *config = wxConfig::Get();

	SetValue(static_cast<bool>(config->Read(key, 0l)));

}


void ColorPicker::Save(wxString const &group)
{
	wxString rkey = group + wxT("red");
	wxString gkey = group + wxT("green");
	wxString bkey = group + wxT("blue");
	wxString akey = group + wxT("alpha");

	wxConfigBase *config = wxConfig::Get();

	config->Write(rkey, GetColour().Red());
	config->Write(gkey, GetColour().Green());
	config->Write(bkey, GetColour().Blue());
	config->Write(akey, GetColour().Alpha());

}



void ColorPicker::Load(wxString const &group)
{
	wxString rkey = group + wxT("red");
	wxString gkey = group + wxT("green");
	wxString bkey = group + wxT("blue");
	wxString akey = group + wxT("alpha");

	wxConfigBase *config = wxConfig::Get();

	int r = config->Read(rkey, 0l);
	int g = config->Read(gkey, 0l);
	int b = config->Read(bkey, 0l);
	int a = config->Read(akey, 0l);

	wxColour col;
	col.Set(r,g,b,a);

	SetColour(col);

}

void LayerPicker::Save(wxString const &group)
{
	wxString key = group + wxT("layer");

	wxConfigBase *config = wxConfig::Get();

	config->Write(key, static_cast<long>(GetSelection()));

}

void LayerPicker::Load(wxString const &group)
{
	wxString key = group + wxT("layer");

	wxConfigBase *config = wxConfig::Get();

	SetSelection(config->Read(key, 0l));

}


