#pragma once
#include <string>

bool CopySelfToAppData(std::wstring& outDestPath);
bool RegisterLogonTask(const std::wstring& exePath);
