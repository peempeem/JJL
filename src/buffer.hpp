#include "buffer.h"

// CFIFO

template <class T>
CFIFO<T>::CFIFO()
{

}

template <class T>
void CFIFO<T>::insert(uint8_t byte)
{
    uint8_t* data = (uint8_t*) this;
    for (unsigned i = 0; i < sizeof(T) - 1; i++)
        data[i] = data[i + 1];
    data[sizeof(T) - 1] = byte;
}

// SBUFFER

template <class T, unsigned LEN>
SBuffer<T, LEN>::SBuffer()
{

}

template <class T, unsigned LEN>
inline T* SBuffer<T, LEN>::data()
{
    return (T*) _data;
}

template <class T, unsigned LEN>
inline uint8_t* SBuffer<T, LEN>::raw()
{
    return _data;
}

template <class T, unsigned LEN>
inline unsigned SBuffer<T, LEN>::size()
{
    return LEN;
}

// DBUFFER

template <class T>
DBuffer<T>::DBuffer(unsigned length) : _size(length)
{
    _data = new uint8_t[length];
}

template <class T>
DBuffer<T>::~DBuffer()
{
    delete[] _data;
}

template <class T>
inline T* DBuffer<T>::data()
{
    return (T*) _data;
}

template <class T>
inline uint8_t* DBuffer<T>::raw()
{
    return _data;
}

template <class T>
inline unsigned DBuffer<T>::size()
{
    return _size;
}

// RINGBUFFER

template <unsigned LEN>
RingBuffer<LEN>::RingBuffer() : _put(0), _get(0)
{

}

template <unsigned LEN>
inline bool RingBuffer<LEN>::available()
{
    return _put != _get;
}

template <unsigned LEN>
void RingBuffer<LEN>::put(uint8_t byte)
{
    _data[_put] = byte;
    _put = (_put + 1) % LEN;
    if (_put == _get)
        _get = (_get + 1) % LEN;
}

template <unsigned LEN>
uint8_t RingBuffer<LEN>::get()
{
    uint8_t byte = _data[_get];
    if (_put != _get)
        _get = (_get + 1) % LEN;
    return byte;
}