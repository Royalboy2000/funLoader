#pragma once

#include "common_windows_headers.h"

namespace Crypto {

    /**
     * @brief Decrypts a buffer in place using a rolling and mutating XOR key.
     *
     * The decryption algorithm is as follows:
     * For each byte in the buffer:
     *   decrypted_byte = current_byte ^ (key & 0xFF)
     *   key = (key + previous_decrypted_byte) ^ some_constant; // Key mutation
     *   key = (key * another_constant_multiplier + another_constant_adder); // Rolling aspect
     *
     * @param buf Pointer to the buffer to be decrypted.
     * @param len Length of the buffer in bytes.
     * @param initial_key The initial DWORD key for decryption.
     */
    void JITDecrypt(BYTE* buf, SIZE_T len, DWORD initial_key);

    // Helper function to encrypt data using the same logic (for testing or payload prep)
    // This would typically be done offline by a separate tool.
    void JITEncrypt(BYTE* buf, SIZE_T len, DWORD initial_key);

} // namespace Crypto
