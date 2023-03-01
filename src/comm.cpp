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

Message::Message(uint8_t type, const std::vector<unsigned>& address, const uint8_t* data, uint16_t datalen) : _valid(true)
{
    _dataSize = datalen + address.size();
    _msgSize = sizeof(MSG::HEADER) + _dataSize;
    _msg = (msg_t*) new uint8_t[_msgSize];
    _msg->header.data.type = type;
    _msg->header.data.addrlen = (uint8_t) address.size();
    _msg->header.data.datalen = datalen;
    std::memcpy(_msg->data, data, datalen);
    _write = datalen;
    for (auto it = address.rbegin(); it != address.rend(); ++it)
        _msg->data[_write++] = (uint8_t) *it;
    _msg->header.data.datacs = checksum32(_msg->data, _dataSize);
    _msg->header.cs = checksum32((uint8_t*) &_msg->header.data, sizeof(MSG::HEADER::DATA));
}

void Message::free()
{
    if (_msg)
    {
        delete[] (uint8_t*) _msg;
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

void Message::copy(Message& msg)
{
    msg = *this;
    msg_t* __msg = (msg_t*) new uint8_t[_msgSize];
    memcpy(__msg, _msg, _msgSize);
    _msg = __msg;
}

int Message::currentAddress()
{
    if (!isValid() || !_msg->header.data.addrlen)
        return -1;
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
    if (!isValid() || !_msg->header.data.addrlen)
        return;
    
    _write--;
    _dataSize--;
    _msgSize--;
    _msg->header.data.addrlen -= 1;
    _msg->header.data.datacs -= _msg->data[_dataSize];
    _msg->header.cs = checksum32((uint8_t*) &_msg->header.data, sizeof(MSG::HEADER::DATA));
    _valid = false;
}

msg_t* Message::getMsg()
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

MessageHub::MessageHub(std::vector<MessageBroker>* brokers, bool isMaster, float heartbeatRate, unsigned timeout) : _brokers(brokers), _isMaster(isMaster), _timeout(timeout)
{
    _broker_data = std::vector<bd_t>(brokers->size());

    for (bd_t& bd : _broker_data)
    {
        bd.heartbeat.setRate(heartbeatRate);
    }
    
    if (isMaster)
        _address = 0;
    else
        _address = -1;
}

void MessageHub::update()
{
    for (unsigned i = 0; i < _brokers->size(); i++)
    {
        MessageBroker& broker = (*_brokers)[i];

        while (!broker.messages.empty())
        {
            Message& msg = broker.messages.front();

            _broker_data[i].last = sysMillis();

            if (_address == -1)
            {
                if (msg.type() == MESSAGES::SET_MASTER_ADDR)
                {
                    msg_set_master_addr_t* data = (msg_set_master_addr_t*) msg.getData();
                    _address = data->address;
                    _masterPath = std::vector<unsigned>(data->pathSize);
                    for (unsigned i = 0; i < (unsigned) data->pathSize; i++)
                        _masterPath[i] = data->path[i];
                }
                msg.free();
            }
            else
            {
                if (msg.type() == MESSAGES::HEARTBEAT)
                {
                    msg_heartbeat_t* data = (msg_heartbeat_t*) msg.getData();
                    if (!data->isSetup)
                    {
                        msg_connection_t conn;
                        conn.address0 = _address;
                        conn.broker0 = i;
                        conn.address1 = data->address;
                        conn.broker1 = data->broker;
                        conn.isSetup = data->isSetup;
                        Message conf = Message(MESSAGES::CONF_NODE, _masterPath, (uint8_t*) &conn, sizeof(msg_connection_t));
                        send(conf);
                    }
                    else
                    {
                        _broker_data[i].connectedAddress = data->address;
                        _broker_data[i].broker = data->broker;
                    }
                    msg.free();
                }
                else if (msg.currentAddress() == _address)
                {
                    if (msg.addressLength() == 1)
                    {
                        switch (msg.type())
                        {
                            case MESSAGES::SET_MASTER_ADDR:
                            {
                                sendBroker(msg, ((msg_set_master_addr_t*) msg.getData())->broker);
                                break;
                            }
                            
                            case MESSAGES::PING:
                            {
                                msg_ping_t* ping = (msg_ping_t*) msg.getData();
                                std::vector<unsigned> ret = std::vector<unsigned>(ping->size);
                                for (unsigned i = 0; i < ping->size; i++)
                                    ret[i] = ping->returnAddress[i];
                                msg_reping_t reping;
                                reping.address = _address;
                                reping.id = ping->id;
                                reping.alive = sysMillis();
                                Message smsg = Message(MESSAGES::REPING, ret, (uint8_t*) &reping, sizeof(reping));
                                send(smsg);
                                msg.free();
                                break;
                            }

                            default:
                                messages.push(msg);
                        }   
                    }
                    else
                    {
                        msg.popAddress();
                        if (!send(msg))
                            msg.free();
                    }
                }
                else
                    msg.free();
            }
            broker.messages.pop();
        }

        if (_broker_data[i].heartbeat.isReady())
        {
            msg_heartbeat_t data;
            data.isSetup = _address != -1;
            data.address = _address;
            data.broker = i;
            Message msg = Message(MESSAGES::HEARTBEAT, std::vector<unsigned>(), (uint8_t*) &data, sizeof(msg_heartbeat_t));
            sendBroker(msg, i);
        }

        if (_address != -1 && _broker_data[i].connectedAddress != -1 && sysMillis() - _broker_data[i].last >= _timeout)
        {
            msg_connection_t conn;
            conn.address0 = _address;
            conn.address1 = _broker_data[i].connectedAddress;
            conn.broker0 = i;
            conn.broker1 = _broker_data[i].broker;
            conn.isSetup = true;
            _broker_data[i].connectedAddress = -1;
            Message smsg = Message(MESSAGES::NODE_DC, _masterPath, (uint8_t*) &conn, sizeof(msg_connection_t));
            send(smsg);
        }
    }
}

bool MessageHub::send(Message& msg)
{
    if (msg.currentAddress() == -1)
    {
        messages.push(msg);
        return true;
    }
    for (unsigned j = 0; j < _brokers->size(); j++)
    {
        if (_broker_data[j].connectedAddress == msg.currentAddress())
        {
            (*_brokers)[j].send(msg); 
            return true;
        }
    }
    return false;
}

void MessageHub::sendBroker(Message& msg, unsigned broker)
{
    if (broker >= _brokers->size())
    {
        msg.free();
        return;
    }
    (*_brokers)[broker].send(msg);
}