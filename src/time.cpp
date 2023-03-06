#include "time.h"
#define _USE_MATH_DEFINES
#include <math.h>

Rate::Rate() : _inverseRate(1000), _enabled(false)
{

}

Rate::Rate(float rate)
{
    _inverseRate = 1000 / rate;
    enable();
}

void Rate::setRate(float rate)
{
    _inverseRate = 1000 / rate;
    enable();
}

void Rate::ring()
{
    _last = 0;
}

void Rate::set()
{
    _last = sysMillis();
}

bool Rate::isReady()
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

void Rate::reset()
{
    _last = sysMillis();
}

void Rate::enable()
{
    if (_enabled)
        reset();
    _enabled = true;
}

void Rate::disable()
{
    _enabled = false;
}

float Rate::getStage(bool noChange) 
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

float Rate::getStageCos(bool noChange)
{
    float stage = getStage(noChange);
    return 0.5f - cos(stage * 2 * M_PI) / 2.0f;
}