#include "connector.h"
#include <Windows.h>
#include <tlhelp32.h>
#include <winternl.h>

int findPID() {
    HANDLE snapshot;
    PROCESSENTRY32 processEntry;
    int pid = 0;
    BOOL result;

    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    processEntry.dwSize = sizeof(PROCESSENTRY32);

    result = Process32First(snapshot, &processEntry);

    while (result) {
        if (wcscmp(processEntry.szExeFile, L"notepad.exe") == 0) {
            pid = processEntry.th32ProcessID;
            break;
        }
        result = Process32Next(snapshot, &processEntry);
    }

    CloseHandle(snapshot);
    return pid;
}
