#pragma once
#include <stdint.h>

typedef struct __attribute__((packed)) {
    int8_t heat;
    uint8_t climate_temp;
    uint8_t climate_hum;
    uint8_t rumor;
    uint8_t daylight;
    uint8_t presence;
    // PIR
} sensors_t;

typedef struct __attribute__((packed)) // Status
{
    uint32_t devicetime;
    sensors_t sensors;
    uint8_t fan;
    uint8_t source;
    uint8_t charge_status;
    uint8_t charge;
    uint8_t memory;
    uint8_t link;
} dev_stat_t;

typedef struct __attribute__((packed)) // Status
{
    uint32_t devicetime;
    uint8_t id;
    uint8_t value;

} log_record_t;