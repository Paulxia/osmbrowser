#ifndef __LOGIC_H__
#define __LOGIC_H__

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
			m_children = NULL;
		}
		virtual ~LogicalExpression()
		{
			if (m_children)
				m_children->DestroyList();
		}

		void AddChildren(LogicalExpression *c)
		{
			if (!m_children)
			{
				m_children = c;
			}
			else
			{
				ListObject *l = m_children;
				while (l->m_next)
				{
					l = l->m_next;
				}
				l->m_next = c;
			}
		}

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
				if (!l->GetValue(o))
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
				if (l->GetValue(o))
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

		enum OPERATOR
		{
			NOT,
			AND,
			OR,
			TAG,
			INVALID
		};
		
	
		OPERATOR MatchOperator(char const *s, int *pos);
		
		char *ParseString(char const *f, int *pos, char *logError, unsigned maxLogErrorSize, unsigned *errorPos);
		
		LogicalExpression *ParseMultiple(char const *f, int *pos, char *logError, unsigned maxLogErrorSize, unsigned *errorPos);

		LogicalExpression *ParseSingle(char const *f, int *pos, char *logError, unsigned maxLogErrorSize, unsigned *errorPos);

		LogicalExpression *Parse(char const *from, char *logError, unsigned maxLogErrorSize, unsigned *errorPos)
		{
			int pos = 0;
			return ParseSingle(from, &pos, logError, maxLogErrorSize, errorPos);
		}
};


#endif

