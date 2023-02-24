#include "time.h"

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
