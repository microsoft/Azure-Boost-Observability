// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <stdbool.h>
#include <stdio.h>
#include <windows.h>

#include "WinFunc.h"
#include "PerformanceStatistics.h"

#include "json.hpp"

extern bool gJsonOutput;
extern nlohmann::json gJson;

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
