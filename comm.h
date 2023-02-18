#pragma once

#include <stdint.h>
#include <vector>

uint16_t checksum16(uint8_t* data, unsigned len);

typedef struct MSG_HEADER
{
    uint8_t     type;
    uint8_t     addrlen;
    uint16_t    datalen;
    uint16_t    dcs;
    uint16_t    hcs;
} msgh_t;

typedef struct MSG
{
    msgh_t  header;
    uint8_t data[];
} msg_t;

class Message
{   
    public:
        Message();
        Message(uint8_t type, std::vector<uint8_t> address, const uint8_t* data, uint16_t datalen);

        bool setHeader(const msgh_t* header);
        bool insertData(uint8_t byte);
        bool popCurrentAddress();
        
        uint8_t getCurrentAddress();
        uint8_t* getData() { return _msg ? _msg->data : NULL; }
        msg_t* getMsg() { return _msg; }
        unsigned msgSize() { return _msgsize; }

    private:
        msg_t* _msg;
        unsigned _write;
        unsigned _msgsize;
};
