#include "AntiDebug.h" // Includes common_windows_headers.h
#include "sysopen.h"   // Includes common_windows_headers.h
// <winternl.h> should be covered by common_windows_headers.h

// Make sure NTSTATUS SUCCESS macro is available
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

// Define STATUS_SUCCESS if not available
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

// Define STATUS_UNSUCCESSFUL if not available (typically from ntstatus.h)
#ifndef STATUS_UNSUCCESSFUL
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)
#endif

// For SetProcessMitigationPolicy (requires Windows.h, already included via AntiDebug.h)
// Example policy - this would need specific research for useful policies.
// typedef enum _PROCESS_MITIGATION_POLICY {
//    ProcessDEPPolicy,
//    ProcessASLRPolicy,
//    ProcessDynamicCodePolicy, // Example: Opt-out of dynamic code generation restrictions
//    ProcessStrictHandleCheckPolicy,
//    ProcessSystemCallDisablePolicy,
//    ProcessMitigationOptionsMask,
//    ProcessExtensionPointDisablePolicy,
//    ProcessControlFlowGuardPolicy, // Example: CFG settings
//    ProcessSignaturePolicy,
//    ProcessFontDisablePolicy,
//    ProcessImageLoadPolicy,
//    ProcessSystemCallFilterPolicy,
//    ProcessPayloadRestrictionPolicy,
//    ProcessChildProcessPolicy,
//    ProcessSideChannelIsolationPolicy,
//    ProcessUserShadowStackPolicy, // Example: For CET
//    MaxProcessMitigationPolicy
//} PROCESS_MITIGATION_POLICY;


BOOL AntiDebug::HideFromDbg() {
    NTSTATUS status = STATUS_UNSUCCESSFUL; // Default to unsuccessful

    // 1. Hide thread from debugger
    // ThreadHideFromDebugger is 0x11 (17)
    // This call tells the kernel that this thread should not be exposed to a debugger.
    // If a debugger is already attached, its effectiveness might be limited.
    // The first parameter is a handle to the thread to hide. GetCurrentThread() is a pseudo-handle.
    // For NtSetInformationThread, a real handle might be needed if GetCurrentThread() doesn't work directly
    // with the syscall version (it often does, but worth noting).
    // The ThreadInformation parameter is not used for ThreadHideFromDebugger, so it's NULL, and length is 0.

    // Assuming NtSetInformationThread is available via sysopen.h and syscalls.asm
    status = NtSetInformationThread(
        GetCurrentThread(),         // ThreadHandle
        ThreadHideFromDebugger,     // ThreadInformationClass (0x11)
        NULL,                       // ThreadInformation (not used for this class)
        0                           // ThreadInformationLength (not used for this class)
    );

    if (!NT_SUCCESS(status)) {
        // Optionally log or handle the error, but for stealth, often best to fail silently.
        // For debugging this module: printf("NtSetInformationThread(ThreadHideFromDebugger) failed: 0x%X\n", status);
    }

    // 2. SetProcessMitigationPolicy tweaks (Optional and requires careful selection)
    // Example: Disabling dynamic code generation or enabling certain CFG/CET features might deter some debuggers or analysis.
    // This is highly dependent on what specific behavior is desired.
    // Using SetProcessMitigationPolicy requires specific structures and flags.
    //
    // Example (Conceptual - requires proper policy structures and flags):
    // PROCESS_MITIGATION_DYNAMIC_CODE_POLICY dynamicCodePolicy = {0};
    // dynamicCodePolicy.ProhibitDynamicCode = 1; // Example: Prohibit dynamic code
    // if (!SetProcessMitigationPolicy(ProcessDynamicCodePolicy, &dynamicCodePolicy, sizeof(dynamicCodePolicy))) {
    //     // Failed to set policy
    //     // For debugging this module: printf("SetProcessMitigationPolicy failed. Error: %lu\n", GetLastError());
    // }

    // ProcessUserShadowStackPolicy example for CET (if hardware supports)
    // PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY shadowStackPolicy = {0};
    // shadowStackPolicy.EnableUserShadowStack = 1;
    // shadowStackPolicy.EnableUserShadowStackStrictMode = 1; // More restrictive
    // if (!SetProcessMitigationPolicy(ProcessUserShadowStackPolicy, &shadowStackPolicy, sizeof(shadowStackPolicy))) {
    //     // Failed to set policy
    // }
    //
    // Note: SetProcessMitigationPolicy is a Win32 API. If direct syscalls are preferred for everything,
    // then the underlying NtSetInformationProcess with appropriate class would be needed, which is more complex.
    // For now, this feature is more of a placeholder for further research on specific policies.

    return NT_SUCCESS(status); // Return based on NtSetInformationThread success for now.
}


// --- Hardware/Environment Fingerprinting Implementations ---

#include <intrin.h> // For __cpuid, __rdtsc

BOOL AntiDebug::CheckRDTSC() {
    volatile UINT64 start_cycle, end_cycle;
    volatile UINT32 cycles_diff; // Using volatile to prevent optimizations

    // Simple timing check: execute some benign instructions and measure RDTSC difference.
    // Anomalously small differences might indicate RDTSC emulation or fast-forwarding in VMs.
    // This is a very basic check and can be fooled.

    // Warm-up RDTSC
    start_cycle = __rdtsc();
    end_cycle = __rdtsc();

    start_cycle = __rdtsc();
    // Some NOPs or simple arithmetic operations as a small delay
    for (volatile int i = 0; i < 10; ++i) { /* delay */ }
    end_cycle = __rdtsc();

    cycles_diff = (UINT32)(end_cycle - start_cycle);

    // Threshold is empirical and highly unreliable across different CPUs/VMs.
    // A very low cycle count (e.g., < 100-500 for a few NOPs) might be suspicious.
    // Hypervisors might also trap RDTSC and return consistent but plausible values.
    if (cycles_diff < 100 || cycles_diff > 100000) { // Extremely small or large might be odd
        // A more sophisticated check would involve measuring longer operations
        // or looking for inconsistencies in RDTSC frequency.
        // For now, this is a very basic flag.
        // printf("[!] RDTSC check: Suspicious cycle count: %u\n", cycles_diff);
        return TRUE; // Suspected VM/sandbox
    }

    // Second check: RDTSC delta between two consecutive calls
    // Some VMs might return 0 or very small fixed values.
    UINT64 t1 = __rdtsc();
    UINT64 t2 = __rdtsc();
    if ((t2 - t1) == 0 || (t2-t1) > 0x100000) { // if delta is 0 or abnormally large
        // printf("[!] RDTSC check: Consecutive call delta suspicious: %llu\n", (t2-t1));
        return TRUE;
    }


    return FALSE; // No obvious VM indicator from this basic RDTSC check
}

BOOL AntiDebug::CheckCPUIDFeatures() {
    int cpuInfo[4] = {0}; // For __cpuid output (EAX, EBX, ECX, EDX)

    // 1. Check for Hypervisor presence bit (EAX=1, ECX bit 31)
    __cpuid(cpuInfo, 1);
    if ((cpuInfo[2] >> 31) & 1) {
        // printf("[!] CPUID check: Hypervisor bit set in ECX.\n");
        return TRUE; // Hypervisor detected
    }

    // 2. Check for Vendor ID string (EAX=0x40000000 - Hypervisor specific range)
    // Common hypervisor vendor strings:
    // "KVMKVMKVM\0\0\0" (KVM)
    // "Microsoft Hv" (Hyper-V)
    // "VMwareVMware" (VMware)
    // "XenVMMXenVMM" (Xen)
    // "VBoxVBoxVBox" (VirtualBox)

    char vendorId[13];
    __cpuid(cpuInfo, 0x40000000); // Get max hypervisor CPUID function number and vendor ID part 1
    memcpy(vendorId, &cpuInfo[1], 4); // EBX
    memcpy(vendorId + 4, &cpuInfo[2], 4); // ECX
    memcpy(vendorId + 8, &cpuInfo[3], 4); // EDX
    vendorId[12] = '\0';

    if (strcmp(vendorId, "KVMKVMKVM\0\0\0") == 0 ||
        strncmp(vendorId, "Microsoft Hv", 12) == 0 || // Microsoft Hyper-V
        strcmp(vendorId, "VMwareVMware") == 0 ||
        strcmp(vendorId, "XenVMMXenVMM") == 0 ||
        strcmp(vendorId, "VBoxVBoxVBox") == 0 ||
        strcmp(vendorId, "prl hyperv\0\0") == 0 || // Parallels
        strcmp(vendorId, "lrpepyh vr\0\0") == 0) { // QEMU (often with KVM)
        // printf("[!] CPUID check: Known VM vendor ID: %s\n", vendorId);
        return TRUE;
    }

    // Some hypervisors might not use 0x40000000 or might use different strings.
    // One can also check EAX=0 to get the standard vendor ID and see if it's unusual.
    // Example: __cpuid(cpuInfo, 0); -> check for "GenuineIntel", "AuthenticAMD"
    // If it's something else, it might be suspicious but not definitive.

    return FALSE; // No obvious VM indicator from CPUID
}

// Placeholder stubs for more complex checks
BOOL AntiDebug::CheckSMBIOS() {
    // Implementation would involve WMI queries ("SELECT * FROM Win32_BIOS" or "Win32_SystemEnclosure")
    // or direct parsing of SMBIOS tables (requires admin rights and complex parsing).
    // Look for manufacturer/serial strings like "VMware", "VirtualBox", "QEMU", etc.
    // printf("[?] SMBIOS check: Not implemented.\n");
    return FALSE;
}

BOOL AntiDebug::CheckDiskDriveSerials() {
    // Implementation would involve WMI queries ("SELECT * FROM Win32_DiskDrive")
    // Look for serial numbers/models like "VMware", "VBOX", "QEMU", "Msft Virtual Disk".
    // printf("[?] Disk Drive check: Not implemented.\n");
    return FALSE;
}

BOOL AntiDebug::CheckUSBDevices() {
    // Implementation involves SetupAPI (SetupDiGetClassDevs, etc.) to enumerate USB devices.
    // Look for absence of common physical devices or presence of virtual USB controllers.
    // printf("[?] USB Devices check: Not implemented.\n");
    return FALSE;
}


BOOL AntiDebug::PerformHWChecks() {
    if (CheckRDTSC()) {
        return TRUE; // VM/Sandbox detected
    }
    if (CheckCPUIDFeatures()) {
        return TRUE; // VM/Sandbox detected
    }
    if (CheckSMBIOS()) { // Placeholder
        return TRUE;
    }
    if (CheckDiskDriveSerials()) { // Placeholder
        return TRUE;
    }
    if (CheckUSBDevices()) { // Placeholder
        return TRUE;
    }

    // Add calls to other checks from the original antidbg() in load.cpp like RAM and Disk Size
    // This function is meant to *augment* those, not fully replace them without migration.
    // For now, this is a standalone set of new checks.

    return FALSE; // No VM/Sandbox detected by these new checks
}
