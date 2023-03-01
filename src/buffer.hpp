#include <stdint.h>

template <unsigned LEN>
class Buffer
{
    public:
        Buffer()
        {
            
        }
        
        void insert(uint8_t byte)
        {
            _data[_idx++] = byte;
            if (_idx >= LEN)
                _idx = 0;
            if (++_size > LEN)
                _size = LEN;
        }

        bool memcmp(const uint8_t* arr)
        {
            for (unsigned i = 0; i < LEN; i++)
            {
                if (arr[i] != _data[(i + _idx) % LEN])
                    return false;
            }
            return true;
        }

        void copy(uint8_t* dst)
        {
            for (unsigned i = 0; i < LEN; i++)
                dst[i] = _data[(i + _idx) % LEN];
        }

        bool isFull()
        {
            return _size == LEN;
        }

    private:
        unsigned    _size = 0;
        unsigned    _idx = 0;
        uint8_t     _data[LEN];
};

template <class T, unsigned LEN>
class SBuffer
{
    public:
        SBuffer<T, LEN>()
        {

        }

        inline T* data()
        {
            return (T*) &_data;
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
        DBuffer<T>(unsigned length) : _size(length)
        {
            _data = new uint8_t[length];
        }

        ~DBuffer<T>()
        {
            delete[] _data;
        }

        inline T* data()
        {
            return (T*) &_data;
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
            if (_put == _get)
                _get = (_get + 1) % LEN;
            _put = (_put + 1) % LEN;
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
