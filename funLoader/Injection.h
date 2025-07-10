#pragma once

#include <Windows.h>

namespace Injection {

    /**
     * @brief Injects shellcode into a target process using Asynchronous Procedure Calls (APCs).
     *        This function assumes the shellcode has already been written to the target process's memory.
     *
     * @param hProc Handle to the target process.
     * @param pRemoteShellcodeAddr Pointer to the shellcode in the target process's address space.
     *                             This address will be used as the APC routine.
     * @return TRUE if APCs were successfully queued to one or more threads, FALSE otherwise.
     *         Note: Successful queuing does not guarantee execution.
     */
    BOOL InjectViaAPC(HANDLE hProc, PVOID pRemoteShellcodeAddr);


    // --- Process Hollowing (Conceptual - More Complex Implementation) ---
    // BOOL InjectViaProcessHollowing(const wchar_t* targetProcessPath, const wchar_t* payloadPath);
    // Or:
    // BOOL InjectViaProcessHollowing(const wchar_t* targetProcessPath, BYTE* payloadPE, SIZE_T payloadPESize);

} // namespace Injection
