#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>

// Custom window messages (shared between mainwindow and pages)
#define WM_USER_LOG           (WM_USER + 1)   // lParam = new std::string*
#define WM_USER_ORDER_UPDATED (WM_USER + 2)

// UTF-8 <-> Wide string conversions
inline std::wstring toWide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (n <= 0) return L"";
    std::wstring w(n - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
    return w;
}

inline std::string toUtf8(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (n <= 0) return "";
    std::string s(n - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], n, nullptr, nullptr);
    return s;
}

// Get text from a Win32 EDIT or STATIC control as UTF-8 string
inline std::string getWindowText(HWND hwnd) {
    int len = GetWindowTextLengthW(hwnd);
    if (len <= 0) return "";
    std::wstring w(len, L'\0');
    GetWindowTextW(hwnd, &w[0], len + 1);
    return toUtf8(w);
}

// Set text on a Win32 control from UTF-8 string
inline void setWindowText(HWND hwnd, const std::string& text) {
    SetWindowTextW(hwnd, toWide(text).c_str());
}

// Get the directory of the running executable as UTF-8
inline std::string getExeDir() {
    wchar_t buf[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring path(buf);
    auto pos = path.rfind(L'\\');
    if (pos != std::wstring::npos) path = path.substr(0, pos);
    return toUtf8(path);
}

// Simple Win32 control creation helpers
inline HWND createLabel(HWND parent, const std::string& text,
                         int x, int y, int w, int h,
                         DWORD style = WS_CHILD | WS_VISIBLE | SS_LEFT) {
    return CreateWindowExW(0, L"STATIC", toWide(text).c_str(), style,
        x, y, w, h, parent, nullptr, GetModuleHandleW(nullptr), nullptr);
}

inline HWND createEdit(HWND parent, UINT id,
                        int x, int y, int w, int h,
                        bool multiline = false, bool readOnly = false) {
    DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
    if (multiline) style = WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL
                         | ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL;
    if (readOnly)  style |= ES_READONLY;
    return CreateWindowExW(0, L"EDIT", L"", style,
        x, y, w, h, parent, reinterpret_cast<HMENU>((UINT_PTR)id),
        GetModuleHandleW(nullptr), nullptr);
}

inline HWND createButton(HWND parent, const std::string& text, UINT id,
                          int x, int y, int w, int h) {
    return CreateWindowExW(0, L"BUTTON", toWide(text).c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, w, h, parent, reinterpret_cast<HMENU>((UINT_PTR)id),
        GetModuleHandleW(nullptr), nullptr);
}

inline HWND createCheckbox(HWND parent, const std::string& text, UINT id,
                             int x, int y, int w, int h) {
    return CreateWindowExW(0, L"BUTTON", toWide(text).c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        x, y, w, h, parent, reinterpret_cast<HMENU>((UINT_PTR)id),
        GetModuleHandleW(nullptr), nullptr);
}

inline HWND createComboBox(HWND parent, UINT id,
                             int x, int y, int w, int h) {
    return CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
        x, y, w, h, parent, reinterpret_cast<HMENU>((UINT_PTR)id),
        GetModuleHandleW(nullptr), nullptr);
}

// String helpers
inline std::string replaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.size(), to);
        pos += to.size();
    }
    return str;
}

inline bool startsWith(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

// Set a default font on a control
inline void setDefaultFont(HWND hwnd) {
    static HFONT hFont = nullptr;
    if (!hFont) {
        hFont = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    }
    SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
}
