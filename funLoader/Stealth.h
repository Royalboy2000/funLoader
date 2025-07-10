#pragma once

#include <Windows.h>
#include <winternl.h> // For PEB and LDR structures

namespace Stealth {

    /**
     * @brief Unlinks the current module from various PEB loader lists
     *        (InLoadOrderModuleList, InMemoryOrderModuleList, InInitializationOrderModuleList).
     *
     * @param hModuleBase The base address of the module to unlink. If NULL, attempts to use
     *                    the base address of the module calling this function (though this
     *                    can be tricky if this code is not part of the main module being hidden).
     *                    It's best to provide the specific module base to hide.
     * @return TRUE if unlinking operations were attempted (does not guarantee success or full invisibility),
     *         FALSE if PEB or LDR data could not be accessed.
     */
    BOOL UnlinkFromPEB(HMODULE hModuleBase);


    /**
     * @brief Attempts to unmap the module's own image from memory.
     *        This is a highly dangerous operation and can lead to instability or crashes
     *        if not all code execution has moved out of the module or if there are pending
     *        references to its memory. Primarily for very advanced scenarios.
     *
     * @param hModuleBase The base address of the module to unmap.
     * @return NTSTATUS result of NtUnmapViewOfSection.
     */
    // NTSTATUS UnmapSelf(HMODULE hModuleBase); // VAD unlinking is more complex than just unmapping self


    // Note on VAD (Virtual Address Descriptor) Unlinking:
    // True VAD unlinking is a kernel-mode technique. User-mode approaches are limited
    // and often involve trying to hide memory regions by manipulating page table entries
    // (which is also kernel-mode) or by unmapping sections of the module.
    // Unmapping the module itself (NtUnmapViewOfSection) is the closest user-mode equivalent
    // but doesn't directly manipulate the VAD tree in the kernel. It simply removes the mapping.
    // This makes the memory available but doesn't "unlink" it from a kernel structure in the same
    // way PEB unlinking works for loader data.

} // namespace Stealth
