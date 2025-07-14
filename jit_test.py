import os

def rotr(d, n):
    return (d >> n) | (d << (32 - n)) & 0xFFFFFFFF

def encrypt(buf, key_seed):
    key = key_seed ^ 0xDEADBEEF
    encrypted = bytearray()
    for byte in buf:
        encrypted_byte = byte ^ (key & 0xFF)
        encrypted.append(encrypted_byte)
        key = rotr(key + encrypted_byte + 0x1337, 5)
    return encrypted

def decrypt(buf, key_seed):
    key = key_seed ^ 0xDEADBEEF
    decrypted = bytearray()
    for byte in buf:
        decrypted_byte = byte ^ (key & 0xFF)
        decrypted.append(decrypted_byte)
        key = rotr(key + decrypted_byte + 0x1337, 5)
    return decrypted

def main():
    # Create a dummy shellcode.bin for testing
    with open("shellcode.bin", "wb") as f:
        f.write(os.urandom(256))

    with open("shellcode.bin", "rb") as f:
        original_data = bytearray(f.read())

    key_seed = 0x12345678

    # Encrypt the data
    encrypted_data = encrypt(original_data, key_seed)
    with open("encrypted.bin", "wb") as f:
        f.write(encrypted_data)

    # Decrypt the data
    decrypted_data = decrypt(encrypted_data, key_seed)
    with open("decrypted.bin", "wb") as f:
        f.write(decrypted_data)

    # Compare the decrypted data with the original data
    if original_data == decrypted_data:
        print("SUCCESS: Decryption matches original.")
    else:
        for i, (a, b) in enumerate(zip(original_data, decrypted_data)):
            if a != b:
                print(f"FAILURE: Mismatch at byte {i}")
                break

if __name__ == "__main__":
    main()
