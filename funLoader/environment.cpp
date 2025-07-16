#include "environment.h"
#include <Windows.h>

int CheckEnvironment() {
    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof(memoryStatus);
    if (!GlobalMemoryStatusEx(&memoryStatus)) {
        return -1;
    }
    DWORD RAM = (DWORD)(memoryStatus.ullTotalPhys / 1024 / 1024);
    if (RAM < 4096) {
        return -1;
    }

    ULARGE_INTEGER freeBytesAvailableToCaller, totalNumberOfBytes, totalNumberOfFreeBytes;
    if (!GetDiskFreeSpaceExW(L"C:\\", &freeBytesAvailableToCaller, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        return -1;
    }
    DWORD diskSize = (DWORD)(totalNumberOfBytes.QuadPart / 1024 / 1024 / 1024);
    if (diskSize < 50) {
        return -1;
    }

    return 0;
}
