#pragma once

// Define WIN32_LEAN_AND_MEAN to reduce header size and potential conflicts.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Define a common target version for Windows API.
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // Windows 7 or later
#endif

#include <windows.h>  // Primary include for Windows APIs and types.
                     // This should correctly include ntdef.h, winnt.h, and parts of winternl.h.

// Explicitly include winternl.h AFTER windows.h to ensure full NT type definitions
// like PEB, LDR_DATA_TABLE_ENTRY, OBJECT_ATTRIBUTES, THREADINFOCLASS are available if
// a lean windows.h doesn't provide them sufficiently for direct use.
#include <winternl.h>

// ntstatus.h for specific status codes if not covered by windows.h/winnt.h.
// If C4005 warnings (macro redefinition for STATUS_*) persist, this might be removed
// and essential status codes defined manually with #ifndef.
// #include <ntstatus.h> // Keep this commented out for now to avoid C4005 warnings.

// Other common system headers
#include <TlHelp32.h>
#if defined(_MSC_VER)
#include <intrin.h> // For __readgsqword
#endif

// C++ Standard Library headers (guard for C compatibility)
#ifdef __cplusplus
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <random>
#include <time.h>
// #include <iostream>
#endif

// Common macro definitions
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

// Manually define essential STATUS_ codes to avoid ntstatus.h conflicts for now
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
// Add more as needed by the project, for example:
#ifndef STATUS_TIMEOUT
#define STATUS_TIMEOUT                   ((NTSTATUS)0x00000102L)
#endif
#ifndef STATUS_PENDING
#define STATUS_PENDING                   ((NTSTATUS)0x00000103L)
#endif
#ifndef STATUS_ACCESS_VIOLATION
#define STATUS_ACCESS_VIOLATION          ((NTSTATUS)0xC0000005L)
#endif
#ifndef STATUS_INVALID_HANDLE
#define STATUS_INVALID_HANDLE            ((NTSTATUS)0xC0000008L)
#endif
#ifndef STATUS_NO_MEMORY
#define STATUS_NO_MEMORY                 ((NTSTATUS)0xC0000017L)
#endif
