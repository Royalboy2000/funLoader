# FunLoader

FunLoader is a C++ tool for injecting shellcode into remote processes using various techniques, including direct memory injection and APC injection. It incorporates several features to evade detection, such as dynamic API resolution, anti-debugging checks, and sandbox detection.

## Features

- **Shellcode Injection**: Injects shellcode into a target process.
- **Dynamic API Resolution**: Resolves Windows API functions at runtime to obscure its functionality.
- **Anti-Debugging and Sandbox Detection**: Includes checks to detect if it's running in a debugger or a sandbox environment.
- **Multiple Injection Techniques**: Can be extended to support various injection methods.
- **Payload Encryption**: Supports payload encryption and in-memory decryption.
- **Persistence**: The compiled executable will copy itself to `%APPDATA%\\SystemTools\\audiodriver.exe` and add registry keys to run on startup.

## Project Structure

The project is composed of a C++ application (`funLoader`) and a set of Python helper scripts.

### C++ Application (`funLoader`)

The `funLoader` application is the core of the project. It is responsible for:

- **Finding a target process**: It searches for a specific process (e.g., `notepad.exe`) or creates a new one.
- **Allocating memory**: It allocates memory in the target process.
- **Writing the payload**: It writes the shellcode into the allocated memory.
- **Executing the payload**: It uses techniques like APC injection to execute the shellcode.
- **Installing Persistence**: It copies itself to a hidden directory and sets up registry keys to ensure it runs on system startup.

The C++ code is organized into the following files:

- `funLoader/funLoader.vcxproj`: The Visual Studio project file.
- `funLoader/load.cpp`: The main entry point of the application.
- `funLoader/apc.cpp`: Implements the APC injection technique.
- `funLoader/find.cpp`: Contains the logic for finding the target process.
- `funLoader/resolver.cpp`: Implements the dynamic API resolution.
- `funLoader/apis.h`, `funLoader/connector.h`, `funLoader/jitdecrypt.h`, `funLoader/resolver.h`, `funLoader/syscalls.h`: Header files defining data structures, function prototypes, and constants.
- `funLoader/syscalls.asm`: Assembly code for direct syscalls.

### Python Scripts

The Python scripts provide supporting functionality for the project:

- `compile.py`: Compiles the `funLoader` C++ project using MSBuild. It automatically finds the Visual Studio installation path.
- `encrypt.py`: Encrypts a given payload file using a custom algorithm.
- `binencode.py`: Encodes a binary file using a simple XOR cipher.
- `raw_to_shellcode.py`: Converts a raw binary file into a C-style shellcode array.
- `jit_test.py`: A test script to verify the encryption and decryption logic.

## How to Compile and Run

1. **Prerequisites**:
   - Windows operating system.
   - Visual Studio with C++ development tools installed.

2. **Compile the `funLoader` application**:
   ```bash
   python compile.py
   ```
   This will create `funLoader.exe` in the `x64/Release` directory.

3. **Prepare the payload**:
   You can use a tool like Metasploit or Cobalt Strike to generate shellcode. Save the raw shellcode to a file (e.g., `shellcode.bin`).

4. **Encrypt the payload**:
   ```bash
   python encrypt.py shellcode.bin encrypted_payload.bin 0x12345678 --format c_array
   ```
   This will create a C-style array of the encrypted payload in `encrypted_payload.bin`.

5. **Update the payload in `load.cpp`**:
   Replace the `payload` array in `funLoader/load.cpp` with the new encrypted payload.

6. **Recompile the project**:
   Run `python compile.py` again.

7. **Run `funLoader.exe`**:
   The executable will attempt to inject the shellcode into `notepad.exe`. It will also install itself for persistence.

## Disclaimer

This tool is intended for educational purposes only. The author is not responsible for any misuse of this software.
