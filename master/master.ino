#include "comm.h"
#include "graph.h"
#include "time.h"
#include "buffer.h"
#include "hash.h"
#include "config.h"

#define RX      26
#define TX      27
#define PING_TIMEOUT    1000

typedef struct PING_DATA
{
    JJL::Rate ping = JJL::Rate(5);
    unsigned last = JJL::sysMillis();
    unsigned start = JJL::sysMillis();
    int ms = -1;
    uint8_t id = 0;
} ping_t;

HardwareSerial port(1);
std::vector<JJL::MessageBroker> brokers = std::vector<JJL::MessageBroker>(1);
JJL::MessageHub hub(&brokers, true);
JJL::Graph graph;
JJL::Hash<ping_t> pings;
JJL::Rate refresh(30);

JJL::Rate red(1.5);
JJL::Rate green(1);
JJL::Rate blue(0.5);

JJL::Rate debug(1);

void setup()
{
    Serial.begin(115200);

    port.begin(100000, SERIAL_8N1, RX, TX);
    graph.newNode();
}

void update()
{
    while (port.available())
        brokers[0]._comm_in(port.read());
    
    hub.update();

    while (brokers[0]._comm_out_peek())
    {
        JJL::Message* msg = brokers[0]._comm_out_peek();
        port.write((uint8_t*) msg->getMsg(), msg->size());
        brokers[0]._comm_out_pop();
    }
}

void loop()
{
    update();

    while (hub.messages.size())
    {
        JJL::Message& msg = hub.messages.front();

        switch (msg.type())
        {
            case JJL::MESSAGES::CONF_NODE:
            {
                JJL::msg_connection_t* conn = (JJL::msg_connection_t*) msg.getData();

                if (!conn->isSetup)
                {
                    int node = graph.getNode(conn->address0, conn->broker0);
                    if (node == -1)
                        node = graph.newNode();
                    conn->address1 = node;             
                }

                graph.connect(conn->address0, conn->address1, conn->broker0, conn->broker1);

                std::vector<unsigned> path = graph.path(0, conn->address1);
                if (!path.size())
                    break;
                
                std::vector<unsigned> rpath = std::vector<unsigned>(path.size());
                for (unsigned i = 0; i < path.size(); i++)
                    rpath[i] = path[path.size() - 1 - i];

                path.erase(path.begin());
                path.erase(--path.end());
                rpath.erase(rpath.begin());
                
                JJL::DBuffer<JJL::msg_set_master_addr_t> buf(sizeof(JJL::msg_set_master_addr_t) + rpath.size());
                buf.data()->broker = conn->broker0;
                buf.data()->address = conn->address1;
                buf.data()->pathSize = rpath.size();
                for (unsigned i = 0; i < rpath.size(); i++)
                    buf.data()->path[i] = rpath[i];
                
                JJL::Message smsg(JJL::MESSAGES::SET_MASTER_ADDR, path, buf.raw(), buf.size());
                if (conn->address0 == 0)
                    hub.sendBroker(smsg, conn->broker0);
                else
                    hub.send(smsg);
                break;
            }

            case JJL::MESSAGES::REPING:
            {
                JJL::msg_reping_t* reping = (JJL::msg_reping_t*) msg.getData();
                ping_t* ping = pings.at(reping->address);
                if (ping)
                {
                    if (ping->id != reping->id)
                        break;
                    ping->ms = JJL::sysMillis() - ping->start;
                    ping->last = JJL::sysMillis();
                    ping->id++;
                }
                break;
            }
        }

        msg.free();
        hub.messages.pop();
    }

    for (unsigned node : graph)
    {
        if (!node)
            continue;

        ping_t& ping = pings[node];
        
        if (ping.ping.isReady())
        {
            ping.start = JJL::sysMillis();
            
            std::vector<unsigned> path = graph.path(0, node);
            if (!path.size())
                continue;
            std::vector<unsigned> rpath = std::vector<unsigned>(path.size());
            for (unsigned i = 0; i < path.size(); i++)
                rpath[i] = path[path.size() - 1 - i];
            path.erase(path.begin());
            rpath.erase(rpath.begin());

            JJL::DBuffer<JJL::msg_ping_t> msg(sizeof(JJL::msg_ping_t) + rpath.size());
            msg.data()->id = ping.id;
            msg.data()->returnPathSize = rpath.size();
            for (unsigned i = 0; i < rpath.size(); i++)
                msg.data()->returnPath[i] = rpath[i];
            
            JJL::Message smsg(JJL::MESSAGES::PING, path, msg.raw(), msg.size());
            hub.send(smsg);
        }
        if (JJL::sysMillis() - ping.last > PING_TIMEOUT)
        {
            graph.removeNode(node);
            pings.remove(node);
        }
    }

    if (refresh.isReady())
    {
        for (unsigned node : graph)
        {
            if (node == 0)
                continue;
            std::vector<unsigned> path = graph.path(0, node);
            if (path.size() > 0)
            {
                path.erase(path.begin());

                JJL::SBuffer<JJL::msg_set_lights_t, sizeof(JJL::msg_set_lights_t) + sizeof(JJL::msg_set_lights_t::rgb_t) * 15> buf;
                buf.data()->size = 15;
                for (unsigned i = 0; i < 15; i++)
                {
                    JJL::float2 pixpos = graph.pixelPosition(node, i);
                    float stage = graph.distance2Triangle(graph.distance(pixpos) / 2);
                    buf.data()->data[i].r = (uint8_t) (red.getStageCos(stage) * 255);
                    buf.data()->data[i].g = (uint8_t) (green.getStageCos(stage) * 255);
                    buf.data()->data[i].b = (uint8_t) (blue.getStageCos(stage) * 255);
                }
                
                JJL::Message smsg(JJL::MESSAGES::SET_LIGHTS, path, buf.raw(), buf.size());
                hub.send(smsg);  
            }
        }
    }

    if (debug.isReady())
    {
        char buf[80];
        for (unsigned i = 0; i < 20; i++)
            buf[i] = '\n';
        Serial.write(buf, 20);
        Serial.println("Nodes List");
        for (unsigned i = 0; i < 80; i++)
            buf[i] = '=';
        Serial.write(buf, 80);
        Serial.println("");
        for (unsigned node : graph)
        {   
            Serial.print("Node ");
            Serial.print(node);
            Serial.print(": Position (");
            JJL::float2 pos = graph.position(node);
            Serial.print(pos.x);
            Serial.print(", ");
            Serial.print(pos.y);
            Serial.print(")\t Angle: ");
            Serial.print(graph.angle(node));
            Serial.print("\t Ping (");
            Serial.print(node ? pings[node].ms : 0);
            Serial.print(" ms) \t Path: ");
            std::vector<unsigned> path = graph.path(0, node);
            for (unsigned i = 0; i < path.size(); i++)
            {
                Serial.print(path[i]);
                if (i < path.size() - 1)
                    Serial.print("->");
            }
            Serial.println("");
        }

        //Serial.println(100 * (1 - ESP.getFreeHeap() / 327680.0f)); 
    }
}