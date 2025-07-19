#include <iostream>
#include <Windows.h>
#ifndef CONNECTOR_H
#define CONNECTOR_H
#ifdef __cplusplus
extern "C" {
#endif
	int findPID(const wchar_t* processName);
	int findExplorerPID();
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // CONNECTOR_H
