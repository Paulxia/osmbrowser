#include "osm.h"
#include <assert.h> // for laye memory allocation checking
#include <stdlib.h>
#include <string.h>

OSMTAG *create_tag(char const *key, char const *value, OSMTAG *tail)
{
    OSMTAG *ret = malloc(sizeof(OSMTAG));
    assert(ret);

    ret->key = strdup(key);
    assert(ret->key);

    ret->value = strdup(value);
    assert(ret->value);

    ret->next = tail;

    return ret;
}


// frees the memory for all tags in the list
void destroy_tags(OSMTAG *tags)
{
    // first clean up the rest of the list
    if (tags->next)
        destroy_tags(tags->next);

    free(tags->key);
    free(tags->value);
    free(tags);
}

// -----------------------------------------------

// setup a node in the space pointed to. this is a bit rivial, but for consistency with ways and relations it is implemented anyway
void create_node(OSMNODE *node, unsigned id, double lat, double lon)
{
    node->id = id;
    node->lat = lat;
    node->lon = lon;
    node->tags = NULL;
}

void node_add_tag(OSMNODE *node, char const *key, char const *value)
{
    node->tags = create_tag(key, value, node->tags);
}

// frees all tags and resets members
void node_reset(OSMNODE *node)
{
    if (node->tags)
        destroy_tags(node->tags);

    create_node(node, 0, 0.0, 0.0);
}

// -----------------------------------------------


OSMNODEREF *create_noderef(unsigned id, OSMNODEREF *tail)
{
    OSMNODEREF *ret = malloc(sizeof(OSMNODEREF));
    assert(ret);
    ret->id = id;
    ret->next = tail;
    return ret;
}

// destroys all in the list
void destroy_noderefs(OSMNODEREF *refs)
{
    if (refs->next)
    {
        destroy_noderefs(refs->next);
    }

    free(refs);
}

// -----------------------------------------------

// initilize a way in the pointed to memory
void create_way(OSMWAY *way, unsigned id)
{
    way->id = id;
    way->nodes = NULL;
    way->resolvedNodes = NULL;
    way->tags = NULL;
    way->numResolvedNodes = 0;
}



void way_add_tag(OSMWAY *way, char const *key, char const *value)
{
    // add the tag to the head of the list
    way->tags = create_tag(key, value, way->tags);
}

void way_add_noderef(OSMWAY *way, unsigned id)
{
    way->nodes = create_noderef(id, way->nodes);
}

// frees tags, noderefs and resolvedNodes
void way_reset(OSMWAY *way)
{
    destroy_tags(way->tags);
    if (way->nodes)
    {
        destroy_noderefs(way->nodes);
    }
    if (way->resolvedNodes)
    {
        free(way->resolvedNodes);
    }

    create_way(way, 0);
}

// -----------------------------------------------

OSMWAYREF *create_wayref(unsigned id, OSMWAYREF *tail)
{
    OSMWAYREF *ret = malloc(sizeof(OSMWAYREF));
    assert(ret);

    ret->id = id;
    ret->next = tail;

    return ret;
}


void destroy_wayrefs(OSMWAYREF *wayrefs)
{
    if (wayrefs->next)
    {
        destroy_wayrefs(wayrefs->next);
    }

    free(wayrefs);
}

// -----------------------------------------------

void create_relation(OSMRELATION *r, unsigned id)
{
    r->id = id;
    r->nodes = NULL;
    r->ways = NULL;
    r->tags = NULL;

    r->resolvedNodes = NULL;
    r->numResolvedNodes = 0;
    r->resolvedWays = NULL;
    r->numResolvedWays = 0;
}


void relation_add_noderef(OSMRELATION *r, unsigned id)
{
    r->nodes = create_noderef(id, r->nodes);
}

void relation_add_wayref(OSMRELATION *r, unsigned id)
{
    r->ways = create_wayref(id, r->ways);
}

void relation_add_tag(OSMRELATION *r, char const *k, char const *v)
{
    r->tags = create_tag(k, v, r->tags);
}

void relation_reset(OSMRELATION *r)
{
    if (r->tags)
        destroy_tags(r->tags);

    if (r->nodes)
        destroy_noderefs(r->nodes);

    if (r->ways)
        destroy_wayrefs(r->ways);

    if (r->resolvedNodes)
        free(r->resolvedNodes);

    if (r->resolvedWays)
        free(r->resolvedWays);

    create_relation(r, 0);
}


// -----------------------------------------------
// some static helper functions

static OSMNODE *find_node(unsigned id, OSMNODE *nodes, unsigned num)
{
    unsigned i;

    // stupid linear search. change to binary search later
    for (i = 0; i < num ; i++)
    {
        if (nodes[i].id == id)
        {
            return nodes + i;
        }
    }

    return NULL;

}

static OSMWAY *find_way(unsigned id, OSMWAY *ways, unsigned num)
{
    unsigned i;

    // stupid linear search. change to binary search later
    for (i = 0; i < num ; i++)
    {
        if (ways[i].id == id)
        {
            return ways + i;
        }
    }

    return NULL;
}




// this will scan all noderefs and look up the nodes and fill the resolvedNodes array
// it returns the number of nodes
static unsigned resolve_nodes(OSMNODEREF *refs, OSMNODE ***resolvedNodes, OSMNODE *nodes, unsigned numNodes)
{
    // first count the number of nodes in this way
    int i;
    int nodeCount = 0;
    OSMNODEREF *r;

    if (!refs)
        return 0;

    for (r = refs; r; r = r->next)
    {
        nodeCount++;
    }

    (*resolvedNodes) = malloc(sizeof(OSMNODE *) * nodeCount);

    // add them in reverse, since they got reversed during adding to the list
    for (i = nodeCount - 1, r = refs ; i >=0; i--, r = r->next)
    {
        (*resolvedNodes)[i] = find_node(r->id, nodes, numNodes);
    }

    return nodeCount;
}

static unsigned resolve_ways(OSMWAYREF *refs, OSMWAY ***resolvedWays, OSMWAY *ways, unsigned numWays)
{
    int i;
    int wayCount = 0;
    OSMWAYREF *r;

    if (!refs)
        return 0;

    for (r = refs; r; r = r->next)
    {
        wayCount++;
    }

    (*resolvedWays) = malloc(sizeof(OSMWAY *) * wayCount);

    // add them in reverse, since they got reversed during adding to the list
    for (i = wayCount - 1, r = refs ; i >=0; i--, r = r->next)
    {
        (*resolvedWays)[i] = find_way(r->id, ways, numWays);
    }

    return wayCount;
}


// -----------------------------------------------

#define INITIAL_COUNTS 1024

OSMDATA *create_osmdata()
{
    OSMDATA *ret = malloc(sizeof(OSMDATA));

    assert(ret);

    ret->numNodes = ret->numWays = ret->numRelations = 0;
    ret->maxNumNodes = ret->maxNumWays = ret->maxNumRelations = INITIAL_COUNTS;

    ret->nodes = malloc(sizeof(OSMNODE) * INITIAL_COUNTS);
    ret->ways = malloc(sizeof(OSMWAY) * INITIAL_COUNTS);
    ret->relations = malloc(sizeof(OSMRELATION) * INITIAL_COUNTS);

    assert(ret->nodes && ret->ways && ret->relations);

    ret->parsingState = PARSE_TOPLEVEL;

    ret->minlat = ret->maxlat = ret->minlon = ret->maxlon = 0;

    return ret;
}

void osmdata_add_node(OSMDATA *data, unsigned id, double lat, double lon)
{
    assert(data->parsingState == PARSE_TOPLEVEL);

    if (!data->numNodes)
    {
        data->minlat = data->maxlat = lat;
        data->minlon = data->maxlon = lon;
    }
    else
    {
        if (lat < data->minlat)
            data->minlat = lat;

        if (lon < data->minlon)
            data->minlon = lon;

        if (lat > data->maxlat)
            data->maxlat = lat;

        if (lon > data->maxlon)
            data->maxlon = lon;
    }


    if (data->numNodes >= data->maxNumNodes)
    {
        data->maxNumNodes *=2;
        data->nodes = realloc(data->nodes, sizeof(OSMNODE) * data->maxNumNodes);
        assert(data->nodes);
    }

    create_node(data->nodes + data->numNodes, id, lat, lon);
    data->parsingState = PARSE_NODE;
}

void osmdata_end_node(OSMDATA *data)
{
    data->numNodes++;
    data->parsingState = PARSE_TOPLEVEL;
}

void osmdata_add_way(OSMDATA *data, unsigned id)
{
    assert(data->parsingState == PARSE_TOPLEVEL);

    if (data->numWays >= data->maxNumWays)
    {
        data->maxNumWays *=2;
        data->ways = realloc(data->ways, sizeof(OSMWAY) * data->maxNumWays);
        assert(data->ways);
    }

    create_way(data->ways + data->numWays, id);
    data->parsingState = PARSE_WAY;
}

void osmdata_end_way(OSMDATA *data)
{
    data->numWays++;
    data->parsingState = PARSE_TOPLEVEL;
}

void osmdata_add_relation(OSMDATA *data, unsigned id)
{
    assert(data->parsingState == PARSE_TOPLEVEL);

    if (data->numRelations >= data->maxNumRelations)
    {
        data->maxNumRelations *=2;
        data->relations = realloc(data->relations, sizeof(OSMRELATION) * data->maxNumRelations);
        assert(data->relations);
    }

    create_relation(data->relations + data->numRelations, id);
    data->parsingState = PARSE_RELATION;
}

void osmdata_end_relation(OSMDATA *data)
{
    data->numRelations++;
    data->parsingState = PARSE_TOPLEVEL;
}


void osmdata_add_noderef(OSMDATA *data, unsigned id)
{
    if (data->parsingState == PARSE_WAY)
    {
        way_add_noderef(data->ways + data->numWays, id);
    }
    else
    {
        assert(data->parsingState == PARSE_RELATION);
        relation_add_noderef(data->relations + data->numRelations, id);
    }
}


void osmdata_add_wayref(OSMDATA *data, unsigned id)
{
    assert(data->parsingState == PARSE_RELATION);

    relation_add_wayref(data->relations + data->numRelations, id);
}

void osmdata_add_tag(OSMDATA *data, char const *k, char const *v)
{
    // add the tag to the currenly parsed element
    switch (data->parsingState)
    {
        case PARSE_TOPLEVEL:
            abort();
            break;
        case PARSE_NODE:
            node_add_tag(data->nodes + data->numNodes, k, v);
            break;
        case PARSE_WAY:
            way_add_tag(data->ways + data->numWays, k, v);
            break;
        case PARSE_RELATION:
            relation_add_tag(data->relations + data->numRelations, k, v);
            break;
    }
}

// call this after parsinf to resolve all refs
void osmdata_resolve(OSMDATA *data)
{
    int i;
    for (i = 0; i < data->numWays; i++)
    {
        data->ways[i].numResolvedNodes = resolve_nodes(data->ways[i].nodes, &(data->ways[i].resolvedNodes), data->nodes, data->numNodes);
        // we could free the refs here to regain some memory.
    }
    for (i = 0; i < data->numRelations; i++)
    {
        data->relations[i].numResolvedNodes = resolve_nodes(data->relations[i].nodes, &(data->relations[i].resolvedNodes), data->nodes, data->numNodes);
        data->relations[i].numResolvedWays = resolve_ways(data->relations[i].ways, &(data->relations[i].resolvedWays), data->ways, data->numWays);
    }

}

void osmdata_destroy(OSMDATA *data)
{
    int i;

    for (i = 0; i < data->numNodes; i++)
    {
        node_reset(&(data->nodes[i]));
    }

    free(data->nodes);
    
    for (i = 0; i < data->numWays; i++)
    {
        way_reset(&(data->ways[i]));
    }

    free(data->ways);

    for (i = 0; i < data->numRelations; i++)
    {
        relation_reset(&(data->relations[i]));
    }

    free(data->relations);

    free(data);
    
}

