#ifndef __OSMDATA_H__
#define __OSMDATA_H__

#ifdef __cplusplus
 extern "C" {
#endif

// overview data layout
// 
//     OSMDATA
//         NODES[numNodes]              (array of nodes)
//           ID                         (each node in the array)
//           LAT
//           LON
//           TAG->TAG->TAG...           (<- linked list)
//         WAYS[numWays]
//           ID
//           NODEPOINTERS[numNodesInWay]  (array of pointers which point into the array of nodes above)
//           TAG->TAG->TAG...           (<- linked list)
//         RELATIONS[numRelations]
//           ID
//           WAYPOINTERS[numWaysInRelation] (array of pointers which point into the array of ways above)
//           NODEPOINTERS[numNodesInRelation] (array of pointers which point into the array of nodes above)
//           TAG->TAG->TAG...           (<- linked list)
//
//           NODEREFS and WAYREFS are used during parsing, but unneeded afterwards


// to keep memory management simple (though maybe inefficient) we'll store stuff in linked lists
// during parsing, since then we don't need to know in advance how much we need to allocate and
// can avoid slow reallocs
typedef struct OSMTAG
{
    char *key;
    char *value;
    struct OSMTAG *next;

} OSMTAG;

// create a tag (copies the strings, so you can free the afterwards
OSMTAG *create_tag(char const *key, char const *value, OSMTAG *tail);

// frees the memory for all tags in the list
void destroy_tags(OSMTAG *tags);

typedef struct OSMNODE
{
    IDOBJECT o;
    double lat;
    double lon;

    // this is a linked list of tags, the will end up in the reverse order as in the file since we'll add
    // new tags to the head of the list
    OSMTAG *tags;
} OSMNODE;

// setup a node in the space pointed to
void node_create(OSMNODE *node, unsigned id, double lat, double lon);
void node_add_tag(OSMNODE *node, char const *key, char const *value);

// frees all tags (and maybe ste lat/lon/id to 0 for debugging?)
void node_reset(OSMNODE *node);

typedef struct OSMNODEREF
{
    unsigned id;
    struct OSMNODEREF *next;
} OSMNODEREF;

OSMNODEREF *create_noderef(unsigned id, OSMNODEREF *tail);
// destroys all in the list
void destroy_noderefs(OSMNODEREF *refs);

typedef struct OSMWAY
{
    unsigned id;
    OSMNODEREF *nodes;
    OSMTAG *tags;

    // this is filled by way_resolve() by looking up
    // all nodes
    OSMNODE **resolvedNodes;
    unsigned numResolvedNodes;
} OSMWAY;

// initilize a way in the pointed to memory
void create_way(OSMWAY *way, unsigned id);
void way_add_tag(OSMWAY *way, char const *key, char const *value);
void way_add_noderef(OSMWAY *way, unsigned id);

// frees tags, noderefs and resolvedNodes
void way_reset(OSMWAY *way);

typedef struct OSMWAYREF
{
    unsigned id;
    struct OSMWAYREF *next;
} OSMWAYREF;

OSMWAYREF *create_wayref(unsigned id, OSMWAYREF *tail);
void destroy_wayrefs(OSMWAYREF *wayrefs);

typedef struct OSMRELATION
{
    unsigned id;
    OSMNODEREF *nodes;
    OSMWAYREF *ways;
    OSMTAG *tags;

    OSMNODE **resolvedNodes;
    unsigned numResolvedNodes;
    OSMWAY **resolvedWays;
    unsigned numResolvedWays;
    
} OSMRELATION;

void create_relation(OSMRELATION *r, unsigned id);

void relation_add_noderef(OSMRELATION *r, unsigned id);
void relation_add_wayref(OSMRELATION *r, unsigned id);
void relation_add_tag(OSMRELATION *r, char const *k, char const *v);

void relation_reset(OSMRELATION *r);

typedef enum
{
    PARSE_TOPLEVEL,
    PARSE_NODE,
    PARSE_WAY,
    PARSE_RELATION
} PARSINGSTATE;

typedef struct OSMDATA
{
    // since we probably want to sort the nodes/ways, and we're going to have a *lot* of nodes
    // we don't want the overhead of a linked list, so these are plain arrays
    // which means we'll have to do a little more bookkeeping memorywise
    OSMNODE *nodes;
    unsigned numNodes;
    unsigned maxNumNodes;
    
    OSMWAY *ways;
    unsigned numWays;
    unsigned maxNumWays;
    
    OSMRELATION *relations;
    unsigned numRelations;
    unsigned maxNumRelations;

    PARSINGSTATE parsingState;

    // bounding box;
    double minlat, maxlat, minlon, maxlon;
    
} OSMDATA;

OSMDATA *create_osmdata();
void osmdata_add_node(OSMDATA *data, unsigned id, double lat, double lon);
void osmdata_end_node(OSMDATA *data);
void osmdata_add_way(OSMDATA *data, unsigned id);
void osmdata_end_way(OSMDATA *data);
void osmdata_add_relation(OSMDATA *data, unsigned id);
void osmdata_end_relation(OSMDATA *data);

void osmdata_add_noderef(OSMDATA *data, unsigned id);
void osmdata_add_wayref(OSMDATA *data, unsigned id);
void osmdata_add_tag(OSMDATA *data, char const *k, char const *v);

// call this after parsinf to resolve all refs
void osmdata_resolve(OSMDATA *data);

void osmdata_destroy(OSMDATA *data);

#ifdef __cplusplus
 }
#endif


#endif
