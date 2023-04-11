#include "stm32EL.h"
#include "comm.h"
#include "time.h"
#include "buffer.h"
#include "neopixels.h"

#ifndef ESP32

extern TIM_HandleTypeDef htim2;

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

#define NUM_BROKERS     3
#define RX_BUFSIZE_IT   64
#define RX_BUFSIZE      256
#define NUM_PIXELS      15

std::vector<JJL::MessageBroker> brokers = std::vector<JJL::MessageBroker>(NUM_BROKERS);
JJL::MessageHub hub(&brokers);
JJL::NeoPixels pixels(&htim2, TIM_CHANNEL_1, 80, NUM_PIXELS);

typedef struct UART_DATA
{
    volatile bool sending = false;
    uint8_t rxbuf[RX_BUFSIZE_IT];
    JJL::RingBuffer<RX_BUFSIZE> data;
} uartd_t;

uartd_t broker_data[NUM_BROKERS];

UART_HandleTypeDef* broker2UART(unsigned broker)
{
    switch (broker)
    {
        case 0:
            return &huart3;
        case 1:
            return &huart2;
        case 2:
            return &huart1;
        default:
            return NULL;
    }
}

int UART2Broker(UART_HandleTypeDef* huart)
{
    if (huart == &huart3)
        return 0;
    else if (huart == &huart2)
        return 1;
    else if (huart == &huart1)
        return 2;
    return -1;
}

void sendNext(unsigned broker)
{
    if (!broker_data[broker].sending)
    {
        JJL::Message* msg = brokers[broker]._comm_out_peek();
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

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
  HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_1);
}

void JJL::stm32EventLoop()
{    
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
                {
                    // set lights
                    pixels.set(i, msl->data[i].r, msl->data[i].g, msl->data[i].b);
                    

                }
                pixels.send();
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

#endif
