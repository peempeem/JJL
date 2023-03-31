#pragma once
#include <stdint.h>

namespace JJL
{
    template <class T>
    class CFIFO : public T
    {
        public:
            CFIFO();
            
            void insert(uint8_t byte);
    };

    template <class T, unsigned LEN>
    class SBuffer
    {
        public:
            SBuffer();

            inline T* data();
            inline uint8_t* raw();
            inline unsigned size();

        private:
            uint8_t _data[LEN];
    };

    template <class T>
    class DBuffer
    {
        public:
            DBuffer(unsigned length);
            ~DBuffer();

            inline T* data();
            inline uint8_t* raw();
            inline unsigned size();

        private:
            uint8_t* _data;
            unsigned _size;
    };

    template <unsigned LEN>
    class RingBuffer
    {
        public:
            RingBuffer();

            inline bool available();
            void put(uint8_t byte);
            uint8_t get();
        
        private:
            uint8_t _data[LEN];
            int _put;
            int _get;
    };
}

#include "buffer.hpp"
