#ifndef KEYBOARD_HOTKEY_H
#define KEYBOARD_HOTKEY_H

#include <Windows.h>
#include <unordered_map>
#include <vector>
#include <string>

using namespace std;

vector<int> ParseHotkeyString(const string& hotkey, char delimiter);
vector<string> HotkeyToStringList(vector<int>& hotkey);
string HotkeyToString(vector<int>& hotkey);

template <typename A, typename B>
unordered_map<B, A> ReverseMap(const unordered_map<A, B>& mapForReverse);
bool IsHotkeyPressed(const std::vector<int>& keys);

#endif