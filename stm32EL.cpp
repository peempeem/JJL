#include "stm32EL.h"
#include "comm.h"

extern UART_HandleTypeDef huart1;

#define NUM_BROKERS 1

std::vector<MessageBroker> brokers = std::vector<MessageBroker>(NUM_BROKERS);

typedef struct UART_SEND_STATUS
{
    bool sending = false;
    bool receiving = false;
} uss_t;

volatile uss_t broker_data[NUM_BROKERS];

UART_HandleTypeDef* broker2UART(unsigned broker)
{
    switch (broker)
    {
        case 0:
            return &huart1;
        default:
            return NULL;
    }
}

void sendNext(unsigned broker)
{
    if (!broker_data[broker].sending)
    {
        Message* msg = brokers[broker]._comm_out_peek();
        if (msg)
        {
            HAL_UART_Transmit_IT(broker2UART(broker), (uint8_t*) msg->get(), msg->size());
            broker_data[broker].sending = true;
        }
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart == &huart1)
    {
        broker_data[0].sending = false;
        brokers[0]._comm_out_pop();
        sendNext(0);            
    }
}

MessageHub hub(&brokers);

void stm32EventLoop()
{
    while (true)
    {
        hub.update();
        sendNext(0);
    }
    
}