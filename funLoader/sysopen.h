#pragma once

// Code below is adapted from @modexpblog. Read linked article for more details.
// https://www.mdsec.co.uk/2020/12/bypassing-user-mode-hooks-and-direct-invocation-of-system-calls-for-red-teams

#ifndef SW2_HEADER_H_
#define SW2_HEADER_H_

#include <windows.h> // This should be the primary include for Windows types.
                     // ntdef.h and winternl.h are included by windows.h.

// THREADINFOCLASS is defined in winternl.h (included via windows.h).
// We will use the standard definition for the NtSetInformationThread prototype.
// The custom THREADINFOCLASS_SYS enum is removed to avoid conflicts.

#define SW2_SEED 0xD756B6EA
#define SW2_ROL8(v) (v << 8 | v >> 24)
#define SW2_ROR8(v) (v >> 8 | v << 24)
#define SW2_ROX8(v) ((SW2_SEED % 2) ? SW2_ROL8(v) : SW2_ROR8(v))
#define SW2_MAX_ENTRIES 500
#define SW2_RVA2VA(Type, DllBase, Rva) (Type)((ULONG_PTR) DllBase + Rva)

// SW2 specific structures
typedef struct _SW2_SYSCALL_ENTRY
{
	DWORD Hash;
	DWORD Address;
} SW2_SYSCALL_ENTRY, * PSW2_SYSCALL_ENTRY;

typedef struct _SW2_SYSCALL_LIST
{
	DWORD Count;
	SW2_SYSCALL_ENTRY Entries[SW2_MAX_ENTRIES];
} SW2_SYSCALL_LIST, * PSW2_SYSCALL_LIST;

// These PEB/LDR structures are simplified versions for the syscall mechanism.
// They are named SW2_ to avoid conflict with full definitions in winternl.h if ever mixed directly,
// though winternl.h definitions are usually preferred if full PEB access is needed elsewhere.
typedef struct _SW2_PEB_LDR_DATA {
	BYTE Reserved1[8];
	PVOID Reserved2[3];
	LIST_ENTRY InMemoryOrderModuleList;
} SW2_PEB_LDR_DATA, * PSW2_PEB_LDR_DATA;

typedef struct _SW2_LDR_DATA_TABLE_ENTRY {
	PVOID Reserved1[2];
	LIST_ENTRY InMemoryOrderLinks;
	PVOID Reserved2[2];
	PVOID DllBase;
	// The custom ApiResolver.cpp was trying to access fields like 'BaseDllName'
	// which are part of the full LDR_DATA_TABLE_ENTRY, not this simplified SW2 version.
	// This will need to be addressed in ApiResolver.cpp.
} SW2_LDR_DATA_TABLE_ENTRY, * PSW2_LDR_DATA_TABLE_ENTRY;

typedef struct _SW2_PEB {
	BYTE Reserved1[2];
	BYTE BeingDebugged;
	BYTE Reserved2[1];
	PVOID Reserved3[2];
	PSW2_PEB_LDR_DATA Ldr;
} SW2_PEB, * PSW2_PEB;

DWORD SW2_HashSyscall(PCSTR FunctionName);
BOOL SW2_PopulateSyscallList(void);
EXTERN_C DWORD SW2_GetSyscallNumber(DWORD FunctionHash);

// UNICODE_STRING, OBJECT_ATTRIBUTES, CLIENT_ID, PS_ATTRIBUTE, PS_ATTRIBUTE_LIST
// are defined in <ntdef.h> and <winternl.h>, which are included by <windows.h>.
// We should rely on those standard definitions to avoid C2011 redefinition errors.
// The EXTERN_C function prototypes below will use these standard types.

// InitializeObjectAttributes is also typically defined in standard headers (e.g. <ntdef.h>)
// We ensure it's available. If not, this provides a basic one.
#ifndef InitializeObjectAttributes
#define InitializeObjectAttributes( p, n, a, r, s ) { \
	(p)->Length = sizeof( OBJECT_ATTRIBUTES );        \
	(p)->RootDirectory = r;                           \
	(p)->Attributes = a;                              \
	(p)->ObjectName = n;                              \
	(p)->SecurityDescriptor = s;                      \
	(p)->SecurityQualityOfService = NULL;             \
}
#endif

// Syscall function prototypes
// These use standard Windows types like HANDLE, PVOID, ULONG, ACCESS_MASK, NTSTATUS,
// PUNICODE_STRING, POBJECT_ATTRIBUTES, PCLIENT_ID, PPS_ATTRIBUTE_LIST etc.
// Adding NTAPI to ensure calling convention matches system headers for extern "C" functions.

EXTERN_C NTSTATUS NTAPI NtCreateThreadEx(
	OUT PHANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN HANDLE ProcessHandle,
	IN PVOID StartRoutine,
	IN PVOID Argument OPTIONAL,
	IN ULONG CreateFlags,
	IN SIZE_T ZeroBits,
	IN SIZE_T StackSize,
	IN SIZE_T MaximumStackSize,
	IN PPS_ATTRIBUTE_LIST AttributeList OPTIONAL);

EXTERN_C NTSTATUS NTAPI NtAllocateVirtualMemory(
	IN HANDLE ProcessHandle,
	IN OUT PVOID* BaseAddress,
	IN ULONG ZeroBits,
	IN OUT PSIZE_T RegionSize,
	IN ULONG AllocationType,
	IN ULONG Protect);

EXTERN_C NTSTATUS NTAPI NtClose(
	IN HANDLE Handle);

EXTERN_C NTSTATUS NTAPI NtWriteVirtualMemory(
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	IN PVOID Buffer,
	IN SIZE_T NumberOfBytesToWrite,
	OUT PSIZE_T NumberOfBytesWritten OPTIONAL);

EXTERN_C NTSTATUS NTAPI NtCreateProcess(
	OUT PHANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN HANDLE ParentProcess,
	IN BOOLEAN InheritObjectTable,
	IN HANDLE SectionHandle OPTIONAL,
	IN HANDLE DebugPort OPTIONAL,
	IN HANDLE ExceptionPort OPTIONAL);

EXTERN_C NTSTATUS NTAPI NtOpenProcess(
	OUT PHANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID ClientId OPTIONAL);

EXTERN_C NTSTATUS NTAPI NtProtectVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID* BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG NewProtect,
    OUT PULONG OldProtect
    );

// For NtSetInformationThread, we use the standard THREADINFOCLASS from winternl.h
EXTERN_C NTSTATUS NTAPI NtSetInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass, // Using standard THREADINFOCLASS
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength
    );

EXTERN_C NTSTATUS NTAPI NtFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG FreeType
    );

// For conceptual UnmapSelf in Stealth.cpp / Process Hollowing:
// EXTERN_C NTSTATUS NtUnmapViewOfSection(
//     IN HANDLE ProcessHandle,
//     IN PVOID BaseAddress OPTIONAL
//     );


#endif // SW2_HEADER_H_
