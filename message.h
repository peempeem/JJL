#pragma once

#include <stdint.h>

enum MESSAGES
{
    // to master messages
    CONF_NODE,
    NODE_DC,

    // node messages
    HEARTBEAT,
    SET_MASTER_ADDR,
    SET_LIGHTS
};

typedef struct CONNECTION
{
    uint8_t address;
    uint8_t broker0;
    uint8_t broker1;
} connection_t;

typedef struct MSG_SET_MASTER_ADDR
{
    uint8_t address;
    uint8_t pathSize;
    uint8_t path[];
} msg_set_master_addr_t;

typedef struct MSG_SET_LIGHTS
{
    typedef struct RGB8
    {
        uint8_t r, g, b;
    } rgb8_t;

    uint8_t size;
    rgb8_t data[];
} msg_set_lights_t;
