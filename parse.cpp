// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#include "parse.h"
#include "osm.h"
#include <expat.h>
#include <string.h>
#include <assert.h>

// op windows heeft expat dit nodig. als het niet gedefinieerd is definieer het als niks
// stel dat we ooit op windows moeten werken dan is het er vast bij getypt
#ifndef XMLCALL
	#define XMLCALL
#endif

static XML_Char const *get_attribute(const XML_Char *name, const XML_Char **attrs)
{
	int count = 0;

	while (attrs[count *2])
	{
		if (!strcmp(name, attrs[count*2]))
		{
			return attrs[count * 2+1];
		}
		count++;
	}

	return NULL;
}

static void ReadAttribs(OsmData *o, XML_Char const **attrs)
{
	XML_Char const *keys[] =
	{
		"user",
		"uid",
		"visible",
		"version",
		"changeset",
		"timestamp"
	};

	int size = sizeof(keys) / sizeof( XML_Char *);

	for (int i = 0; i < size; i++)
	{
		char const *v = get_attribute(keys[i], attrs);

		if (v)
			o->AddAttribute(keys[i], v);
	}
}

void XMLCALL start_element_handler(void *user_data, const XML_Char *name, const XML_Char **attrs)
{
	OsmData *o = (OsmData *)user_data;

	if (!(o->m_elementCount % 1000000))
	{
		printf("parsed %uM elements\n", o->m_elementCount/1000000);

		double a,s;
		int m;
		o->m_nodes.GetStatistics(&a, &s, &m);
		printf(" statistics: a %g s %g max %d | ", a, s, m);
		o->m_ways.GetStatistics(&a, &s, &m);
		printf("a %g s %g max %d | ", a, s, m);
		o->m_relations.GetStatistics(&a, &s, &m);
		printf("a %g s %g max %d \n", a, s, m);
		
	}

	if (!strcmp(name, "node"))
	{
		XML_Char const *latS = get_attribute("lat", attrs);
		XML_Char const *lonS = get_attribute("lon", attrs);
		XML_Char const *idS = get_attribute("id", attrs);
		double lat = strtod(latS, NULL);
		double lon = strtod(lonS, NULL);
		unsigned id = strtoul(idS, NULL, 0);

		assert(latS && lonS && idS); // in case it didn't crash on the strto* functions

		o->StartNode(id, lat, lon);

		ReadAttribs(o, attrs);
	}
	else if (!strcmp(name, "way"))
	{
		XML_Char const *idS = get_attribute("id", attrs);
		unsigned id = strtoul(idS, NULL, 0);
		assert(idS);
		o->StartWay(id);
		ReadAttribs(o, attrs);
	}
	else if (!strcmp(name, "relation"))
	{
		XML_Char const *idS = get_attribute("id", attrs);
		unsigned id = strtoul(idS, NULL, 0);

		assert(idS);
		o->StartRelation(id);
		ReadAttribs(o, attrs);
	}
	else if (!strcmp(name, "tag"))
	{
		XML_Char const *key = get_attribute("k", attrs);
		XML_Char const *value = get_attribute("v", attrs);
		assert(key && value);

		o->AddTag(key, value);
	}
	else if (!strcmp(name, "nd"))
	{
		XML_Char const *idS = get_attribute("ref", attrs);
		unsigned id = strtoul(idS, NULL, 0);
		assert(idS);

		o->AddNodeRef(id);
	}
	else if (!strcmp(name, "member"))
	{
		XML_Char const *type = get_attribute("type", attrs);
		XML_Char const *idS = get_attribute("ref", attrs);
		unsigned id = strtoul(idS, NULL, 0);
		assert(idS && type);

		if (!strcmp(type, "node"))
		{
			o->AddNodeRef(id);
		}
		else if (!strcmp(type, "way"))
		{
			o->AddWayRef(id);
		}
	}
}

void XMLCALL end_element_handler(void *user_data, const XML_Char *name)
{
	OsmData *o = (OsmData *)user_data;

	if (!strcmp(name, "node"))
	{
		o->EndNode();
	}
	else if (!strcmp(name, "way"))
	{
		o->EndWay();
	}
	else if (!strcmp(name, "relation"))
	{
		o->EndRelation();
	}
}



OsmData *parse_osm(FILE *file, bool skipAttribs)
{
	char buffer[1024];
	int len;

	// cannot handle 16bit character sets
	// so if expat is configured wrong bail out
	assert(sizeof(XML_Char) == sizeof(char));

	OsmData *ret = new OsmData;

	ret->m_skipAttribs = skipAttribs;

	XML_Parser xml = XML_ParserCreate(NULL);

	XML_SetStartElementHandler(xml, start_element_handler);
	XML_SetEndElementHandler(xml, end_element_handler);

	XML_SetUserData(xml, ret);
	

	unsigned count = 0;
	while (0 != (len = fread(buffer, 1, 1024, file)))
	{
		XML_Parse(xml, buffer, len, feof(file));
		count++;
		if (!(count % 10240))
			printf("parsed %uMB\n", count / 1024);
	}

	XML_ParserFree(xml);

	ret->Resolve();

	return ret;
}

#define MAXTAGSIZE 10240

static void ReadTags(OsmData *d, unsigned count, FILE *f, bool readAttribs = false)
{
	char keybuf[MAXTAGSIZE+1];
	char valbuf[MAXTAGSIZE+1];

	keybuf[MAXTAGSIZE] = valbuf[MAXTAGSIZE] = 0;

	for (unsigned i = 0; i < count; i++)
	{
		int count = 0;
		int c;
		while((c = getc(f)))
		{
			if (count < MAXTAGSIZE)
			{
				keybuf[count] = c;
			}
			count++;
		}

		if (count < MAXTAGSIZE)
		{
			keybuf[count] = 0;
		}
		// read val
		count = 0;
		while((c = getc(f)))
		{
			if (count < MAXTAGSIZE)
			{
				valbuf[count] = c;
			}
			count++;
		}

		if (count < MAXTAGSIZE)
		{
			valbuf[count] = 0;
		}

		if (readAttribs)
		{
			d->AddAttribute(keybuf, valbuf);
		}
		else
		{
			d->AddTag(keybuf,valbuf);
		}

	}


	
}

static void ReadNode(OsmData *d, FILE *f)
{
	double lat, lon;
	unsigned id, tagCount;
	int ret;
	ret = fread(&id, sizeof(id), 1, f);
	assert(ret == sizeof(id));
	ret = fread(&lat, sizeof(lat), 1, f);
	assert(ret == sizeof(lat));
	ret = fread(&lon, sizeof(lon), 1, f);
	assert(ret == sizeof(lon));
	ret =fread(&tagCount, sizeof(tagCount), 1, f);
	assert(ret == sizeof(tagCount));

	d->StartNode(id, lat, lon);
	ReadTags(d, tagCount, f);
	d->EndNode();

}

static void ReadWay(OsmData *d, FILE *f)
{
	unsigned id, tagCount, nodeRefCount;
	int ret;
	ret = fread(&id, sizeof(id), 1, f);
	assert(ret == sizeof(id));
	d->StartWay(id);
	ret = fread(&nodeRefCount, sizeof(nodeRefCount), 1, f);
	assert(ret == sizeof(nodeRefCount));
	for (unsigned i = 0; i < nodeRefCount; i++)
	{
		ret = fread(&id, sizeof(id), 1, f);
		assert(ret == sizeof(id));
		d->AddNodeRef(id);
	}
	ret = fread(&tagCount, sizeof(tagCount), 1, f);
	assert(ret == sizeof(tagCount));
	ReadTags(d, tagCount, f);
	d->EndWay();
}

static void ReadRelation(OsmData *d, FILE *f)
{
	unsigned id, tagCount, nodeRefCount, wayRefCount;
	int ret;
	ret = fread(&id, sizeof(id), 1, f);
	assert(ret == sizeof(id));
	d->StartRelation(id);
	ret = fread(&nodeRefCount, sizeof(nodeRefCount), 1, f);
	assert(ret == sizeof(nodeRefCount));
	for (unsigned i = 0; i < nodeRefCount; i++)
	{
		ret = fread(&id, sizeof(id), 1, f);
		assert(ret == sizeof(id));
		d->AddNodeRef(id);
	}

	ret = fread(&wayRefCount, sizeof(wayRefCount), 1, f);
	assert(ret == sizeof(wayRefCount));
	for (unsigned i = 0; i < wayRefCount; i++)
	{
		ret = fread(&id, sizeof(id), 1, f);
		assert(ret == sizeof(id));
		d->AddWayRef(id);
	}
	ret = fread(&tagCount, sizeof(tagCount), 1, f);
	assert(ret == sizeof(tagCount));
	ReadTags(d, tagCount, f);
	d->EndRelation();
}


OsmData *parse_binary(FILE *f, bool skipAttribs)
{
	OsmData *ret = new OsmData();


	ret->m_skipAttribs = skipAttribs;
	unsigned count = 0;
	while (!feof(f))
	{
		if (!(count % (1024*1024)))
		{
			printf("Read %dM elements \n", count / (1024*1024));
		}

		count++;
		
		int c = fgetc(f);

		if (feof(f))
		{
			break;
		}

		switch(c)
		{
			case 'N':
				ReadNode(ret, f);
				break;
			case 'W':
				ReadWay(ret, f);
				break;
			case 'R':
				ReadRelation(ret, f);
				break;
			default:
				printf("illegal element at position %u\n", count);
				abort();
				break;
		}
	}


	ret->Resolve();

	return ret;
}

static void WriteTags(OsmTag *tags, FILE *f)
{
	unsigned zero = 0;

	if (!tags)
	{
		fwrite(&zero, sizeof(zero), 1, f);
		return;
	}

	unsigned size = tags->GetSize();

	fwrite(&(size), sizeof(size), 1, f);

	for (OsmTag *t = tags; t; t = static_cast<OsmTag *>(t->m_next))
	{
		fwrite(t->GetKey(), sizeof(char), strlen(t->GetKey()) + 1, f);
		fwrite(t->GetValue(), sizeof(char), strlen(t->GetValue()) + 1, f);
	}
	
}

void write_binary(OsmData *d, FILE *f)
{
	unsigned zero = 0;
	printf("writing nodes...\n" );
	for (OsmNode *n = static_cast<OsmNode *>(d->m_nodes.m_content); n ; n = static_cast<OsmNode *>(n->m_next))
	{
		fputc('N', f);
		fwrite(&(n->m_id), sizeof(n->m_id), 1, f);
		fwrite(&(n->m_lat), sizeof(n->m_lat), 1, f);
		fwrite(&(n->m_lon), sizeof(n->m_lon), 1, f);

		WriteTags(n->m_tags, f);
	}

	printf("writing ways...\n" );
	for (OsmWay *w = static_cast<OsmWay *>(d->m_ways.m_content); w; w = static_cast<OsmWay *>(w->m_next))
	{
		fputc('W', f);
		fwrite(&(w->m_id), sizeof(w->m_id), 1, f);

		if (w->m_nodeRefs) // if the noderefs still exists, this means the way is not fully resolved, so use the refs
		{
			unsigned size = w->m_nodeRefs->GetSize();
			fwrite(&(size), sizeof(size), 1, f);

			for (IdObject *i = w->m_nodeRefs; i; i = static_cast<IdObject *>(i->m_next))
			{
				fwrite(&(i->m_id), sizeof(i->m_id), 1, f);
			}
			
		}
		else if (w->m_resolvedNodes) // the refs don't exists, so the way must be fully resolved
		{
			fwrite(&(w->m_numResolvedNodes), sizeof(w->m_numResolvedNodes), 1, f);

			for (unsigned  i = 0; i < w->m_numResolvedNodes; i++)
			{
				unsigned id = w->m_resolvedNodes[i]->m_id;
				fwrite(&(id), sizeof(id), 1, f);
			}
		}
		else
		{
			fwrite(&zero, sizeof(zero), 1, f);
		}

		WriteTags(w->m_tags, f);
	}


	printf("writing relations...\n" );
	for (OsmRelation *r = static_cast<OsmRelation *>(d->m_relations.m_content); r; r = static_cast<OsmRelation *>(r->m_next))
	{
		fputc('R', f);
		fwrite(&(r->m_id), sizeof(r->m_id), 1, f);

		if (r->m_nodeRefs)
		{
			unsigned size = r->m_nodeRefs->GetSize();
			fwrite(&(size), sizeof(size), 1, f);

			for (IdObject *i = r->m_nodeRefs; i; i = static_cast<IdObject *>(i->m_next))
			{
				fwrite(&(i->m_id), sizeof(i->m_id), 1, f);
			}
			
		}
		else if (r->m_resolvedNodes) // the refs don't exists, so the way must be fully resolved
		{
			fwrite(&(r->m_numResolvedNodes), sizeof(r->m_numResolvedNodes), 1, f);

			for (unsigned  i = 0; i < r->m_numResolvedNodes; i++)
			{
				unsigned id = r->m_resolvedNodes[i]->m_id;
				fwrite(&(id), sizeof(id), 1, f);
			}
		}
		else
		{
			fwrite(&zero, sizeof(zero), 1, f);
		}

		if (r->m_wayRefs)
		{
			unsigned size = r->m_wayRefs->GetSize();
			fwrite(&(size), sizeof(size), 1, f);

			for (IdObject *i = r->m_wayRefs; i; i = static_cast<IdObject *>(i->m_next))
			{
				fwrite(&(i->m_id), sizeof(i->m_id), 1, f);
			}
			
		}
		else if (r->m_resolvedWays) // the refs don't exists, so the way must be fully resolved
		{
			fwrite(&(r->m_numResolvedWays), sizeof(r->m_numResolvedWays), 1, f);

			for (unsigned  i = 0; i < r->m_numResolvedWays; i++)
			{
				unsigned id = r->m_resolvedWays[i]->m_id;
				fwrite(&(id), sizeof(id), 1, f);
			}
		}
		else
		{
			fwrite(&zero, sizeof(zero), 1, f);
		}

		WriteTags(r->m_tags, f);
	}
	printf("done writing\n");
}
