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

		void AddChildren(LogicalExpression *c)
		{
			m_children = static_cast<LogicalExpression *>(ListObject::Concat(m_children, c));
		}
		bool m_disabled;
		LogicalExpression *m_children;
		virtual bool GetValue(IdObjectWithTags *o) = 0;
};

class Not
	: public LogicalExpression
{
	public:
		virtual bool GetValue(IdObjectWithTags *o)
		{
			return !(m_children->GetValue(o));
		}

};

class And
	: public LogicalExpression
{
	public:
		bool GetValue(IdObjectWithTags *o)
		{
			for (LogicalExpression *l = m_children; l; l = static_cast<LogicalExpression *>(l->m_next))
			{
				if (!l->GetValue(o) && !l->m_disabled)
					return false;
			}

			return true;
		}

};

class Or
	: public LogicalExpression
{
	public:
		bool GetValue(IdObjectWithTags *o)
		{
			for (LogicalExpression *l = m_children; l; l = static_cast<LogicalExpression *>(l->m_next))
			{
				if (l->GetValue(o) && !l->m_disabled)
					return true;
			}

			return false;
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

		bool Valid()
		{
			return m_tag->Valid();
		}
		
		bool GetValue(IdObjectWithTags *o)
		{
			return o->HasTag(*m_tag);
		}
	private:
		OsmTag *m_tag;

};

class ExpressionParser
{
	public:

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

		enum E_COLORS
		{
			EC_BRACKET,
			EC_OPERATOR,
			EC_STRING,
			EC_ERROR
		};

		virtual void SetColor(int from, int to, E_COLORS color)
		{
		}

        
};


#endif

