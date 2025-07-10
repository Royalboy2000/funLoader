#include "Crypto.h"

// Constants for key mutation and rolling. These can be any values.
// They should be the same for encryption and decryption.
#define KEY_MUTATE_CONSTANT 0x42
#define KEY_ROLL_MULTIPLIER 0x10101011
#define KEY_ROLL_ADDER 0x1234567F

void Crypto::JITDecrypt(BYTE* buf, SIZE_T len, DWORD initial_key) {
    if (!buf || len == 0) {
        return;
    }

    DWORD current_key = initial_key;
    BYTE previous_decrypted_byte = 0; // Initial state for key mutation

    for (SIZE_T i = 0; i < len; ++i) {
        BYTE current_byte = buf[i];
        BYTE key_byte = (BYTE)(current_key & 0xFF);

        BYTE decrypted_byte = current_byte ^ key_byte;
        buf[i] = decrypted_byte; // Decrypt in place

        // Mutate and roll the key for the next byte
        // The mutation uses the *decrypted* byte from the current step
        current_key = (current_key + decrypted_byte) ^ KEY_MUTATE_CONSTANT;
        current_key = (current_key * KEY_ROLL_MULTIPLIER) + KEY_ROLL_ADDER;

        // For a simpler rolling key, without feedback from decrypted byte:
        // current_key = (initial_key >> (i % 24)) | (initial_key << ((i % 24) - 24)); // Example simple roll
        // current_key += SOME_CONSTANT_PER_BYTE; // Example simple increment
    }
}

void Crypto::JITEncrypt(BYTE* buf, SIZE_T len, DWORD initial_key) {
    if (!buf || len == 0) {
        return;
    }

    DWORD current_key = initial_key;
    // BYTE previous_original_byte = 0; // For encryption, mutation uses the *original* byte

    for (SIZE_T i = 0; i < len; ++i) {
        BYTE original_byte = buf[i]; // This is the plaintext byte
        BYTE key_byte = (BYTE)(current_key & 0xFF);

        buf[i] = original_byte ^ key_byte; // Encrypt in place

        // Mutate and roll the key for the next byte
        // The mutation uses the *original* (plaintext) byte that was just encrypted
        current_key = (current_key + original_byte) ^ KEY_MUTATE_CONSTANT;
        current_key = (current_key * KEY_ROLL_MULTIPLIER) + KEY_ROLL_ADDER;
    }
}
