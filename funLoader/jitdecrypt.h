#pragma once
#include <Windows.h>

__forceinline unsigned int rotr(unsigned int d, int n) {
    return (d >> n) | (d << (32 - n));
}

void JITDecrypt(BYTE* buf, SIZE_T len, DWORD keySeed) {
    DWORD key = keySeed ^ 0xDEADBEEF;
    for (SIZE_T i = 0; i < len; i++) {
        buf[i] ^= (BYTE)(key);
        key = rotr(key + buf[i] + 0x1337, 5);
    }
}
