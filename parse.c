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

XML_Char const *get_attribute(const XML_Char *name, const XML_Char **attrs)
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

void XMLCALL start_element_handler(void *user_data, const XML_Char *name, const XML_Char **attrs)
{
    OSMDATA *o = (OSMDATA *)user_data;

    if (!strcmp(name, "node"))
    {
        XML_Char const *latS = get_attribute("lat", attrs);
        XML_Char const *lonS = get_attribute("lon", attrs);
        XML_Char const *idS = get_attribute("id", attrs);
        double lat = strtod(latS, NULL);
        double lon = strtod(lonS, NULL);
        unsigned id = strtoul(idS, NULL, 0);

        assert(latS && lonS && idS); // in case it didn't crash on the strto* functions

        osmdata_add_node(o, id, lat, lon);

        
    }
    else if (!strcmp(name, "way"))
    {
        XML_Char const *idS = get_attribute("id", attrs);
        unsigned id = strtoul(idS, NULL, 0);
        assert(idS);
        osmdata_add_way(o, id);

    }
    else if (!strcmp(name, "relation"))
    {
        XML_Char const *idS = get_attribute("id", attrs);
        unsigned id = strtoul(idS, NULL, 0);

        assert(idS);
        osmdata_add_relation(o, id);

    }
    else if (!strcmp(name, "tag"))
    {
        XML_Char const *key = get_attribute("k", attrs);
        XML_Char const *value = get_attribute("v", attrs);
        assert(key && value);

        osmdata_add_tag(o, key, value);
    }
    else if (!strcmp(name, "nd"))
    {
        XML_Char const *idS = get_attribute("ref", attrs);
        unsigned id = strtoul(idS, NULL, 0);
        assert(idS);

        osmdata_add_noderef(o, id);
    }
    else if (!strcmp(name, "member"))
    {
        XML_Char const *type = get_attribute("type", attrs);
        XML_Char const *idS = get_attribute("ref", attrs);
        unsigned id = strtoul(idS, NULL, 0);
        assert(idS && type);

        if (!strcmp(type, "node"))
        {
            osmdata_add_noderef(o, id);
        }
        else if (!strcmp(type, "way"))
        {
            osmdata_add_wayref(o, id);
        }
    }
}

void XMLCALL end_element_handler(void *user_data, const XML_Char *name)
{
    OSMDATA *o = (OSMDATA *)user_data;

    if (!strcmp(name, "node"))
    {
        osmdata_end_node(o);
    }
    else if (!strcmp(name, "way"))
    {
        osmdata_end_way(o);
    }
    else if (!strcmp(name, "relation"))
    {
        osmdata_end_relation(o);
    }
}



OSMDATA *parse_osm(FILE *file)
{
    char buffer[1024];
    int len;

    // cannot handle 16bit character sets
    // so if expat is configured wrong bail out
    assert(sizeof(XML_Char) == sizeof(char));

    OSMDATA *ret = create_osmdata();

    XML_Parser xml = XML_ParserCreate(NULL);

    XML_SetStartElementHandler(xml, start_element_handler);
    XML_SetEndElementHandler(xml, end_element_handler);

    XML_SetUserData(xml, ret);
    

    while (0 != (len = fread(buffer, 1, 1024, file)))
    {
        XML_Parse(xml, buffer, len, feof(file));
    }

    XML_ParserFree(xml);

    osmdata_resolve(ret);

    return ret;
}


