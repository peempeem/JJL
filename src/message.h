#pragma once

#include <stdint.h>

enum MESSAGES
{
    CONF_NODE,
    NODE_DC,
    HEARTBEAT,
    SET_MASTER_ADDR,
    SET_LIGHTS,
    PING
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

typedef struct __attribute__((packed)) RGB8
{
    uint8_t r, g, b;

    RGB8()
    {

    }

    RGB8(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b)
    {

    }
} rgb8_t;

typedef struct MSG_SET_LIGHTS
{
    uint8_t size;
    rgb8_t data[];
} msg_set_lights_t;

typedef struct MSG_PING
{
    uint8_t size;
    uint8_t returnAddress[];
} msg_ping_t;
