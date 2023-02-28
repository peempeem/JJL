#include "stm32EL.h"
#include "comm.h"
#include "time.h"

#ifndef ESP32

extern UART_HandleTypeDef huart2;

#define NUM_BROKERS 1
#define BUFSIZE     32

std::vector<MessageBroker> brokers = std::vector<MessageBroker>(NUM_BROKERS);
MessageHub hub(&brokers);

typedef struct UART_DATA
{
    volatile bool sending = false;
    uint8_t rxbuf[BUFSIZE];
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
        {
            HAL_UART_Transmit_IT(broker2UART(broker), (uint8_t*) msg->getMsg(), msg->size());
            broker_data[broker].sending = true;
        }
    }
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
            brokers[broker]._comm_in(broker_data[broker].rxbuf[i]);
        HAL_UARTEx_ReceiveToIdle_IT(huart, broker_data[broker].rxbuf, BUFSIZE);
        char buf[] = "ayo!!\n\r";
        Message msg = Message(0, std::vector<unsigned>(), (uint8_t*) buf, sizeof(buf));
        hub.sendBroker(msg, 0);
    }
}


void stm32EventLoop()
{
    for (unsigned i = 0; i < NUM_BROKERS; i++)
        HAL_UARTEx_ReceiveToIdle_IT(broker2UART(i), broker_data[0].rxbuf, BUFSIZE);

    while (true)
    {
        hub.update();

        while (hub.messages.size())
        {
            Message& msg = hub.messages.front();

            switch (msg.type())
            {
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
        
        for (unsigned i = 0; i < NUM_BROKERS; i++)
            sendNext(0);
    }
}

#endif
