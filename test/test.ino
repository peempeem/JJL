#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "comm.h"

#define RX1 26
#define TX1 27
#define RX2 14
#define TX2 12
#define BAUD    115200
#define NUM_LED 1

HardwareSerial port1(1);
HardwareSerial port2(2);
std::vector<MessageBroker> brokers = std::vector<MessageBroker>(2);
MessageHub hub(&brokers);
Adafruit_NeoPixel strip(NUM_LED, 25, NEO_RGB + NEO_KHZ800);

void setup()
{
    Serial.begin(BAUD);
    port1.begin(BAUD, SERIAL_8N1, RX1, TX1);
    port2.begin(BAUD, SERIAL_8N1, RX2, TX2);
    strip.begin();
}

void loop()
{
    while (port1.available())
        brokers[0]._comm_in(port1.read());
    while (port2.available())
    {
        uint8_t byte = port2.read();
        Serial.write(byte);
        brokers[1]._comm_in(byte);
    }
        
    
    hub.update();

    while (hub.messages.size())
    {
        Message& msg = hub.messages.front();

        switch (msg.type())
        {
            case MESSAGES::SET_LIGHTS:
            {
                msg_set_lights_t* msl = (msg_set_lights_t*) msg.getData();
                for (unsigned i = 0; i < msl->size; i++)
                    strip.setPixelColor(i, msl->data[i].r, msl->data[i].g, msl->data[i].b);
                strip.show();
                msg.free();
                break;
            }

            case MESSAGES::SET_MASTER_ADDR:
            {
                msg_set_master_addr_t* msma = (msg_set_master_addr_t*) msg.getData();
                hub.sendBroker(msg, msma->broker);
                break;
            }

            default:
                msg.free();
        }

        hub.messages.pop();
    }

    while (brokers[0]._comm_out_peek())
    {
        Message* msg = brokers[0]._comm_out_peek();
        port1.write((uint8_t*) msg->getMsg(), msg->size());
        brokers[0]._comm_out_pop();
    }

    while (brokers[1]._comm_out_peek())
    {
        Message* msg = brokers[1]._comm_out_peek();
        port2.write((uint8_t*) msg->getMsg(), msg->size());
        brokers[1]._comm_out_pop();
    }
}