#pragma once
#include <Windows.h>

#include <Windows.h>

#define SYSCALL_NTALLOCATEVIRTUALMEMORY 0x18
#define SYSCALL_NTWRITEVIRTUALMEMORY 0x3a
#define SYSCALL_NTCREATETHREADEX 0xc1
#define SYSCALL_NTCLOSE 0xf

EXTERN_C NTSTATUS NtAllocateVirtualMemory(
    HANDLE ProcessHandle,
    PVOID* BaseAddress,
    ULONG_PTR ZeroBits,
    PSIZE_T RegionSize,
    ULONG AllocationType,
    ULONG Protect
);

EXTERN_C NTSTATUS NtWriteVirtualMemory(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    SIZE_T NumberOfBytesToWrite,
    PSIZE_T NumberOfBytesWritten
);

EXTERN_C NTSTATUS NtCreateThreadEx(
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
    PVOID AttributeList
);

EXTERN_C NTSTATUS NtClose(
    HANDLE Handle
);
