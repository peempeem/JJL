#include "buffer.h"

// CFIFO

template <class T>
JJL::CFIFO<T>::CFIFO()
{

}

template <class T>
void JJL::CFIFO<T>::insert(uint8_t byte)
{
    uint8_t* data = (uint8_t*) this;
    for (unsigned i = 0; i < sizeof(T) - 1; i++)
        data[i] = data[i + 1];
    data[sizeof(T) - 1] = byte;
}

// SBUFFER

template <class T, unsigned LEN>
JJL::SBuffer<T, LEN>::SBuffer()
{

}

template <class T, unsigned LEN>
inline T* JJL::SBuffer<T, LEN>::data()
{
    return (T*) _data;
}

template <class T, unsigned LEN>
inline uint8_t* JJL::SBuffer<T, LEN>::raw()
{
    return _data;
}

template <class T, unsigned LEN>
inline unsigned JJL::SBuffer<T, LEN>::size()
{
    return LEN;
}

// DBUFFER

template <class T>
JJL::DBuffer<T>::DBuffer(unsigned length) : _size(length)
{
    _data = new uint8_t[length];
}

template <class T>
JJL::DBuffer<T>::~DBuffer()
{
    delete[] _data;
}

template <class T>
inline T* JJL::DBuffer<T>::data()
{
    return (T*) _data;
}

template <class T>
inline uint8_t* JJL::DBuffer<T>::raw()
{
    return _data;
}

template <class T>
inline unsigned JJL::DBuffer<T>::size()
{
    return _size;
}

// RINGBUFFER

template <unsigned LEN>
JJL::RingBuffer<LEN>::RingBuffer() : _put(0), _get(0)
{

}

template <unsigned LEN>
inline bool JJL::RingBuffer<LEN>::available()
{
    return _put != _get;
}

template <unsigned LEN>
void JJL::RingBuffer<LEN>::put(uint8_t byte)
{
    _data[_put] = byte;
    _put = (_put + 1) % LEN;
    if (_put == _get)
        _get = (_get + 1) % LEN;
}

template <unsigned LEN>
uint8_t JJL::RingBuffer<LEN>::get()
{
    uint8_t byte = _data[_get];
    if (_put != _get)
        _get = (_get + 1) % LEN;
    return byte;
}