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
        int getNode(unsigned node, unsigned vert);
        unsigned numNodes();
        std::vector<unsigned> path(unsigned start, unsigned end);
    
    private:
        typedef struct NODE
        {
            bool used, placed;
            float x, y, angle;
            int connections[MAX_NODE_VERTICES];
            unsigned weights[MAX_NODE_VERTICES];

            NODE() : used(false), placed(false)
            {

            }

            NODE(unsigned address) : used(true), placed(false)
            {
                for (unsigned i = 0; i < MAX_NODE_VERTICES; i++)
                    connections[i] = -1;

            }
        } node_t;

        std::vector<node_t> _adjList;
        std::queue<unsigned> _reuse;
        unsigned _numNodes;

        void _place(unsigned node);
        bool _valid(unsigned node, bool placed=false);
        float _distance(const node_t& node1, const node_t& node2);
        void _calcOffset(const node_t& node, unsigned vert, float& x, float& y);
        int _getNode(unsigned node, unsigned vert);
};