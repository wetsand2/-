#include "framework.h"
#include "LR_PROB.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <tchar.h>
#include <sstream>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

#define MAX_LOADSTRING 100
#define IDC_HOST_INPUT 101
#define IDC_RESOLVE_BUTTON 102
#define IDC_RESULT_BOX 103

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void ResolveHostname(HWND hwnd, HWND hostInput, HWND resultBox);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_LRPROB, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LRPROB));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_LRPROB);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 500, 300, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hostInput;
    static HWND resolveButton;
    static HWND resultBox;

    switch (message) {
    case WM_CREATE:
        // Hostname input
        hostInput = CreateWindow(L"EDIT", L"", WS_BORDER | WS_CHILD | WS_VISIBLE,
            50, 50, 300, 20, hWnd, (HMENU)IDC_HOST_INPUT, hInst, nullptr);

        // Resolve button
        resolveButton = CreateWindow(L"BUTTON", L"Resolve", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            370, 50, 80, 20, hWnd, (HMENU)IDC_RESOLVE_BUTTON, hInst, nullptr);

        // Result box
        resultBox = CreateWindow(L"EDIT", L"", WS_BORDER | WS_CHILD | WS_VISIBLE | ES_READONLY,
            50, 100, 400, 20, hWnd, (HMENU)IDC_RESULT_BOX, hInst, nullptr);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_RESOLVE_BUTTON) {
            ResolveHostname(hWnd, hostInput, resultBox);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void ResolveHostname(HWND hwnd, HWND hostInput, HWND resultBox)
{
    WSADATA wsaData;
    int wsaerr = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaerr != 0) {
        SetWindowText(resultBox, L"WSAStartup failed!");
        return;
    }

    WCHAR hostBuffer[256];
    GetWindowText(hostInput, hostBuffer, 256);

    char host[256] = { 0 };
    size_t convertedChars = 0;

    // Преобразование строки
    errno_t err = wcstombs_s(&convertedChars, host, sizeof(host), hostBuffer, _TRUNCATE);
    if (err != 0) {
        SetWindowText(resultBox, L"Ошибка преобразования имени хоста.");
        WSACleanup();
        return;
    }

    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    int res = getaddrinfo(host, nullptr, &hints, &result);
    if (res != 0) {
        std::wstringstream ss;
        ss << L"Ошибка getaddrinfo: " << res << L" (" << gai_strerrorA(res) << L")";
        SetWindowText(resultBox, ss.str().c_str());
        WSACleanup();
        return;
    }

    // Конвертация IP-адреса
    sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(result->ai_addr);
    WCHAR ipStr[INET_ADDRSTRLEN];
    InetNtop(AF_INET, &addr->sin_addr, ipStr, INET_ADDRSTRLEN);

    SetWindowText(resultBox, ipStr);

    freeaddrinfo(result);
    WSACleanup(); // Завершаем работу с Winsock
}
