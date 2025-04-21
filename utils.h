#pragma once

#include "framework.h"

#include <string>
#include <iomanip>
#include <sstream>

using namespace std;

#define ConsoleOut(formatString,...) printf(formatString"\n", __VA_ARGS__)

void CreateConsoleWindow();
void DestroyConsoleWindow();
string PathCombine(const string& path1, const string& path2);
string FormatDouble(double value, int maxDigitsAfterPoint = 6);
