#ifndef __RULECONTROL_H__
#define __RULECONTROL_H__

#include <wx/textctrl.h>
#include <wx/clrpicker.h>
#include <wx/checkbox.h>
#include <wx/button.h>

#include "s_expr.h"
#include "osmcanvas.h"

class RuleControl
	: public wxTextCtrl, public ExpressionParser
{
	public:
		RuleControl(wxWindow *parent, OsmCanvas *canvas, wxSize const &size = wxDefaultSize);

		virtual ~RuleControl();

		bool Evaluate(IdObjectWithTags *o);

	private:
		DECLARE_EVENT_TABLE();

		void OnText(wxCommandEvent &evt);

		virtual void SetColor(int from, int to, E_COLORS color);

		LogicalExpression *m_expr;
		OsmCanvas *m_canvas;
};

class ColorPicker
	: public wxColourPickerCtrl
{
	public:
		ColorPicker(wxWindow *parent, OsmCanvas *canvas)
			: wxColourPickerCtrl(parent, -1)
		{
		   m_canvas = canvas;
		}

		DECLARE_EVENT_TABLE();

	private:
		void OnChanged(wxColourPickerEvent &evt)
		{
			m_canvas->Redraw();
		}

		OsmCanvas *m_canvas;
};

class PolyCheckBox
	: public wxCheckBox
{
	public:
		PolyCheckBox(wxWindow *parent, OsmCanvas *canvas)
			:	wxCheckBox(parent, -1, wxT("polygon"))
		{
			m_canvas = canvas;
		}


		DECLARE_EVENT_TABLE();
	private:
		void OnChanged(wxCommandEvent &evt)
		{
			m_canvas->Redraw();
		}
		OsmCanvas *m_canvas;
};


class ColorRules
{
	public:
		ColorRules(wxWindow *parent, wxSizer *sizer, OsmCanvas *canvas)
		{
			m_max = 1024;
			m_num = 0;
			m_pickers = new ColorPicker *[m_max];
			m_rules = new RuleControl *[m_max];
			m_checkBoxes = new PolyCheckBox *[m_max];

			m_parent = parent;
			m_canvas = canvas;

			m_sizer = new wxBoxSizer(wxVERTICAL);

			sizer->Add(m_sizer, 1, wxEXPAND);
		}

		void Add();
//	private:
		ColorPicker **m_pickers;
		RuleControl **m_rules;
		PolyCheckBox **m_checkBoxes;

		int m_max;
		int m_num;

		wxWindow *m_parent;
		wxSizer *m_sizer;
		OsmCanvas *m_canvas;
};


class AddButton
	: public wxButton
{
	public:
		AddButton(wxWindow *parent, ColorRules *rules)
			: wxButton(parent, -1, wxT("Add Rule"))
		{
			m_rules = rules;
		}

		void OnClick(wxCommandEvent &evt)
		{
			m_rules->Add();
		}
		DECLARE_EVENT_TABLE()

	ColorRules *m_rules;
};


#endif

