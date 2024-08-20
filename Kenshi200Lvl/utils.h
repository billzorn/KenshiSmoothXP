#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>

#define ConsoleOut(formatString,...) printf(formatString"\n", __VA_ARGS__)

using namespace std;

string PathCombine(const string& path1, const string& path2);
string FormatDouble(double value, int maxDigitsAfterPoint = 6);
vector<string> Split(const string& s, char delim);
vector<string> Split(string s, string delimiter);
long long GetCurrentTimestampUs();

#endif