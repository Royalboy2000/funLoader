#pragma once

#include <Windows.h>
#include <stdint.h>

// CRC32 hashes for API functions
#define CRC32_CREATEPROCESSW 0x16b32446
#define CRC32_OPENPROCESS 0x3DEE4534
#define CRC32_NTALLOCATEVIRTUALMEMORY 0xf5bd3734
#define CRC32_NTWRITEVIRTUALMEMORY 0x5a344378
#define CRC32_NTCREATETHREADEX 0xaf28d289
#define CRC32_NTCLOSE 0x23af593a
#define CRC32_GLOBALMEMORYSTATUSEX 0x8b7c4b8e
#define CRC32_CREATEFILEW 0x7c7ea48
#define CRC32_DEVICEIOCONTROL 0x2f5a914c

// Structure to hold the resolved function pointers
typedef struct _API_FUNCTIONS {
    FARPROC pCreateProcessW;
    FARPROC pOpenProcess;
    FARPROC pNtAllocateVirtualMemory;
    FARPROC pNtWriteVirtualMemory;
    FARPROC pNtCreateThreadEx;
    FARPROC pNtClose;
    FARPROC pGlobalMemoryStatusEx;
    FARPROC pCreateFileW;
    FARPROC pDeviceIoControl;
} API_FUNCTIONS, *PAPI_FUNCTIONS;
