#pragma once
#include <Windows.h>
#include <winternl.h>

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

typedef struct _PS_ATTRIBUTE_LIST PS_ATTRIBUTE_LIST, *PPS_ATTRIBUTE_LIST;

// Correct 11‑arg signature for the x64 NtCreateThreadEx syscall
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateThreadEx(
    _Out_    PHANDLE             ThreadHandle,
    _In_     ACCESS_MASK         DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES  ObjectAttributes,
    _In_     HANDLE              ProcessHandle,
    _In_     PVOID               StartRoutine,
    _In_opt_ PVOID               Argument,
    _In_     ULONG               CreateFlags,
    _In_     ULONG_PTR           ZeroBits,
    _In_     SIZE_T              StackSize,
    _In_     SIZE_T              MaximumStackSize,
    _In_opt_ PPS_ATTRIBUTE_LIST  AttributeList
);

EXTERN_C NTSTATUS NtClose(
    HANDLE Handle
);
