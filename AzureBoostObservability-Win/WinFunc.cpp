// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <stdbool.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <string.h>

#include "WinFunc.h"

extern bool gVerbose;

void printDiskIdentity(HANDLE hDevice)
{
    STORAGE_PROPERTY_QUERY query;
    DWORD bytesReturned;
    BYTE buffer[1024];

    ZeroMemory(&query, sizeof(query));
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    if (DeviceIoControl(
            hDevice,
            IOCTL_STORAGE_QUERY_PROPERTY,
            &query,
            sizeof(query),
            &buffer,
            sizeof(buffer),
            &bytesReturned,
            NULL))
    {
        const STORAGE_DEVICE_DESCRIPTOR* desc = (STORAGE_DEVICE_DESCRIPTOR*)buffer;
        const char* serial = desc->SerialNumberOffset ? (char*)buffer + desc->SerialNumberOffset : "N/A";
        const char* model = desc->ProductIdOffset ? (char*)buffer + desc->ProductIdOffset : "N/A";
        printf_s("Serial: %s\n  Model: %s\n", serial, model);
    }
    else
    {
        printStderr("[E] Could not query device identity. Error: %lu\n", GetLastError());
    }
}

void printSystemError(unsigned long _ulErrorCode, const char* _strFunc)
{
    WCHAR lpMsgBuf[1024];

    #pragma warning(push)
    #pragma warning(disable: 25028)
    DWORD count = FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        _ulErrorCode,
        0x0409,
        lpMsgBuf,
        ARRAYSIZE(lpMsgBuf),
        NULL);
    #pragma warning(pop)

    if (count != 0)
    {
        printStderr("[E] %s: (error code = %d) %ls", _strFunc,
                _ulErrorCode, lpMsgBuf);
    }
    else
    {
        printStderr("[E] Format message failed.  Error: %d\n",
                GetLastError());
    }
}

HANDLE createFileHandle(const char* _strDeviceNo)
{
    HANDLE hDevice;
    TCHAR strDeviceName[256];

    // Try to open the disk via the drive letter
    _stprintf_s(strDeviceName, 256, _T("\\\\.\\%hs:"), _strDeviceNo);
    hDevice = CreateFile(strDeviceName,
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL);

    if (hDevice == INVALID_HANDLE_VALUE)
    {
        printStderr("[E] Failed opening %S err: %d\n", strDeviceName, GetLastError());
    }
    else
    {
        printStdout("[I] Successfully opened %S\n", strDeviceName);
        return hDevice;
    }

    // Attempt to open the drive via the disk number instead.
    _stprintf_s(strDeviceName, 256, _T("\\\\.\\PhysicalDrive%hs"), _strDeviceNo);
    hDevice = CreateFile((LPCWSTR)strDeviceName,
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL);

    // If we still haven't been able to open the disk, we report a failure
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        printStderr("[E] Failed opening %S err: %d\n", strDeviceName, GetLastError());
        return hDevice;
    }
    printStdout("[I] Successfully opened %S\n", strDeviceName);

    return hDevice;
}

static DWORD s_dwOSBuildNumber = 0;
static DWORD s_dwOSMajorVersion = 0;

typedef NTSTATUS(WINAPI* PRtlGetVersion)(PRTL_OSVERSIONINFOEXW _pInfo);

void getOsVersion(void)
{
    RTL_OSVERSIONINFOEXW osInfo;
    ZeroMemory(&osInfo, sizeof(RTL_OSVERSIONINFOEXW));
    osInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

    HMODULE mod = GetModuleHandle(_T("ntdll.dll"));
    if (mod == 0)
    {
        printSystemError(GetLastError(), "GetModuleHandle");
        return;
    }

    PRtlGetVersion fpGetOSVer =
        (PRtlGetVersion)GetProcAddress(mod, "RtlGetVersion");
    if (fpGetOSVer == NULL)
    {
        printSystemError(GetLastError(), "GetProcAddress");
        return;
    }

    fpGetOSVer(&osInfo);

    // checking OS version (Windows 10)
    // see also
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_osversioninfoexw
    if ((osInfo.dwMajorVersion == 10) && (osInfo.dwMinorVersion == 0) &&
        (osInfo.wProductType ==
         VER_NT_WORKSTATION))
    {  // not Windows Server 2016/2019
        s_dwOSBuildNumber = osInfo.dwBuildNumber;
        s_dwOSMajorVersion = osInfo.dwMajorVersion;
        if (22000 <= s_dwOSBuildNumber)
        {
            s_dwOSMajorVersion = 11;  // Windows 11 !!!
        }

        printStdout("[I] Running on Windows %d build %d", s_dwOSMajorVersion,
               s_dwOSBuildNumber);
    }
}

void printStdout(const char* _strFormat, ...)
{
    if (!gVerbose)
        return;

    va_list args;
    va_start(args, _strFormat);
    vfprintf(stdout, _strFormat, args);
    va_end(args);
}

void printStderr(const char* _strFormat, ...)
{
    if (!gVerbose)
        return;

    va_list args;
    va_start(args, _strFormat);
    vfprintf(stderr, _strFormat, args);
    va_end(args);
}
