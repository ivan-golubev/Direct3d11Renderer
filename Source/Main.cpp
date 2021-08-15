#ifndef UNICODE
#define UNICODE
#endif // !UNICODE

#include <windows.h>
#include "D3DRenderer.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void MainLoop();

awesome::D3DRenderer renderer{};

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPWSTR /*lpCmdLine*/, _In_ int nShowCmd)
{
    const wchar_t CLASS_NAME[] = L"Direct 3D 11 renderer";
    /* register a windows class */
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    
    if (!RegisterClass(&wc)) {
        MessageBoxA(0, "Failed to register the window class", "Fatal Error", MB_OK);
        return GetLastError();
    }

    RECT initialRect = { 0, 0, 1280, 960 };
    AdjustWindowRectEx(&initialRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);
    LONG initialWidth = initialRect.right - initialRect.left;
    LONG initialHeight = initialRect.bottom - initialRect.top;

    HWND windowHandle = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        CLASS_NAME,                     // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        0, 0, initialWidth, initialHeight,

        nullptr,       // Parent window    
        nullptr,       // Menu
        hInstance,  // Instance handle
        nullptr       // Additional application data
    );

    if (!windowHandle) {
        MessageBoxA(0, "CreateWindowEx failed", "Fatal Error", MB_OK);
        return GetLastError();
    }

    ShowWindow(windowHandle, nShowCmd);
    renderer.Init(windowHandle);
    MainLoop();
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPCREATESTRUCT a = LPCREATESTRUCT(lParam);

    LRESULT result = 0;
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
    default:
        result = DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return result;
}

void MainLoop() {
    MSG msg = { };

    long currentTime = 0;
    LARGE_INTEGER Frequency;
    if (!QueryPerformanceFrequency(&Frequency))
        exit(-1); // Hardware does not support high-res counter

    while (GetMessage(&msg, NULL, 0, 0))
    {
        unsigned long long dt{0};
        {
            unsigned long long prevTime = currentTime;
            LARGE_INTEGER perfCount;
            QueryPerformanceCounter(&perfCount);
            currentTime = perfCount.QuadPart;
            dt = (currentTime - prevTime) * 1000ULL / Frequency.QuadPart;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        renderer.Render(dt);
    }
}
