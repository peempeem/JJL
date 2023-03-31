#include "time.h"
#define _USE_MATH_DEFINES
#include <math.h>

JJL::Rate::Rate() : _inverseRate(1000), _enabled(false)
{

}

JJL::Rate::Rate(float rate)
{
    _inverseRate = 1000 / rate;
    enable();
}

void JJL::Rate::setRate(float rate)
{
    _inverseRate = 1000 / rate;
    enable();
}

void JJL::Rate::ring()
{
    _last = 0;
}

void JJL::Rate::set()
{
    _last = sysMillis();
}

bool JJL::Rate::isReady()
{
    if (!_enabled)
        return false;
    uint32_t time = sysMillis();
    if (time - _last > _inverseRate)
    {
        _last = time;
        return true;
    }
    return false;
}

void JJL::Rate::reset()
{
    _last = JJL::sysMillis();
}

void JJL::Rate::enable()
{
    if (_enabled)
        reset();
    _enabled = true;
}

void JJL::Rate::disable()
{
    _enabled = false;
}

float JJL::Rate::getStage(bool noChange) 
{
    if (!_enabled)
        return 0;
    uint32_t time = sysMillis();
    float stage = (time - _last) / (float) _inverseRate;
    if (stage > 1) 
    {
        stage = 1;
        if (!noChange)
            _last = time;
    }
    return stage;
}

float JJL::Rate::getStageCos(float offset, bool noChange)
{
    float stage = getStage(noChange) + offset;
    return 0.5f - cos(stage * 2 * M_PI) / 2.0f;
}