#include "comm.h"

#define FNVOB   0x811c9dc5
#define FNVP    0x01000193

unsigned JJL::hash32(const uint8_t* data, unsigned len)
{
    union
    {
        struct   
        {
            uint8_t zero;
            uint8_t one;
            uint8_t two;
            uint8_t three;
        } bytes;
        unsigned value = FNVOB;   
    } hash;
    
    for (unsigned i = 0; i < len; i++)
    {
        hash.value *= FNVP;
        hash.bytes.zero ^= data[i];
    }
    return hash.value;
}

JJL::Message::Message() : _msg(NULL), _write(0), _dataSize(0), _msgSize(0), _valid(false)
{

}

JJL::Message::Message(uint16_t type, const std::vector<unsigned>& address, const uint8_t* data, unsigned len) : _valid(true)
{
    _dataSize = len + address.size();
    _msgSize = sizeof(MSG::HEADER) + _dataSize;
    _msg = (msg_t*) new uint8_t[_msgSize];
    _msg->header.data.type = type;
    _msg->header.data.addrlen = (uint16_t) address.size();
    _msg->header.data.len = len;
    std::memcpy(_msg->data, data, len);
    _write = len;
    for (auto it = address.rbegin(); it != address.rend(); ++it)
        _msg->data[_write++] = (uint8_t) *it;
    _msg->header.data.hash = hash32(_msg->data, _dataSize);
    _msg->header.hash = hash32((uint8_t*) &_msg->header.data, sizeof(MSG::HEADER::DATA));
}

void JJL::Message::free()
{
    if (_msg)
    {
        delete[] (uint8_t*) _msg;
        _msg = NULL;
    }
}

bool JJL::Message::insertData(uint8_t byte)
{
    if (!_head)
    {
        _fifo.insert(byte);
        if (hash32((uint8_t*) &_fifo.data, sizeof(MSG::HEADER::DATA)) == _fifo.hash)
        {
            _dataSize = _fifo.data.len + (unsigned) _fifo.data.addrlen;
            _msgSize = sizeof(MSG::HEADER) + _dataSize;
            _msg = (msg_t*) new uint8_t[_msgSize];
            _msg->header = _fifo;
            _write = 0;
            _head = true;
        }
        return false;
    }
    else
    {
        if (_write >= _dataSize)
        {
            if (!isValid())
            {
                free();
                _head = false;
                return false;
            }
            return true;
        }
        
        _msg->data[_write++] = byte;

        if (_write >= _dataSize)
        {
            if (!isValid())
            {
                free();
                _head = false;
                return false;
            }
            return true;
        }
        return false;   
    }
}

bool JJL::Message::isValid()
{
    if (!_valid)
        _valid = _msg && _write >= _dataSize && hash32(_msg->data, _dataSize) == _msg->header.data.hash;
    return _valid;
}

int JJL::Message::type()
{
    if (!isValid())
        return -1;
    return _msg->header.data.type;
}

int JJL::Message::currentAddress()
{
    if (!isValid() || !_msg->header.data.addrlen)
        return -1;
    return _msg->data[_dataSize - 1];
}

int JJL::Message::addressLength()
{
    if (!isValid())
        return -1;
    return _msg->header.data.addrlen;
}

void JJL::Message::popAddress()
{
    if (!isValid() || !_msg->header.data.addrlen)
        return;
    _write--;
    _dataSize--;
    _msgSize--;
    _msg->header.data.addrlen--;
    _msg->header.data.hash = hash32(_msg->data, _dataSize);
    _msg->header.hash = hash32((uint8_t*) &_msg->header.data, sizeof(MSG::HEADER::DATA));
}

uint8_t* JJL::Message::getMsg()
{
    return (uint8_t*) _msg;
}

uint8_t* JJL::Message::getData()
{
    if (_msg)
        return _msg->data;
    return NULL;
}

unsigned JJL::Message::size()
{
    return _msgSize;
}

unsigned JJL::Message::dataSize()
{
    return _dataSize;
}

JJL::MessageBroker::MessageBroker()
{

}

void JJL::MessageBroker::send(const Message& msg)
{
    _sendq.emplace(msg);
}

void JJL::MessageBroker::_comm_in(uint8_t byte)
{
    if (_recvmsg.insertData(byte))
    {
        messages.push(_recvmsg);
        _recvmsg = Message();
    }  
}

JJL::Message* JJL::MessageBroker::_comm_out_peek()
{
    if (_sendq.size())
        return &_sendq.front();
    return NULL;
}

void JJL::MessageBroker::_comm_out_pop()
{
    if (!_sendq.size())
        return;
    _sendq.front().free();
    _sendq.pop();
}

JJL::MessageHub::MessageHub(std::vector<MessageBroker>* brokers, bool isMaster, float heartbeatRate, unsigned timeout) : _brokers(brokers), _isMaster(isMaster), _timeout(timeout)
{
    _broker_data = std::vector<bd_t>(brokers->size());

    for (bd_t& bd : _broker_data)
        bd.heartbeat.setRate(heartbeatRate);
    
    _address = isMaster ? 0 : -1;
}

void JJL::MessageHub::update()
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
                    _lastMaster = sysMillis();
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
                                std::vector<unsigned> ret = std::vector<unsigned>(ping->returnPathSize);
                                for (unsigned i = 0; i < ping->returnPathSize; i++)
                                    ret[i] = ping->returnPath[i];
                                if (!ret.back())
                                    _lastMaster = sysMillis();
                                    
                                msg_reping_t reping;
                                reping.address = _address;
                                reping.id = ping->id;
                                reping.alive = sysMicros();
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
                        send(msg);
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
    }

    if (_address != 0 && _address != -1 && sysMillis() - _lastMaster >= _timeout)
        _address = -1;
}

bool JJL::MessageHub::send(Message& msg)
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
    msg.free();
    return false;
}

void JJL::MessageHub::sendBroker(Message& msg, unsigned broker)
{
    if (broker >= _brokers->size())
    {
        msg.free();
        return;
    }
    (*_brokers)[broker].send(msg);
}