#pragma once

#include <stdint.h>

namespace JJL
{
    enum MESSAGES
    {
        CONF_NODE,
        NODE_DC,
        HEARTBEAT,
        SET_MASTER_ADDR,
        SET_LIGHTS,
        PING,
        REPING
    };

    typedef struct MSG_CONNECTION
    {
        uint8_t address0;
        uint8_t address1;
        uint8_t broker0;
        uint8_t broker1;
        uint8_t isSetup;
    } msg_connection_t;

    typedef struct MSG_HEARTBEAT
    {
        uint8_t isSetup;
        uint8_t address;
        uint8_t broker;
    } msg_heartbeat_t;

    typedef struct MSG_SET_MASTER_ADDR
    {
        uint8_t broker;
        uint8_t address;
        uint8_t pathSize;
        uint8_t path[];
    } msg_set_master_addr_t;

    typedef struct MSG_SET_LIGHTS
    {
        typedef struct __attribute__((packed)) RGB
        {
            uint8_t r, g, b;
        } rgb_t;

        uint8_t size;
        rgb_t data[];
    } msg_set_lights_t;

    typedef struct MSG_PING
    {
        uint8_t id;
        uint8_t returnPathSize;
        uint8_t returnPath[];
    } msg_ping_t;

    typedef struct MSG_REPING
    {
        uint8_t id;
        uint8_t address;
        uint32_t alive;
    } msg_reping_t;
}
