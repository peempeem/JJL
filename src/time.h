#pragma once

#include "hal.h"

#ifndef RATE_H
#define RATE_H

class Rate {
    public:
        Rate();
        Rate(float rate);

        void    setRate(float rate);
        void    ring();
        void    set();
        bool    isReady();
        void    reset();
        void    enable();
        void    disable();
        float   getStage(bool noChange=false);
        float   getStageCos(bool noChange=false);

    private:
        uint32_t    _inverseRate = 0;
        uint32_t    _last = 0;
        bool        _enabled = false;
};

#endif
