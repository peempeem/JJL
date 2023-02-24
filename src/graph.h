#pragma once

#include <vector>
#include <list>
#include <queue>

#define MAX_NODE_VERTICES 3

class Graph
{
    public:
        Graph();

        unsigned newNode();
        void removeNode(unsigned node);
        void connect(unsigned n1, unsigned n2, unsigned v1, unsigned v2, unsigned w1=1, unsigned w2=1);
        void disconnect(unsigned n1, unsigned n2);
        std::vector<unsigned> path(unsigned start, unsigned end);
    
    private:
        typedef struct NODE
        {
            bool used, placed;
            unsigned address;
            float x, y, angle;
            int connections[MAX_NODE_VERTICES];
            unsigned weights[MAX_NODE_VERTICES];

            NODE() : used(false), placed(false)
            {

            }

            NODE(unsigned address) : used(true), address(address), placed(false)
            {
                for (unsigned i = 0; i < MAX_NODE_VERTICES; i++)
                    connections[i] = -1;

            }
        } node_t;

        std::vector<node_t> _adjList;
        std::queue<unsigned> _reuse;

        void place(unsigned node);
        bool valid(unsigned node, bool placed=false);
        float distance(node_t node1, node_t node2);
};