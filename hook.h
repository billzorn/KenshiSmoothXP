#pragma once

#include <MinHook.h>

#include <vector>
#include <string>
#include <span>
#include <fstream>
#include <iostream>

using namespace std;

DWORD RvaToOffset(DWORD rva, PIMAGE_SECTION_HEADER sectionHeader, unsigned int numberOfSections);
DWORD FindPatternInFile(const string& filename, const span<const BYTE>& pattern, const char* mask);
