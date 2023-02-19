#include "comm.h"

uint32_t checksum32(const uint8_t* data, unsigned len)
{
    uint32_t sum = 0;
    for (unsigned i = 0; i < len; i++)
        sum += data[i];
    return sum;
}

Message::Message() : _msg(NULL), _valid(false)
{

}

Message::Message(uint8_t type, const std::vector<uint8_t>& address, const uint8_t* data, uint16_t datalen) : _valid(true)
{
    unsigned dataSize = datalen + address.size();
    _msgSize = sizeof(MSG::HEADER) + dataSize;
    _msg = (msg_t*) new uint8_t[_msgSize];
    _msg->header.data.type = type;
    _msg->header.data.addrlen = (uint8_t) address.size();
    _msg->header.data.datalen = datalen;
    std::memcpy(_msg->data, data, datalen);
    _write = datalen;
    for (auto it = address.rbegin(); it != address.rend(); ++it)
        _msg->data[_write++] = *it;
    _msg->header.data.datacs    = checksum32(_msg->data, dataSize);
    _msg->header.cs             = checksum32((uint8_t*) &_msg->header.data, sizeof(MSG::HEADER::DATA));
}

void Message::free()
{
    if (_msg)
    {
        delete[] _msg;
        _msg = NULL;
    }
}

bool Message::insertData(uint8_t byte)
{
    if (!_head)
    {
        if (_msg)
            return true;
        _headBuf.insert(byte);
        if (_headBuf.isFull())
        {
            uint8_t buf[sizeof(MSG::HEADER)];
            _headBuf.copy(buf);
            msg_t* msg = (msg_t*) buf;
            if (checksum32((uint8_t*) &msg->header.data, sizeof(MSG::HEADER::DATA)) == msg->header.cs)
            {
                _dataSize = msg->header.data.datalen + msg->header.data.addrlen;
                _msgSize = sizeof(MSG::HEADER) + _dataSize;
                _msg = (msg_t*) new uint8_t[_msgSize];
                _msg->header = msg->header;
                _head = true;
            }
        }
        return false;
    }

    if (_write >= _dataSize)
        return true;
    
    _msg->data[_write++] = byte;

    return _write >= _dataSize;
}

bool Message::isValid()
{
    if (!_valid)
        _valid = _msg && _write >= _dataSize && checksum32(_msg->data, _dataSize) == _msg->header.data.datacs;
    return _valid;
}

int Message::currentAddress()
{
    if (!isValid())
        return -1;
    else if (_msg->header.data.addrlen == 0)
        return 0;
    return _msg->data[_dataSize - 1];
}

void Message::popAddress()
{
    if (!isValid() || _msg->header.data.addrlen == 0)
        return;
    
    _write--;
    _dataSize--;
    _msgSize--;
    _msg->header.data.addrlen -= 1;
    _msg->header.data.datacs -= _msg->data[_dataSize];
    _msg->header.cs = checksum32((uint8_t*) &_msg->header.data, sizeof(MSG::HEADER::DATA));
    _valid = false;
}

msg_t* Message::get()
{
    return _msg;
}

uint8_t* Message::getData()
{
    if (_msg)
        return _msg->data;
    return NULL;
}

unsigned Message::size()
{
    return _msgSize;
}

MessageBroker::MessageBroker()
{

}

void MessageBroker::send(const Message& msg)
{
    _sendq.emplace(msg);
}

Message& MessageBroker::peek()
{
    return _recvq.front();
}

void MessageBroker::pop()
{
    if (!_recvq.size())
        return;
    _recvq.front().free();
    _recvq.pop();
}

void MessageBroker::_comm_in(uint8_t byte)
{
    if (_recvmsg.insertData(byte))
    {
        if (_recvmsg.isValid())
            _recvq.push(_recvmsg);
        _recvmsg = Message();
    }
}

Message* MessageBroker::_comm_out_peek()
{
    if (_sendq.size())
        return &_sendq.front();
    return NULL;
}

void MessageBroker::_comm_out_pop()
{
    if (!_sendq.size())
        return;
    _sendq.front().free();
    _sendq.pop();
}
