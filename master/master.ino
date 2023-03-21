#include <Arduino.h>
#include "comm.h"
#include "graph.h"
#include "time.h"
#include "buffer.h"
#include "hash.h"

#define RX      26
#define TX      27
#define BAUD    1000000
#define PING_TIMEOUT    2000

typedef struct PING_DATA
{
    Rate ping = Rate(4);
    unsigned last = sysMillis();
    unsigned start = sysMillis();
    int ms = -1;
    uint8_t id = 0;
} ping_t;

HardwareSerial port(1);
std::vector<MessageBroker> brokers = std::vector<MessageBroker>(1);
MessageHub hub(&brokers, true);
Graph graph;
Hash<ping_t> pings;
Rate update(60);

Rate red(1);
Rate green(1.5f);
Rate blue(0.5f);

Rate debug(1);

void setup()
{
    Serial.begin(115200);
    port.begin(BAUD, SERIAL_8N1, RX, TX);
    graph.newNode();
}

void loop()
{
    while (port.available())
        brokers[0]._comm_in(port.read());

    hub.update();

    while (hub.messages.size())
    {
        Message& msg = hub.messages.front();

        switch (msg.type())
        {
            case MESSAGES::CONF_NODE:
            {
                msg_connection_t* conn = (msg_connection_t*) msg.getData();

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
                
                DBuffer<msg_set_master_addr_t> buf(sizeof(msg_set_master_addr_t) + rpath.size());
                buf.data()->broker = conn->broker0;
                buf.data()->address = conn->address1;
                buf.data()->pathSize = rpath.size();
                for (unsigned i = 0; i < rpath.size(); i++)
                    buf.data()->path[i] = rpath[i];
                
                Message smsg(MESSAGES::SET_MASTER_ADDR, path, buf.raw(), buf.size());
                if (conn->address0 == 0)
                    hub.sendBroker(smsg, conn->broker0);
                else
                    hub.send(smsg);
                break;
            }

            case MESSAGES::REPING:
            {
                msg_reping_t* reping = (msg_reping_t*) msg.getData();
                ping_t* ping = pings.at(reping->address);
                if (ping)
                {
                    if (ping->id != reping->id)
                        break;
                    ping->ms = sysMillis() - ping->start;
                    ping->last = sysMillis();
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
            ping.start = sysMillis();
            
            std::vector<unsigned> path = graph.path(0, node);
            if (!path.size())
                continue;
            std::vector<unsigned> rpath = std::vector<unsigned>(path.size());
            for (unsigned i = 0; i < path.size(); i++)
                rpath[i] = path[path.size() - 1 - i];
            path.erase(path.begin());
            rpath.erase(rpath.begin());

            DBuffer<msg_ping_t> msg(sizeof(msg_ping_t) + rpath.size());
            msg.data()->id = ping.id;
            msg.data()->returnPathSize = rpath.size();
            for (unsigned i = 0; i < rpath.size(); i++)
                msg.data()->returnPath[i] = rpath[i];
            
            Message smsg(MESSAGES::PING, path, msg.raw(), msg.size());
            hub.send(smsg);
        }
        if (sysMillis() - ping.last > PING_TIMEOUT)
        {
            graph.removeNode(node);
            pings.remove(node);
        }
    }

    if (update.isReady())
    {
        std::vector<unsigned> path = graph.path(0, 1);
        
        if (path.size() > 0)
        {
            path.erase(path.begin());

            SBuffer<msg_set_lights_t, sizeof(msg_set_lights_t) + sizeof(msg_set_lights_t::rgb_t) * 1> buf;
            buf.data()->size = 1;
            buf.data()->data[0].r = (uint8_t) (red.getStageCos() * 255) % 256;
            buf.data()->data[0].g = (uint8_t) (green.getStageCos() * 255) % 256;
            buf.data()->data[0].b = (uint8_t) (blue.getStageCos() * 255) % 256;
            
            Message smsg(MESSAGES::SET_LIGHTS, path, buf.raw(), buf.size());
            hub.send(smsg);
            
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
            float2 pos = graph.position(node);
            Serial.print(pos.x);
            Serial.print(", ");
            Serial.print(pos.y);
            Serial.print(")\t Ping (");
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

        Serial.println(100 * (1 - ESP.getFreeHeap() / 327680.0f)); 
    }

    while (brokers[0]._comm_out_peek())
    {
        Message* msg = brokers[0]._comm_out_peek();
        port.write((uint8_t*) msg->getMsg(), msg->size());
        brokers[0]._comm_out_pop();
    }
}