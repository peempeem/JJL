#pragma once

#include <vector>
#include <list>
#include <queue>

namespace JJL
{
    #define MAX_NODE_VERTICES 3

    typedef struct FLOAT2
    {
        float x, y;
    } float2;

    class Graph
    {
        public:
            struct Node
            {
                bool used, placed;
                float x, y, angle;
                int connections[MAX_NODE_VERTICES];
                unsigned weights[MAX_NODE_VERTICES];

                Node();
                Node(unsigned address);
            };

            class Itterator
            {
                public:
                    Itterator(unsigned idx, std::vector<Node>* table);

                    unsigned operator*();
                    Itterator& operator++();
                    Itterator operator++(int);
                    bool operator!=(const Itterator& other);
                
                private:
                    unsigned _idx;
                    std::vector<Node>* _table;
            };

            Graph();

            Itterator begin();
            Itterator end();

            unsigned newNode();
            void removeNode(unsigned node);
            void connect(unsigned n1, unsigned n2, unsigned v1, unsigned v2, unsigned w1=1, unsigned w2=1);
            void disconnect(unsigned n1, unsigned n2);
            int getNode(unsigned node, unsigned vert);
            unsigned numNodes();
            float2 position(unsigned node);
            std::vector<unsigned> path(unsigned start, unsigned end);
        
        private:
            std::vector<Node> _adjList;
            std::queue<unsigned> _reuse;
            unsigned _numNodes;

            void _place(unsigned node);
            bool _valid(unsigned node, bool placed=false);
            float _distance(const Node& node1, const Node& node2);
            void _calcOffset(const Node& node, unsigned vert, float& x, float& y);
            int _getNode(unsigned node, unsigned vert);
    };

}