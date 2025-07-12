#include "syscalls.h"

__declspec(naked) NTSTATUS NtAllocateVirtualMemory(
    HANDLE ProcessHandle,
    PVOID* BaseAddress,
    ULONG_PTR ZeroBits,
    PSIZE_T RegionSize,
    ULONG AllocationType,
    ULONG Protect)
{
    __asm {
        mov eax, SYSCALL_NTALLOCATEVIRTUALMEMORY
        syscall
        ret
    }
}

__declspec(naked) NTSTATUS NtWriteVirtualMemory(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    SIZE_T NumberOfBytesToWrite,
    PSIZE_T NumberOfBytesWritten)
{
    __asm {
        mov eax, SYSCALL_NTWRITEVIRTUALMEMORY
        syscall
        ret
    }
}

__declspec(naked) NTSTATUS NtCreateThreadEx(
    PHANDLE ThreadHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    HANDLE ProcessHandle,
    PVOID StartRoutine,
    PVOID Argument,
    ULONG CreateFlags,
    ULONG_PTR ZeroBits,
    SIZE_T StackSize,
    SIZE_T MaximumStackSize,
    PVOID AttributeList)
{
    __asm {
        mov eax, SYSCALL_NTCREATETHREADEX
        syscall
        ret
    }
}

__declspec(naked) NTSTATUS NtClose(
    HANDLE Handle)
{
    __asm {
        mov eax, SYSCALL_NTCLOSE
        syscall
        ret
    }
}
