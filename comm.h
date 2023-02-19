#pragma once

#include <stdint.h>
#include <vector>
#include <queue>
#include <cstring>
#include "buffer.hpp"

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
        Message(uint8_t type, const std::vector<uint8_t>& address, const uint8_t* data, uint16_t datalen);

        void free();

        bool insertData(uint8_t byte);
        bool isValid();

        int currentAddress();
        void popAddress();

        msg_t* get();
        uint8_t* getData();
        unsigned size();

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
        MessageBroker();

        void send(const Message& msg);
        Message& peek();
        void pop();

        void _comm_in(uint8_t byte);
        Message* _comm_out_peek();
        void _comm_out_pop();
    
    private:
        std::queue<Message> _sendq;
        std::queue<Message> _recvq;
        Message _recvmsg;
};

class MessageHub
{
    public:
        MessageBroker broker0;
        MessageBroker broker1;
        MessageBroker broker2;

        MessageHub();

        void update();


};