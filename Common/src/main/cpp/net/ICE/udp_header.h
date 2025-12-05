#pragma once
#include <stdint.h>
enum Command_t {
    DATA=0,
    ASK,
    SHUTDOWN,
    START,
    END
    };
struct udp_header {
    uint32_t rel_msec:25;
    Command_t com:3;
    bool side:1;
    bool fin:1;
    bool ack:1;
    uint16_t trans_id;
    uint16_t index;
    };

