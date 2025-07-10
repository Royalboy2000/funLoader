#pragma once

// Define WIN32_LEAN_AND_MEAN to reduce header size and potential conflicts.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Define a common target version for Windows API.
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // Windows 7 or later
#endif

#include <windows.h>  // Now lean due to WIN32_LEAN_AND_MEAN

// After a lean windows.h, explicitly include headers for NT structures and types.
// ntdef.h provides many basic NT types (NTSTATUS, PVOID, HANDLE, UNICODE_STRING, OBJECT_ATTRIBUTES, LIST_ENTRY etc.)
// and is often a prerequisite for winternl.h.
#include <ntdef.h>

// winternl.h provides PEB, LDR_DATA_TABLE_ENTRY, THREADINFOCLASS, more OBJECT_ATTRIBUTES details,
// and many Nt* function prototypes (though we declare our own in sysopen.h for direct syscalls).
#include <winternl.h>

// ntstatus.h for NTSTATUS values (like STATUS_SUCCESS, STATUS_WAIT_0 etc.).
// With WIN32_LEAN_AND_MEAN, windows.h/winnt.h should define fewer STATUS_* codes,
// making ntstatus.h the primary source, hopefully avoiding C4005 redefinition warnings.
#include <ntstatus.h>

// Other common system headers
#include <TlHelp32.h> // For CreateToolhelp32Snapshot, THREADENTRY32, etc.

// For __readgsqword and other intrinsics (MSVC specific)
#if defined(_MSC_VER)
#include <intrin.h>
#endif

// C++ Standard Library headers (guard for C compatibility if this header were ever included by a .c file directly)
#ifdef __cplusplus
#include <vector>
#include <string>
#include <map>
#include <algorithm> // For std::transform
#include <random>    // For std::mt19937 and std::uniform_int_distribution
#include <time.h>      // For seeding random number generator (used in Memory.cpp)
// #include <iostream> // Only if widespread console debugging is needed and included by .cpp files directly
#endif

// Common project-specific macro definitions (e.g., NT_SUCCESS)
// NT_SUCCESS is usually in ntdef.h or winnt.h (via windows.h), but to be safe:
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

// Specific STATUS_ codes can be defined here with #ifndef if ntstatus.h doesn't cover them
// or if ntstatus.h is conditionally excluded in some builds.
// For now, relying on ntstatus.h to provide these.
/*
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L)
#endif
// Add other custom status code defines here if needed, for example:
#ifndef STATUS_INFO_LENGTH_MISMATCH
#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)
#endif
*/
