#ifndef __OSMDATA_H__
#define __OSMDATA_H__

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
//#include <stdio.h>
#include <wx/hashmap.h>


class DRect
{
    public:
        DRect()
        {
            m_x = m_y = m_h = 0;
            m_w = -1;
        }
        
        DRect(double minX, double minY, double maxX = -1, double maxY = 0)
        {
            m_x = minX;
            m_y = minY;
            m_w = maxX - minX;
            m_h = maxY - minY;
        }

        bool IsEmpty()
        {
            return m_w < 0;
        }

        void MakeEmpty()
        {
            m_x = m_y = m_h = 0;
            m_w = -1;
        }

        void Include(double x, double y)
        {
            if (m_w < 0)
            {
                m_x = x;
                m_y = y;
                m_w = m_h = 0;
            }
            else
            {
                if (x < m_x)
                {
                    m_w += m_x - x;
                    m_x = x;
                }
                else if (x > m_x + m_w)
                {
                    m_w = x - m_x;
                }

                if (y < m_y)
                {
                    m_h += m_y - y;
                    m_y = y;
                }
                else if (y > m_y + m_h)
                {
                    m_h = y - m_y;
                }
            }
        }

        double Right() const
        {
            return m_x + m_w;
        }

        void SetRight(double x)
        {
            m_w = x - m_x;
        }

        double Top() const
        {
            return m_y + m_h;
        }

        void SetTop(double y)
        {
            m_h = y - m_y;
        }

        void SetSize(double w, double h)
        {
            m_w = w;
            m_h = h;
        }

        double m_x, m_y;
        double m_w, m_h;

        DRect Add(DRect const &other)
        {
            DRect ret = *this;
            if (other.m_w < 0)
            {
                return ret;
            }
            
            ret.Include(other.m_x, other.m_y);
            ret.Include(other.Right(), other.Top());
            return ret;
        }
        
        DRect InterSect(DRect const &other)
        {
            DRect ret = *this;
            if (m_w < 0 || other.m_w < 0)
            {
                ret.MakeEmpty();
                return ret;;
            }
        
            double r = Right();
            double ro = other.Right();
            double t = Top();
            double to = other.Top();

            if (ro < ret.m_x || to < ret.m_y || other.m_x > r || other.m_y > t)
            {
                ret.MakeEmpty();
            }
            else
            {
                ret.m_x = ret.m_x > other.m_x ? ret.m_x : other.m_x;
                ret.m_y = ret.m_y > other.m_y ? ret.m_y : other.m_y;

                ret.SetRight(r < ro ? r : ro);
                ret.SetTop(t < to ? t : to);
            }
            return ret;
        }


        bool Contains(double x, double y)
        {
            if (m_w < 0)
            {
                return false;
            }
            return (x >= m_x && x < m_x + m_w && y >= m_y && y < m_y + m_h);
        }

        bool OverLaps(DRect const &other)
        {
            
            return !(InterSect(other).IsEmpty());
        }
        
};


class ListObject
{
    public:
      ListObject(ListObject *next = NULL)
      {
        m_next = next;

//        m_size = m_next ? m_next->m_size + 1 : 1;
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
        if (!m_next)
            return 1;

        return m_next->GetSize() + 1;
        //return m_size;
      }

      ListObject *m_next;
//      unsigned m_size;

};


class TagIndex
{
    public:
        static TagIndex Create(unsigned k, unsigned v = 0)
        {
            TagIndex ret;
            assert(k >=0);
            assert(k < 0xFFFFFFFF - 1);
            assert(v >=0);
            assert(v < 0xFFFFFFFF - 1);
            ret.m_keyIndex =  k;
            ret.m_valueIndex = v;

            return ret;
        }
        
        unsigned m_keyIndex;
        unsigned m_valueIndex;

        static unsigned MaxNumKeys() {return 0xFFFFFFFF -1;}
        static unsigned MaxNumValues() {return 0xFFFFFFFF - 1; }

        static TagIndex CreateInvalid()
        {
            TagIndex t;
            t.m_keyIndex = t.m_valueIndex = 0xFFFFFFFF;

            return t;

        }

        bool Valid()
        {

            return !(m_keyIndex == 0xFFFFFFFF && m_valueIndex == 0xFFFFFFFF);
        }

        // a tagIndex whose value-part = 0 will match only the key part
        bool operator==(const TagIndex &other)
        {
            return (m_keyIndex == other.m_keyIndex) && (!m_valueIndex || !(other.m_valueIndex) || (m_valueIndex == other.m_valueIndex));
        }

};

WX_DECLARE_STRING_HASH_MAP( unsigned , StringToIndexMapper );


class TagStore
{
    public:
    TagStore()
    {
        m_keys = NULL;
        m_numKeys = 0;
        m_maxNumKeys = 0;

        m_maxNumValues = m_numValues = NULL;
        m_values = NULL;
        m_valueMappers  = NULL;

        GrowKeys(1024);
        
    }

    unsigned GetNumKeys()
    {
        return m_numKeys;
    }
    
    char const *GetKey(unsigned keyIndex)
    {
        assert(keyIndex >=0);
        assert(keyIndex < m_numKeys);
        return m_keys[keyIndex];
    }

    unsigned GetNumValues(unsigned keyIndex)
    {
        assert(keyIndex >=0);
        assert(keyIndex  < m_numKeys);
        
        return m_numValues[keyIndex];
    }

    char const *GetValue(unsigned keyIndex, unsigned valueIndex)
    {
        assert(keyIndex >=0);
        assert(keyIndex  < m_numKeys);
        assert(valueIndex >=0);
        assert(valueIndex < m_numValues[keyIndex]);

        return m_values[keyIndex][valueIndex];
    }

    void GrowKeys(unsigned amount)
    {
        char **newKeys = new char *[m_maxNumKeys + amount];
        char ***newValues = new char **[m_maxNumKeys + amount];
        unsigned *newNumValues =  new unsigned[m_maxNumKeys + amount];
        unsigned *newMaxNumValues = new unsigned[m_maxNumKeys + amount];
        StringToIndexMapper **newMappers = new StringToIndexMapper *[m_maxNumKeys + amount];

        for (unsigned i = 0; i < m_maxNumKeys; i++)
        {
            newKeys[i] = m_keys[i];
            newValues[i] = m_values[i];
            newNumValues[i] = m_numValues[i];
            newMaxNumValues[i] = m_maxNumValues[i];
            newMappers[i] = m_valueMappers[i];
        }

        for (unsigned i = 0; i < amount; i++)
        {
            newKeys[i + m_maxNumKeys] = NULL;
            newValues[i + m_maxNumKeys] = NULL;
            newMaxNumValues[i + m_maxNumKeys] = 0;
            newNumValues[i + m_maxNumKeys] = 0;
            newMappers[i + m_maxNumKeys] = NULL;
        }


        delete [] m_keys;
        m_keys = newKeys;

        delete [] m_values;
        m_values = newValues;

        delete [] m_numValues;
        m_numValues = newNumValues;
        
        delete [] m_maxNumValues;
        m_maxNumValues = newMaxNumValues;

        delete m_valueMappers;
        m_valueMappers = newMappers;

        m_maxNumKeys += amount;
    }


    void GrowValues(unsigned  key)
    {
        unsigned  amount = m_maxNumValues[key];

        if (!amount)
            amount = 1;


        char **newValues = new char *[m_maxNumValues[key] + amount];

        for (unsigned i = 0; i < m_maxNumValues[key]; i++)
            newValues[i] = m_values[key][i];

        for (unsigned i = 0; i < amount; i++)
            newValues[i + m_maxNumValues[key]] = NULL;
            
        delete m_values[key];

        m_values[key] = newValues;
        m_maxNumValues[key] += amount;
        

    }
    
    ~TagStore()
    {
        for (unsigned i = 0; i < m_numKeys; i++)
        {
            free(m_keys[i]);
            delete m_valueMappers[i];
            for (unsigned j = 0; j < m_numValues[i]; j++)
            {
                free(m_values[i][j]);
            }
            delete [] m_values[i];
        }
        
        delete [] m_keys;
        delete [] m_values;
        delete [] m_maxNumValues;
        delete [] m_numValues;
    }

    TagIndex FindOrAdd(char const *key, char const *value)
    {
        TagIndex t = Find(key, value);

        if (t.Valid())
            return t;


        unsigned k = 0;

        if (!FindKey(key, &k))
        {
            k = AddKey(key);
        }

        if (!value)
        {
            return TagIndex::Create(k);
        }

        unsigned v = 0;


        if (!FindValue(k, value, &v));
        {
            v = AddValue(k, value);
        }
        
        return TagIndex::Create(k, v + 1);
    }

    TagIndex Find(char const *key, char const *value)
    {
        unsigned k = 0;


        if (!FindKey(key, &k))
            return TagIndex::CreateInvalid();

        if (!value)
        {
            return TagIndex::Create(k);
        }

        unsigned v = 0;

        if (!FindValue(k, value, &v))
            return TagIndex::CreateInvalid();

        return TagIndex::Create(k, v + 1);
    }

    char const *GetKey(TagIndex index)
    {
        assert(index.m_keyIndex >=0 && index.m_keyIndex < m_numKeys);
        
        return m_keys[index.m_keyIndex];
    }
    
    char const *GetValue(TagIndex index)
    {
        assert(index.m_keyIndex >=0 && index.m_keyIndex < m_numKeys);
        assert(index.m_valueIndex >= 1 && index.m_valueIndex - 1 < m_numValues[index.m_keyIndex]);

        return m_values[index.m_keyIndex][index.m_valueIndex - 1];
    }

    private:
    bool FindKey(char const *key, unsigned *k)
    {
        StringToIndexMapper::iterator f = m_keyMapper.find(wxString(key, wxConvUTF8));

        if (f == m_keyMapper.end())
            return false;

        *k = f->second;

        return true;
    }

    
    bool FindValue(unsigned key, char const *value, unsigned *v)
    {

        if (m_valueMappers[key])
        {
            StringToIndexMapper::iterator f = m_valueMappers[key]->find(wxString(value, wxConvUTF8));

            if (f == m_valueMappers[key]->end())
                return false;
        
            *v = f->second;
        
            return true;
        
        }
    
        for (unsigned i = 0; i < m_numValues[key]; i++)
        {
            if (!strcmp(value, m_values[key][i]))
            {
                *v = i;
                return true;
            }
        }

        return false;
    }

    unsigned AddKey(char const *k)
    {
        assert(m_numKeys < TagIndex::MaxNumKeys());

        if (m_numKeys >= m_maxNumKeys)
            GrowKeys(m_maxNumKeys ? m_maxNumKeys : 1024);

        m_keys[m_numKeys] = strdup(k);
        assert(m_keys[m_numKeys]);

        m_keyMapper.insert(StringToIndexMapper::value_type(wxString(k, wxConvUTF8), m_numKeys));

        m_numKeys++;

        return m_numKeys -1;
    }
    
    unsigned AddValue(unsigned key, char const *value)
    {
//        if (m_numValues[key] >= TagIndex::MaxNumValues())
//        {
//          printf("key = %d (%s) numv %d (%s)!\n", key, m_keys[key], m_numValues[key], value);
//        }
    
        assert(m_numValues[key] < TagIndex::MaxNumValues());

        if (m_numValues[key] >= m_maxNumValues[key])
            GrowValues(key);

        m_values[key][m_numValues[key]] = strdup(value);
        assert(m_values[key][m_numValues[key]]);


        if (m_numValues[key] > 10)
        {
            if (!m_valueMappers[key])
            {
                m_valueMappers[key] = new StringToIndexMapper;
            }
            m_valueMappers[key]->insert(StringToIndexMapper::value_type(wxString(value, wxConvUTF8), m_numValues[key]));
        }


        m_numValues[key]++;
        
        return m_numValues[key] - 1;
    }

    


    char **m_keys;
    unsigned m_numKeys;
    unsigned m_maxNumKeys;

    char ***m_values;
    unsigned *m_numValues;
    unsigned *m_maxNumValues;

    StringToIndexMapper **m_valueMappers;
    StringToIndexMapper m_keyMapper;
    

    
};

class OsmTag
    : public ListObject
{
    public:
    OsmTag(char const *key, char const *value = NULL, OsmTag *next = NULL);
    OsmTag(bool noCreate, char const *key, char const *value = NULL, OsmTag *next = NULL);
    OsmTag(OsmTag const &other);
    ~OsmTag();


    static bool KeyExists(char const *key);

    bool Valid()
    {
        return m_index.Valid();
    }

    TagIndex Index() { return m_index; }

    char const *GetKey()
    {
        return m_tagStore->GetKey(m_index);
    }

    char const *GetValue()
    {
        return m_tagStore->GetValue(m_index);
    }

    bool HasTag(OsmTag const &other)
    {
        if (m_index == other.m_index)
        {
            return true;
        }
        if (!m_next)
            return false;

        return static_cast<OsmTag *>(m_next)->HasTag(other);

    }

    static TagStore *m_tagStore;
    TagIndex m_index;
    
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
                m_listSize = tail ? tail->m_listSize + 1 : 1;
            }

            ~ObjectList()
            {
                if (m_next)
                    delete m_next;
                    
                delete m_object;
            }
            unsigned m_listSize;
            ObjectList *m_next;
            IdObject *m_object;
        };
    public:
        IdObjectStore(unsigned bitmaskSize);
        ~IdObjectStore();

        void GetStatistics(double *avgLen, double *standardDeviation, int *maxlen)
        {
            unsigned long long s1 = 0, s2 = 0;
            *maxlen = 0;
            int c;
            for (int i = 0; i < m_size; i++)
            {
                c = m_locator[i] ? m_locator[i]->m_listSize : 0;
                s1 += c;
                s2 += c * c;
                if (c > *maxlen)
                {
                    *maxlen = c;
                }
            }

            *avgLen = static_cast<double>(s1) / m_size;
            *standardDeviation = sqrt(static_cast<double>(s2 * m_size - s1 * s1))/ m_size;
        }

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

        bool HasTag(OsmTag const &tag)
        {
            return m_tags ? m_tags->HasTag(tag) : false;
        }

        bool HasTag(char const *key, char const *value = NULL)
        {
            OsmTag t(key, value);
            return m_tags ? m_tags->HasTag(t) : false;
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

    DRect GetBB()
    {
        DRect ret;
        for (unsigned i = 0; i < m_numResolvedNodes; i++)
        {
            if (m_resolvedNodes[i])
            {
                ret.Include(m_resolvedNodes[i]->m_lon, m_resolvedNodes[i]->m_lat);
            }
        }
        return ret;
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


    DRect GetBB()
    {
        DRect ret = OsmWay::GetBB();
        for (unsigned i = 0; i < m_numResolvedWays; i++)
        {
            if (m_resolvedWays[i])
            {
                ret.Add(m_resolvedWays[i]->GetBB());
            }
        }
        return ret;
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
    unsigned m_elementCount;
};



#endif
