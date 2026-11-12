// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <stdbool.h>
#include <windows.h>

void printDiskIdentity(HANDLE hDevice);
void getOsVersion(void);
void printSystemError(unsigned long _ulErrorCode, const char* _strFunc);
void printStdout(_Printf_format_string_ const char* _strFormat, ...);
void printStderr(_Printf_format_string_ const char* _strFormat, ...);

HANDLE createFileHandle(const char* _strDeviceNo);
