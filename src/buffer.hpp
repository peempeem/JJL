#include <stdint.h>

template<unsigned LEN>
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