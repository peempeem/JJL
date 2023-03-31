#pragma once

#include "hal.h"

namespace JJL
{
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
            float   getStageCos(float offset=0.0f, bool noChange=false);

        private:
            uint32_t    _inverseRate = 0;
            uint32_t    _last = 0;
            bool        _enabled = false;
    };
}
