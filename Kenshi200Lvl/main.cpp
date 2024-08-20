#include <main.h>

using namespace std;

string targetProcessName = "kenshi_x64.exe";
DWORD targetProcessAddr = 0x0;
float maxLevel = 201.0f;
float fadeLevel = 70.0f;
bool beepSound = true;
bool showConsole = false;

string exitHotkey = "Ctrl+Alt+Z";

vector<BYTE> LEVELING_FUNCTION_PATTERN = {

    0x40, 0x53, 0x48, 0x83, 0xEC, 0x30, 0xF3, 0x0F,
    0x10, 0x05, 0x0A, 0x65, 0xDB, 0x00, 0x0F, 0x29,
    0x74, 0x24, 0x20, 0xF3, 0x0F, 0x10, 0x31, 0x0F,
    0x28, 0xDA, 0x48, 0x8B, 0xD9, 0xF3, 0x0F, 0x5E,
    0xC2, 0xF3, 0x0F, 0x5C, 0xDE, 0xF3, 0x0F, 0x59,
    0xD8, 0x0F, 0x57, 0xC0, 0x0F, 0x2F, 0xC1, 0xF3,
    0x0F, 0x59, 0xDB, 0x73, 0x47, 0x0F, 0x2F, 0xC3,
    0x73, 0x42, 0xF3, 0x0F, 0x10, 0x05, 0x56, 0xD3,
    0xDB, 0x00, 0x0F, 0x2F, 0xC8, 0x77, 0x35, 0x0F,
    0x2F, 0xD8, 0x77, 0x30, 0xF3, 0x0F, 0x59, 0xD9,
    0xF3, 0x0F, 0x58, 0xDE, 0xF3, 0x0F, 0x11, 0x19,
    0x0F, 0x28, 0xC3, 0xFF, 0x15, 0x27, 0x2B, 0x98,
    0x01, 0x84, 0xC0, 0x74, 0x17, 0x0F, 0x28, 0xC6,
    0xF3, 0x0F, 0x11, 0x33, 0xFF, 0x15, 0x16, 0x2B,
    0x98, 0x01, 0x84, 0xC0, 0x74, 0x06, 0xC7, 0x03,
    0x00, 0x00, 0xA0, 0x41, 0x0F, 0x28, 0x74, 0x24,
    0x20, 0x48, 0x83, 0xC4, 0x30, 0x5B, 0xC3, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
};

const char LEVELING_FUNCTION_MASK[] =
    "xxxxxxxx"
    "xx????xx"
    "xxxxxxxx"
    "xxxxxxxx"
    "xxxxxxxx"
    "xxxxxxxx"
    "xxx??xxx"
    "??xxxx??"
    "??xxx??x"
    "xx??xxxx"
    "xxxxxxxx"
    "xxxxx???"
    "?xx??xxx"
    "xxxxxx??"
    "??xx??xx"
    "xxxxxxxx"
    "xxxxxxxx"
    "xxxxxxxx";

//

typedef void (*OriginalFunctionType)(float*, float, float);
OriginalFunctionType originalFunction = nullptr;

void HK_AdjustValueBasedOnFactors(float* valuePointer, float factor1, float factor2)
{
    float val;
    const float invFactor2 = 1.0f / factor2;

    if (*valuePointer < fadeLevel)
    {
        float normalizedDifference = (factor2 - *valuePointer) * invFactor2;
        val = normalizedDifference * normalizedDifference;
    }
    else
    {
        float normalizedProgress = (*valuePointer - fadeLevel) / (maxLevel - fadeLevel);
        float baseDifficulty = (factor2 - fadeLevel) * invFactor2;
        baseDifficulty *= baseDifficulty;

        val = baseDifficulty * (1.0f - normalizedProgress);
    }

    // NaN Check
    if (val == val && val > 0.0f && factor1 > 0.0f && factor1 <= 20.0f && val <= 20.0f)
    {
        *valuePointer += val * factor1;
    }
}

void DetachDLL(LPVOID absoluteAddr, HMODULE hModule)
{
    if (absoluteAddr) MH_DisableHook(absoluteAddr);
    FreeLibraryAndExitThread(hModule, 0);
}

bool SetupHook(LPVOID absoluteAddr)
{
    if (MH_Initialize() != MH_OK)
    {
        cerr << "Failed to initialize MinHook." << endl;
        return false;
    }

    if (MH_CreateHook(absoluteAddr, &HK_AdjustValueBasedOnFactors, (LPVOID*)&originalFunction) != MH_OK)
    {
        cerr << "Failed to create the hook." << endl;
        return false;
    }

    if (MH_EnableHook(absoluteAddr) != MH_OK)
    {
        cerr << "Failed to enable the hook." << endl;
        return false;
    }

    return true;
}

void ProcessHotkeyPress(HMODULE hModule, vector<int> hotkeyCodes)
{
    HWND consoleWindow = GetConsoleWindow();
    while (true)
    {
        if (IsHotkeyPressed(hotkeyCodes))
        {
            if (beepSound) Beep(1500, 200);

            this_thread::sleep_for(chrono::milliseconds(300));

            if (showConsole) DestroyConsoleWindow();

            LPVOID absoluteAddr = (LPVOID)(targetProcessAddr + (DWORD_PTR)GetModuleHandleA(targetProcessName.c_str()));
            DetachDLL(absoluteAddr, hModule);

            break;
        }

        this_thread::sleep_for(chrono::milliseconds(50));
    }
}

void MainThreadFunction(HMODULE hModule)
{
    vector<int> defaultHotkeyCodes = ParseHotkeyString(exitHotkey, '+');

    //

    char dllPath[MAX_PATH];
    GetModuleFileNameA(hModule, dllPath, MAX_PATH);
    string directory = filesystem::path(dllPath).parent_path().string();

    string configPath = PathCombine(directory, "config.ini");

    //

    CSimpleIniA ini;
    ini.SetUnicode();

    SI_Error rc = ini.LoadFile(configPath.c_str());
    if (rc != SI_OK)
    {
        if (showConsole)
        {
            CreateConsoleWindow();
        }
        cout << "Failed to read config.ini. Loading a default values ..." << endl;
    }
    else
    {
        targetProcessName = ini.GetValue("Parameters", "Process", targetProcessName.c_str());
        maxLevel = max<float>((float)ini.GetDoubleValue("Parameters", "Max Level", maxLevel), 101.0f);
        fadeLevel = clamp<float>((float)ini.GetDoubleValue("Parameters", "Fade Level", fadeLevel), 0.0f, 101.0f);
        exitHotkey = ini.GetValue("Parameters", "Exit Hotkey", exitHotkey.c_str());
        beepSound = !!ini.GetLongValue("Parameters", "Beep", beepSound);
        showConsole = !!ini.GetLongValue("Parameters", "Debug Console", showConsole);

        if (showConsole)
        {
            CreateConsoleWindow();
        }
    }

    cout << "Loading config from: " << configPath << " ..." << endl;

    vector<int> hotkeyCodes = ParseHotkeyString(exitHotkey, '+');

    if (hotkeyCodes.size() <= 0)
    {
        hotkeyCodes = defaultHotkeyCodes;
    }

    //

    string hotkeyString = HotkeyToString(hotkeyCodes);

    ConsoleOut("");
    ConsoleOut("Process: %s", targetProcessName.c_str());
    ConsoleOut("Max Level: %s", FormatDouble(maxLevel, 4).c_str());
    ConsoleOut("Fade Level: %s", FormatDouble(fadeLevel, 4).c_str());
    ConsoleOut("Exit Hotkey: %s", hotkeyString.c_str());
    ConsoleOut("");

    //
    
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    targetProcessAddr = FindPatternInFile(exePath, LEVELING_FUNCTION_PATTERN, LEVELING_FUNCTION_MASK);
    if (targetProcessAddr)
    {
        LPVOID absoluteAddr = (LPVOID)(targetProcessAddr + (DWORD_PTR)GetModuleHandleA(targetProcessName.c_str()));
        if (SetupHook(absoluteAddr))
        {
            cout << "Function successfully hooked. Do not close this window" << endl;

            ProcessHotkeyPress(hModule, hotkeyCodes);

            return;
        }
    }

    cout << "Error: Cannot find target function in " << exePath << endl;

    DetachDLL(0, hModule);
    this_thread::sleep_for(chrono::milliseconds(200));
    exit(1);
}

bool consoleCreated = false;

int APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThreadFunction, hModule, 0, nullptr);
    }

    return TRUE;
}