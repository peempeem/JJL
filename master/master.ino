#include <Arduino.h>
#include "comm.h"
#include "graph.h"
#include "time.h"
#include "buffer.h"

#define RX      26
#define TX      27
#define BAUD    115200

HardwareSerial port(1);
std::vector<MessageBroker> brokers = std::vector<MessageBroker>(1);
MessageHub hub(&brokers, true);
Graph graph;
Rate update(30);
Rate red(1);
Rate green(1.5f);
Rate blue(0.5f);

void setup()
{
    Serial.begin(BAUD);
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
                    {
                        node = graph.newNode();
                        Serial.println("Creating new node");
                    }
                    conn->address1 = node;             
                }
                Serial.print("Connecting node ");
                Serial.println(conn->address1);
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
                if (conn->address0)
                    hub.send(smsg);
                else
                    hub.sendBroker(smsg, conn->broker0);
                break;
            }

            case MESSAGES::NODE_DC:
            {

            }
        }

        msg.free();
        hub.messages.pop();
    }

    if (update.isReady())
    {
        SBuffer<msg_set_lights_t, sizeof(msg_set_lights_t) + sizeof(msg_set_lights_t::rgb_t) * 1> buf;
        buf.data()->size = 1;
        buf.data()->data[0].r = (uint8_t) (red.getStageCos() * 255) % 256;
        buf.data()->data[1].g = (uint8_t) (green.getStageCos() * 255) % 256;
        buf.data()->data[2].b = (uint8_t) (blue.getStageCos() * 255) % 256;
        Message smsg(MESSAGES::SET_LIGHTS, graph.path(0, 1), buf.raw(), buf.size());
        hub.send(smsg);
    }

    while (brokers[0]._comm_out_peek())
    {
        Message* msg = brokers[0]._comm_out_peek();
        port.write((uint8_t*) msg->getMsg(), msg->size());
        brokers[0]._comm_out_pop();
    }
}