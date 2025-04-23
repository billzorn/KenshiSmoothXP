# KenshiSmoothXP
DLL plugin mod with changes to the way XP gain works in Kenshi:
- [Stochastic rounding](https://en.wikipedia.org/wiki/Rounding#Stochastic_rounding) is used to allow slow XP gain at high levels, rather than the gained XP always rounding down to 0.
- Custom [level multiplier curve](https://www.reddit.com/r/Kenshi/comments/1hisvzq/kenshi_fact_of_the_day_11/) allows for (very slow) XP gain past level 101.

## Building
Install/clone the requirements, open the solution with [Visual Studio](https://visualstudio.microsoft.com/vs/community/) (tested with VS 2022 Community), and hit Build.

Probably set the target to Release / x64.

## Requirements

### [MinHook](https://www.codeproject.com/KB/winsdk/LibMinHook.aspx)
Install with [vcpkg](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-msbuild?pivots=shell-powershell#1---set-up-vcpkg); the project is configured to use it as a static library.

`.\vcpkg install minhook:x64-windows-static`

### [SimpleIni](https://github.com/brofield/simpleini)
Also install with vcpkg as a static library. Visual Studio doesn't seem to have any issues finding it.

`.\vcpkg install simpleini:x64-windows-static`

### [pcg-cpp](https://www.pcg-random.org/)
This is a header-only C++ pseudorandom number generator library by Melissa O'Neill. Also install with vcpkg.

`.\vcpkg install pcg:x64-windows-static`
