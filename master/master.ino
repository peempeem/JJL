#include <Arduino.h>
#include "comm.h"
#include "graph.h"
#include "time.h"

#define RX  26
#define TX  27
#define BAUD    115200

HardwareSerial port(1);
HardwareSerial p(2);
HardwareSerial debug(2);
std::vector<MessageBroker> brokers = std::vector<MessageBroker>(2);
MessageHub hub(&brokers, true);
Graph graph;
Rate update(30);
Rate blink(1);

void setup()
{
    Serial.begin(BAUD);
    port.begin(BAUD, SERIAL_8N1, RX, TX);
    p.begin(BAUD, SERIAL_8N1, 14, 12);
    graph.newNode();
}

void loop()
{
    while (port.available())
        brokers[0]._comm_in(port.read());
    
    while (p.available())
    {
        Serial.write(p.read());
    }
    
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
                
                unsigned msmaSize = sizeof(msg_set_master_addr_t) + rpath.size();
                uint8_t* buf = new uint8_t[msmaSize];
                msg_set_master_addr_t* msma = (msg_set_master_addr_t*) buf;
                msma->broker = conn->broker0;
                msma->address = conn->address1;
                msma->pathSize = rpath.size();
                for (unsigned i = 0; i < rpath.size(); i++)
                    msma->path[i] = rpath[i];
                Message smsg(MESSAGES::SET_MASTER_ADDR, path, buf, msmaSize);
                delete[] buf;
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
        uint8_t* buf = new uint8_t[sizeof(msg_set_lights_t) + sizeof(rgb8_t) * 1];
        msg_set_lights_t* msl = (msg_set_lights_t*) buf;
        msl->size = 1;
        uint8_t temp = (uint8_t) (blink.getStage() * 255) % 256;
        msl->data[0] = RGB8(temp, temp, temp);
        std::vector<unsigned> path;
        path.push_back(1);
        Message smsg(MESSAGES::SET_LIGHTS, path, buf, sizeof(msg_set_lights_t) + sizeof(rgb8_t) * 1);
        hub.send(smsg);
        delete[] buf;
        msl = (msg_set_lights_t*) smsg.getData();
    }

    while (brokers[0]._comm_out_peek())
    {
        Message* msg = brokers[0]._comm_out_peek();
        port.write((uint8_t*) msg->getMsg(), msg->size());
        brokers[0]._comm_out_pop();
    }
}