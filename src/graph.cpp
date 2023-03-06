#include "graph.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <limits>
#include <algorithm>
#include "hash.h"

Graph::Graph()
{

}

Graph::Itterator Graph::begin()
{
    unsigned i = 0;
    for (; i < _adjList.size(); i++)
    {
        if (_adjList[i].used)
            break;
    }
    return Itterator(i, &_adjList);
}

Graph::Itterator Graph::end()
{
    return Itterator(_adjList.size(), &_adjList);
}

unsigned Graph::newNode()
{
    unsigned address;
    if (_reuse.size())
    {
        address = _reuse.front();
        _adjList[address] = NODE(address);
        _reuse.pop();
    }
    else
    {
        address = _adjList.size();
        _adjList.emplace_back(address);
        if (_adjList.size() == 1)
        {
            _adjList[0].x = 0;
            _adjList[0].y = 0;
            _adjList[0].angle = 270.0f;
            _adjList[0].placed = true;
        }
    }
    _numNodes++;
    return address;  
}

void Graph::removeNode(unsigned node)
{
    if (node >= _adjList.size())
        return;
    _reuse.push(node);
    for (unsigned i = 0; i < MAX_NODE_VERTICES; i++)
    {
        int nn = _adjList[node].connections[i];
        if (nn != -1)
            disconnect(node, nn);
    }
    _adjList[node].used = false;
    _numNodes--;
}

void Graph::connect(unsigned n1, unsigned n2, unsigned v1, unsigned v2, unsigned w1, unsigned w2)
{
    if (!_valid(n1) || !_valid(n2) || n1 == n2 || v1 >= MAX_NODE_VERTICES || v2 >= MAX_NODE_VERTICES)
        return;

    node_t& nn1 = _adjList[n1];
    node_t& nn2 = _adjList[n2];
    
    int nn = nn1.connections[v1];
    if (nn != -1)
        disconnect(n1, nn);
    nn1.connections[v1] = n2;
    nn1.weights[v1] = w1;

    nn = nn2.connections[v2];
    if (nn != -1)
        disconnect(n1, nn);
    nn2.connections[v2] = n1;
    nn2.weights[v2] = w2;

    _place(n1);
    _place(n2);
}

void Graph::disconnect(unsigned n1, unsigned n2)
{
    if (!_valid(n1) || !_valid(n2))
        return;
    
    node_t& nn1 = _adjList[n1];
    node_t& nn2 = _adjList[n2];
    unsigned n1c = 0;
    unsigned n2c = 0;

    for (unsigned i = 0; i < MAX_NODE_VERTICES; i++)
    {
        if (nn1.connections[i] == n2)
            nn1.connections[i] = -1;
        if (nn1.connections[i] != -1)
            n1c++;
        if (nn2.connections[i] == n1)
            nn2.connections[i] = -1;
        if (nn2.connections[i] != -1)
            n2c++;
    }
    
    if (n1 != 0 && n1c == 0 && n1 != 0)
        nn1.placed = false;
    if (n2 != 0 && n2c == 0 && n2 != 0)
        nn2.placed = false;
}

int Graph::getNode(unsigned node, unsigned vert)
{
    if (!_valid(node) || vert >= MAX_NODE_VERTICES)
        return -1;
    if (_adjList[node].connections[vert] == -1)
        return _getNode(node, vert);
    return _adjList[node].connections[vert];
}

unsigned Graph::numNodes()
{
    return _numNodes;
}

float2 Graph::position(unsigned node)
{
    if (_valid(node))
        return { _adjList[node].x, _adjList[node].y };
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
    if (!_valid(start, true) || !_valid(end, true))
        return p;
    
    if (start == end)
    {
        p.push_back(start);
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
            if (!_valid(sucN, true))
                continue;
            astar_t sucA = ASTAR_DATA(sucN, qnn->g + q.weights[successor], _distance(suc, g));

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

void Graph::_place(unsigned node)
{
    if (!_valid(node))
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
        break;
    }

    for (unsigned i = 0; i < MAX_NODE_VERTICES; i++)
        _place(n.connections[i]);
}

bool Graph::_valid(unsigned node, bool placed)
{
    bool v = node <= _adjList.size() && _adjList[node].used;
    if (placed)
        return v && _adjList[node].placed;
    return v;
}

float Graph::_distance(const node_t& node1, const node_t& node2)
{
    return sqrtf(powf(node2.x - node1.x, 2) + powf(node2.y - node1.y, 2));
}

void Graph::_calcOffset(const node_t& node, unsigned vert, float& x, float& y)
{
    float angle;
    switch (vert)
    {
        case 0:
            angle = 180.0f;
            break;

        case 1:
            angle = 60.0f;
            break;
        
        case 2:
            angle = -60.0f;
            break;
        
        default:
            angle = 0;
    }

    angle += node.angle;
    x = node.x + 2 * cosf(M_PI * angle / 180.0f);
    y = node.y + 2 * sinf(M_PI * angle / 180.0f);
}

int Graph::_getNode(unsigned node, unsigned vert)
{
    for (unsigned i = 0; i < _adjList.size(); i++)
    {
        if (_valid(i, true) && i != node)
        {
            if (_distance(_adjList[i], _adjList[node]) < 0.1f)
                return i;
        }
    }
    return -1;
}
