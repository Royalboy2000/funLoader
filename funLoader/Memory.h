#pragma once

#include <Windows.h>

namespace Memory {

    /**
     * @brief Allocates memory with a randomized base address within a larger reserved region,
     *        and with random padding pages around the committed region.
     *
     * @param hProc Handle to the target process. If NULL, allocates in the current process.
     * @param size The essential size of the memory to be committed and made executable.
     * @param pAllocatedBase Pointer to receive the base address of the *committed* region.
     * @param pReservedRegionSize Pointer to receive the total size of the *reserved* region (optional, can be NULL).
     * @param protect The desired memory protection for the committed region (e.g., PAGE_EXECUTE_READWRITE).
     * @return NTSTATUS Status of the operation. STATUS_SUCCESS on success.
     */
    NTSTATUS RandomAlloc(HANDLE hProc, SIZE_T size, PVOID* pAllocatedBase, PSIZE_T pReservedRegionSize, ULONG protect);

    // Helper to get system allocation granularity
    DWORD GetAllocGranularity();

    /**
     * @brief Detects and repairs inline hooks in the .text section of the currently loaded ntdll.dll
     *        by comparing it with a fresh copy from disk and restoring differing bytes.
     * @return TRUE if successful (or no hooks found), FALSE on critical error.
     */
    BOOL RepairNtdll();

} // namespace Memory
