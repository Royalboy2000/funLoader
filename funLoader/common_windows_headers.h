#pragma once

// Define appropriate _WIN32_WINNT version if specific API levels are needed.
// For broad compatibility, let Windows.h use its default or rely on project settings.
// Example: #define _WIN32_WINNT _WIN32_WINNT_WIN7

// It's often best to avoid WIN32_LEAN_AND_MEAN for projects like this
// that use many parts of the Windows API, including less common NT structures,
// unless you are very sure about what it excludes and that you don't need it.
// If it was defined globally in project settings, it could be a source of issues.
// #undef WIN32_LEAN_AND_MEAN

#include <windows.h>

// After windows.h, we can include other specific headers if absolutely necessary
// and if windows.h doesn't reliably include them for all configurations,
// or if a specific version of a struct/API from a more direct header is needed.
// For now, relying on windows.h to bring in ntdef.h, winnt.h, winternl.h, ntstatus.h.

// For NTSTATUS values, ntstatus.h is sometimes not fully pulled in by windows.h
// depending on SDK configuration or other defines. Including it explicitly
// after windows.h (which defines _NTDEF_ etc.) is generally safe.
#include <ntstatus.h> // For NTSTATUS values like STATUS_SUCCESS, etc.

// Ensure common NTAPI types like POBJECT_ATTRIBUTES, THREADINFOCLASS, etc.
// are available. These are typically in winnt.h or winternl.h, included by windows.h.
// If errors persist for these types, specific sub-headers of windows.h might need
// to be explicitly included here, e.g. <winternl.h>, but this should be a last resort
// as <windows.h> is supposed to manage this.

// For PEB/LDR structures, <winternl.h> is the source. <windows.h> should include it.
// If LDR_DATA_TABLE_ENTRY fields are still missing, an explicit <winternl.h> here might be forced.

// Add any other globally needed system headers here.
// For example, for CreateToolhelp32Snapshot:
#include <TlHelp32.h> // For CreateToolhelp32Snapshot, THREADENTRY32, etc.

// For C++ standard library types used commonly across the project:
#include <vector>
#include <string>
#include <map>
#include <algorithm> // For std::transform
#include <random>    // For std::mt19937 and std::uniform_int_distribution
#include <time.h>      // For seeding random number generator (though <random> is better)
// #include <iostream> // Only if widespread console debugging is needed, better to keep in .cpp

// Ensure NT_SUCCESS macro is available. It's usually in ntdef.h (via windows.h)
// but can be defined if missing.
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

// Common status codes that might not always be defined by default includes
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

#ifndef STATUS_INFO_LENGTH_MISMATCH
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)
#endif

#ifndef STATUS_BUFFER_TOO_SMALL
#define STATUS_BUFFER_TOO_SMALL ((NTSTATUS)0xC0000023L)
#endif

#ifndef STATUS_UNSUCCESSFUL
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)
#endif
