#include "neopixels.h"

JJL::NeoPixels::NeoPixels(TIM_HandleTypeDef* timer, unsigned channel, unsigned ARR, unsigned numPixels) : _timer(timer), _channel(channel), _numPixels(numPixels)
{
    _zero = ARR / 3;
    _one = (ARR * 2) / 3;
    _data = new bgr_t[numPixels];
    _dmaBufSize = numPixels * 24 + 1;
    _dmaBuf = new uint32_t[_dmaBufSize];

    for (unsigned i = 0; i < numPixels; i++)
        _data[i].data = 0;
}

JJL::NeoPixels::~NeoPixels()
{
    delete[] _data;
    delete[] _dmaBuf;
}

void JJL::NeoPixels::set(unsigned i, uint8_t r, uint8_t g, uint8_t b)
{
    _data[i].color.r = r;
    _data[i].color.g = g;
    _data[i].color.b = b;
}

void JJL::NeoPixels::send()
{
    unsigned j = 0;
    for (unsigned i = 0; i < _numPixels; i++)
    {
        for (int bit = 23; bit >= 0; bit--)
        {
            if ((_data[i].data >> bit) & 0x01)
                _dmaBuf[j] = _one;
            else
                _dmaBuf[j] = _zero;
            j++;
        }
    }
    _dmaBuf[_dmaBufSize - 1] = 0;

    HAL_TIM_PWM_Start_DMA(_timer, _channel, _dmaBuf, _dmaBufSize);
}