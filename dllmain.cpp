#include "mod.h"

class ModData
{
public:
    LPVOID TargetProcessAbsoluteAddr = 0x0;

} modData;

mutex modDataMutex;

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
    float oldPointerValue = *valuePointer;

    float val;
    const float invFactor2 = 1.0f / factor2;

    float normalizedDifference = (factor2 - *valuePointer) * invFactor2;
    val = normalizedDifference * normalizedDifference;

    // NaN Check
    if (val == val && val > 0.0f && factor1 > 0.0f && factor1 <= 20.0f && val <= 20.0f)
    {
        *valuePointer += val * factor1;
    }

    lock_guard<mutex> lock(modDataMutex);

    ConsoleOut("ADJUST   vp=%.8f, f1=%.8f, f2=%.8f", oldPointerValue, factor1, factor2);
    ConsoleOut("  INC     +=%.8f, lm=%.8f", val * factor1, val);
    if (oldPointerValue != *valuePointer) {
        ConsoleOut("  UPDATE vp=%.8f", *valuePointer);
    }
    else {
        ConsoleOut("  NOP    vp=%.8f", *valuePointer);
    }
    ConsoleOut("");
}

//

bool SetupLevelingHook(LPVOID absoluteAddr)
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

    return true;
}

bool EnableLevelingHook(LPVOID absoluteAddr)
{
    if (MH_EnableHook(absoluteAddr) != MH_OK)
    {
        cerr << "Failed to enable the hook." << endl;
        return false;
    }

    return true;
}

bool DisableLevelingHook(LPVOID absoluteAddr)
{
    if (MH_DisableHook(absoluteAddr) != MH_OK)
    {
        cerr << "Failed to disable the hook." << endl;
        return false;
    }

    return true;
}

void DetachDLL(LPVOID absoluteAddr, HMODULE hModule)
{
    if (absoluteAddr) DisableLevelingHook(absoluteAddr);
    FreeLibraryAndExitThread(hModule, 0);
}

//

void MainThreadFunction(HMODULE hModule)
{
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    string directory = filesystem::path(exePath).parent_path().string();
    string exeName = filesystem::path(exePath).filename().generic_string();

    //

    CreateConsoleWindow();

    ConsoleOut("Hello World from KenshiSmoothXP");
    ConsoleOut("");

    //

    DWORD relativeAddr = FindPatternInFile(exePath, LEVELING_FUNCTION_PATTERN, LEVELING_FUNCTION_MASK);
    if (relativeAddr)
    {
        modData.TargetProcessAbsoluteAddr = (LPVOID)(relativeAddr + (DWORD_PTR)GetModuleHandleA(exeName.c_str()));
        if (SetupLevelingHook(modData.TargetProcessAbsoluteAddr))
        {
            ConsoleOut("Enabling hook...");
            EnableLevelingHook(modData.TargetProcessAbsoluteAddr);

            ConsoleOut("Function successfully hooked. Do not close this window.");
            ConsoleOut("");
        }
        else {
            ConsoleOut("Failed to set up hook.");
        }
    }
    else {
        ConsoleOut("Error: Cannot find target function in %s", exePath);
    }

    //// I'm not sure if we needed the while(sleep) forever here...
    //DetachDLL(modData.TargetProcessAbsoluteAddr, hModule);
    //this_thread::sleep_for(chrono::milliseconds(200));
    //exit(1);
}

extern "C" void __declspec(dllexport) dllStartPlugin(void)
{
    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThreadFunction, 0, 0, nullptr);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
