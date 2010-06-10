// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#include "s_expr.h"



ExpressionParser::E_OPERATOR ExpressionParser::MatchOperator(char const *s, int *pos, bool *disabled)
{
	char const *operators[] =
	{
		"not", "and", "or", "tag"
	};

	if (s[*pos] == '-')
	{
		*disabled = true;
		(*pos)++;
		m_mustColorDisabled++;
	}

	int count = sizeof(operators)/sizeof(char *);
	for (int i = 0; i < count ; i++)
	{
		if (!strncasecmp(operators[i], s + *pos, strlen(operators[i])))
		{
			int len = strlen(operators[i]);
			SetColorD(*pos, *pos + len, EC_OPERATOR);
			*pos += len;
			return static_cast<ExpressionParser::E_OPERATOR>(i);
		}
	}

	return static_cast<ExpressionParser::E_OPERATOR>(count);
}


char *ExpressionParser::ParseString(char const *f, int *pos, char *logError, unsigned maxLogErrorSize, unsigned *errorPos)
{
	static char buffers[2][1024];
	static int curBuffer = 0;

	curBuffer++;
	curBuffer %= 2;

	EatSpace(f, pos);

	int p = *pos;

	if (f[p] != '"' && f[p] != '\'')
	{
		snprintf(logError, maxLogErrorSize, "expected string value");
		*errorPos= p;
		return NULL;
	}

	int end = f[p];

	p++;
	int i = 0;
	while (f[p] && f[p] != end)
	{
		buffers[curBuffer][i++] = f[p++];

		if (i >= 1023)
			i = 1023;
	}

	buffers[curBuffer][i] = 0;

	if (f[p] != end)
	{
		snprintf(logError, maxLogErrorSize, "expected %c for end of string", end);
		*errorPos= p;
		return NULL;
	}

	p++;

	SetColorD(*pos, p, EC_STRING);

	*pos = p;

	EatSpace(f,pos);
    
	return buffers[curBuffer];
}


LogicalExpression *ExpressionParser::ParseMultiple(char const *f, int *pos, char *logError, unsigned maxLogErrorSize, unsigned *errorPos)
{
	LogicalExpression *ret = ParseSingle(f, pos, logError, maxLogErrorSize, errorPos);

	if (!ret)
	{
		return NULL;
	}

	LogicalExpression *next;

	while (true)
	{
		EatSpace(f, pos);

		if (f[*pos] != '(')
		{
			break;
		}

		next = ParseSingle(f, pos, logError, maxLogErrorSize, errorPos);

		if (!next)
		{
			ret->DestroyList();
			return NULL;
		}
		
		ret = static_cast<LogicalExpression *>(ListObject::Concat(ret, next));
	}

	return ret;
}

void ExpressionParser::EatSpace(char const *s, int *pos)
{
	int p = *pos;

	while (s[p] && isspace(s[p]))
	{
		p++;
	}

	SetColorD(*pos, p, EC_SPACE);

	*pos = p;
}


LogicalExpression *ExpressionParser::ParseSingle(char const *f, int *pos, char *logError, unsigned maxLogErrorSize, unsigned *errorPos)
{
	bool disabled = false;
	LogicalExpression *ret = NULL;
	LogicalExpression *c = NULL;
	E_OPERATOR op = INVALID;

	EatSpace(f, pos);

	int p = *pos;

	if (!(f[p]))
	{
		snprintf(logError, maxLogErrorSize, "unexpected end of expression");
		goto error;
		
	}

	if (f[p] != '(')
	{
		snprintf(logError, maxLogErrorSize, "expected '('");
		goto error;
	}

	SetColorD(p, p+1, EC_BRACKET);

	p++;


	op = MatchOperator(f, &p, &disabled);

	switch(op)
	{
		case NOT: // not
			ret = new Not;
		break;
		case AND:  // and
			ret = new And;
		break;
		case OR:
			ret = new Or;
		break;
		case TAG:
		{
			char const *key = ParseString(f, &p,logError, maxLogErrorSize, errorPos);
			char const *value = ParseString(f, &p,logError, maxLogErrorSize, errorPos);

			if (!key)
			{
				snprintf(logError, maxLogErrorSize, "expected tag key");
				goto error;
			}
			
			if (!OsmTag::KeyExists(key))
			{
				snprintf(logError, maxLogErrorSize, "unknown tag key '%s'", key);
				goto error;
			}

			ret = new Tag(key, value);

		}
		break;
		default:
			snprintf(logError, maxLogErrorSize, "unknown operator");
			goto error;
		break;
	}

	switch(op)
	{
		case NOT:
			c = ParseSingle(f, &p, logError, maxLogErrorSize, errorPos);
			if (!c)
			{
//				snprintf(logError, maxLogErrorSize, "expect subexpression");
				goto error;
			}
		break;
		case AND:
		//fallthrough
		case OR:
			c = ParseMultiple(f, &p, logError, maxLogErrorSize, errorPos);
			if (!c)
			{
//				snprintf(logError, maxLogErrorSize, "expect subexpression(s)");
				goto error;
			}
		break;
		case TAG:
		break;
		default:
		break;
	}


	if (c)
	{
		ret->AddChildren(c);
	}

	EatSpace(f, &p);

	if (f[p] != ')')
	{
		snprintf(logError, maxLogErrorSize, "expected ')'");
		*errorPos = p;
		goto error;
	}

	SetColorD(p, p+1, EC_BRACKET);

	p++;
	*pos = p;
	// clear the errorlog
	*logError = 0;

	ret->m_disabled = disabled;

	if (disabled)
	{
		m_mustColorDisabled--;
	}

	return ret;
	error:

	if (disabled)
	{
		m_mustColorDisabled--;
	}

	if (ret)
	{
		delete ret;
	}
	
	*errorPos = p;

	SetColorD(p, strlen(f), EC_ERROR);
	return NULL;
	

}

