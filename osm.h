#ifndef __OSMDATA_H__
#define __OSMDATA_H__

#include <stdlib.h>

class ListObject
{
    public:
      ListObject(ListObject *next = NULL)
      {
        m_next = next;

        m_size = m_next ? m_next->m_size + 1 : 1;
      }
      
      virtual ~ListObject()
      {
      }
      
      void DestroyList()
      {
        if (m_next)
        {
            m_next->DestroyList();
        }
        delete this;
      }

      unsigned GetSize()
      {
        return m_size;
      }

      ListObject *m_next;
      unsigned m_size;

};

class OsmTag
    : public ListObject
{
    public:
    OsmTag(char const *key, char const *value, OsmTag *next = NULL);
    ~OsmTag();
    char *m_key;
    char *m_value;
};

class IdObject
    : public ListObject
{
    public:
        IdObject(unsigned id = 0, IdObject *next = NULL)
            : ListObject(next)
        {
            m_id = id;
        }
            
        unsigned m_id;
};



class IdObjectStore
{
    private:
        class ObjectList
        {
            public:
            ObjectList(IdObject *object, ObjectList *tail)
            {
                m_next = tail;
                m_object = object;
            }

            ~ObjectList()
            {
                if (m_next)
                    delete m_next;
                    
                delete m_object;
            }
            
            ObjectList *m_next;
            IdObject *m_object;
        };
    public:
        IdObjectStore(unsigned bitmaskSize);
        ~IdObjectStore();

        ObjectList **m_locator;
        IdObject *m_content;
        int m_size;
        unsigned m_mask;

        void AddObject(IdObject *object);
        IdObject *GetObject(unsigned id);
};


class IdObjectWithTags
    : public IdObject
{
    public:
        IdObjectWithTags(unsigned id = 0, IdObjectWithTags *next = NULL)
            : IdObject(id, next)
        {
            m_tags = NULL;
        }
        
        ~IdObjectWithTags()
        {
            if (m_tags)
            {
                m_tags->DestroyList();
            }
        }

        void AddTag(char const *key, char const *value)
        {
            m_tags = new OsmTag(key, value, m_tags);
        }

        OsmTag *m_tags;
};

class OsmNode
    : public IdObjectWithTags
{
    public:

    OsmNode(unsigned id, double lat, double lon, OsmNode *next = NULL)
        : IdObjectWithTags(id, next)
    {
        m_lat = lat;
        m_lon = lon;
    }
    
    double m_lat;
    double m_lon;

};


class OsmWay
    : public IdObjectWithTags
{
    public:

    OsmWay(unsigned id, OsmWay *next = NULL)
        : IdObjectWithTags(id, next)
    {
        m_nodeRefs = NULL;
        m_resolvedNodes = NULL;
        m_numResolvedNodes = 0;
    }

    ~OsmWay()
    {
        if (m_nodeRefs)
        {
            m_nodeRefs->DestroyList();
        }

        if (m_resolvedNodes)
        {
            delete [] m_resolvedNodes;
        }
    }
    

    void AddNodeRef(unsigned id)
    {
        m_nodeRefs = new IdObject(id, m_nodeRefs);
    }

    IdObject *m_nodeRefs;

    void Resolve(IdObjectStore *store);
    // these are only valid after calling resolve
    OsmNode **m_resolvedNodes;
    unsigned m_numResolvedNodes;
};


class OsmRelation
    : public OsmWay
{
    public:
    OsmRelation(unsigned id, OsmRelation *next = NULL)
        : OsmWay(id, next)
    {
        m_wayRefs = NULL;
        m_resolvedWays = NULL;
        m_numResolvedWays = 0;
    }
    
    ~OsmRelation()
    {
        if (m_wayRefs)
        {
            m_wayRefs->DestroyList();
        }

        if (m_resolvedWays)
        {
            delete [] m_resolvedWays;
        }
    }
    
    IdObject *m_wayRefs;
    void AddWayRef(unsigned id)
    {
        m_wayRefs = new IdObject(id, m_wayRefs);
    }
    
    void Resolve(IdObjectStore *nodeStore, IdObjectStore *wayStore);

    OsmWay **m_resolvedWays;
    unsigned m_numResolvedWays;
    
};


class OsmData
{
    public:
    OsmData();

    IdObjectStore m_nodes;
    IdObjectStore m_ways;
    IdObjectStore m_relations;
    

    // bounding box;
    double m_minlat, m_maxlat, m_minlon, m_maxlon;

    // parsing stuff
    void StartNode(unsigned id, double lat, double lon);
    void EndNode();
    void StartWay(unsigned id);
    void EndWay();
    void StartRelation(unsigned id);
    void EndRelation();

    void AddNodeRef(unsigned id);
    void AddWayRef(unsigned id);

    void AddTag(char const *k, char const *v);

    typedef enum
    {
        PARSE_TOPLEVEL,
        PARSE_NODE,
        PARSE_WAY,
        PARSE_RELATION
    } PARSINGSTATE;

    PARSINGSTATE m_parsingState;

    void Resolve();
    
};



#endif
