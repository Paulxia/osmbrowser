#ifndef __LOGICWINDOW_H__
#define __LOGICWINDOW_H__

#include <wx/textctrl.h>
#include "s_expr.h"
#include "osmcanvas.h"

class RuleControl
	: public wxTextCtrl, public ExpressionParser
{
	public:
		RuleControl(wxWindow *parent, OsmCanvas *canvas);

		virtual ~RuleControl();

		bool Evaluate(IdObjectWithTags *o);

	private:
		DECLARE_EVENT_TABLE();

		void OnText(wxCommandEvent &evt);

		virtual void SetColor(int from, int to, E_COLORS color);

		LogicalExpression *m_expr;
		OsmCanvas *m_canvas;
};

#endif

