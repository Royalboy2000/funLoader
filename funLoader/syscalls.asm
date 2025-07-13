.code

PUBLIC NtAllocateVirtualMemory
NtAllocateVirtualMemory proc
    mov r10, rcx
    mov eax, 18h
    syscall
    ret
NtAllocateVirtualMemory endp

PUBLIC NtWriteVirtualMemory
NtWriteVirtualMemory proc
    mov r10, rcx
    mov eax, 3ah
    syscall
    ret
NtWriteVirtualMemory endp

PUBLIC NtCreateThreadEx
NtCreateThreadEx proc
    mov r10, rcx
    mov eax, 0c1h
    syscall
    ret
NtCreateThreadEx endp

PUBLIC NtClose
NtClose proc
    mov r10, rcx
    mov eax, 0fh
    syscall
    ret
NtClose endp

end
