#include <Arduino.h>
#include "comm.h"

#define RX  26
#define TX  27

HardwareSerial port(1);

void setup()
{
    Serial.begin(115200);
    port.begin(115200, SERIAL_8N1, RX, TX);
}

std::vector<MessageBroker> brokers = std::vector<MessageBroker>(1);
MessageHub hub(&brokers);

void loop()
{
    while (port.available())
        brokers[0]._comm_in(port.read());
    hub.update();
    while (brokers[0]._comm_out_peek())
    {
        Message* msg = brokers[0]._comm_out_peek();
        port.write((uint8_t*) msg->get(), msg->size());
        brokers[0]._comm_out_pop();
    }
}