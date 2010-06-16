// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#ifndef __RULECONTROL_H__
#define __RULECONTROL_H__

#include <wx/textctrl.h>
#include <wx/clrpicker.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/combobox.h>
#include <wx/config.h>
#include <wx/choice.h>

#include "s_expr.h"
#include "osmcanvas.h"
#include "frame.h"

#define NUMLAYERS 3

class RuleControl
	: public wxTextCtrl, public ExpressionParser
{
	public:
		RuleControl(wxWindow *parent, OsmCanvas *canvas, wxSize const &size = wxDefaultSize);

		virtual ~RuleControl();

		bool Evaluate(IdObjectWithTags *o);

		void Save(wxString const &group);
		void Load(wxString const &group);

		void SetValueOnEmpty(bool val)
		{
			m_valueOnEmpty = val;
		}
	private:
		DECLARE_EVENT_TABLE();

		void OnText(wxCommandEvent &evt);

		virtual void SetColor(int from, int to, E_COLORS color);

		LogicalExpression *m_expr;
		OsmCanvas *m_canvas;
		bool m_valueOnEmpty;
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

		void Save(wxString const &group);
		void Load(wxString const &group);

		DECLARE_EVENT_TABLE();

	private:
		void OnChanged(wxColourPickerEvent &evt)
		{
			m_canvas->Redraw();
		}

		OsmCanvas *m_canvas;
};

class LayerPicker
	: public wxChoice
{
	public:
		LayerPicker(wxWindow *parent, OsmCanvas *canvas)
			: wxChoice(parent, -1)
		{
			m_canvas = canvas;
			for (int  i = 0; i < NUMLAYERS; i++)
			{
				Append(wxString::Format(wxT("%d"), i));
			}
			SetSelection(0);
		}
		void Save(wxString const &group);
		void Load(wxString const &group);

	private:
		void OnSelect(wxCommandEvent &evt)
		{
			m_canvas->Redraw();
		}

		OsmCanvas *m_canvas;

		DECLARE_EVENT_TABLE();
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

		void Save(wxString const &group);
		void Load(wxString const &group);


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
			m_layers = new LayerPicker *[m_max];

			m_parent = parent;
			m_canvas = canvas;

			m_sizer = new wxBoxSizer(wxVERTICAL);

			sizer->Add(m_sizer, 1, wxEXPAND);
		}
		~ColorRules()
		{
			delete [] m_pickers;
			delete [] m_rules;
			delete [] m_checkBoxes;
			delete [] m_layers;
		}
		
		void Add();
		void Remove (int number);
		void Clear()
		{
			while (m_num)
			{
				Remove(m_num - 1);
			}
		}
		

		void Save(wxString const &name);
		void Load(wxString const &name);
//	private:
		ColorPicker **m_pickers;
		RuleControl **m_rules;
		PolyCheckBox **m_checkBoxes;
		LayerPicker **m_layers;

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


class RulesComboBox
	: public wxComboBox
{
	public:
		RulesComboBox(wxWindow *parent, MainFrame *frame)
			: wxComboBox(parent, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxTE_PROCESS_ENTER | wxCB_SORT)
		{
			m_mainFrame = frame;

			Fill();
		}

		void Fill()
		{
			Clear();
			
			wxConfigBase *config = wxConfigBase::Get();

			config->SetPath(wxT("/rules"));

			wxString group;
			long index = 0;
			for (bool cont = config->GetFirstGroup(group, index); cont; cont = config->GetNextGroup(group, index))
			{
				Append(group);
			}

			config->SetPath(wxT("/"));

		}


	private:

		void OnEnter(wxCommandEvent &evt)
		{
			if (GetValue().Length())
			{
				m_mainFrame->Save(GetValue());
			}

			Fill();
		}

		void OnSelected(wxCommandEvent &evt)
		{
			if (GetValue().Length())
			{
				m_mainFrame->Load(GetValue());
			}
		}

		MainFrame *m_mainFrame;

		DECLARE_EVENT_TABLE();
};

#endif

