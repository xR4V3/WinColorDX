#include <windows.h>
#include <iostream>
#include <fstream>
#include <shellapi.h>
#include <strsafe.h>
#include <math.h>      // для функции pow
#include <sstream>     // для использования wstringstream
#include "resource.h"

struct GammaRamp {
    WORD red[256];
    WORD green[256];
    WORD blue[256];
};

HINSTANCE hInst;
NOTIFYICONDATAW nid;
bool isGammaChanged = false;

// Функция для получения текста последней ошибки
std::wstring getLastErrorString() {
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0) {
        return L"No error";
    }

    LPWSTR messageBuffer = nullptr;
    size_t size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, NULL);

    std::wstring message(messageBuffer, size);
    LocalFree(messageBuffer);
    return message;
}

// Функция для установки гамма-кривой на конкретном устройстве
void setGammaOnDevice(HDC hDC, float gamma) {
    GammaRamp ramp;
    for (int i = 0; i < 256; i++) {
        int value = static_cast<int>(pow(i / 255.0f, gamma) * 65535.0f);
        value = min(max(value, 0), 65535);
        ramp.red[i] = ramp.green[i] = ramp.blue[i] = static_cast<WORD>(value);
    }

    if (!SetDeviceGammaRamp(hDC, &ramp)) {
    }
}

// Функция для восстановления исходной гамма-кривой на конкретном устройстве
void restoreOriginalGammaOnDevice(HDC hDC) {
    GammaRamp ramp;
    for (int i = 0; i < 256; i++) {
        ramp.red[i] = ramp.green[i] = ramp.blue[i] = (WORD)(i << 8);
    }

    if (!SetDeviceGammaRamp(hDC, &ramp)) {
    }
}

// Функция для установки гамма-кривой на всех мониторах
void setGamma(float gamma) {

    DISPLAY_DEVICE displayDevice;
    displayDevice.cb = sizeof(DISPLAY_DEVICE);
    int deviceIndex = 0;

    while (EnumDisplayDevices(NULL, deviceIndex, &displayDevice, 0)) {
        if (displayDevice.StateFlags & DISPLAY_DEVICE_ACTIVE) {
            HDC hDC = CreateDC(NULL, displayDevice.DeviceName, NULL, NULL);
            if (hDC) {
                setGammaOnDevice(hDC, gamma);
                DeleteDC(hDC);
            }
            else {
            }
        }
        deviceIndex++;
    }
}

// Функция для восстановления исходной гамма-кривой на всех мониторах
void restoreOriginalGamma() {

    DISPLAY_DEVICE displayDevice;
    displayDevice.cb = sizeof(DISPLAY_DEVICE);
    int deviceIndex = 0;

    while (EnumDisplayDevices(NULL, deviceIndex, &displayDevice, 0)) {
        if (displayDevice.StateFlags & DISPLAY_DEVICE_ACTIVE) {
            HDC hDC = CreateDC(NULL, displayDevice.DeviceName, NULL, NULL);
            if (hDC) {
                restoreOriginalGammaOnDevice(hDC);
                DeleteDC(hDC);
            }
            else {
            }
        }
        deviceIndex++;
    }
}

// Функция для переключения гамма-кривой
void toggleGamma() {

    if (isGammaChanged) {
        restoreOriginalGamma();
    }
    else {
        setGamma(0.3f);
    }
    isGammaChanged = !isGammaChanged;
}

// Низкоуровневый обработчик клавиатуры
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;
        if (wParam == WM_KEYDOWN && pKeyBoard->vkCode == VK_HOME) {
            toggleGamma();
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Создание значка в трее
void createTrayIcon(HWND hwnd) {

    ZeroMemory(&nid, sizeof(NOTIFYICONDATAW));
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hwnd;
    nid.uID = 1001;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_APP + 1;
    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_WINCOLORDX));

    HRESULT hr = StringCchCopyW(nid.szTip, ARRAYSIZE(nid.szTip), L"WinColor");
    if (FAILED(hr)) {
    }

    if (!Shell_NotifyIconW(NIM_ADD, &nid)) {
    }
    else {
    }
}

// Удаление значка из трея
void removeTrayIcon() {

    if (!Shell_NotifyIconW(NIM_DELETE, &nid)) {
    }
    else {
    }
}

// Обработчик сообщений окна
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        createTrayIcon(hwnd);
        break;
    case WM_APP + 1:
        if (lParam == WM_RBUTTONUP) {
            POINT cursor;
            GetCursorPos(&cursor);
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, 1, L"Exit");
            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
        }
        break;
    case WM_CLOSE:
        removeTrayIcon();
        restoreOriginalGamma();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Основная функция приложения
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;

    WNDCLASSEXW wcex;
    ZeroMemory(&wcex, sizeof(WNDCLASSEXW));
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = L"WinColorClass";
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINCOLORDX));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassExW(&wcex)) {
        return 1;
    }

    HWND hwnd = CreateWindowExW(0, L"WinColorClass", L"WinColor", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 200, NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        return 1;
    }

    ShowWindow(hwnd, SW_HIDE);
    UpdateWindow(hwnd);

    HHOOK hhkLowLevelKybd = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);
    if (hhkLowLevelKybd == NULL) {
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (!UnhookWindowsHookEx(hhkLowLevelKybd)) {
    }
    else {
    }

    return (int)msg.wParam;
}
