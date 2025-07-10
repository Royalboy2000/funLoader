#pragma once

#include "common_windows_headers.h" // Centralized headers (includes Windows.h, vector, string, map)

// Define a function pointer type for resolved functions
typedef FARPROC (WINAPI *RESOLVED_FUNC)();

namespace ApiResolver {

    // Structure to hold resolved API details
    struct ApiDetails {
        FARPROC pFunc;       // Pointer to the function
        DWORD dwHash;        // Hash of the function name
        // Add more details if needed, e.g., module handle
    };

    // Initializes the API resolver, finds and hashes exports from specified modules
    bool Initialize();

    // Gets a function pointer by its hash
    FARPROC GetProcAddressByHash(DWORD dwHash);

    // Simple CRC32 hash function for strings
    DWORD HashStringCRC32(const char* str);

    // Syscall resolution is handled by sysopen.c (SysWhispers2)
    // These functions are removed from ApiResolver to avoid conflict/redundancy.
    // DWORD GetSyscallNumberByHash(DWORD dwHash);
    // bool PopulateSyscallList();

} // namespace ApiResolver
