#pragma once

#include "common_windows_headers.h"

// THREADINFOCLASS is defined in winternl.h (included via common_windows_headers.h -> Windows.h).
// AntiDebug functions should use this standard definition.
// The local _THREADINFOCLASS definition has been removed.

namespace AntiDebug {

    // Value for NtSetInformationThread's ThreadInformationClass parameter
    // to hide a thread from the debugger.
    const ULONG ThreadHideFromDebugger = 0x11;

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
