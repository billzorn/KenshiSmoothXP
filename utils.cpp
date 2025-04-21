#include "utils.h"

void CreateConsoleWindow()
{
    AllocConsole();

    FILE* instream, * outstream, * errstream;
    static_cast<void>(freopen_s(&instream, "CONIN$", "r", stdin));
    static_cast<void>(freopen_s(&outstream, "CONOUT$", "w", stdout));
    static_cast<void>(freopen_s(&errstream, "CONOUT$", "w", stderr));
}

void DestroyConsoleWindow()
{
    FreeConsole();
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);
}

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

string FormatDouble(double value, int maxDigitsAfterPoint)
{
    ostringstream oss;
    oss << fixed << setprecision(maxDigitsAfterPoint) << value;
    string str = oss.str();

    str.erase(str.find_last_not_of('0') + 1, string::npos);
    if (str.back() == '.')
    {
        str.pop_back();
    }

    return str;
}