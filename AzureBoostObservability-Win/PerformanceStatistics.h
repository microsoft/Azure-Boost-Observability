// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <stdint.h>
#include <stdbool.h>

#define VERSION "1.0.0"

#define NVME_LOG_PAGE_PERFORMANCE_STATISTICS   0xD1

#pragma pack (push, 8)

union NVME_PERF_STATS_LOG_PAGE {
    struct {
        union meta{
            struct {
                uint64_t    version;
                BOOLEAN     feature_available;
                BOOLEAN     is_degraded;
            } settings;
            uint8_t    reserved[0x100];
        } metadata;
        uint64_t    total_read_ops;
        uint64_t    total_write_ops;
        uint64_t    total_read_bytes;
        uint64_t    total_write_bytes;
        uint64_t    outstanding_io;
        uint64_t    read_latency_max;
        uint64_t    read_latency_avg;
        uint64_t    write_latency_max;
        uint64_t    write_latency_avg;
        uint64_t    read_dispatch_latency_max;
        uint64_t    read_dispatch_latency_avg;
        uint64_t    write_dispatch_latency_max;
        uint64_t    write_dispatch_latency_avg;
    } counters;
    uint8_t _reserved[0x1000]; // Pad to 0x1000 bytes (4096 bytes)
};

#pragma pack (pop)

static_assert(sizeof(NVME_PERF_STATS_LOG_PAGE) == 0x1000, "NVME_PERF_STATS_LOG_PAGE is not exactly 0x1000 bytes");