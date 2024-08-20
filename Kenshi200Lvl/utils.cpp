#include <iomanip>
#include <sstream>
#include <chrono>

#include <Utils.h>

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

vector<string> Split(const string& s, char delim)
{
    vector<string> result;
    stringstream ss(s);
    string item;

    while (getline(ss, item, delim))
    {
        result.push_back(item);
    }

    return result;
}

vector<string> Split(string s, string delimiter)
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    vector<string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != string::npos)
    {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

long long GetCurrentTimestampUs()
{
    return chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
}