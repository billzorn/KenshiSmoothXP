#include <algorithm>
#include <unordered_map>
#include <string>
#include <sstream>
#include <iostream>
#include <cstring>

#include <keyboard_hotkey.h>
#include <Utils.h>

unordered_map<string, int> keyCodesMap = {

    {"A", 'A'}, {"B", 'B'}, {"C", 'C'}, {"D", 'D'}, {"E", 'E'},
    {"F", 'F'}, {"G", 'G'}, {"H", 'H'}, {"I", 'I'}, {"J", 'J'},
    {"K", 'K'}, {"L", 'L'}, {"M", 'M'}, {"N", 'N'}, {"O", 'O'},
    {"P", 'P'}, {"Q", 'Q'}, {"R", 'R'}, {"S", 'S'}, {"T", 'T'},
    {"U", 'U'}, {"V", 'V'}, {"W", 'W'}, {"X", 'X'}, {"Y", 'Y'},
    {"Z", 'Z'},

    {"1", '1'}, {"2", '2'}, {"3", '3'}, {"4", '4'}, {"5", '5'},
    {"6", '6'}, {"7", '7'}, {"8", '8'}, {"9", '9'}, {"0", '0'},

    {"F1", VK_F1}, {"F2", VK_F2}, {"F3", VK_F3}, {"F4", VK_F4},
    {"F5", VK_F5}, {"F6", VK_F6}, {"F7", VK_F7}, {"F8", VK_F8},
    {"F9", VK_F9}, {"F10", VK_F10}, {"F11", VK_F11}, {"F12", VK_F12},

    {"CTRL", VK_CONTROL}, {"LCTRL", VK_LCONTROL}, {"RCTRL", VK_RCONTROL},
    {"SHIFT", VK_SHIFT}, {"LSHIFT", VK_LSHIFT}, {"RSHIFT", VK_RSHIFT},
    {"ALT", VK_MENU}, {"LALT", VK_LMENU}, {"RALT", VK_RMENU},

    {"SPACE", VK_SPACE}, {"TAB", VK_TAB}, {"BACKSPACE", VK_BACK},
    {"INSERT", VK_INSERT}, {"DELETE", VK_DELETE}, {"HOME", VK_HOME},
    {"END", VK_END}, {"PGUP", VK_PRIOR}, {"PGDOWN", VK_NEXT},

    {"NUM0", VK_NUMPAD0}, {"NUM1", VK_NUMPAD1}, {"NUM2", VK_NUMPAD2},
    {"NUM3", VK_NUMPAD3}, {"NUM4", VK_NUMPAD4}, {"NUM5", VK_NUMPAD5},
    {"NUM6", VK_NUMPAD6}, {"NUM7", VK_NUMPAD7}, {"NUM8", VK_NUMPAD8},
    {"NUM9", VK_NUMPAD9}, {"NUMLOCK", VK_NUMLOCK},
    {"NUM/", VK_DIVIDE}, {"NUM*", VK_MULTIPLY}, {"NUM-", VK_SUBTRACT},
    {"NUM+", VK_ADD}, {"NUMENTER", VK_RETURN}, {"NUM.", VK_DECIMAL},

    {"-", VK_OEM_MINUS}, {",", VK_OEM_COMMA}, {".", VK_OEM_PERIOD},

    {"UP", VK_UP}, {"DOWN", VK_DOWN}, {"LEFT", VK_LEFT}, {"RIGHT", VK_RIGHT}
};

unordered_map<int, string> keyStringMap = ReverseMap(keyCodesMap);

template <typename A, typename B>
unordered_map<B, A> ReverseMap(const unordered_map<A, B>& mapForReverse)
{
    unordered_map<B, A> reversedMap;
    reversedMap.reserve(mapForReverse.size());

    for (const auto& element : mapForReverse)
    {
        reversedMap[element.second] = element.first;
    }

    return reversedMap;
}

vector<int> ParseHotkeyString(const string& hotkey, char delimiter)
{
    vector<string> keyNames = Split(hotkey, delimiter);

    vector<int> keys;
    keys.reserve(keyNames.size());

    for (auto& key : keyNames)
    {
        transform(key.begin(), key.end(), key.begin(), ::toupper);

        int keyCode = keyCodesMap[key];
        if (keyCode == NULL) continue;

        keys.push_back(keyCode);
    }

    return keys;
}

vector<string> HotkeyToStringList(vector<int>& hotkey)
{
    vector<string> keyNames;
    keyNames.reserve(hotkey.size());

    for (auto& key : hotkey)
    {
        if (!keyStringMap.contains(key)) continue;

        string keyName = keyStringMap[key];
        if (keyName.empty()) continue;

        keyNames.push_back(keyName);
    }

    return keyNames;
}

string HotkeyToString(vector<int>& hotkey)
{
    string hotkeyString;
    hotkeyString.reserve(32);

    size_t hotkeyCount = hotkey.size();
    for (int i = 0; i < hotkeyCount; i++)
    {
        if (!keyStringMap.contains(hotkey[i])) continue;

        string keyName = keyStringMap[hotkey[i]];
        if (i == 0) hotkeyString.append(keyName);
        else hotkeyString.append('+' + keyName);
    }

    return hotkeyString;
}