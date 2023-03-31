#pragma once

#include <vector>
#include <list>

namespace JJL
{
    template<class V>
    class Hash
    {
        public:
            struct Node
            {
                unsigned key;
                V value;

                Node();
                Node(unsigned key, const V& value);
            };

            class Itterator
            {
                public:
                    Itterator(unsigned idx, std::vector<Node*>* table);

                    V& operator*();
                    Itterator& operator++();
                    Itterator operator++(int);
                    bool operator!=(const Itterator& other);
                
                private:
                    unsigned _idx;
                    std::vector<Node*>* _table;
            };

            Hash(float resize=0.7f);
            ~Hash();

            V& insert(unsigned key, const V& value);
            void remove(unsigned key);

            V& operator[](unsigned key);
            V* at(unsigned key);

            bool contains(unsigned key);
            unsigned size();

            Itterator begin();
            Itterator end();

        private:
            std::vector<Node*>* _table;
            bool* _probe;
            unsigned _size;
            float _load;
            unsigned _prime;

            V& _insert(unsigned key, const V& value);
            void _resize();
            unsigned _contains(unsigned key);
            unsigned _hash1(unsigned key);
            unsigned _hash2(unsigned key);
    };

    #include "hash.hpp"
}
