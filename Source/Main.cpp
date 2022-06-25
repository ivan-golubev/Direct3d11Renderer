#ifndef UNICODE
#define UNICODE
#endif // !UNICODE

#include <windows.h>
#include "TimeManager.h"
#include "D3DRenderer.h"
#include "InputManager.h"
#include "Camera.h"

#include <iostream>
#include <fstream>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void MainLoop();

awesome::TimeManager timeManager{};
awesome::D3DRenderer renderer{};
awesome::InputManager inputManager{};
awesome::Camera camera{ &inputManager };

void SetFileOutput()
{
    std::ofstream* file = new std::ofstream();
    file->open("cout.txt");
    std::streambuf* sbuf = std::cout.rdbuf();
    std::cout.rdbuf(file->rdbuf());
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPWSTR /*lpCmdLine*/, _In_ int nShowCmd)
{
    //SetFileOutput();

    const wchar_t CLASS_NAME[] = L"Direct 3D 11 renderer";
    /* register a windows class */
    WNDCLASSEXW winClass = {};
    winClass.cbSize = sizeof(WNDCLASSEXW);
    winClass.style = CS_HREDRAW | CS_VREDRAW;
    winClass.lpfnWndProc = &WindowProc;
    winClass.hInstance = hInstance;
    winClass.hIcon = LoadIconW(0, IDI_APPLICATION);
    winClass.hCursor = LoadCursorW(0, IDC_ARROW);
    winClass.lpszClassName = CLASS_NAME;
    winClass.hIconSm = LoadIconW(0, IDI_APPLICATION);
    
    if (!RegisterClassExW(&winClass)) {
        MessageBoxA(0, "Failed to register the window class", "Fatal Error", MB_OK);
        return GetLastError();
    }

    RECT initialRect = { 0, 0, 1024, 768 };
    AdjustWindowRectEx(&initialRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);
    LONG initialWidth = initialRect.right - initialRect.left;
    LONG initialHeight = initialRect.bottom - initialRect.top;

    HWND windowHandle = CreateWindowExW(
        WS_EX_OVERLAPPEDWINDOW,
        winClass.lpszClassName,
        CLASS_NAME,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        initialWidth,
        initialHeight,
        0, 0, hInstance, 0
    );

    if (!windowHandle) {
        MessageBoxA(0, "CreateWindowEx failed", "Fatal Error", MB_OK);
        return GetLastError();
    }

    renderer.Init(windowHandle, &timeManager, &inputManager, &camera);
    MainLoop();
    return 0;
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam);
    if (result)
        return result;

    LPCREATESTRUCT a = LPCREATESTRUCT(lParam);

    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_SIZE:
    {
        renderer.SetWindowsResized(true);
        break;
    }
    case WM_KEYDOWN:
    case WM_KEYUP:
        if (wParam == VK_ESCAPE)
            DestroyWindow(hwnd);
        else
            inputManager.OnWindowMessage(uMsg, static_cast<unsigned int>(wParam));
        break;
    default:
        result = DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return result;
}

void MainLoop() {
    MSG msg = { };
    unsigned long long dt{ 0 };
    bool isRunning = true;

    while (isRunning)
    {
        dt = timeManager.Tick();
        if (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                isRunning = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        renderer.Render(dt);
    }
}
