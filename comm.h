#pragma once

#include <stdint.h>
#include <vector>
#include <queue>
#include <cstring>
#include "buffer.hpp"
#include "message.h"

uint32_t checksum32(const uint8_t* data, unsigned len);

typedef struct MSG
{
    typedef struct HEADER
    {
        typedef struct DATA
        {
            uint8_t     type;
            uint8_t     addrlen;
            uint16_t    datalen;
            uint32_t    datacs;
        } msghd_t;

        msghd_t     data;
        uint32_t    cs;
    } msgh_t;

    msgh_t      header;
    uint8_t     data[];
} msg_t;

class Message
{
    public:
        Message();
        Message(uint8_t type, const std::vector<int>& address, const uint8_t* data, uint16_t datalen);

        void free();

        bool insertData(uint8_t byte);
        bool isValid();

        int type();

        int currentAddress();
        int addressLength();
        void popAddress();

        msg_t* get();
        uint8_t* getData();
        unsigned size();
        unsigned dataSize();

    private:
        msg_t* _msg;
        bool _head = false;
        Buffer<sizeof(MSG::HEADER)> _headBuf;   

        unsigned _write = 0;
        unsigned _dataSize = 0;
        unsigned _msgSize = 0;
        bool     _valid;
};

class MessageBroker
{
    public:
        std::queue<Message> messages;

        MessageBroker();

        void send(const Message& msg);

        void _comm_in(uint8_t byte);
        Message* _comm_out_peek();
        void _comm_out_pop();
    
    private:
        std::queue<Message> _sendq;
        Message _recvmsg;
};

class MessageHub
{
    public:
        std::queue<Message> messages;

        MessageHub(std::vector<MessageBroker>* brokers, bool isMaster=false, float heartbeatRate=2.0f, float identifyRate=0.5f);

        void update();

        void send(const Message& msg);
        void send(uint8_t type, const std::vector<int>& address, const uint8_t* data, uint16_t datalen);

    private:
        typedef struct BROKER_DATA
        {
            int connectedAddress = -1;
            unsigned last = 0;
            Rate heartbeat;
            Rate identify;
        } bd_t;

        std::vector<MessageBroker>* _brokers;
        std::vector<int> _masterPath;
        std::vector<bd_t> _broker_data;

        bool _active = false;
        int _address;        
};