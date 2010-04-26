#include "osm.h"
#include <assert.h> // for lazy memory allocation checking
#include <stdlib.h>
#include <string.h>

TagStore *OsmTag::m_tagStore = 0;

OsmTag::OsmTag(char const *k, char const *v, OsmTag *next)
    : ListObject(next)
{
    if (!m_tagStore)
    {
        m_tagStore= new TagStore;
    }

    m_index = m_tagStore->FindOrAdd(k, v);
}

OsmTag::~OsmTag()
{
}


void OsmWay::Resolve(IdObjectStore *store)
{
    if (!m_nodeRefs)
    {
        return;
    }

    unsigned size = m_nodeRefs->GetSize();

    assert((!m_numResolvedNodes) || (m_numResolvedNodes == size));

    m_numResolvedNodes = size;

    if (!m_resolvedNodes)
    {
        m_resolvedNodes = new OsmNode *[size];
    }

    IdObject *o = m_nodeRefs;
    bool resolvedAll = true;
    for (unsigned i = 0; i < size; i++)
    {
        m_resolvedNodes[i] = (OsmNode *)store->GetObject(o->m_id);
        o = (IdObject *)o->m_next;

        if (!m_resolvedNodes[i])
            resolvedAll = false;
    }

    if (resolvedAll)
    {
        m_nodeRefs->DestroyList();
       m_nodeRefs = NULL;
    }

}

void OsmRelation::Resolve(IdObjectStore *nodeStore, IdObjectStore *wayStore)
{
    OsmWay::Resolve(nodeStore);

    if (!m_wayRefs)
    {
        return;
    }

    unsigned size = m_wayRefs->GetSize();

    assert((!m_numResolvedWays) || (m_numResolvedWays == size));

    m_numResolvedWays = size;

    if (!m_resolvedWays)
    {
        m_resolvedWays = new OsmWay *[size];
    }

    IdObject *o = m_wayRefs;
    bool resolvedAll = true;
    for (unsigned i = 0; i < size; i++)
    {
        m_resolvedWays[i] = (OsmWay *)wayStore->GetObject(o->m_id);
        o = (IdObject *)o->m_next;

        if (!m_resolvedWays[i])
        {
            resolvedAll = false;
        }
    }

    if (resolvedAll)
    {
        m_wayRefs->DestroyList();
        m_wayRefs = NULL;
    }

}

IdObjectStore::IdObjectStore(unsigned bitmaskSize)
{
    m_size = 1 << bitmaskSize;
    m_mask = 0;

    for (unsigned i = 0; i < bitmaskSize; i++)
        m_mask |= 1 << i;

    m_content = NULL;
    m_locator = new ObjectList *[m_size];

    memset(m_locator, 0, sizeof(ObjectList *) * m_size);
    
}


IdObjectStore::~IdObjectStore()
{
    for (int i = 0; i < m_size; i++)
    {
        if (m_locator[i])
            delete m_locator[i];
    }

    delete [] m_locator;
}

void IdObjectStore::AddObject(IdObject *o)
{
    if (!o)
        return;

//    o->m_size = m_content ? m_content->m_size + 1 : 1;

    o->m_next = m_content;
    m_content = o;

    unsigned key = o->m_id & m_mask;

    m_locator[key] = new ObjectList(o, m_locator[key]);
    
}

IdObject *IdObjectStore::GetObject(unsigned id)
{
    unsigned key = id & m_mask;

    ObjectList *o = m_locator[key];

    while (o)
    {
        if (o->m_object->m_id == id)
        {
            return o->m_object;
        }
        o = o->m_next;
    }

    return NULL;
}


OsmData::OsmData()
    : m_nodes(20), m_ways(16), m_relations(16)
{
    m_minlat = m_maxlat = m_minlon = m_maxlon = 0;
    m_parsingState = PARSE_TOPLEVEL;
    m_elementCount = 0;
}

void OsmData::StartNode(unsigned id, double lat, double lon)
{
    assert(m_parsingState == PARSE_TOPLEVEL);

    m_parsingState = PARSE_NODE;

    OsmNode *node = new OsmNode(id, lat, lon);


    if (!m_nodes.m_content)
    {
        m_minlat = m_maxlat = lat;
        m_minlon = m_maxlon = lon;
    }
    else
    {
        if (lat < m_minlat)
            m_minlat = lat;
        else if (lat > m_maxlat)
            m_maxlat = lat;

        if (lon < m_minlon)
            m_minlon = lon;
        else if (lon > m_maxlon)
            m_maxlon = lon;
    }

    m_nodes.AddObject(node);
    m_elementCount++;
}

void OsmData::EndNode()
{
    assert(m_parsingState == PARSE_NODE);

    m_parsingState = PARSE_TOPLEVEL;
}

void OsmData::StartWay(unsigned id)
{
    assert(m_parsingState == PARSE_TOPLEVEL);

    m_parsingState = PARSE_WAY;

    OsmWay *way = new OsmWay(id);

    m_ways.AddObject(way);
    m_elementCount++;
}

void OsmData::EndWay()
{
    static_cast<OsmWay *>(m_ways.m_content)->Resolve(&m_nodes);

    assert(m_parsingState == PARSE_WAY);

    m_parsingState = PARSE_TOPLEVEL;
}

void OsmData::StartRelation(unsigned id)
{
    assert(m_parsingState == PARSE_TOPLEVEL);

    m_parsingState = PARSE_RELATION;

    OsmRelation *rel = new OsmRelation(id);

    m_relations.AddObject(rel);
    m_elementCount++;
}

void OsmData::EndRelation()
{
    static_cast<OsmRelation *>(m_relations.m_content)->Resolve(&m_nodes, &m_ways);
    assert(m_parsingState == PARSE_RELATION);

    m_parsingState = PARSE_TOPLEVEL;
}


void OsmData::AddNodeRef(unsigned id)
{
    switch (m_parsingState)
    {
        default:
            abort();
            break;
        case PARSE_WAY:
            ((OsmWay *)m_ways.m_content)->AddNodeRef(id);
            break;
        case PARSE_RELATION:
            ((OsmRelation *)m_relations.m_content)->AddNodeRef(id);
            break;
    }
}

void OsmData::AddWayRef(unsigned id)
{
    assert(m_parsingState == PARSE_RELATION);

    ((OsmRelation *)(m_relations.m_content))->AddWayRef(id);
}

void OsmData::AddTag(char const *key, char const *value)
{
    switch(m_parsingState)
    {
        default:
            abort();
            break;
        case PARSE_NODE:
            static_cast<IdObjectWithTags *>(m_nodes.m_content)->AddTag(key, value);
            break;
        case PARSE_WAY:
            static_cast<IdObjectWithTags *>(m_ways.m_content)->AddTag(key, value);
            break;
        case PARSE_RELATION:
            static_cast<IdObjectWithTags *>(m_relations.m_content)->AddTag(key, value);
            break;
    }
}

void OsmData::Resolve()
{
    for (OsmWay *w = static_cast<OsmWay *>(m_ways.m_content); w; w = static_cast<OsmWay *>(w->m_next))
    {
        w->Resolve(&m_nodes);
    }
    
    for (OsmRelation *r = static_cast<OsmRelation *>(m_relations.m_content); r; r = static_cast<OsmRelation *>(r->m_next))
    {
        r->Resolve(&m_nodes, &m_ways);
    }
}
