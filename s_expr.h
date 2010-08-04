// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#ifndef __S_EXPRESSION_H__
#define __S_EXPRESSION_H__

#include <ctype.h>
/*
  e =
  (tag key)                      true if a tag with this key exists
  | (tag key value)              true if the key/value pair exists
  | (and e e e e e ...)          true if all e's true
  | (or e e e e ...)             true if any e true
  | (not e)                      true if e false and vv
  | (identity e)                 true if e is true (to esily change a not without changing the whole tree
  
  
*/

#include "osm.h"

class LogicalExpression
	: public ListObject
{
	public:
		LogicalExpression()
			: ListObject(NULL)
		{
			m_disabled = false;
			m_children = NULL;
		}
		virtual ~LogicalExpression()
		{
			if (m_children)
				m_children->DestroyList();
		}

		enum STATE
		{
			S_FALSE,
			S_TRUE,
			S_IGNORE,
			S_INVALID
		};

		void AddChildren(LogicalExpression *c)
		{
			m_children = static_cast<LogicalExpression *>(ListObject::Concat(m_children, c));
		}
		bool m_disabled;
		LogicalExpression *m_children;
		virtual STATE GetValue(IdObjectWithTags *o) = 0;
};

class Not
	: public LogicalExpression
{
	public:
		virtual STATE GetValue(IdObjectWithTags *o)
		{
			if (m_disabled)
				return S_IGNORE;

			STATE states[] = {  S_TRUE, S_FALSE, S_IGNORE, S_INVALID};
			STATE s = m_children->GetValue(o);

			return states[s];
		}

};

class And
	: public LogicalExpression
{
	public:
		STATE GetValue(IdObjectWithTags *o)
		{
			if (m_disabled)
				return S_IGNORE;
		
			int trueCount = 0;
			for (LogicalExpression *l = m_children; l; l = static_cast<LogicalExpression *>(l->m_next))
			{
				if (!l->m_disabled)
				{
					STATE s = l->GetValue(o);
					if (s == S_FALSE)
						return S_FALSE;
					else if (s == S_TRUE)
						trueCount++;

				}
				
			}

			return trueCount ? S_TRUE : S_IGNORE;
		}

};

class Or
	: public LogicalExpression
{
	public:
		STATE GetValue(IdObjectWithTags *o)
		{
			if (m_disabled)
				return S_IGNORE;

			int falseCount = 0;
			for (LogicalExpression *l = m_children; l; l = static_cast<LogicalExpression *>(l->m_next))
			{
				if ( !l->m_disabled)
				{
					STATE s = l->GetValue(o);

					switch(s)
					{
						case S_TRUE:
							return S_TRUE;
							break;
						case S_FALSE:
							falseCount++;
							break;
						default:
							break;
					}
				}
			}

			return falseCount ? S_FALSE : S_IGNORE;
		}

};


class Tag
	: public LogicalExpression
{
	public:
		Tag(char const *key, char const *value)
		{
			m_tag = new OsmTag(true, key, value);
		}

		~Tag()
		{
			delete m_tag;
		}

//		bool Valid()
//		{
//			return m_tag->Valid();
//		}
		
		STATE GetValue(IdObjectWithTags *o)
		{
			if (m_disabled)
				return S_IGNORE;
			return o->HasTag(*m_tag) ? S_TRUE : S_FALSE;
		}
	private:
		OsmTag *m_tag;

};

class RuleDisplay
{
	public:
		enum E_COLORS
		{
			EC_SPACE,
			EC_BRACKET,
			EC_OPERATOR,
			EC_STRING,
			EC_ERROR,
			EC_DISABLED
		};

		virtual void SetColor(int from, int to, E_COLORS color) = 0;
};

class ExpressionParser
{
	public:
		ExpressionParser()
		{
			m_mustColorDisabled = 0;
		}

		enum E_OPERATOR
		{
			NOT,
			AND,
			OR,
			TAG,
			OFF,
			INVALID
		};
		
	

		LogicalExpression *Parse(char const *from, char *logError, unsigned maxLogErrorSize, unsigned *errorPos, RuleDisplay *display = NULL)
		{
			int pos = 0;
			m_display = display;
			return ParseSingle(from, &pos, logError, maxLogErrorSize, errorPos);
		}

	private:
		RuleDisplay *m_display;

		void EatSpace(char const *s, int *pos);

		E_OPERATOR MatchOperator(char const *s, int *pos, bool *disabled);
		
		char *ParseString(char const *f, int *pos, char *logError, unsigned maxLogErrorSize, unsigned *errorPos);
		
		LogicalExpression *ParseMultiple(char const *f, int *pos, char *logError, unsigned maxLogErrorSize, unsigned *errorPos);

		LogicalExpression *ParseSingle(char const *f, int *pos, char *logError, unsigned maxLogErrorSize, unsigned *errorPos);


		void SetColorD(int from, int to, RuleDisplay::E_COLORS color)
		{
			if (!m_display)
			{
				return;
			}
			
			if (m_mustColorDisabled && color != RuleDisplay::EC_ERROR)
			{
				m_display->SetColor(from, to, RuleDisplay::EC_DISABLED);
			}
			else
			{
				m_display->SetColor(from, to, color);
			}
			
		}

		int m_mustColorDisabled;

        
};

class Rule
	: public ExpressionParser
{
	public:

		Rule(wxString const &text, RuleDisplay *display = NULL)
		{
			m_expr = NULL;
			SetRule(text, display);
		}
	
		Rule()
		{
			m_expr = NULL;
			m_errorPos = 0;
		}

		Rule(Rule const &other)
		{
			m_expr = NULL;
			Create(other);
		}

		Rule const &operator=(Rule const &other)
		{
			Create(other);

			return *this;
		}

		~Rule()
		{
			delete m_expr;
		}
		// set a new ruletext. returns true if the text is a valid expression
		bool SetRule(wxString const &text, RuleDisplay *display = NULL)
		{
			delete m_expr;
			m_text = text;

			char errorLog[1024] = {0};
			m_expr = Parse(text.mb_str(wxConvUTF8),  errorLog, 1024, &m_errorPos, display);

			m_errorLog = wxString::FromUTF8(errorLog);

			return m_expr;
		}


		bool IsValid() { return m_expr; }
		
		wxString const &GetErrorLog()
		{
			return m_errorLog;
		}
		
		unsigned int GetErrorPos()
		{
			return m_errorPos;
		}

		LogicalExpression::STATE Evaluate(IdObjectWithTags *o)
		{
			if (!m_expr)
			{
				return LogicalExpression::S_INVALID;
			}

			return m_expr->GetValue(o);
		}
	private:
		void Create(Rule const &other)
		{
			SetRule(other.m_text);
		}
		
		LogicalExpression *m_expr;
		wxString m_text;
		wxString m_errorLog;
		unsigned int m_errorPos;
		
};


#endif

