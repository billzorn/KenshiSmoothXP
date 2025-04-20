#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <cstdio>

using namespace std;

#define ConsoleOut(formatString,...) printf(formatString"\n", __VA_ARGS__)

string PathCombine(const string& path1, const string& path2);
string FormatDouble(double value, int maxDigitsAfterPoint = 6);

#endif