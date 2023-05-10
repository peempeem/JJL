#include "graph.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <limits>
#include <algorithm>
#include "hash.h"

#define PCBX 6.475
#define PCBY 3.738
#define TRIANGLE_ANGLE 90.0f
#define TRIANGLE_RADIUS 8.3f

JJL::float2 pixelsPos[] = 
{
    { 6.475 - PCBX, 8.750 - PCBY },
    { 7.575 - PCBX, 6.875 - PCBY },
    { 5.392 - PCBX, 6.875 - PCBY },
    { 4.301 - PCBX, 5.000 - PCBY },
    { 6.475 - PCBX, 5.000 - PCBY },
    { 8.640 - PCBX, 5.000 - PCBY },
    { 9.702 - PCBX, 3.125 - PCBY },
    { 7.558 - PCBX, 3.125 - PCBY },
    { 5.392 - PCBX, 3.125 - PCBY },
    { 3.248 - PCBX, 3.125 - PCBY },
    { 2.145 - PCBX, 1.250 - PCBY },
    { 4.301 - PCBX, 1.250 - PCBY },
    { 6.475 - PCBX, 1.250 - PCBY },
    { 8.640 - PCBX, 1.250 - PCBY },
    { 10.81 - PCBX, 1.250 - PCBY }
};

JJL::Graph::Node::Node() : used(false), placed(false)
{

}

JJL::Graph::Node::Node(unsigned address) : used(true), placed(false)
{
    for (unsigned i = 0; i < MAX_NODE_VERTICES; i++)
        connections[i] = -1;
}

JJL::Graph::Itterator::Itterator(unsigned idx, std::vector<Node>* table) : _idx(idx), _table(table)
{

}

unsigned JJL::Graph::Itterator::operator*()
{
    return _idx;
}

JJL::Graph::Itterator& JJL::Graph::Itterator::operator++()
{
    while (_idx < _table->size())
    {
        _idx++;
        if ((*_table)[_idx].used)
            break;
    }
    return *this;
}

JJL::Graph::Itterator JJL::Graph::Itterator::operator++(int)
{
    unsigned idx = _idx;
    while (idx < _table->size())
    {
        idx++;
        if ((*_table)[idx].used)
            break;
    }
    return Itterator(idx, _table);
}

bool JJL::Graph::Itterator::operator!=(const Itterator& other)
{
    return _idx != other._idx;
}

JJL::Graph::Graph()
{

}

JJL::Graph::Itterator JJL::Graph::begin()
{
    unsigned i = 0;
    for (; i < _adjList.size(); i++)
    {
        if (_adjList[i].used)
            break;
    }
    return Itterator(i, &_adjList);
}

JJL::Graph::Itterator JJL::Graph::end()
{
    return Itterator(_adjList.size(), &_adjList);
}

unsigned JJL::Graph::newNode()
{
    unsigned address;
    if (_reuse.size())
    {
        address = _reuse.front();
        _adjList[address] = Node(address);
        _reuse.pop();
    }
    else
    {
        address = _adjList.size();
        _adjList.emplace_back(address);
        if (_adjList.size() == 1)
        {
            _adjList[0].pos = { 0, 0 };
            _adjList[0].angle = 270.0f;
            _adjList[0].placed = true;
        }
    }
    _numNodes++;
    return address;  
}

void JJL::Graph::removeNode(unsigned node)
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

void JJL::Graph::connect(unsigned n1, unsigned n2, unsigned v1, unsigned v2, unsigned w1, unsigned w2)
{
    if (!_valid(n1) || !_valid(n2) || n1 == n2 || v1 >= MAX_NODE_VERTICES || v2 >= MAX_NODE_VERTICES)
        return;

    Node& nn1 = _adjList[n1];
    Node& nn2 = _adjList[n2];
    
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

void JJL::Graph::disconnect(unsigned n1, unsigned n2)
{
    if (!_valid(n1) || !_valid(n2))
        return;
    
    Node& nn1 = _adjList[n1];
    Node& nn2 = _adjList[n2];
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

int JJL::Graph::getNode(unsigned node, unsigned vert)
{
    if (!_valid(node) || vert >= MAX_NODE_VERTICES)
        return -1;
    if (_adjList[node].connections[vert] == -1)
        return _getNode(node, vert);
    return _adjList[node].connections[vert];
}

unsigned JJL::Graph::numNodes()
{
    return _numNodes;
}

JJL::float2 JJL::Graph::position(unsigned node)
{
    if (_valid(node))
        return _adjList[node].pos;
    return { -1, -1 };
}

float JJL::Graph::angle(unsigned node)
{
    if (_valid(node))
        return _adjList[node].angle;
    return 0;
}

float JJL::Graph::distance(const JJL::float2& pt)
{
    return sqrtf(powf(pt.x, 2) + powf(pt.y, 2));
}

float JJL::Graph::distance(const JJL::float2& p1, const JJL::float2& p2)
{
    return sqrtf(powf(p2.x - p1.x, 2) + powf(p2.y - p1.y, 2));
}

float JJL::Graph::distance2Triangle(float distance)
{
    return distance / TRIANGLE_RADIUS;
}

JJL::float2 JJL::Graph::pixelPosition(unsigned node, unsigned pixel)
{
    if (!_valid(node) && pixel < 15)
        return { -1, -1 };
    float c = cosf(M_PI * (_adjList[node].angle - TRIANGLE_ANGLE) / 180.0f);
    float s = sinf(M_PI * (_adjList[node].angle - TRIANGLE_ANGLE) / 180.0f);
    return { _adjList[node].pos.x + pixelsPos[pixel].x * c - pixelsPos[pixel].y * s, _adjList[node].pos.y + pixelsPos[pixel].x * s + pixelsPos[pixel].y * c - TRIANGLE_RADIUS };
}

struct AStarData
{
    unsigned node;
    float f, g;
    AStarData* parent;
    
    AStarData()
    {
        
    }

    AStarData(unsigned node) : node(node), parent(NULL), f(0), g(0)
    {
        
    }

    AStarData(unsigned node, float g, float h) : node(node), parent(NULL), f(h + g), g(g)
    {
        
    }
};

std::vector<unsigned> JJL::Graph::path(unsigned start, unsigned end)
{
    std::vector<unsigned> p;
    if (!_valid(start, true) || !_valid(end, true))
        return p;
    
    if (start == end)
    {
        p.push_back(start);
        return p;
    }
    
    std::list<AStarData> aNodes;
    Hash<AStarData*> open;
    Hash<AStarData*> closed;

    aNodes.emplace_back(start);
    open.insert(start, &aNodes.back());

    Node& g = _adjList[end];

    while (open.size())
    {
        unsigned qn;
        AStarData* qnn;
        float f = std::numeric_limits<float>::infinity();
        for (AStarData* node : open)
        {
            if (node->f < f)
            {
                qn = node->node;
                qnn = node;
                f = node->f;
            }
        }

        Node& q = _adjList[qn];

        for (unsigned successor = 0; successor < MAX_NODE_VERTICES; successor++)
        {
            unsigned sucN = q.connections[successor];
            if (sucN == end)
            {
                p.push_back(end);
                AStarData* parent = qnn;
                while (parent)
                {
                    p.push_back(parent->node);
                    parent = parent->parent;
                }
                std::reverse(p.begin(), p.end());
                return p;
            }
            Node& suc = _adjList[sucN];
            if (!_valid(sucN, true))
                continue;
            AStarData sucA(sucN, qnn->g + q.weights[successor], _nodeDistance(suc, g));

            AStarData** node = open.at(sucN);
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

void JJL::Graph::_place(unsigned node)
{
    if (!_valid(node))
        return;

    Node& n = _adjList[node];
    if (n.placed)
        return;
    
    for (unsigned i = 0; i < MAX_NODE_VERTICES; i++)
    {
        if (!_adjList[n.connections[i]].placed)
            continue;

        Node& n2 = _adjList[n.connections[i]];

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

        float angle2;
        switch (j % 3)
        {
            case 0:
                angle2 = 180;
                break;
            
            case 1:
                angle2 = 60;
                break;
            
            case 2:
                angle2 = -60;
                break;
            
            default:
                angle2 = 0;
        }

        n.angle = fmodf(n2.angle + angle, 360.0f);
        n.pos.x = n2.pos.x + TRIANGLE_RADIUS * cosf(M_PI * (n2.angle + angle2) / 180.0f);
        n.pos.y = n2.pos.y + TRIANGLE_RADIUS * sinf(M_PI * (n2.angle + angle2) / 180.0f);
        n.placed = true;
        break;
    }

    for (unsigned i = 0; i < MAX_NODE_VERTICES; i++)
        _place(n.connections[i]);
}

bool JJL::Graph::_valid(unsigned node, bool placed)
{
    bool v = node <= _adjList.size() && _adjList[node].used;
    if (placed)
        return v && _adjList[node].placed;
    return v;
}

float JJL::Graph::_nodeDistance(const Node& node1, const Node& node2)
{
    return distance(node1.pos, node2.pos);
}

void JJL::Graph::_calcOffset(const Node& node, unsigned vert, float& x, float& y)
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
    x = node.pos.x + 2 * cosf(M_PI * angle / 180.0f);
    y = node.pos.y + 2 * sinf(M_PI * angle / 180.0f);
}

int JJL::Graph::_getNode(unsigned node, unsigned vert)
{
    for (unsigned i = 0; i < _adjList.size(); i++)
    {
        if (_valid(i, true) && i != node)
        {
            if (_nodeDistance(_adjList[i], _adjList[node]) < 0.1f)
                return i;
        }
    }
    return -1;
}
