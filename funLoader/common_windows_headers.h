#pragma once

// Define WIN32_LEAN_AND_MEAN to reduce header size and potential conflicts.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Define a common target version for Windows API.
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // Windows 7 or later; adjust as needed
#endif

#include <windows.h>  // Now lean due to WIN32_LEAN_AND_MEAN.

// After a lean windows.h, explicitly include headers for NT structures and types.
// ntdef.h provides many basic NT types and is often a prerequisite for winternl.h
// It defines NTSTATUS, PVOID, HANDLE, ULONG, USHORT, UNICODE_STRING, OBJECT_ATTRIBUTES, LIST_ENTRY etc.
#include <ntdef.h>

// winternl.h provides PEB, LDR_DATA_TABLE_ENTRY, THREADINFOCLASS, more OBJECT_ATTRIBUTES details,
// and many Nt* function prototypes.
#include <winternl.h>

// ntpsapi.h for PPS_ATTRIBUTE_LIST and other process/thread attribute types.
#include <ntpsapi.h>

// ntstatus.h for NTSTATUS values (like STATUS_SUCCESS, STATUS_WAIT_0 etc.).
// With WIN32_LEAN_AND_MEAN, windows.h/winnt.h might define fewer STATUS_* codes,
// making ntstatus.h the primary source, hopefully avoiding C4005 redefinition warnings.
#include <ntstatus.h>

// Other common system headers
#include <TlHelp32.h> // For CreateToolhelp32Snapshot, THREADENTRY32, etc.

// For __readgsqword and other intrinsics (MSVC specific)
#if defined(_MSC_VER)
#include <intrin.h>
#endif

// C++ Standard Library headers (guard for C compatibility)
#ifdef __cplusplus
#include <vector>
#include <string>
#include <map>
#include <algorithm> // For std::transform
#include <random>    // For std::mt19937 and std::uniform_int_distribution
#include <time.h>      // For seeding random number generator (used in Memory.cpp)
// #include <iostream> // Only if widespread console debugging is needed
#endif

// Common project-specific macro definitions
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

// Specific STATUS_ codes. If ntstatus.h is included reliably and comprehensively,
// these manual defines might become redundant. They are guarded by #ifndef.
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L)
#endif
#ifndef STATUS_INFO_LENGTH_MISMATCH
#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)
#endif
#ifndef STATUS_INVALID_PARAMETER
#define STATUS_INVALID_PARAMETER         ((NTSTATUS)0xC000000DL)
#endif
#ifndef STATUS_BUFFER_TOO_SMALL
#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L)
#endif
#ifndef STATUS_UNSUCCESSFUL
#define STATUS_UNSUCCESSFUL              ((NTSTATUS)0xC0000001L)
#endif
