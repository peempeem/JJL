#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "comm.h"

#define RX0         26
#define TX0         27
#define RX1         14
#define TX1         12
#define BAUD        1000000
#define PIN_LED     25
#define NUM_LED     15
#define NUM_BROKERS 2

HardwareSerial ports[] = { HardwareSerial(1), HardwareSerial(2) };
std::vector<JJL::MessageBroker> brokers = std::vector<JJL::MessageBroker>(NUM_BROKERS);
JJL::MessageHub hub(&brokers);
Adafruit_NeoPixel strip(NUM_LED, PIN_LED, NEO_RGB + NEO_KHZ800);

void setup()
{
    Serial.begin(115200);
    ports[0].begin(BAUD, SERIAL_8N1, RX0, TX0);
    ports[1].begin(BAUD, SERIAL_8N1, RX1, TX1);
    strip.begin();
}

void loop()
{
    for (unsigned i = 0; i < NUM_BROKERS; i++)
    {
        while (ports[i].available())
            brokers[i]._comm_in(ports[i].read());
    }   
    
    hub.update();

    while (hub.messages.size())
    {
        JJL::Message& msg = hub.messages.front();

        switch (msg.type())
        {
            case JJL::MESSAGES::SET_LIGHTS:
            {
                JJL::msg_set_lights_t* msl = (JJL::msg_set_lights_t*) msg.getData();
                for (unsigned i = 0; i < msl->size; i++)
                    strip.setPixelColor(i, msl->data[i].r, msl->data[i].g, msl->data[i].b);
                /*Serial.print(msl->data[0].r);
                Serial.print('\t');
                Serial.print(msl->data[0].g);
                Serial.print('\t');
                Serial.print(msl->data[0].b);
                Serial.println('\t');*/
                strip.show();
                //Serial.println(100 * (1 - ESP.getFreeHeap() / 327680.0f));  
                break;
            }
        }

        msg.free();
        hub.messages.pop();
    }

    for (unsigned i = 0; i < NUM_BROKERS; i++)
    {
        while (brokers[i]._comm_out_peek())
        {
            JJL::Message* msg = brokers[i]._comm_out_peek();
            ports[i].write((uint8_t*) msg->getMsg(), msg->size());
            brokers[i]._comm_out_pop();
        }
    }
}
