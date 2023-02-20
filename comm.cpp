#include "comm.h"
#include "time.h"

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

Message::Message(uint8_t type, const std::vector<int>& address, const uint8_t* data, uint16_t datalen) : _valid(true)
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
        _msg->data[_write++] = (uint8_t) *it;
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

int Message::type()
{
    if (!isValid())
        return -1;
    return _msg->header.data.type;
}

int Message::currentAddress()
{
    if (!isValid())
        return -1;
    else if (_msg->header.data.addrlen == 0)
        return 0;
    return _msg->data[_dataSize - 1];
}

int Message::addressLength()
{
    if (!isValid())
        return -1;
    return _msg->header.data.addrlen;
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

unsigned Message::dataSize()
{
    return _dataSize;
}

MessageBroker::MessageBroker()
{

}

void MessageBroker::send(const Message& msg)
{
    _sendq.emplace(msg);
}

void MessageBroker::_comm_in(uint8_t byte)
{
    if (_recvmsg.insertData(byte))
    {
        if (_recvmsg.isValid())
            messages.push(_recvmsg);
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

MessageHub::MessageHub(std::vector<MessageBroker>* brokers, bool isMaster, float heartbeatRate, float identifyRate) : _brokers(brokers)
{
    _broker_data = std::vector<bd_t>(brokers->size());

    for (bd_t& bd : _broker_data)
    {
        bd.heartbeat.setRate(heartbeatRate);
        bd.identify.setRate(identifyRate);
    }
    
    if (isMaster)
    {
        _address = 1;
        _active = false;
    }
    else
        _active = true;
}

void MessageHub::update()
{
    for (unsigned i = 0; i < _brokers->size(); i++)
    {
        MessageBroker& broker = (*_brokers)[i];

        if (broker.messages.empty())
            continue;
        
        Message& msg = broker.messages.front();

        if (!_active)
        {
            if (msg.type() == MESSAGES::SET_MASTER_ADDR)
            {
                msg_set_master_addr_t* data = (msg_set_master_addr_t*) msg.getData();
                _address = data->address;
                _masterPath.clear();
                _masterPath.reserve(data->pathSize);
                for (unsigned i = 0; i < data->pathSize; i++)
                    _masterPath[i] = data->path[i];
                _active = true;
            }

            msg.free();
        }
        else
        {
            _broker_data[i].last = sysMillis();
            _broker_data[i].heartbeat.reset();

            if (msg.type() == MESSAGES::HEARTBEAT)
            {
                int address = ((msg_heartbeat_t*) msg.getData())->address;
                if (address == 0)
                {
                    msg_confg_node_t msgData;
                    msgData.address = _address;
                    msgData.broker = i;
                    send(MESSAGES::CONF_NODE, _masterPath, (uint8_t*) &msgData, sizeof(msgData));
                }
                else
                    _broker_data[i].connectedAddress = address;
                msg.free();
            }
            else if (msg.currentAddress() == _address)
            {
                if (msg.addressLength() == 1)
                {
                    // message for me
                }
                else
                {
                    msg.popAddress();
                    bool found = false;
                    for (unsigned j = 0; j < _brokers->size(); j++)
                    {
                        if (_broker_data[j].connectedAddress == msg.currentAddress())
                        {
                            (*_brokers)[j].send(msg); 
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        msg.free();
                }
                
            }
            else
                msg.free();
        }

        broker.messages.pop();
    }
}