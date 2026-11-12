// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "WinFunc.h"
#include "PerformanceStatistics.h"

#include "json.hpp"

extern bool gJsonOutput;
extern nlohmann::json gJson;

// Heuristically determine the format of the logpage. The legacy layout has is_degraded at byte 0 and counters starting at byte 8;
// the current layout has version at byte 0, feature_available at byte 8, is_degraded at byte 9, and counters starting at byte 256.
// We leverage the fact that legacy is_degraded and current version are overlapped as well as the legacy counters with the current is_degraded and feature_available.
bool isLogPageLegacyFormat(const NVME_PERF_STATS_LOG_PAGE& logPage, const NVME_PERF_STATS_LOG_PAGE_LEGACY& legacyLogPage)
{
    const uint64_t zeroCounters[13] = {0}; // There are 13 counters.

    const bool currentCountersAreZero = memcmp(&logPage.counters.total_read_ops, zeroCounters, sizeof(zeroCounters)) == 0;
    const bool legacyCountersAreZero = memcmp(&legacyLogPage.total_read_ops, zeroCounters, sizeof(zeroCounters)) == 0;
    // Skip total_read_ops: it overlaps the current metadata flags.
    const bool legacyCountersAreZeroAfterMetadata = memcmp(&legacyLogPage.total_write_ops, zeroCounters, sizeof(zeroCounters) - sizeof(uint64_t)) == 0;

    if (logPage.counters.metadata.settings.is_degraded == TRUE &&
        logPage.counters.metadata.settings.version == 0U &&
        logPage.counters.metadata.settings.feature_available == FALSE)
    {
        // Legacy total_read_ops == 256 overlaps the current degraded flag, but completed reads
        // also have nonzero total_read_bytes, so a valid legacy page fails the zero-counter check.
        if(currentCountersAreZero && legacyCountersAreZeroAfterMetadata)
        {
            return false; // current degraded
        }
        else
        {
            return true; // legacy non degraded
        }
    }
    // compare version instead of legacy is_degraded to take into account versions of 2 and above
    if (logPage.counters.metadata.settings.version == 0U)
    {
        return true; // legacy non degraded
    }
    if (logPage.counters.metadata.settings.version == 1U)
    {
        if (currentCountersAreZero && legacyCountersAreZero) 
        {
            return true; // legacy degraded
        }
    }
    return false;
}

void printPerfStatsLogPageLegacy(const char* _strDeviceNo, NVME_PERF_STATS_LOG_PAGE_LEGACY& perfStatsLogPage)
{
    if (gJsonOutput)
    {
        gJson[_strDeviceNo] = {
            {"outstanding_io", perfStatsLogPage.outstanding_io},
            {"is_degraded", (BOOLEAN)perfStatsLogPage.is_degraded},
            {"total_read_ops", perfStatsLogPage.total_read_ops},
            {"total_read_bytes", perfStatsLogPage.total_read_bytes},
            {"total_write_ops", perfStatsLogPage.total_write_ops},
            {"total_write_bytes", perfStatsLogPage.total_write_bytes},
            {"read_latency_max", perfStatsLogPage.bqe_read_latency_max},
            {"read_latency_avg", perfStatsLogPage.bqe_read_latency_avg},
            {"write_latency_max", perfStatsLogPage.bqe_write_latency_max},
            {"write_latency_avg", perfStatsLogPage.bqe_write_latency_avg},
            {"read_dispatch_latency_max", perfStatsLogPage.sched_read_disk_latency_max},
            {"read_dispatch_latency_avg", perfStatsLogPage.sched_read_disk_latency_avg},
            {"write_dispatch_latency_max", perfStatsLogPage.sched_write_disk_latency_max},
            {"write_dispatch_latency_avg", perfStatsLogPage.sched_write_disk_latency_avg}
        };
    }
    else
    {
        printf_s("Disk: %s\n", _strDeviceNo);
        printf_s("  LogPage Version: Legacy\n");
        printf_s("  Degraded performance: %s\n", perfStatsLogPage.is_degraded ? "Yes" : "No");
        printf_s("  Total Ops\n");
        printf_s("    Read Ops: %llu\n", perfStatsLogPage.total_read_ops);
        printf_s("    Write Ops: %llu\n", perfStatsLogPage.total_write_ops);
        printf_s("  Total Bytes\n");
        printf_s("    Read Bytes: %llu\n", perfStatsLogPage.total_read_bytes);
        printf_s("    Write Bytes: %llu\n", perfStatsLogPage.total_write_bytes);
        printf_s("  Outstanding I/Os: %llu\n", perfStatsLogPage.outstanding_io);
        printf_s("  Ops Latency Max/Avg (us)\n");
        printf_s("    Read Latency Max: %llu\n", perfStatsLogPage.bqe_read_latency_max);
        printf_s("    Read Latency Avg: %llu\n", perfStatsLogPage.bqe_read_latency_avg);
        printf_s("    Write Latency Max: %llu\n", perfStatsLogPage.bqe_write_latency_max);
        printf_s("    Write Latency Avg: %llu\n", perfStatsLogPage.bqe_write_latency_avg);
        printf_s("  Dispatch Latency Max/Avg (us)\n");
        printf_s("    Read Dispatch Latency Max: %llu\n", perfStatsLogPage.sched_read_disk_latency_max);
        printf_s("    Read Dispatch Latency Avg: %llu\n", perfStatsLogPage.sched_read_disk_latency_avg);
        printf_s("    Write Dispatch Latency Max: %llu\n", perfStatsLogPage.sched_write_disk_latency_max);
        printf_s("    Write Dispatch Latency Avg: %llu\n", perfStatsLogPage.sched_write_disk_latency_avg);
        printf_s("\n");
    }
}

void printPerfStatsLogPage(const char* _strDeviceNo, NVME_PERF_STATS_LOG_PAGE& perfStatsLogPage)
{
    if (gJsonOutput)
    {
        gJson[_strDeviceNo] = {
            {"outstanding_io", perfStatsLogPage.counters.outstanding_io},
            {"is_degraded", perfStatsLogPage.counters.metadata.settings.is_degraded},
            {"total_read_ops", perfStatsLogPage.counters.total_read_ops},
            {"total_read_bytes", perfStatsLogPage.counters.total_read_bytes},
            {"total_write_ops", perfStatsLogPage.counters.total_write_ops},
            {"total_write_bytes", perfStatsLogPage.counters.total_write_bytes},
            {"read_latency_max", perfStatsLogPage.counters.read_latency_max},
            {"read_latency_avg", perfStatsLogPage.counters.read_latency_avg},
            {"write_latency_max", perfStatsLogPage.counters.write_latency_max},
            {"write_latency_avg", perfStatsLogPage.counters.write_latency_avg},
            {"read_dispatch_latency_max", perfStatsLogPage.counters.read_dispatch_latency_max},
            {"read_dispatch_latency_avg", perfStatsLogPage.counters.read_dispatch_latency_avg},
            {"write_dispatch_latency_max", perfStatsLogPage.counters.write_dispatch_latency_max},
            {"write_dispatch_latency_avg", perfStatsLogPage.counters.write_dispatch_latency_avg}
        };
    }
    else
    {
        printf_s("Disk: %s\n", _strDeviceNo);
        printf_s("  LogPage Version: %llu\n", perfStatsLogPage.counters.metadata.settings.version);
        printf_s("  Degraded performance: %s\n", perfStatsLogPage.counters.metadata.settings.is_degraded ? "Yes" : "No");
        printf_s("  Total Ops\n");
        printf_s("    Read Ops: %llu\n", perfStatsLogPage.counters.total_read_ops);
        printf_s("    Write Ops: %llu\n", perfStatsLogPage.counters.total_write_ops);
        printf_s("  Total Bytes\n");
        printf_s("    Read Bytes: %llu\n", perfStatsLogPage.counters.total_read_bytes);
        printf_s("    Write Bytes: %llu\n", perfStatsLogPage.counters.total_write_bytes);
        printf_s("  Outstanding I/Os: %llu\n", perfStatsLogPage.counters.outstanding_io);
        printf_s("  Ops Latency Max/Avg (us)\n");
        printf_s("    Read Latency Max: %llu\n", perfStatsLogPage.counters.read_latency_max);
        printf_s("    Read Latency Avg: %llu\n", perfStatsLogPage.counters.read_latency_avg);
        printf_s("    Write Latency Max: %llu\n", perfStatsLogPage.counters.write_latency_max);
        printf_s("    Write Latency Avg: %llu\n", perfStatsLogPage.counters.write_latency_avg);
        printf_s("  Dispatch Latency Max/Avg (us)\n");
        printf_s("    Read Dispatch Latency Max: %llu\n", perfStatsLogPage.counters.read_dispatch_latency_max);
        printf_s("    Read Dispatch Latency Avg: %llu\n", perfStatsLogPage.counters.read_dispatch_latency_avg);
        printf_s("    Write Dispatch Latency Max: %llu\n", perfStatsLogPage.counters.write_dispatch_latency_max);
        printf_s("    Write Dispatch Latency Avg: %llu\n", perfStatsLogPage.counters.write_dispatch_latency_avg);
        printf_s("\n");
    }
}


int getPerfStatsLogPage(const char* _strDeviceNo, HANDLE _hDevice, __out_bcount(bufferLength) PVOID buffer, ULONG bufferLength)
{
    int iResult = -1;
    ULONG returnedLength = 0;

    ULONG perfStatsLogPageSize = sizeof(NVME_PERF_STATS_LOG_PAGE);

    PSTORAGE_PROPERTY_QUERY query = NULL;
    PSTORAGE_PROTOCOL_SPECIFIC_DATA protocolData = NULL;
    PSTORAGE_PROTOCOL_DATA_DESCRIPTOR protocolDataDescr = NULL;

    ZeroMemory(buffer, bufferLength);

    query = (PSTORAGE_PROPERTY_QUERY)buffer;
    protocolDataDescr = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR)buffer;
    protocolData = (PSTORAGE_PROTOCOL_SPECIFIC_DATA)query->AdditionalParameters;

    query->PropertyId = StorageDeviceProtocolSpecificProperty;
    query->QueryType = PropertyStandardQuery;

    protocolData->ProtocolType = ProtocolTypeNvme;
    protocolData->DataType = NVMeDataTypeLogPage;

    // Check the following page for appropriate values for "RequestValue"s.
    // STORAGE_PROTOCOL_NVME_DATA_TYPE enumeration (ntddstor.h)
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddstor/ne-ntddstor-_storage_protocol_nvme_data_type

    protocolData->ProtocolDataRequestValue = NVME_LOG_PAGE_PERFORMANCE_STATISTICS;
    // Lower and higher 32-bit offset
    protocolData->ProtocolDataRequestSubValue = 0;
    protocolData->ProtocolDataRequestSubValue2 = 0;

    protocolData->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
    protocolData->ProtocolDataLength = perfStatsLogPageSize;

    // Fill buffer with dummy data
    NVME_PERF_STATS_LOG_PAGE perfStatsLogPage = *(NVME_PERF_STATS_LOG_PAGE*)((PCHAR)protocolData + protocolData->ProtocolDataOffset);
    memset(&perfStatsLogPage, 0x0, sizeof(NVME_PERF_STATS_LOG_PAGE));

    // Send request down.
    iResult = DeviceIoControl(_hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
        buffer, bufferLength, buffer, bufferLength,
        &returnedLength, NULL);

    if (!iResult) goto error_exit;

    // Validate the returned data.
    if ((protocolDataDescr->Version !=
        sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR)) ||
        (protocolDataDescr->Size != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR)))
    {
        fprintf_s(stderr,
            "[E] NVMeGetPerformanceStatisticsLogPage: Data descriptor header not "
            "valid.\n");
        iResult = -1;
        goto error_exit;
    }

    protocolData = &protocolDataDescr->ProtocolSpecificData;

    if ((protocolData->ProtocolDataOffset <
        sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA)) ||
        (protocolData->ProtocolDataLength < perfStatsLogPageSize))
    {
        fprintf_s(stderr,
            "[E] NVMeGetPerformanceStatisticsLogPage: ProtocolData Offset/Length not "
            "valid.\n");
        iResult = -1;
        goto error_exit;
    }

    perfStatsLogPage = *(NVME_PERF_STATS_LOG_PAGE*)((PCHAR)protocolData + protocolData->ProtocolDataOffset);

    // Copy the performance statistics log page into a legacy structure for compatibility checks to avoid pointer casting.
    NVME_PERF_STATS_LOG_PAGE_LEGACY legacyLogPage;
    memcpy(&legacyLogPage, &perfStatsLogPage, sizeof(legacyLogPage));
    if (isLogPageLegacyFormat(perfStatsLogPage, legacyLogPage))
    {
        printPerfStatsLogPageLegacy(_strDeviceNo, legacyLogPage);
    }
    else
    {
        printPerfStatsLogPage(_strDeviceNo, perfStatsLogPage);
    }

    iResult = 0;

error_exit:

    return iResult;
}

int nvmeGetLogPage(const char* _strDeviceNo, HANDLE _hDevice)
{
    int iResult = -1;

    ULONG bufferLength = offsetof(STORAGE_PROPERTY_QUERY, AdditionalParameters) + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) + sizeof(NVME_PERF_STATS_LOG_PAGE);

    PVOID buffer = malloc(bufferLength);
    if (buffer == NULL)
    {
        printSystemError(GetLastError(), "malloc");
        goto error_exit;
    }

    iResult = getPerfStatsLogPage(_strDeviceNo, _hDevice, buffer, bufferLength);

error_exit:
    if (buffer != NULL)
    {
        free(buffer);
    }

    return iResult;
}
