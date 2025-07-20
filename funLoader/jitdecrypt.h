#pragma once
#include <Windows.h>

__forceinline unsigned int rotr(unsigned int d, int n) {
    return (d >> n) | (d << (32 - n));
}

inline void JITDecrypt(BYTE* buf, SIZE_T len, DWORD keySeed) {
    DWORD key = keySeed ^ 0xDEADBEEF;
    for (SIZE_T i = 0; i < len; i++) {
        BYTE original_byte = buf[i];
        buf[i] ^= (BYTE)(key);
        key = rotr(key + original_byte + 0x1337, 5);
    }
}
