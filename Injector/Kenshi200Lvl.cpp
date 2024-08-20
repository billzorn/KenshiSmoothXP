#include <iostream>
#include <vector>
#include <thread>
#include <fstream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>

#include <SimpleIni.h>

using namespace std;

#define ConsoleOut(formatString,...) printf(formatString"\n", __VA_ARGS__)

string PathCombine(const string& path1, const string& path2)
{
    if (path1.empty()) return path2;
    if (path2.empty()) return path1;

    char sep = '/';
#ifdef _WIN32
    sep = '\\';
#endif

    string result = path1;
    if (result.back() != sep)
    {
        result += sep;
    }
    result += path2;

    return result;
}

DWORD GetProcessIdByName(string processName)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe))
    {
        CloseHandle(hSnapshot);
        return 0;
    }

    WCHAR wProcessName[260];
    MultiByteToWideChar(CP_ACP, 0, processName.c_str(), -1, wProcessName, sizeof(wProcessName) / sizeof(wProcessName[0]));

    do
    {
        if (wcscmp(pe.szExeFile, wProcessName) == 0)
        {
            CloseHandle(hSnapshot);
            return pe.th32ProcessID;
        }
    } while (Process32Next(hSnapshot, &pe));

    CloseHandle(hSnapshot);
    return 0;
}

bool InjectDLL(DWORD processId, const char* dllPath, const char* iniPath)
{
    if (!filesystem::exists(dllPath))
    {
        cerr << "Dll not exists" << endl;
        return false;
    }

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess)
    {
        cerr << "Failed to open target process" << endl;
        return false;
    }

    void* pDllPath = VirtualAllocEx(hProcess, 0, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
    if (!pDllPath)
    {
        cerr << "Failed to allocate memory in target process" << endl;
        CloseHandle(hProcess);
        return false;
    }

    if (!WriteProcessMemory(hProcess, pDllPath, dllPath, strlen(dllPath) + 1, 0))
    {
        cerr << "Failed to write into target process's memory" << endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hLoadThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, pDllPath, 0, 0);
    if (!hLoadThread)
    {
        cerr << "Failed to create remote thread in target process" << endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hLoadThread, INFINITE);

    VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
    CloseHandle(hLoadThread);

    return true;
}

int main()
{
    string targetProcess = "kenshi_x64.exe";
    string dllName = "Kenshi200Lvl.dll";
    int injectingDelayMs = 1500;
    int timeoutTimeMs = 4000;

    //

    char currentPath[MAX_PATH];
    GetModuleFileNameA(NULL, currentPath, MAX_PATH);
    string directory = filesystem::path(currentPath).parent_path().string();

    string configPath = PathCombine(directory, "config.ini");
    cout << "Loading config from: " << configPath << " ..." << endl;

    CSimpleIniA ini;
    ini.SetUnicode();

    SI_Error rc = ini.LoadFile(configPath.c_str());
    if (rc != SI_OK)
    {
        cout << "Failed to read config.ini. Loading a default values ..." << endl;
    }
    else
    {
        targetProcess = ini.GetValue("Parameters", "Process", targetProcess.c_str());
        dllName = ini.GetValue("Parameters", "Dll", dllName.c_str());
    }

    ConsoleOut("");
    ConsoleOut("Process: %s", targetProcess.c_str());
    ConsoleOut("Dll: %s", dllName.c_str());
    ConsoleOut("");

    //

    DWORD processId = 0;

    cout << "Waiting for " << targetProcess.c_str() << endl;

    while (!(processId = GetProcessIdByName(targetProcess.c_str())))
    {
        this_thread::sleep_for(chrono::milliseconds(500));
    }

    cout << targetProcess.c_str() << " found! Injecting in " << injectingDelayMs / 1000.0 << " sec" << endl;
    this_thread::sleep_for(chrono::milliseconds(injectingDelayMs));

    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    string path(exePath);
    size_t pos = path.find_last_of("\\/");
    path = path.substr(0, pos + 1);

    if (InjectDLL(processId, (path + dllName).c_str(), (path + "config.ini").c_str()))
    {
        cout << "DLL injected! Closing this window in " << timeoutTimeMs / 1000.0 << " sec ..." << endl;
        this_thread::sleep_for(chrono::milliseconds(timeoutTimeMs));
    }
    else
    {
        cerr << "DLL inject FAILED" << endl;
        system("pause");
    }

    //

    return 0;
}