#pragma once
#include <Windows.h>

void LaunchInMemoryModule(HANDLE targetProcess, void* buffer, size_t size);
