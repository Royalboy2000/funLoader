#pragma once

#include <Windows.h>

// Forward declaration for _THREADINFOCLASS if not available through Windows.h
// For ThreadHideFromDebugger (usually value 0x11)
typedef enum _THREADINFOCLASS {
    ThreadBasicInformation,
    ThreadTimes,
    ThreadPriority,
    ThreadBasePriority,
    ThreadAffinityMask,
    ThreadImpersonationToken,
    ThreadDescriptorTableEntry,
    ThreadEnableAlignmentFaultFixup,
    ThreadEventPair_Reusable,
    ThreadQuerySetWin32StartAddress,
    ThreadZeroTlsCell,
    ThreadPerformanceCount,
    ThreadAmILastThread,
    ThreadIdealProcessor,
    ThreadPriorityBoost,
    ThreadSetTlsArrayAddress,
    ThreadIsIoPending,
    ThreadHideFromDebugger = 0x11 // Important value for this module
    // ... other values
} THREADINFOCLASS;


namespace AntiDebug {

    /**
     * @brief Attempts to hide the current thread from debuggers using NtSetInformationThread.
     *        Also, optionally applies process mitigation policies.
     * @return TRUE if operations appear successful, FALSE otherwise.
     */
    BOOL HideFromDbg();

    // --- Hardware/Environment Fingerprinting Checks ---

    /**
     * @brief Performs RDTSC timing checks to detect VMs/sandboxes.
     * @return TRUE if a VM/sandbox is suspected, FALSE otherwise.
     */
    BOOL CheckRDTSC();

    /**
     * @brief Checks CPUID information for known VM vendor strings or suspicious features.
     * @return TRUE if a VM/sandbox is suspected, FALSE otherwise.
     */
    BOOL CheckCPUIDFeatures();

    /**
     * @brief (Conceptual) Checks SMBIOS information for VM indicators.
     *        Implementation would likely involve WMI or direct SMBIOS table parsing.
     * @return TRUE if a VM/sandbox is suspected, FALSE otherwise.
     */
    BOOL CheckSMBIOS(); // Placeholder

    /**
     * @brief (Conceptual) Checks disk drive serial numbers/models for VM indicators via WMI.
     * @return TRUE if a VM/sandbox is suspected, FALSE otherwise.
     */
    BOOL CheckDiskDriveSerials(); // Placeholder

    /**
     * @brief (Conceptual) Probes for connected USB devices to detect suspicious configurations.
     * @return TRUE if a VM/sandbox is suspected, FALSE otherwise.
     */
    BOOL CheckUSBDevices(); // Placeholder

    /**
     * @brief Main function to call all hardware/environment checks.
     *        This would augment the existing antidbg() checks.
     * @return TRUE if any check indicates a sandbox/VM, FALSE otherwise.
     */
    BOOL PerformHWChecks();


} // namespace AntiDebug
