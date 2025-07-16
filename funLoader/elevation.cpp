#include "elevation.h"
#include <Windows.h>
#include <shellapi.h>
#include <string>

bool EnsureElevated() {
    BOOL isElevated = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &cbSize)) {
            isElevated = elevation.TokenIsElevated;
        }
    }
    if (hToken) {
        CloseHandle(hToken);
    }

    if (!isElevated) {
        wchar_t currentExePath[MAX_PATH];
        GetModuleFileNameW(NULL, currentExePath, MAX_PATH);
        ShellExecuteW(NULL, L"runas", currentExePath, NULL, NULL, SW_NORMAL);
        exit(0);
    }

    return true;
}
