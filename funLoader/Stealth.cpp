#include "Stealth.h"
#include "sysopen.h" // For NtUnmapViewOfSection if we were to implement UnmapSelf

// Helper macro for PEB LDR list traversal
// LIST_ENTRY is defined in winternl.h as:
// typedef struct _LIST_ENTRY {
//    struct _LIST_ENTRY *Flink;
//    struct _LIST_ENTRY *Blink;
// } LIST_ENTRY, *PLIST_ENTRY;

// LDR_DATA_TABLE_ENTRY, PEB_LDR_DATA, PPEB_LDR_DATA (PLDR_DATA) are defined in <winternl.h>,
// which is included via Stealth.h -> windows.h. We will use these standard definitions.
// The custom _MY_LDR_DATA_TABLE_ENTRY has been removed.

// Function to safely unlink a module from a doubly linked list
void UnlinkModuleEntry(LIST_ENTRY* pEntry) {
    if (pEntry && pEntry->Flink && pEntry->Blink) {
        LIST_ENTRY* pPrev = pEntry->Blink;
        LIST_ENTRY* pNext = pEntry->Flink;

        pPrev->Flink = pNext;
        pNext->Blink = pPrev;
    }
}


BOOL Stealth::UnlinkFromPEB(HMODULE hModuleBase) {
    if (!hModuleBase) { // If no hModuleBase provided, this technique is harder to apply correctly to "self" generically
        return FALSE;   // without knowing which module is "self" from the caller's perspective.
    }

#if defined(_WIN64)
    PPEB pPeb = (PPEB)__readgsqword(0x60);
#else
    PPEB pPeb = (PPEB)__readfsdword(0x30);
#endif

    if (!pPeb || !pPeb->Ldr) {
        return FALSE;
    }

    PPEB_LDR_DATA pLdr = pPeb->Ldr; // Use PPEB_LDR_DATA type from winternl.h

    // Traverse InLoadOrderModuleList
    for (PLIST_ENTRY pListEntry = pLdr->InLoadOrderModuleList.Flink;
         pListEntry != &pLdr->InLoadOrderModuleList;
         pListEntry = pListEntry->Flink) {

        // Calculate the base of the LDR_DATA_TABLE_ENTRY structure from the InLoadOrderLinks field
        PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(pListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks); // Use standard LDR_DATA_TABLE_ENTRY
        if (pEntry->DllBase == hModuleBase) {
            UnlinkModuleEntry(&pEntry->InLoadOrderLinks);
            // Also remove from other lists by finding the same DllBase.
            // The standard LDR_DATA_TABLE_ENTRY contains all three list links.
            UnlinkModuleEntry(&pEntry->InMemoryOrderLinks);
            UnlinkModuleEntry(&pEntry->InInitializationOrderLinks);

            // Optional: Zero out DllBase, SizeOfImage, etc. in the LDR entry itself.
            // This is more aggressive. For now, just unlinking.
            // pEntry->DllBase = NULL;
            // pEntry->SizeOfImage = 0;
            // SecureZeroMemory(&pEntry->BaseDllName, sizeof(pEntry->BaseDllName));
            // SecureZeroMemory(&pEntry->FullDllName, sizeof(pEntry->FullDllName));


            return TRUE; // Found and attempted to unlink
        }
    }

    // If not found by iterating InLoadOrderModuleList, it's possible the module isn't fully linked
    // or there's an issue. For robustness, one could iterate all three lists independently.
    // However, a module should appear in all if correctly loaded.

    return FALSE; // Module not found or already unlinked (or PEB/LDR issue)
}


/*
// --- Conceptual NtUnmapViewOfSection for "VAD unlinking" (more accurately, self-unmapping) ---
// This would require NtUnmapViewOfSection to be added to sysopen.h and syscalls.asm
// Hash for NtUnmapViewOfSection (SW2): (Example, needs actual calculation) 0xXXXXXXXX

NTSTATUS Stealth::UnmapSelf(HMODULE hModuleBase) {
    if (!hModuleBase) {
        return STATUS_INVALID_PARAMETER;
    }

    // IMPORTANT: This is extremely dangerous.
    // All execution must have moved out of this module's memory space.
    // Any active threads, return addresses on stacks, or data pointers into this module
    // will cause immediate crashes.
    // This is typically done by a small shellcode stub that copies itself to allocated RX memory,
    // then calls this function on the original module base, and then continues execution from the new location.

    // return NtUnmapViewOfSection(GetCurrentProcess(), (PVOID)hModuleBase);

    return STATUS_NOT_IMPLEMENTED; // Placeholder
}
*/
