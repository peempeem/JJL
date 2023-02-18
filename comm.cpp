#include "comm.h"

uint16_t checksum16(const uint8_t* data, unsigned len)
{
    uint16_t sum = 0;
    for (unsigned i = 0; i < len; i++)
        sum += data[i];
    return sum;
}

Message::Message() : _msg(NULL), _write(0)
{

}

Message::Message(uint8_t type, std::vector<uint8_t> address, const uint8_t* data, uint16_t datalen)
{
    _msgsize = sizeof(msg_t) + (unsigned) datalen + address.size();
    _msg = (msg_t*) new uint8_t[_msgsize];
    _msg->header.type = type;
    _msg->header.addrlen = (uint8_t) address.size();
    _msg->header.datalen = datalen;
    memcpy(_msg->data, data, (unsigned) datalen);
    _write = (unsigned) datalen;
    for (auto it = address.rbegin(); it != address.rend(); ++it) {
        _msg->data[_write] = *it;
        _write++;
    }
    _msg->header.dcs = checksum16(_msg->data, (unsigned) datalen + address.size());
    _msg->header.hcs = checksum16((uint8_t*) &_msg->header, sizeof(msgh_t));
}

bool Message::setHeader(const msgh_t* header)
{
    if (!_msg && checksum16((const uint8_t*) header, sizeof(msgh_t)) == header->hcs)
    {
        _msgsize = sizeof(msg_t) + (unsigned) header->addrlen + (unsigned) header->datalen;
        _msg = (msg_t*) new uint8_t[_msgsize];
        _msg->header = *header;
        return true;
    }
    return false;
}

bool Message::insertData(uint8_t byte)
{
    if (_msg && _write < (unsigned) _msg->header.datalen + (unsigned) _msg->header.addrlen)
    {
        _msg->data[_write++] = byte;
        return true;
    }
    return false;
}

bool Message::popCurrentAddress()
{
    if (_msg && (unsigned) _msg->header.addrlen > 1)
    {
        _msg->header.addrlen--;
        _write--;
        return true;
    }
    return false;
}

uint8_t Message::getCurrentAddress()
{
    return _msg ? _msg->data[(unsigned) _msg->header.datalen + (unsigned) _msg->header.addrlen - 1] : 0;
}
