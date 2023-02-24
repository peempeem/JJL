#include "graph.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <limits>
#include <algorithm>
#include "hash.h"

Graph::Graph()
{

}

unsigned Graph::newNode()
{
    if (_reuse.size())
    {
        unsigned address = _reuse.front();
        _adjList[address] = NODE(address);
        return address;
        _reuse.pop();
    }
    else
    {
        unsigned address = _adjList.size();
        _adjList.emplace_back(address);
        if (_adjList.size() == 1)
        {
            _adjList[0].x = 0;
            _adjList[0].y = 0;
            _adjList[0].angle = 270.0f;
            _adjList[0].placed = true;
        }
        return address;
    }       
}

void Graph::removeNode(unsigned node)
{
    if (node >= _adjList.size())
        return;
    _reuse.push(node);
    _adjList[node].used = false;
}

void Graph::connect(unsigned n1, unsigned n2, unsigned v1, unsigned v2, unsigned w1, unsigned w2)
{
    if (!valid(n1) || !valid(n2) || v1 >= MAX_NODE_VERTICES || v2 >= MAX_NODE_VERTICES)
        return;
    
    _adjList[n1].connections[v1] = n2;
    _adjList[n1].weights[v1] = w1;

    _adjList[n2].connections[v2] = n1;
    _adjList[n2].weights[v2] = w2;

    place(n1);
    place(n2);
}

typedef struct ASTAR_DATA
{
    unsigned node;
    float f, g;
    ASTAR_DATA* parent;
    
    ASTAR_DATA()
    {
        
    }

    ASTAR_DATA(unsigned node) : node(node), parent(NULL), f(0), g(0)
    {
        
    }

    ASTAR_DATA(unsigned node, float g, float h) : node(node), parent(NULL), f(h + g), g(g)
    {
        
    }
} astar_t;

std::vector<unsigned> Graph::path(unsigned start, unsigned end)
{
    std::vector<unsigned> p;
    if (!valid(start, true) || !valid(end, true))
        return p;
    
    if (start == end)
    {
        p.push_back(start);
        p.push_back(end);
        return p;
    }
    
    std::list<astar_t> aNodes;
    Hash<astar_t*> open;
    Hash<astar_t*> closed;

    aNodes.emplace_back(start);
    open.insert(start, &aNodes.back());

    node_t& g = _adjList[end];

    while (open.size())
    {
        unsigned qn;
        astar_t* qnn;
        float f = std::numeric_limits<float>::infinity();
        for (astar_t* node : open)
        {
            if (node->f < f)
            {
                qn = node->node;
                qnn = node;
                f = node->f;
            }
        }

        node_t& q = _adjList[qn];

        for (unsigned successor = 0; successor < MAX_NODE_VERTICES; successor++)
        {
            unsigned sucN = q.connections[successor];
            if (sucN == end)
            {
                p.push_back(end);
                astar_t* parent = qnn;
                while (parent)
                {
                    p.push_back(parent->node);
                    parent = parent->parent;
                }
                std::reverse(p.begin(), p.end());
                return p;
            }
            node_t& suc = _adjList[sucN];
            if (!valid(sucN, true))
                continue;
            astar_t sucA = ASTAR_DATA(sucN, qnn->g + q.weights[successor], distance(suc, g));

            astar_t** node = open.at(sucN);
            if (node && (*node)->f < sucA.f)
                continue;

            node = closed.at(sucN);
            if (node && (*node)->f < sucA.f)
                continue;
            
            sucA.parent = qnn;
            aNodes.push_back(sucA);
            open.insert(sucN, &aNodes.back());
        }

        closed.insert(qn, qnn);
        open.remove(qn);
    }

    return p;
}

void Graph::place(unsigned node)
{
    if (!valid(node))
        return;

    node_t& n = _adjList[node];
    if (n.placed)
        return;
    
    for (unsigned i = 0; i < MAX_NODE_VERTICES; i++)
    {
        if (!_adjList[n.connections[i]].placed)
            continue;

        node_t& n2 = _adjList[n.connections[i]];

        unsigned j = 0;
        for (; j < MAX_NODE_VERTICES; j++)
        {
            if (n2.connections[j] == node)
                break;
        }
        if (j == MAX_NODE_VERTICES)
            continue;
        j = MAX_NODE_VERTICES * i + j;
        
        float angle;
        switch (j)
        {
            case 0: // 0 -> 0
                angle = 180;
                break;
            
            case 1: // 0 -> 1
                angle = 60;
                break;
            
            case 2: // 0 -> 2
                angle = -60;
                break;
            
            case 3: // 1 -> 0
                angle = 60;
                break;
            
            case 4: // 1 -> 1
                angle = 180;
                break;
            
            case 5: // 1 -> 2
                angle = -60;
                break;
            
            case 6: // 2 -> 0
                angle = 60;
                break;
            
            case 7: // 2 -> 1
                angle = -60;
                break;
            
            case 8: // 2 -> 2
                angle = 180;
                break;
            
            default:
                angle = 0;
        }

        n.angle = fmodf(n2.angle + angle, 360.0f);
        n.x = n2.x + 2 * cosf(M_PI * n.angle / 180.0f);
        n.y = n2.y + 2 * sinf(M_PI * n.angle / 180.0f);
        n.placed = true;
    }
}

bool Graph::valid(unsigned node, bool placed)
{
    bool v = node <= _adjList.size() && _adjList[node].used;
    if (placed)
        return v && _adjList[node].placed;
    return v;
}

float Graph::distance(node_t node1, node_t node2)
{
    return sqrtf(powf(node2.x - node1.x, 2) + powf(node2.y - node1.y, 2));
}