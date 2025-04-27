#include "mod.h"

enum class LvlmultMethod {
    Vanilla = 0,
    Smooth = 1,
    Custom = 2
};

typedef void (*OriginalFunctionType)(float*, float, float);

class ModConfig
{
public:
    string configName = "KenshiSmoothXP.ini";
    string configPath;
    time_t configLastEditTimestamp = 0;

    LvlmultMethod lvlmultMethod = LvlmultMethod::Vanilla;
    double transitionPoint = 0.95;
    bool showConsole = TRUE;
    int32_t showStats = 0;

    bool modInitialized = FALSE;
} modConfig;

class ModData
{
public:
    LPVOID TargetProcessAbsoluteAddr = 0x0;
    OriginalFunctionType originalFunction = nullptr;
    bool hookEnabled = FALSE;

    pcg32 rng;
} modData;

bool modNeedsInit = TRUE;
mutex modDataMutex;

class ModStats
{
public:
    uint64_t countsByGain[8] = { 0 };
    uint64_t countsByLvl[13] = { 0 };

    uint64_t srndUpLo = 0;
    uint64_t srndDownLo = 0;
    uint64_t srndUpHi = 0;
    uint64_t srndDownHi = 0;
} modStats;

mutex modStatsMutex;

void ClearStats()
{
    lock_guard<mutex> lock(modStatsMutex);

    for (int i = 0; i < 8; i++) {
        modStats.countsByGain[i] = 0;
    }

    for (int i = 0; i < 13; i++) {
        modStats.countsByLvl[i] = 0;
    }

    modStats.srndUpLo = 0;
    modStats.srndDownLo = 0;
    modStats.srndUpHi = 0;
    modStats.srndDownHi = 0;
}

void UpdateStats(float lvl, float xp, bool roundUp, bool lvlHi)
{
    lock_guard<mutex> lock(modStatsMutex);

    if (xp < 0.00001f) {
        modStats.countsByGain[0]++;
    }
    else if (xp < 0.0001f) {
        modStats.countsByGain[1]++;
    }
    else if (xp < 0.001f) {
        modStats.countsByGain[2]++;
    }
    else if (xp < 0.01f) {
        modStats.countsByGain[3]++;
    }
    else if (xp < 0.1f) {
        modStats.countsByGain[4]++;
    }
    else if (xp < 1.0f) {
        modStats.countsByGain[5]++;
    }
    else if (xp < 10.0f) {
        modStats.countsByGain[6]++;
    }
    else {
        modStats.countsByGain[7]++;
    }

    if (lvl < 0.0f) {
        modStats.countsByLvl[0]++;
    }
    else if (lvl < 100.0f) {
        int lvl_idx = int(lvl / 10.0f) + 1;
        lvl_idx = std::max(1, lvl_idx);
        lvl_idx = std::min(lvl_idx, 10);
        modStats.countsByLvl[lvl_idx]++;
    }
    else if (lvl < 110.0f) {
        modStats.countsByLvl[11]++;
    }
    else {
        modStats.countsByLvl[12]++;
    }

    if (lvlHi) {
        if (roundUp) {
            modStats.srndUpHi++;
        }
        else {
            modStats.srndDownHi++;
        }
    }
    else {
        if (roundUp) {
            modStats.srndUpLo++;
        }
        else {
            modStats.srndDownLo++;
        }
    }
}

void ShowStats()
{
    lock_guard<mutex> lock(modStatsMutex);

    constexpr uint64_t histw = 50;

    static const char* gain_labels[8] = {
        "[ < 0.00001]",
        "[ < 0.0001 ]",
        "[ < 0.001  ]",
        "[ < 0.01   ]",
        "[ < 0.1    ]",
        "[ < 1.0    ]",
        "[ < 10.0   ]",
        "[<= 20.0   ]",
    };

    static const char* lvl_labels[13] = {
        "[    < 0   ]",
        "[  0 - 10  ]",
        "[ 10 - 20  ]",
        "[ 20 - 30  ]",
        "[ 30 - 40  ]",
        "[ 40 - 50  ]",
        "[ 50 - 60  ]",
        "[ 60 - 70  ]",
        "[ 70 - 80  ]",
        "[ 80 - 90  ]",
        "[ 90 - 100 ]",
        "[100 - 110 ]",
        "[110+      ]",
    };

    uint64_t gain_max = 0;
    for (int i = 0; i < 8; i++) {
        if (modStats.countsByGain[i] > gain_max) {
            gain_max = modStats.countsByGain[i];
        }
    }

    uint64_t lvl_max = 0;
    for (int i = 0; i < 13; i++) {
        if (modStats.countsByLvl[i] > lvl_max) {
            lvl_max = modStats.countsByLvl[i];
        }
    }

    auto currentZone = chrono::current_zone();

    auto now = chrono::system_clock::now();
    chrono::zoned_time localTime(currentZone, now);

    uint64_t totalMilliseconds = chrono::duration_cast<chrono::milliseconds>(localTime.get_local_time().time_since_epoch()).count();

    uint64_t hour = (totalMilliseconds / 3600000) % 24;
    uint64_t minute = (totalMilliseconds / 60000) % 60;
    uint64_t second = (totalMilliseconds / 1000) % 60;
    uint64_t millisecond = totalMilliseconds % 1000;

    printf("-- [%02lld:%02lld:%02lld.%03lld] global level gain summary --", hour, minute, second, millisecond);
    std::cout << std::endl;

    std::cout << "Exp gain by raw quantity:" << std::endl;
    for (int i = 0; i < 8; i++) {
        printf("  %s %8lld ", gain_labels[i], modStats.countsByGain[i]);
        for (uint64_t j = 0; j < (modStats.countsByGain[i] * histw) / gain_max; j++) {
            std::cout << "*";
        }
        std::cout << std::endl;
    }

    std::cout << "Exp gain by level of gaining character:" << std::endl;
    for (int i = 0; i < 13; i++) {
        printf("  %s %8lld ", lvl_labels[i], modStats.countsByLvl[i]);
        for (uint64_t j = 0; j < (modStats.countsByLvl[i] * histw) / lvl_max; j++) {
            std::cout << "*";
        }
        std::cout << std::endl;
    }

    constexpr double cutoff_xp = (1.0 / 262144.0);

    std::cout << "Stochastic roundings (xp increment above 1/262144)";
    std::cout << std::endl;
    std::cout << "  UP: " << modStats.srndUpLo << ", DOWN: " << modStats.srndDownLo;
    printf(" (%.3f%% up)", 100.0 * ((double)modStats.srndUpLo / (double)(modStats.srndUpLo + modStats.srndDownLo)));
    std::cout << std::endl;

    std::cout << "Stochastic roundings (xp increment below 1/262144, vanilla xp gain impossible)";
    std::cout << std::endl;
    std::cout << "  UP: " << modStats.srndUpHi << ", DOWN: " << modStats.srndDownHi;
    printf(" (%.3f%% up)", 100.0 * ((double)modStats.srndUpHi / (double)(modStats.srndUpHi + modStats.srndDownHi)));
    std::cout << std::endl << std::endl;
}

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

static inline float float_srnd29(double x, uint32_t r)
{
    // stochastic rounding; the double has 29 more bits of significand
    // compared to the float (53 - 24 = 29), so if r is uniform IID
    // random bits, our probability of rounding up if we add 29 of them
    // to the low bits of x is (low bits of x) / 2^29, or exactly what
    // we would expect if our output were a random variable that was
    // always a representble float, but with expected value exactly
    // equal to x.

    // This analysis doesn't work if x is small (subnormal for float)
    // in that case, we should have moved the bits higher up,
    // or if x is not finite, but we shouldn't be trying to round
    // any values that look like those (or in any case we shouldn't
    // care about the result)

    uint64_t u = *(uint64_t*)&x;
    u = (u + (uint64_t)(r >> 3)) & 0xffffffffe0000000ull;
    return (float)(*(double*)&u);
}

void HK_AdjustLevel_Vanilla_Clean(float* lvlPointer, float xpGained, float dFactor)
{
    if (xpGained <= 0.0) {
        // vanilla clamp, might as well return now
        return;
    }

    double lvl = *lvlPointer;
    double xp = xpGained;
    double d = dFactor;

    if (!(isfinite(lvl) && isfinite(xp) && isfinite(d))) {
        // the computation will fail anyway, so abort now.
        // also avoids checks later
        return;
    }

    double lvlmult = lvlmult_vanilla(lvl, d);

    if (lvlmult <= 0.0 || lvlmult > 20.0) {
        // vanilla's weird clamping on lvlmult
        return;
    }

    else if (xp > 20.0) {
        // not quite faithful vanilla clamp: cap raw xp gain at 20,
        // but don't set it to 0
        xp = 20.0;
    }

    double y = fma(lvlmult, xp, lvl);
    uint32_t r;
    {
        lock_guard<mutex> lock(modDataMutex);
        r = modData.rng();
    }
    *lvlPointer = float_srnd29(y, r);
}

void HK_AdjustLevel_Vanilla(float* lvlPointer, float xpGained, float dFactor)
{
    if (modConfig.showConsole && modConfig.showStats < -1) {
        ConsoleOut("ADJUST   lvl=%.8f, xp=%.8f, d=%.8f", *lvlPointer, xpGained, dFactor);
    }

    if (xpGained <= 0.0) {
        // vanilla clamp, might as well return now
        if (modConfig.showConsole && modConfig.showStats < -1) {
            ConsoleOut("  ZERO");
            ConsoleOut("");
        }
        return;
    }

    double lvl = *lvlPointer;
    double xp = xpGained;
    double d = dFactor;

    if (!(isfinite(lvl) && isfinite(xp) && isfinite(d))) {
        // the computation will fail anyway, so abort now.
        // also avoids checks later
        if (modConfig.showConsole && modConfig.showStats < -1) {
            ConsoleOut("  NOT FINITE ???");
            ConsoleOut("");
        }
        return;
    }

    double lvlmult = lvlmult_vanilla(lvl, d);

    if (lvlmult <= 0.0 || lvlmult > 20.0) {
        // vanilla's weird clamping on lvlmult
        if (modConfig.showConsole && modConfig.showStats < -1) {
            ConsoleOut("  CLAMP lvlmult=%.8f", lvlmult);
            ConsoleOut("");
        }
        return;
    }

    else if (xp > 20.0) {
        // not quite faithful vanilla clamp: cap raw xp gain at 20,
        // but don't set it to 0
        xp = 20.0;
    }

    double y = fma(lvlmult, xp, lvl);
    uint32_t r;
    {
        lock_guard<mutex> lock(modDataMutex);
        r = modData.rng();
    }
    float oldLvl = *lvlPointer;
    *lvlPointer = float_srnd29(y, r);

    double xp_gained = xp * lvlmult;
    bool rounded_up = y < (double)*lvlPointer;

    constexpr double cutoff_xp = (1.0 / 262144.0);
    UpdateStats(oldLvl, xpGained, rounded_up, xp_gained < cutoff_xp);

    if (modConfig.showConsole) {
        if (modConfig.showStats < -1) {
            ConsoleOut("  INC      +=%.8f, lvlmult=%.8f", xp_gained, lvlmult);
            if (oldLvl != *lvlPointer) {
                ConsoleOut("  UPDATE lvl=%.8f", *lvlPointer);
            }
            else {

                ConsoleOut("  NOP    lvl=%.8f", *lvlPointer);
            }
            ConsoleOut("");
        }
        else if (modConfig.showStats == -1) {
            if (oldLvl != *lvlPointer) {
                ConsoleOut("XP GAIN xp=%.8f, lvl=%.8f -> %.8f", xpGained, oldLvl, *lvlPointer);
            }
            else {

                ConsoleOut("XP NOP  xp=%.8f, lvl=%.8f", xpGained, oldLvl);
            }
        }
    }

    if (r < 0x0fffffff) {
        ShowStats();
    }
}

void HK_AdjustValueBasedOnFactors(float* valuePointer, float factor1, float factor2)
{
    float oldPointerValue = *valuePointer;

    float val;
    const float invFactor2 = 1.0f / factor2;

    float normalizedDifference = (factor2 - *valuePointer) * invFactor2;
    val = normalizedDifference * normalizedDifference;

    double lvl = *valuePointer;
    double d = factor2;

    ConsoleOut("LVLMULT lvl=%.16f, d=%.16f", lvl, d);

    double lm_orig = val;
    double lm_vanilla = lvlmult_vanilla(lvl, d);
    ConsoleOut("  orig     =%.16f", lm_orig);
    ConsoleOut("  vanilla  =%.16f", lm_vanilla);

    double a = 0.95;
    double t = a * d;
    if (lvl >= t) {
        double lm_mod = lvlmult_mod(lvl, d, t);
        double lm_ratio = lvlmult_ratio(lvl, d, a);
        ConsoleOut("  mod      =%.16f", lm_mod);
        ConsoleOut("  ratio    =%.16f", lm_ratio);

        double _1_m_a = 1.0 - a;
        double _1_m_a_cubed = _1_m_a * _1_m_a * _1_m_a;
        double lm_pre = lvlmult_ratio_precomp(lvl, d, a, _1_m_a_cubed);
        ConsoleOut("  precomp  =%.16f", lm_pre);
    }

    if (lvl >= d - 1.0) {
        double lm_dm1_ref = lvlmult_mod(lvl, d, d - 1.0);
        double lm_dm1 = lvlmult_dm1(lvl, d);
        ConsoleOut("  dm1 ref  =%.16f", lm_dm1_ref);
        ConsoleOut("  dm1      =%.16f", lm_dm1);
    }

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

    if (modNeedsInit) {
        ConsoleOut("    MOD NOT INITED ???");
    }
    else {
        uint32_t random_bias = modData.rng() >> 3;
        ConsoleOut("    RBIAS %8x", random_bias);
    }

    ConsoleOut("");
}

// take the lock on modData before calling these;
// the functions do not take the lock themselves to avoid
// interacting with it recursively

bool SetupLevelingHook(LPVOID absoluteAddr)
{
    if (MH_Initialize() != MH_OK)
    {
        cerr << "Failed to initialize MinHook." << endl;
        return false;
    }

    OriginalFunctionType modFunction;
    switch (modConfig.lvlmultMethod)
    {
    case LvlmultMethod::Vanilla:
        if (modConfig.showConsole) {
            modFunction = &HK_AdjustLevel_Vanilla;
        }
        else {
            modFunction = &HK_AdjustLevel_Vanilla_Clean;
        }
        break;
    case LvlmultMethod::Smooth:
    case LvlmultMethod::Custom:
    default:
        // should be unreachable
        //if (modConfig.showConsole) {
        //    modFunction = &HK_AdjustLevel_Vanilla;
        //}
        //else {
        //    modFunction = &HK_AdjustLevel_Vanilla_Clean;
        //}
        modFunction = &HK_AdjustValueBasedOnFactors;
        break;
    }

    if (MH_CreateHook(absoluteAddr, modFunction, (LPVOID*)&modData.originalFunction) != MH_OK)
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

    modData.hookEnabled = TRUE;
    return true;
}

bool DisableLevelingHook(LPVOID absoluteAddr)
{
    if (MH_DisableHook(absoluteAddr) != MH_OK)
    {
        cerr << "Failed to disable the hook." << endl;
        return false;
    }

    modData.hookEnabled = FALSE;
    return true;
}

void DetachDLL(LPVOID absoluteAddr, HMODULE hModule)
{
	if (modData.hookEnabled && absoluteAddr)
	{
		DisableLevelingHook(absoluteAddr);
	}
	FreeLibraryAndExitThread(hModule, 0);
}

//

void MainThreadFunction(HMODULE hModule)
{
	// make sure the process attach entrypoint has initialized the mod first
	bool waitForInit = TRUE;
	while (waitForInit) {
		{ lock_guard<mutex> lock(modDataMutex); waitForInit = modNeedsInit; }
		if (waitForInit) {
			ConsoleOut("sleep 100ms waiting for mod init...");
			this_thread::sleep_for(chrono::milliseconds(100));
		}
	}
	ConsoleOut("Hello World from KenshiSmoothXP");
	ConsoleOut("");

    //

    lock_guard<mutex> lock(modDataMutex);

    if (modConfig.modInitialized) {
        ConsoleOut("Mod is already initialized, thread %d returning.", this_thread::get_id());
        ConsoleOut("");
        return;
    }

    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    string directory = filesystem::path(exePath).parent_path().string();
    string exeName = filesystem::path(exePath).filename().generic_string();

    string configPath = PathCombine(directory, modConfig.configName);
    bool configExists = filesystem::exists(configPath);
    if (configExists)
    {
        // read the config

        CSimpleIniA ini;
        ini.SetUnicode();

        modConfig.configPath = configPath;
        modConfig.configLastEditTimestamp = filesystem::last_write_time(modConfig.configPath).time_since_epoch().count();

        SI_Error rc = ini.LoadFile(modConfig.configPath.c_str());
        if (rc == SI_OK)
        {
            modConfig.showConsole = !!ini.GetLongValue("Parameters", "ShowConsole", modConfig.showConsole);

            if (modConfig.showConsole) {
                ConsoleOut("**** we would have created the console window here ****");
                ConsoleOut("");
            }

            auto lvlmultMethodData = ini.GetLongValue("Parameters", "LvlmultMethod", (long)modConfig.lvlmultMethod);
            switch (lvlmultMethodData) {
            case (long)LvlmultMethod::Vanilla:
                modConfig.lvlmultMethod = LvlmultMethod::Vanilla;
                break;
            case (long)LvlmultMethod::Smooth:
                modConfig.lvlmultMethod = LvlmultMethod::Smooth;
                break;
            case (long)LvlmultMethod::Custom:
                modConfig.lvlmultMethod = LvlmultMethod::Custom;
                break;
            default:
                if (modConfig.showConsole) {
                    ConsoleOut("Unknown LvlmultMethod %d, setting to 0 (vanilla)", lvlmultMethodData);
                    ConsoleOut("");
                }
                modConfig.lvlmultMethod = LvlmultMethod::Vanilla;
                break;
            }

            auto transitionPointData = ini.GetDoubleValue("Parameters", "TransitionPoint", modConfig.transitionPoint);
            if (transitionPointData < 0.0) {
                if (modConfig.showConsole) {
                    ConsoleOut("TransitionPoint %f must be at least 0, setting to 0.0", transitionPointData);
                    ConsoleOut("");
                }
                modConfig.transitionPoint = 0.0;
            }
            else if (transitionPointData <= 1.0) {
                modConfig.transitionPoint = transitionPointData;
            }
            else {
                if (modConfig.showConsole) {
                    ConsoleOut("TransitionPoint %f must be at most 1, setting to 1.0", transitionPointData);
                    ConsoleOut("");
                }
                modConfig.transitionPoint = 1.0;
            }

            modConfig.showStats = ini.GetLongValue("Parameters", "ShowStats", modConfig.showStats);

            if (modConfig.showConsole) {
                ConsoleOut("Loaded configuration from %s", modConfig.configPath.c_str());
                ConsoleOut("  LvlmultMethod   = %d", (long)modConfig.lvlmultMethod);
                ConsoleOut("  TransitionPoint = %.8f", modConfig.transitionPoint);
                ConsoleOut("  ShowConsole     = %d", modConfig.showConsole);
                ConsoleOut("  ShowStats       = %d", modConfig.showStats);
                ConsoleOut("");
            }
        }
        else {
            if (modConfig.showConsole) {
                ConsoleOut("**** we would have created the console window here ****");
                ConsoleOut("");
            }

            if (modConfig.showConsole) {
                ConsoleOut("Failed to load config from %s, error=%d", modConfig.configPath.c_str(), rc);
                ConsoleOut("");
                ConsoleOut("Default configuration:");
                ConsoleOut("  LvlmultMethod   = %d", (long)modConfig.lvlmultMethod);
                ConsoleOut("  TransitionPoint = %.8f", modConfig.transitionPoint);
                ConsoleOut("  ShowConsole     = %d", modConfig.showConsole);
                ConsoleOut("  ShowStats       = %d", modConfig.showStats);
                ConsoleOut("");
            }
        }
    }
    else {
        // use default settings, already loaded

        modConfig.configPath = "";
        modConfig.configLastEditTimestamp = 0;

        if (modConfig.showConsole) {
            ConsoleOut("**** we would have created the console window here ****");
            ConsoleOut("");
        }

        ConsoleOut("No config file found at '%s', using default configuration:", modConfig.configPath.c_str());
        ConsoleOut("  LvlmultMethod   = %d", (long)modConfig.lvlmultMethod);
        ConsoleOut("  TransitionPoint = %.8f", modConfig.transitionPoint);
        ConsoleOut("  ShowConsole     = %d", modConfig.showConsole);
        ConsoleOut("  ShowStats       = %d", modConfig.showStats);
        ConsoleOut("");
    }

    //

    DWORD relativeAddr = FindPatternInFile(exePath, LEVELING_FUNCTION_PATTERN, LEVELING_FUNCTION_MASK);
    if (relativeAddr)
    {
        modData.TargetProcessAbsoluteAddr = (LPVOID)(relativeAddr + (DWORD_PTR)GetModuleHandleA(exeName.c_str()));
        if (SetupLevelingHook(modData.TargetProcessAbsoluteAddr))
        {
            ConsoleOut("Enabling hook...");
            EnableLevelingHook(modData.TargetProcessAbsoluteAddr);

            ConsoleOut("  Target address: %16llx", modData.TargetProcessAbsoluteAddr);
            ConsoleOut("  Original fn:    %16llx", modData.originalFunction);

            ConsoleOut("Function successfully hooked. Do not close this window.");
            ConsoleOut("");
        }
        else {
            ConsoleOut("Failed to set up hook.");
            ConsoleOut("");
        }
    }
    else {
        ConsoleOut("Error: Cannot find target function in %s", exePath);
        ConsoleOut("");
    }

    ConsoleOut("Initialization complete, thread %d returning.", this_thread::get_id());
    ConsoleOut("");

    modConfig.modInitialized = TRUE;
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
	{
		lock_guard<mutex> lock(modDataMutex);

		CreateConsoleWindow();

		ConsoleOut("DLL_PROCESS_ATTACH");

		if (modNeedsInit) {
			pcg_extras::seed_seq_from<random_device> seed_source;
			modData.rng.seed(seed_source);

			ConsoleOut("-- thread id %d inited rng --", this_thread::get_id());

            modNeedsInit = FALSE;
		}

		ConsoleOut("");
	}
	break;
	case DLL_THREAD_ATTACH:
	{
		// ConsoleOut("DLL_THREAD_ATTACH");
	}
	break;
	case DLL_THREAD_DETACH:
	{
		// ConsoleOut("DLL_THREAD_DETACH");
	}
	break;
	case DLL_PROCESS_DETACH:
	{
		lock_guard<mutex> lock(modDataMutex);

		ConsoleOut("DLL_PROCESS_DETACH");

		// not clear if this is doing anything, or if it even needs to...
		DetachDLL(modData.TargetProcessAbsoluteAddr, hModule);

		ConsoleOut("detached and unhooked...");
		ConsoleOut("");
	}
	break;
	}
	return TRUE;
}
