# funLoader

`funLoader` is a small project that demonstrates how to load and execute shellcode in a remote process.

## How to Build and Run

### 1. Create a Payload

This project uses a simple XOR-encoded payload. You can use the `raw_to_shellcode.py` script to generate a payload from a raw binary file.

For example, to create a payload from a Metasploit shellcode file named `payload.bin`, you would run the following command:

```
python raw_to_shellcode.py payload.bin
```

This will output a C-style char array that you can copy and paste into the `load.cpp` file, replacing the existing `payload` variable.

### 2. Compile the Project

The easiest way to compile the project is to use the provided `compile.py` script. This script will automatically find your Visual Studio installation and build the project.

```
python compile.py
```

Alternatively, you can open the `funLoader.sln` file in Visual Studio and build the solution manually.

### 3. Run the Executable

Once the project is compiled, you can run the executable from the `x64/Release` directory.

```
x64/Release/funLoader.exe
```

The executable will inject the payload into a new instance of `explorer.exe` and execute it.

## How it Works

The project works in the following steps:

1.  **Find a target process:** The `findPID()` function in `find.cpp` finds the process ID of `explorer.exe`. If it can't find an existing instance, it creates a new one.
2.  **Allocate memory:** The `NtAllocateVirtualMemory` syscall is used to allocate memory in the target process.
3.  **Write the payload:** The `NtWriteVirtualMemory` syscall is used to write the XOR-encoded payload to the allocated memory.
4.  **Create a new thread:** The `NtCreateThreadEx` syscall is used to create a new thread in the target process, which starts execution at the beginning of the payload.
5.  **Close the handle:** The `NtClose` syscall is used to close the handle to the target process.
