// Force system headers first to ensure correct type definitions
#include <windows.h>
#include <winternl.h> // Explicitly include for PEB, LDR_DATA_TABLE_ENTRY, etc.

#include "Memory.h"
#include "sysopen.h" // For NTAPI function prototypes like NtAllocateVirtualMemory, NtProtectVirtualMemory
#include <random>    // For std::mt19937 and std::uniform_int_distribution
#include <time.h>      // For seeding random number generator

// Make sure NTSTATUS SUCCESS macro is available if not already from other includes
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

// Define STATUS_SUCCESS if not available (typically from ntdef.h or similar)
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif


DWORD Memory::GetAllocGranularity() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwAllocationGranularity;
}

NTSTATUS Memory::RandomAlloc(HANDLE hProc, SIZE_T size, PVOID* pAllocatedBase, PSIZE_T pReservedRegionSizeOut, ULONG protect) {
    if (!pAllocatedBase || size == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    static bool seeded = false;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = true;
    }

    DWORD allocGranularity = GetAllocGranularity();
    SIZE_T page_size = 4096; // Common page size, or use GetSystemInfo(&si); si.dwPageSize;

    // Calculate total size to reserve: requested size + random padding before/after
    // Padding should be multiple of page_size for commit granularity, and total reserved region
    // should be multiple of allocGranularity for reservation.

    // Number of padding pages (e.g., 1 to 5 pages before and after)
    int paddingPagesBefore = (rand() % 5 + 1);
    int paddingPagesAfter = (rand() % 5 + 1);

    SIZE_T actualCommitSize = (size + page_size - 1) / page_size * page_size; // Align commit size to page boundary

    SIZE_T reservedSize = actualCommitSize +
                          (paddingPagesBefore * page_size) +
                          (paddingPagesAfter * page_size);

    // Align reservedSize up to allocation granularity
    reservedSize = (reservedSize + allocGranularity - 1) / allocGranularity * allocGranularity;

    if (pReservedRegionSizeOut) {
        *pReservedRegionSizeOut = reservedSize;
    }

    PVOID reservedBase = NULL;
    SIZE_T actualReservedSize = reservedSize; // NtAllocateVirtualMemory wants this to be a pointer to SIZE_T

    // 1. Reserve a large region of memory
    NTSTATUS status = NtAllocateVirtualMemory(
        hProc ? hProc : GetCurrentProcess(),
        &reservedBase,
        0, // ZeroBits
        &actualReservedSize, // RegionSize (will be updated to actual reserved size)
        MEM_RESERVE,
        PAGE_NOACCESS // Reserve with no access initially
    );

    if (!NT_SUCCESS(status) || !reservedBase) {
        // Failed to reserve memory
        return status;
    }

    // 2. Calculate a random offset within the reserved region for the commit
    // The commit must be page-aligned within the reserved region.
    // Max offset allows the actualCommitSize to fit.
    SIZE_T maxOffsetInPages = (actualReservedSize - actualCommitSize) / page_size;
    if (maxOffsetInPages <= paddingPagesBefore) { // Ensure we at least have the intended paddingPagesBefore
        maxOffsetInPages = paddingPagesBefore;
    }

    SIZE_T randomOffsetInPages = 0;
    if (maxOffsetInPages > 0) {
         // We want some pages before, so start random offset after paddingPagesBefore
        if (maxOffsetInPages > paddingPagesBefore) {
            randomOffsetInPages = paddingPagesBefore + (rand() % (maxOffsetInPages - paddingPagesBefore + 1));
        } else {
            randomOffsetInPages = paddingPagesBefore; // Default to just after initial padding if space is tight
        }
    }
     // Ensure randomOffsetInPages doesn't exceed total available pages minus commit pages
    if (randomOffsetInPages * page_size + actualCommitSize > actualReservedSize) {
        randomOffsetInPages = (actualReservedSize - actualCommitSize) / page_size;
    }


    PVOID commitBase = (PBYTE)reservedBase + (randomOffsetInPages * page_size);
    SIZE_T commitSizeForAlloc = actualCommitSize;


    // 3. Commit a portion of the reserved memory
    status = NtAllocateVirtualMemory(
        hProc ? hProc : GetCurrentProcess(),
        &commitBase, // This needs to be the actual base for commit
        0,           // ZeroBits
        &commitSizeForAlloc,    // RegionSize
        MEM_COMMIT,
        PAGE_READWRITE // Commit with RW first, then change protection
    );

    if (!NT_SUCCESS(status)) {
        // Failed to commit, free the reserved region
        SIZE_T sizeToFree = 0; // Must be 0 for MEM_RELEASE
        NtFreeVirtualMemory(hProc ? hProc : GetCurrentProcess(), &reservedBase, &sizeToFree, MEM_RELEASE);
        return status;
    }

    // 4. Change protection of the committed region
    ULONG oldProtect;
    status = NtProtectVirtualMemory(
        hProc ? hProc : GetCurrentProcess(),
        &commitBase,
        &commitSizeForAlloc, // Size of the region to protect (actual committed size)
        protect,       // New protection
        &oldProtect
    );

    if (!NT_SUCCESS(status)) {
        // Failed to protect, free the committed and reserved memory
        SIZE_T sizeToFree = 0;
        NtFreeVirtualMemory(hProc ? hProc : GetCurrentProcess(), &reservedBase, &sizeToFree, MEM_RELEASE); // Release the whole reserved block
        return status;
    }

    *pAllocatedBase = commitBase;
    return STATUS_SUCCESS;
}


// Helper function to get the base address of a loaded module (e.g., ntdll.dll)
PVOID GetLoadedModuleBase(const wchar_t* moduleName) {
    // This is a simplified GetModuleHandle. A more robust version would walk PEB.
    // For ntdll, kernel32, user32, they are typically loaded, but good to be robust.
    // Using __readgsqword(0x60) for PEB as in ApiResolver.cpp GetModuleBase
    #if defined(_WIN64)
        PPEB peb = (PPEB)__readgsqword(0x60);
    #else
        PPEB peb = (PPEB)__readfsdword(0x30);
    #endif

    if (!peb || !peb->Ldr) {
        return NULL;
    }

    PLDR_DATA_TABLE_ENTRY pLdrEntry = (PLDR_DATA_TABLE_ENTRY)peb->Ldr->InMemoryOrderModuleList.Flink;
    while (pLdrEntry->DllBase != NULL) {
        // Simple case-insensitive comparison
        std::wstring currentModuleName(pLdrEntry->BaseDllName.Buffer, pLdrEntry->BaseDllName.Length / sizeof(wchar_t));
        if (_wcsicmp(currentModuleName.c_str(), moduleName) == 0) {
            return pLdrEntry->DllBase;
        }
        pLdrEntry = (PLDR_DATA_TABLE_ENTRY)pLdrEntry->InMemoryOrderLinks.Flink;
        if ((PVOID)pLdrEntry == (PVOID)&peb->Ldr->InMemoryOrderModuleList) break; // Reached end of list
    }
    // Fallback if not found or for custom loaded modules (though ntdll should be in PEB LDR)
    return GetModuleHandleW(moduleName);
}


BOOL Memory::RepairNtdll() {
    PVOID loadedNtdllBase = GetLoadedModuleBase(L"ntdll.dll");
    if (!loadedNtdllBase) {
        //fprintf(stderr, "Failed to get loaded ntdll.dll base address.\n");
        return FALSE;
    }

    // Get path to system's ntdll.dll
    wchar_t systemNtdllPath[MAX_PATH];
    UINT pathLen = GetSystemDirectoryW(systemNtdllPath, MAX_PATH);
    if (pathLen == 0 || pathLen + wcslen(L"\\ntdll.dll") + 1 > MAX_PATH) {
        //fprintf(stderr, "Failed to get system directory path or path too long.\n");
        return FALSE;
    }
    wcscat_s(systemNtdllPath, MAX_PATH, L"\\ntdll.dll");

    // Create a file handle for the on-disk ntdll.dll
    HANDLE hFile = CreateFileW(systemNtdllPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        //fprintf(stderr, "Failed to open on-disk ntdll.dll. Error: %lu\n", GetLastError());
        return FALSE;
    }

    // Create a file mapping object
    HANDLE hMapping = CreateFileMappingW(hFile, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL);
    if (hMapping == NULL) {
        //fprintf(stderr, "Failed to create file mapping for ntdll.dll. Error: %lu\n", GetLastError());
        CloseHandle(hFile);
        return FALSE;
    }

    // Map a view of the file
    PVOID diskNtdllBase = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (diskNtdllBase == NULL) {
        //fprintf(stderr, "Failed to map view of ntdll.dll. Error: %lu\n", GetLastError());
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return FALSE;
    }

    // PE header parsing for both loaded and disk versions
    PIMAGE_DOS_HEADER dosHeaderLoaded = (PIMAGE_DOS_HEADER)loadedNtdllBase;
    PIMAGE_NT_HEADERS ntHeadersLoaded = (PIMAGE_NT_HEADERS)((PBYTE)loadedNtdllBase + dosHeaderLoaded->e_lfanew);

    PIMAGE_DOS_HEADER dosHeaderDisk = (PIMAGE_DOS_HEADER)diskNtdllBase;
    PIMAGE_NT_HEADERS ntHeadersDisk = (PIMAGE_NT_HEADERS)((PBYTE)diskNtdllBase + dosHeaderDisk->e_lfanew);

    PIMAGE_SECTION_HEADER textSectionLoaded = NULL;
    PIMAGE_SECTION_HEADER textSectionDisk = NULL;

    // Find .text section in loaded ntdll
    PIMAGE_SECTION_HEADER currentSection = IMAGE_FIRST_SECTION(ntHeadersLoaded);
    for (WORD i = 0; i < ntHeadersLoaded->FileHeader.NumberOfSections; ++i, ++currentSection) {
        if (strcmp((char*)currentSection->Name, ".text") == 0) {
            textSectionLoaded = currentSection;
            break;
        }
    }

    // Find .text section in disk ntdll
    currentSection = IMAGE_FIRST_SECTION(ntHeadersDisk);
    for (WORD i = 0; i < ntHeadersDisk->FileHeader.NumberOfSections; ++i, ++currentSection) {
        if (strcmp((char*)currentSection->Name, ".text") == 0) {
            textSectionDisk = currentSection;
            break;
        }
    }

    if (!textSectionLoaded || !textSectionDisk) {
        //fprintf(stderr, ".text section not found in one or both ntdll images.\n");
        UnmapViewOfFile(diskNtdllBase);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return FALSE;
    }

    // Ensure sections are comparable (same size, though disk might be slightly different due to alignment for file)
    // We will compare up to the smaller of the two virtual sizes, or preferably use loaded module's .text size.
    if (textSectionLoaded->Misc.VirtualSize == 0) {
        UnmapViewOfFile(diskNtdllBase);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return FALSE;
    }

    PBYTE textBaseLoaded = (PBYTE)loadedNtdllBase + textSectionLoaded->VirtualAddress;
    PBYTE textBaseDisk = (PBYTE)diskNtdllBase + textSectionDisk->VirtualAddress; // Use disk's RVA for its base
    SIZE_T textSize = textSectionLoaded->Misc.VirtualSize; // Size to compare and potentially repair

    // Compare and repair
    // Before writing, we need to ensure the memory is writable.
    ULONG oldProtection;
    NTSTATUS status = NtProtectVirtualMemory(
        GetCurrentProcess(), // Repairing our own process
        (PVOID*)&textBaseLoaded, // Address of base address
        &textSize,          // Size of region
        PAGE_EXECUTE_READWRITE,
        &oldProtection
    );

    if (!NT_SUCCESS(status)) {
        //fprintf(stderr, "Failed to change .text section to RW. Status: 0x%X\n", status);
        UnmapViewOfFile(diskNtdllBase);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return FALSE;
    }

    BOOL modified = FALSE;
    if (memcmp(textBaseLoaded, textBaseDisk, textSize) != 0) {
        // Sections differ, copy from disk to memory
        memcpy(textBaseLoaded, textBaseDisk, textSize);
        modified = TRUE;
        //fprintf(stdout, "ntdll.dll .text section repaired.\n");
    } else {
        //fprintf(stdout, "ntdll.dll .text section clean, no repair needed.\n");
    }

    // Restore original protection
    ULONG tempProtection; // This will receive the protection state *after* our RW setting.
                        // We need to restore 'oldProtection' which was before RW.
    status = NtProtectVirtualMemory(
        GetCurrentProcess(),
        (PVOID*)&textBaseLoaded,
        &textSize,
        oldProtection, // Restore original protection
        &tempProtection // Dummy, not used
    );

    // Not checking status for restore here, as main job (repair) is done.
    // If it fails, .text might remain RWX, which is not ideal but better than hooked.

    // Cleanup
    UnmapViewOfFile(diskNtdllBase);
    CloseHandle(hMapping);
    CloseHandle(hFile);

    return TRUE; // Success
}
