#pragma once

#include <vector>
#include <list>


template <class V>
class Hash
{
    public:
        typedef struct NODE
        {
            unsigned key;
            V value;

            NODE()
            {

            }

            NODE(unsigned key, const V& value) : key(key), value(value)
            {

            }
        } node_t;

        class Itterator
        {
            public:
                Itterator(unsigned idx, std::vector<node_t*>* table) : _idx(idx), _table(table)
                {

                }

                V& operator*()
                {
                    return (*_table)[_idx]->value;
                }

                Itterator& operator++()
                {
                    while (_idx < _table->size())
                    {
                        _idx++;
                        if ((*_table)[_idx])
                            break;
                    }
                    return *this;
                }

                Itterator operator++(int)
                {
                    unsigned idx = _idx;
                    while (idx < _table->size())
                    {
                        idx++;
                        if ((*_table)[idx])
                            break;
                    }
                    return Itterator(idx, _table);
                }

                bool operator!=(const Itterator& other)
                {
                    return _idx != other._idx;
                }
            
            private:
                unsigned _idx;
                std::vector<node_t*>* _table;
        };

        Hash() : _size(0)
        {
            _table = new std::vector<node_t*>(8);
        }

        ~Hash()
        {
            for (unsigned i = 0; i < (*_table).size(); i++)
            {
                if ((*_table)[i])
                    delete (*_table)[i];
            }
            delete _table;
        }

        V& insert(unsigned key, const V& value)
        {
            if (_size > _table->size() * 0.7f)
            {
                std::vector<node_t*>* newTable = new std::vector<node_t*>(_table->size() * 2);
                for (node_t* node : *_table)
                {
                    if (node)
                    {
                        unsigned idx = node->key % newTable->size();
                        while (true)
                        {
                            if (!(*newTable)[idx])
                            {
                                (*newTable)[idx] = node;
                                break;
                            }
                            idx = (idx + 1) % newTable->size();
                        }
                    }
                }

                delete _table;
                _table = newTable;
            }

            unsigned idx = key % _table->size();
            while (true)
            {
                if (!(*_table)[idx])
                {
                    (*_table)[idx] = new NODE(key, value);
                    _size++;
                    return (*_table)[idx]->value;
                    
                }
                idx = (idx + 1) % _table->size();
            }
        }

        void remove(unsigned key)
        {
            unsigned idx = _contains(key);
            if (idx != (unsigned) -1)
            {
                delete (*_table)[idx];
                (*_table)[idx] = NULL;
                _size--;
            }   
        }

        V& operator[](unsigned key)
        {
            unsigned idx = _contains(key);
            if (idx != (unsigned) -1)
                return (*_table)[idx]->value;
            return insert(key, V());
        }

        bool contains(unsigned key)
        {
            return _contains(key) != (unsigned) -1;
        }

        V* at(unsigned key)
        {
            unsigned idx = _contains(key);
            if (idx != (unsigned) -1)
                return &(*_table)[idx]->value;
            return NULL;
        }

        unsigned size()
        {
            return _size;
        }

        Itterator begin()
        {
            unsigned i = 0;
            for (; i < _table->size(); i++)
            {
                if ((*_table)[i])
                    break;
            }
            return Itterator(i, _table);
        }

        Itterator end()
        {
            return Itterator(_table->size(), _table);
        }

    private:
        std::vector<node_t*>* _table;
        unsigned _size;

        unsigned _contains(unsigned key)
        {
            unsigned start = key % _table->size();
            unsigned idx = start;
            do 
            {
                if ((*_table)[idx] && (*_table)[idx]->key == key)
                    return idx;
                idx = (idx + 1) % _table->size();
            } 
            while (start != idx);
            return -1;
        }
};
