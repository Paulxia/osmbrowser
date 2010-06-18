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
  | (identity e)                 true if e is true (to esily change a not without changing hte whole tree
  
  
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
		
	
		E_OPERATOR MatchOperator(char const *s, int *pos, bool *disabled);
		
		char *ParseString(char const *f, int *pos, char *logError, unsigned maxLogErrorSize, unsigned *errorPos);
		
		LogicalExpression *ParseMultiple(char const *f, int *pos, char *logError, unsigned maxLogErrorSize, unsigned *errorPos);

		LogicalExpression *ParseSingle(char const *f, int *pos, char *logError, unsigned maxLogErrorSize, unsigned *errorPos);

		LogicalExpression *Parse(char const *from, char *logError, unsigned maxLogErrorSize, unsigned *errorPos)
		{
			int pos = 0;
			return ParseSingle(from, &pos, logError, maxLogErrorSize, errorPos);
		}

		void EatSpace(char const *s, int *pos);


		enum E_COLORS
		{
			EC_SPACE,
			EC_BRACKET,
			EC_OPERATOR,
			EC_STRING,
			EC_ERROR,
			EC_DISABLED
		};

		virtual void SetColor(int from, int to, E_COLORS color)
		{
		}

	private:
		void SetColorD(int from, int to, E_COLORS color)
		{
			if (m_mustColorDisabled && color != EC_ERROR)
			{
				SetColor(from, to, EC_DISABLED);
			}
			else
			{
				SetColor(from, to, color);
			}
			
		}

		int m_mustColorDisabled;

        
};


#endif

