#include "stm32EL.h"
#include "comm.h"
#include "time.h"
#include "buffer.h"

#ifndef ESP32

extern UART_HandleTypeDef huart2;

#define NUM_BROKERS     1
#define RX_BUFSIZE_IT   64
#define RX_BUFSIZE      512

std::vector<MessageBroker> brokers = std::vector<MessageBroker>(NUM_BROKERS);
MessageHub hub(&brokers);

typedef struct UART_DATA
{
    volatile bool sending = false;
    uint8_t rxbuf[RX_BUFSIZE_IT];
    RingBuffer<RX_BUFSIZE> data;
} uartd_t;

uartd_t broker_data[NUM_BROKERS];

UART_HandleTypeDef* broker2UART(unsigned broker)
{
    switch (broker)
    {
        case 0:
            return &huart2;
        default:
            return NULL;
    }
}

int UART2Broker(UART_HandleTypeDef* huart)
{
    if (huart == &huart2)
        return 0;
    return -1;
}

void sendNext(unsigned broker)
{
    if (!broker_data[broker].sending)
    {
        Message* msg = brokers[broker]._comm_out_peek();
        if (msg)
            broker_data[broker].sending = HAL_UART_Transmit_IT(broker2UART(broker), (uint8_t*) msg->getMsg(), msg->size()) == HAL_OK;
    }
}

void recvNext(unsigned broker)
{
    HAL_UARTEx_ReceiveToIdle_IT(broker2UART(broker), broker_data[broker].rxbuf, RX_BUFSIZE_IT);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    int broker = UART2Broker(huart);
    if (broker != -1)
    {
        broker_data[broker].sending = false;
        brokers[broker]._comm_out_pop();
        sendNext(broker);
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size)
{
    int broker = UART2Broker(huart);
    if (broker != -1)
    {
        for (unsigned i = 0; i < (unsigned) size; i++)
            broker_data[broker].data.put(broker_data[broker].rxbuf[i]);
        recvNext(broker);
    }
}

void stm32EventLoop()
{
    while (true)
    {
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
                    {
                        // set lights
                        

                    }
                    break;
                }
            }

            msg.free();
            hub.messages.pop();
        }
        
        for (unsigned i = 0; i < NUM_BROKERS; i++)
        {
            sendNext(i);
            recvNext(i);
            while (broker_data[i].data.available())
                brokers[i]._comm_in(broker_data[i].data.get());
        }
    }
}

#endif
