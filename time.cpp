#include "time.h"


Rate::Rate() : _inverseRate(1000000)
{

}

Rate::Rate(float rate)
{ 
    _inverseRate = 1000000 / rate;
    enable();
}

void Rate::setRate(float rate)
{
    _inverseRate = 1000000 / rate;
    enable();
}

bool Rate::isReady()
{
    if (!_enabled)
        return false;
    uint64_t time = sysMicros();
    if (time - _last > _inverseRate)
    {
        _last = time;
        return true;
    }
    return false;
}

void Rate::reset()
{
    _last = sysMicros();
}

void Rate::enable()
{
    if (_enabled)
    {
        reset();
        _enabled = true;
    }
}

void Rate::disable()
{
    _enabled = false;
}

float Rate::getStage(bool noChange) 
{
    if (!_enabled)
        return 0;
    uint64_t time = sysMicros();
    float stage = (time - _last) / (float) _inverseRate;
    if (stage > 1) 
    {
        stage = 1;
        if (!noChange)
            _last = time;
    }
    return stage;
}
