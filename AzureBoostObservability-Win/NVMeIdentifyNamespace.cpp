// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <stdbool.h>
#include <stdio.h>
#include <windows.h>
#include <stdint.h>
#include <nvme.h>

#include "WinFunc.h"
#include "NVMeIdentifyNamespace.h"
#include "json.hpp"

extern bool gJsonOutput;
extern nlohmann::json gJson;

int nvmeIdentifyNamespace(const char* _strDeviceNo, HANDLE _hDevice, __out_bcount(bufferLength) PVOID buffer, ULONG bufferLength)
{
    int iResult = -1;
    ULONG returnedLength = 0;

    ULONG identifyNamespaceDataSize = sizeof(NVME_IDENTIFY_NAMESPACE_DATA);

    PSTORAGE_PROPERTY_QUERY query = NULL;
    PSTORAGE_PROTOCOL_SPECIFIC_DATA protocolData = NULL;
    PSTORAGE_PROTOCOL_DATA_DESCRIPTOR protocolDataDescr = NULL;

    ULONGLONG namespaceSize = 0;
    UCHAR lbaFormatIndex = 0;
    UCHAR lbaDataSize = 0;
    ULONG sectorSize = 0;
    double diskSizeGiB = 0.0;
    char nguidStr[64] = { 0 };
    bool hasNguid = false;

    ZeroMemory(buffer, bufferLength);

    query = (PSTORAGE_PROPERTY_QUERY)buffer;
    protocolDataDescr = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR)buffer;
    protocolData = (PSTORAGE_PROTOCOL_SPECIFIC_DATA)query->AdditionalParameters;

    query->PropertyId = StorageDeviceProtocolSpecificProperty;
    query->QueryType = PropertyStandardQuery;

    protocolData->ProtocolType = ProtocolTypeNvme;
    protocolData->DataType = NVMeDataTypeIdentify;
    protocolData->ProtocolDataRequestValue = NVME_IDENTIFY_CNS_SPECIFIC_NAMESPACE;
    protocolData->ProtocolDataRequestSubValue = 0;
    protocolData->ProtocolDataRequestSubValue2 = 0;
    protocolData->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
    protocolData->ProtocolDataLength = sizeof(NVME_IDENTIFY_NAMESPACE_DATA);

    // Fill buffer with dummy data
    PNVME_IDENTIFY_NAMESPACE_DATA identifyNamespaceData = (PNVME_IDENTIFY_NAMESPACE_DATA)((PCHAR)protocolData + protocolData->ProtocolDataOffset);
    memset(identifyNamespaceData, 0x0, sizeof(NVME_IDENTIFY_NAMESPACE_DATA));

    printStdout("Retrieving Identify Namespace data for disk %s\n",
           _strDeviceNo);
    // Send request down.
    iResult = DeviceIoControl(_hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
        buffer, bufferLength, buffer, bufferLength,
        &returnedLength, NULL);

    if (!iResult) 
    {
        printSystemError(GetLastError(), "DeviceIoControl");
        goto error_exit;
    }

    // Validate the returned data.
    if ((protocolDataDescr->Version !=
        sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR)) ||
        (protocolDataDescr->Size != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR)))
    {
        printStderr("[E] NVMeGetPerformanceStatisticsLogPage: Data descriptor header not "
            "valid.\n");
        iResult = -1;
        goto error_exit;
    }

    protocolData = &protocolDataDescr->ProtocolSpecificData;

    if ((protocolData->ProtocolDataOffset <
        sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA)) ||
        (protocolData->ProtocolDataLength < identifyNamespaceDataSize))
    {
        printStderr("[E] NVMeGetPerformanceStatisticsLogPage: ProtocolData Offset/Length not "
            "valid.\n");
        iResult = -1;
        goto error_exit;
    }

    identifyNamespaceData = (PNVME_IDENTIFY_NAMESPACE_DATA)((PCHAR)protocolData + protocolData->ProtocolDataOffset);

    // Extract namespace information
    namespaceSize = identifyNamespaceData->NSZE;       // Namespace Size (in logical blocks)

    // Get the current LBA format index and calculate sector size
    lbaFormatIndex = identifyNamespaceData->FLBAS.LbaFormatIndex;
    lbaDataSize = identifyNamespaceData->LBAF[lbaFormatIndex].LBADS;  // Power of 2
    sectorSize = (lbaDataSize > 0) ? (1UL << lbaDataSize) : 512;      // Default to 512 if invalid

    // Calculate disk size in GiB
    diskSizeGiB = static_cast<double>(namespaceSize) * sectorSize / (1024.0 * 1024.0 * 1024.0);

    // Format NGUID (16 bytes)
    for (int i = 0; i < 16; i++) {
        if (identifyNamespaceData->NGUID[i] != 0) {
            hasNguid = true;
            break;
        }
    }
    if (hasNguid) {
        sprintf_s(nguidStr, sizeof(nguidStr),
            "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
            identifyNamespaceData->NGUID[0], identifyNamespaceData->NGUID[1],
            identifyNamespaceData->NGUID[2], identifyNamespaceData->NGUID[3],
            identifyNamespaceData->NGUID[4], identifyNamespaceData->NGUID[5],
            identifyNamespaceData->NGUID[6], identifyNamespaceData->NGUID[7],
            identifyNamespaceData->NGUID[8], identifyNamespaceData->NGUID[9],
            identifyNamespaceData->NGUID[10], identifyNamespaceData->NGUID[11],
            identifyNamespaceData->NGUID[12], identifyNamespaceData->NGUID[13],
            identifyNamespaceData->NGUID[14], identifyNamespaceData->NGUID[15]);
    }

    if (gJsonOutput)
    {
        gJson[_strDeviceNo] = {
            {"NGUID", hasNguid ? std::string(nguidStr) : "undefined"},
            {"DiskSizeGiB", diskSizeGiB},
            {"VS", ((strlen((char *)identifyNamespaceData->VS) == 0) ? "undefined" : std::string((char*)identifyNamespaceData->VS))}
        };
    }
    else
    {
        printf_s("Disk: %s\n", _strDeviceNo);
        printf_s("  NGUID: %s\n", hasNguid ? nguidStr : "undefined");
        printf_s("  Disk Size: %.2f GiB\n", diskSizeGiB);
        printf_s("  Vendor Specific: %s\n", ((strlen((char *)identifyNamespaceData->VS) == 0) ? "undefined" : (char *)identifyNamespaceData->VS));
        printf_s("\n");
    }

    iResult = 0;

error_exit:

    return iResult;
}

int nvmeGetFriendlyName(const char* _strDeviceNo, HANDLE _hDevice)
{
    int iResult = -1;

    ULONG bufferLength = offsetof(STORAGE_PROPERTY_QUERY, AdditionalParameters) + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) + sizeof(NVME_IDENTIFY_NAMESPACE_DATA);

    PVOID buffer = malloc(bufferLength);
    if (buffer == NULL)
    {
        printSystemError(GetLastError(), "malloc");
        goto error_exit;
    }

    iResult = nvmeIdentifyNamespace(_strDeviceNo, _hDevice, buffer, bufferLength);

error_exit:
    if (buffer != NULL)
    {
        free(buffer);
    }

    return iResult;
}
