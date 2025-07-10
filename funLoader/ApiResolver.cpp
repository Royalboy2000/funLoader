// Force system headers first to ensure correct type definitions
#include <windows.h>
#include <winternl.h> // For PEB, LDR_DATA_TABLE_ENTRY etc.

#include "ApiResolver.h"
#include <vector>
#include <algorithm> // For std::transform
#include <iostream> // For debugging, remove later

// Global map to store resolved APIs <Hash, FunctionAddress> for general Win32 functions
static std::map<DWORD, FARPROC> g_ResolvedFunctions;
// g_ResolvedSyscalls map is removed as syscall resolution is handled by sysopen.c

// Helper to get module base address from PEB
PVOID GetModuleBase(const wchar_t* moduleName) {
    PPEB peb = (PPEB)__readgsqword(0x60); // For x64. Use __readfsdword(0x30) for x86
    if (!peb || !peb->Ldr) {
        return NULL;
    }

    PLDR_DATA_TABLE_ENTRY pLdrEntry = (PLDR_DATA_TABLE_ENTRY)peb->Ldr->InMemoryOrderModuleList.Flink;
    while (pLdrEntry->DllBase != NULL) {
        std::wstring currentModuleName(pLdrEntry->BaseDllName.Buffer, pLdrEntry->BaseDllName.Length / sizeof(wchar_t));
        // Convert both to lowercase for case-insensitive comparison
        std::transform(currentModuleName.begin(), currentModuleName.end(), currentModuleName.begin(), ::towlower);
        std::wstring targetModuleName(moduleName);
        std::transform(targetModuleName.begin(), targetModuleName.end(), targetModuleName.begin(), ::towlower);

        if (currentModuleName == targetModuleName) {
            return pLdrEntry->DllBase;
        }
        pLdrEntry = (PLDR_DATA_TABLE_ENTRY)pLdrEntry->InMemoryOrderLinks.Flink;
    }
    return NULL;
}


DWORD ApiResolver::HashStringCRC32(const char* str) {
    // Basic CRC32 implementation (example, consider a more robust one if needed)
    DWORD crc = 0xFFFFFFFF;
    while (*str) {
        char ch = *str++;
        for (int i = 0; i < 8; ++i) {
            crc = (crc ^ ch) & 1 ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
            ch >>= 1;
        }
    }
    return ~crc;
}

bool ApiResolver::Initialize() {
    // Modules to resolve general Win32 functions from (excluding ntdll.dll for this resolver)
    const wchar_t* modulesToParse[] = {
        L"kernel32.dll",
        L"user32.dll",
        L"advapi32.dll" // Example: if you need functions like RegOpenKeyEx, etc.
        // Add other DLLs like shell32.dll, gdi32.dll as needed
    };

    for (const wchar_t* modName : modulesToParse) {
        // Skip ntdll.dll explicitly if it were ever added to the list by mistake
        if (wcscmp(modName, L"ntdll.dll") == 0) {
            continue;
        }

        HMODULE hMod = (HMODULE)GetModuleBase(modName);
        if (!hMod) {
            hMod = LoadLibraryW(modName);
            if (!hMod) {
                // std::cerr << "Failed to load module: " << modName << std::endl; // Debug
                continue;
            }
        }

        PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hMod;
        if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            continue;
        }

        PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hMod + pDosHeader->e_lfanew);
        if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE) {
            continue;
        }

        PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)hMod +
            pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

        if (pExportDir == nullptr || pExportDir->NumberOfNames == 0 ||
            pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size == 0) {
            continue;
        }

        PDWORD pdwAddressOfFunctions = (PDWORD)((BYTE*)hMod + pExportDir->AddressOfFunctions);
        PDWORD pdwAddressOfNames = (PDWORD)((BYTE*)hMod + pExportDir->AddressOfNames);
        PWORD pwAddressOfNameOrdinales = (PWORD)((BYTE*)hMod + pExportDir->AddressOfNameOrdinals);

        for (DWORD i = 0; i < pExportDir->NumberOfNames; ++i) {
            const char* szFuncName = (const char*)((BYTE*)hMod + pdwAddressOfNames[i]);
            FARPROC pFuncAddr = (FARPROC)((BYTE*)hMod + pdwAddressOfFunctions[pwAddressOfNameOrdinales[i]]);

            DWORD dwHash = HashStringCRC32(szFuncName);
            g_ResolvedFunctions[dwHash] = pFuncAddr;
            // std::cout << "Resolved (Win32): " << szFuncName << " Hash: 0x" << std::hex << dwHash << " Addr: 0x" << pFuncAddr << std::endl; // Debug
        }
    }

    // Syscall population is handled by sysopen.c (SysWhispers2 mechanism)
    // No longer calling PopulateSyscallList() here.
    return !g_ResolvedFunctions.empty(); // Return true if at least some functions were resolved.
}

FARPROC ApiResolver::GetProcAddressByHash(DWORD dwHash) {
    auto it = g_ResolvedFunctions.find(dwHash);
    if (it != g_ResolvedFunctions.end()) {
        return it->second;
    }
    return NULL;
}

// GetProcAddressByHash is kept for resolving general Win32 API functions.

// PopulateSyscallList and GetSyscallNumberByHash are removed.
// Syscall resolution is handled by the existing sysopen.c (SysWhispers2) mechanism.

// The text below was markdown/comments and has been removed to fix compilation errors.
// ```
// **Important Considerations for `PopulateSyscallList`:**
// ... (rest of the comment block) ...
// ```
