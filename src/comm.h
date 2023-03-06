#pragma once
#include <stdint.h>
#include <vector>
#include <queue>
#include <cstring>
#include "buffer.h"
#include "message.h"
#include "time.h"

unsigned hash32(const uint8_t* data, unsigned len);

class Message
{
    public:
        Message();
        Message(uint16_t type, const std::vector<unsigned>& address, const uint8_t* data, unsigned len);

        void free();

        bool insertData(uint8_t byte);
        bool isValid();

        int type();
        int currentAddress();
        int addressLength();
        void popAddress();

        uint8_t* getMsg();
        unsigned size();

        uint8_t* getData();
        unsigned dataSize();

    private:
        typedef struct MSG
        {
            typedef struct HEADER
            {
                typedef struct DATA
                {
                    uint16_t type = 0;
                    uint16_t addrlen = 0;
                    unsigned len = 0;
                    unsigned hash = 0;
                } msghd_t;

                msghd_t     data;
                unsigned    hash = 0;
            } msgh_t;

            msgh_t      header;
            uint8_t     data[];
        } msg_t;

        msg_t*      _msg;
        bool        _head = false;
        unsigned    _write;
        unsigned    _dataSize;
        unsigned    _msgSize;
        bool        _valid;
        CFIFO<MSG::HEADER> _fifo = CFIFO<MSG::HEADER>();  
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
        std::queue<Message> _sendq = std::queue<Message>();
        Message _recvmsg = Message();
};

class MessageHub
{
    public:
        std::queue<Message> messages;

        MessageHub(std::vector<MessageBroker>* brokers, bool isMaster=false, float heartbeatRate=4, unsigned timeout=3000);

        void update();

        bool send(Message& msg);
        void sendBroker(Message& msg, unsigned broker);

    private:
        typedef struct BROKER_DATA
        {
            int connectedAddress = -1;
            int broker = -1;
            unsigned last = 0;
            Rate heartbeat;
        } bd_t;

        std::vector<MessageBroker>* _brokers;
        std::vector<unsigned> _masterPath;
        std::vector<bd_t> _broker_data;

        bool _isMaster;
        int _address;
        unsigned _timeout;
        unsigned _lastMaster;
};