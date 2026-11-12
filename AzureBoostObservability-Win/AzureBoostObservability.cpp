// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <stdint.h>
#include <stdio.h>
#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>

#include "PerformanceStatistics.h"
#include "NVMeGetLogPage.h"
#include "NVMeIdentifyNamespace.h"
#include "WinFunc.h"

#include "json.hpp"

#define MAX_DISKS_TO_QUERY  32

#pragma warning(disable : 6301)

char* gProgramName = NULL;
bool gVerbose = false;
bool gJsonOutput = false;

nlohmann::json gJson;

static void printUsage()
{
    const char* cPtr = gProgramName;

    if (cPtr == NULL)
    {
        cPtr = "Azure-Boost-Observability.exe";
    }

    // cPtr holds the last occurrence of '\'
    printf_s("%s: Sample Utility tool for NVMe drive (version %s)\n\n", cPtr, VERSION);
    printf_s("Usage: %s [/json] [/verbose] <command> [<physical-drive-no>|<drive-letter> <namespace-id>]\n\n", cPtr);
    printf_s("Commands:\n");
    printf_s("  /perf          - Get performance statistics log page\n");
    printf_s("  /identify      - Get friendly name of the NVMe namespace\n\n");

    printf_s("Options:\n");
    printf_s("  /verbose       - Enable verbose output\n");
    printf_s("  /json          - Get output in JSON format\n\n");
    
    printf_s("Arguments:\n");
    printf_s("  <physical-drive-no>: physical number of drive to be accessed\n");
    printf_s("  or...\n");
    printf_s("  <drive-letter>: letter of the drive to be accessed\n");
    printf_s("    (you can be confirm it with \"Disk Management\" in Control Panel)\n\n");

    printf_s("If <physical-drive-no> or <drive-letter> is not specified, all NVMe drives\n");
    printf_s("in the system will be queried.\n\n");
}

typedef int (*PCALLBACK)(const char* _strDeviceNo, HANDLE _hDevice);

int processDevice(const char* strDevice, PCALLBACK pCallback)
{
    int iResult = -1;

    printStdout("\n[I] Target device: %s\n", strDevice);

    HANDLE hDevice = createFileHandle((const char*)(strDevice));
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        printSystemError(GetLastError(), "createFileHandle");
        return iResult;
    }

    iResult = pCallback(strDevice, hDevice);
    if (iResult)
    {
        printSystemError(GetLastError(), "_pCallback");
    }    

    if (hDevice && hDevice != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hDevice);
    }

    return iResult;
}

int processAllDevices(PCALLBACK pCallback)
{
    int iResult = -1;

    printStdout("[I] Target device: All NVMe devices\n");

    // The user wants to get statistics for all disks
    int driveNumber = 0;

    while (driveNumber < MAX_DISKS_TO_QUERY)
    {
        char buffer[10];
        sprintf_s(buffer, sizeof(buffer), "%d", driveNumber);

        // Perform query
        iResult = processDevice(buffer, pCallback);
        if (iResult)
        {
            printSystemError(GetLastError(), "pCallback");
            // set success if we processed at least one disk
            if (driveNumber > 0)
            {
                iResult = 0;
            }
            break;
        }

        driveNumber++;
    }
    // If we couldn't process beyond the first disk, report usage
    if (GetLastError() && driveNumber == 1)
    {
        printStderr("[E] Failed to process disk %d, stopping execution\n", driveNumber - 1);
        printUsage();
    }

    return iResult;
}

int main(_In_ int _argc, _In_reads_(_argc) char* _argv[])
{
    int iResult = -1;
    char** processedArgs = &_argv[1];
    _argc--;

    // Convert to just the executable name
    // Find the last occurrence of '\'
    char* tmpPtr = strchr(_argv[0], '\\');
    while (tmpPtr != NULL)
    {
        // Move past the current backslash
        gProgramName = tmpPtr + 1;
        // Search from the new position
        tmpPtr = strchr(gProgramName, '\\');
    }

    while (processedArgs[0] != NULL && _argc > 1)
    {
        if (_stricmp(processedArgs[0], "/verbose") == 0)
        {
            gVerbose = true;
            printStdout("[I] Verbose output enabled\n");
        }
        else if (_stricmp(processedArgs[0], "/json") == 0)
        {
            gJsonOutput = true;
            printStdout("[I] JSON output enabled\n");
        }
        else
        {
            break;
        }

        processedArgs++;
        _argc--;
    }

    printStdout("[I] nvmetool-win version %s\n\n", VERSION);
    getOsVersion();

    if (_argc < 1 || processedArgs[0] == NULL)
    {
        printUsage();
        return -1;
    }

    printStdout("[I] Running command: %s\n", processedArgs[0]);
    if (_stricmp(processedArgs[0], "/perf") == 0 && _argc >= 1)
    {
        if (_argc == 1)
        {
            iResult = processAllDevices(nvmeGetLogPage);
        }
        else if (_argc >= 2)
        {
            iResult = processDevice((const char *)(processedArgs[1]), nvmeGetLogPage);
        }
    }
    else if (_stricmp(processedArgs[0], "/identify") == 0 && _argc >= 1)
    {
        if (_argc == 1)
        {
            iResult = processAllDevices(nvmeGetFriendlyName);
        }
        else if (_argc >= 2)
        {
            iResult = processDevice((const char *)(processedArgs[1]), nvmeGetFriendlyName);
        }
    }
    else if (_stricmp(processedArgs[0], "/?") == 0 || _stricmp(processedArgs[0], "/help") == 0)
    {
        printUsage();
        return 0;
    }

    if (gJsonOutput && !gJson.is_null() && !gJson.empty())
    {
        printf_s("%s\n", gJson.dump(4).c_str());
    }

    return iResult;
}
