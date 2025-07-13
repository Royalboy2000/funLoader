import argparse

def rotr(d, n):
    return (d >> n) | (d << (32 - n)) & 0xFFFFFFFF

def jit_decrypt(buf, key_seed):
    key = key_seed ^ 0xDEADBEEF
    decrypted = bytearray()
    for byte in buf:
        decrypted_byte = byte ^ (key & 0xFF)
        decrypted.append(decrypted_byte)
        key = rotr(key + decrypted_byte + 0x1337, 5)
    return decrypted

def main():
    parser = argparse.ArgumentParser(description="Encrypt a payload using the JITDecrypt algorithm.")
    parser.add_argument("input_file", help="Path to the raw payload file.")
    parser.add_argument("output_file", help="Path to the output file for the encrypted payload.")
    parser.add_argument("key_seed", type=lambda x: int(x, 0), help="The key seed for the encryption.")
    parser.add_argument("--format", choices=["c_array", "raw"], default="c_array", help="Output format.")
    args = parser.parse_args()

    with open(args.input_file, "rb") as f:
        payload = f.read()

    encrypted_payload = jit_decrypt(payload, args.key_seed)

    if args.format == "c_array":
        c_array = "unsigned char payload[] = {"
        for i, byte in enumerate(encrypted_payload):
            if i % 16 == 0:
                c_array += "\n    "
            c_array += f"0x{byte:02x}, "
        c_array = c_array.strip(", ") + "\n};"
        with open(args.output_file, "w") as f:
            f.write(c_array)
    else:
        with open(args.output_file, "wb") as f:
            f.write(encrypted_payload)

if __name__ == "__main__":
    main()
