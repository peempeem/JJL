#pragma once
#include <stdint.h>

template <class T>
class CFIFO : public T
{
    public:
        CFIFO()
        {
            
        }
        
        void insert(uint8_t byte)
        {
            uint8_t* data = (uint8_t*) this;
            for (unsigned i = 0; i < sizeof(T) - 1; i++)
                data[i] = data[i + 1];
            data[sizeof(T) - 1] = byte;
        }
};

template <class T, unsigned LEN>
class SBuffer
{
    public:
        SBuffer()
        {

        }

        inline T* data()
        {
            return (T*) _data;
        }

        inline uint8_t* raw()
        {
            return _data;
        }

        inline unsigned size()
        {
            return LEN;
        }

    private:
        uint8_t _data[LEN];
};

template <class T>
class DBuffer
{
    public:
        DBuffer(unsigned length) : _size(length)
        {
            _data = new uint8_t[length];
        }

        ~DBuffer()
        {
            delete[] _data;
        }

        inline T* data()
        {
            return (T*) _data;
        }

        inline uint8_t* raw()
        {
            return _data;
        }

        inline unsigned size()
        {
            return _size;
        }

    private:
        uint8_t* _data;
        unsigned _size;
};

template <unsigned LEN>
class RingBuffer
{
    public:
        RingBuffer() : _put(0), _get(0)
        {

        }

        void put(uint8_t byte)
        {
            _data[_put] = byte;
            _put = (_put + 1) % LEN;
            if (_put == _get)
                _get = (_get + 1) % LEN;
        }

        inline bool available()
        {
            return _put != _get;
        }

        uint8_t get()
        {
            uint8_t byte = _data[_get];
            if (_put != _get)
                _get = (_get + 1) % LEN;
            return byte;
        }
    
    private:
        uint8_t _data[LEN];
        int _put;
        int _get;
};
