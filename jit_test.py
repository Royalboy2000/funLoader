#!/usr/bin/env python3

import os
import sys

# --- Configuration (no command‑line args needed) ---
INPUT_FILE   = "shellcode.bin"
ENCRYPT_FILE = "encrypted.bin"
DECRYPT_FILE = "decrypted.bin"
KEY_SEED     = 0x12345678

# --- Helper functions ---
def rotr32(val, r):
    return ((val >> r) | (val << (32 - r))) & 0xFFFFFFFF

def encrypt(buf, key_seed):
    key = key_seed ^ 0xDEADBEEF
    out = bytearray()
    for b in buf:
        eb = b ^ (key & 0xFF)
        out.append(eb)
        key = rotr32(key + eb + 0x1337, 5)
    return out

def decrypt(buf, key_seed):
    key = key_seed ^ 0xDEADBEEF
    out = bytearray()
    for b in buf:
        db = b ^ (key & 0xFF)
        out.append(db)
        key = rotr32(key + b + 0x1337, 5)
    return out

# --- Main logic ---
def main():
    # 1. Load original shellcode
    if not os.path.isfile(INPUT_FILE):
        print(f"Error: '{INPUT_FILE}' not found.")
        # Create a dummy shellcode.bin for testing
        with open(INPUT_FILE, "wb") as f:
            f.write(os.urandom(256))
        print(f"Created dummy '{INPUT_FILE}'.")

    with open(INPUT_FILE, "rb") as f:
        original = bytearray(f.read())
    print(f"Loaded {len(original)} bytes from {INPUT_FILE}")

    # 2. Encrypt
    encrypted = encrypt(original, KEY_SEED)
    with open(ENCRYPT_FILE, "wb") as f:
        f.write(encrypted)
    print(f"Wrote {len(encrypted)} bytes to {ENCRYPT_FILE}")

    # 3. Decrypt
    decrypted = decrypt(encrypted, KEY_SEED)
    with open(DECRYPT_FILE, "wb") as f:
        f.write(decrypted)
    print(f"Wrote {len(decrypted)} bytes to {DECRYPT_FILE}")

    # 4. Verify
    if original == decrypted:
        print("SUCCESS: Decrypted data matches original!")
    else:
        # find first mismatch
        for i, (o, d) in enumerate(zip(original, decrypted)):
            if o != d:
                print(f"FAILURE: Byte #{i} differs: {o:#02x} != {d:#02x}")
                break
        else:
            # lengths differ?
            print("FAILURE: Data lengths differ.")

if __name__ == "__main__":
    main()
