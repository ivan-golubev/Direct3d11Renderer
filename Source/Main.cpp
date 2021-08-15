#ifndef UNICODE
#define UNICODE
#endif // !UNICODE

#include <windows.h>
#include <d3d11_1.h>
#include <assert.h>

/* Good candidates for incapsulation */
ID3D11Device1* d3d11Device{ nullptr };
ID3D11DeviceContext1* d3d11DeviceContext{ nullptr };
IDXGISwapChain1* d3d11SwapChain{ nullptr };
ID3D11RenderTargetView* d3d11FrameBufferView{ nullptr };

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int RegisterDirect3DDevice();
void SetupDebugLayer();
int CreateSwapChain(HWND hwnd);
int CreateFrameBuffer();

#define CHECK(func_call){\
int code = func_call;\
if (code != 0)\
    return code;\
}

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

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        CLASS_NAME,                     // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        0, 0, initialWidth, initialHeight,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (!hwnd) {
        MessageBoxA(0, "CreateWindowEx failed", "Fatal Error", MB_OK);
        return GetLastError();
    }

    ShowWindow(hwnd, nShowCmd);
    CHECK(RegisterDirect3DDevice())
    CHECK(CreateSwapChain(hwnd))
    CHECK(CreateFrameBuffer())

    // Run the message loop.
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        FLOAT backgroundColor[4] = { 0.1f, 0.2f, 0.6f, 1.0f };
        d3d11DeviceContext->ClearRenderTargetView(d3d11FrameBufferView, backgroundColor);

        d3d11SwapChain->Present(1, 0);
    }
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
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
    default:
        result = DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return result;
}

int RegisterDirect3DDevice() {
    ID3D11Device* baseDevice;
    ID3D11DeviceContext* baseDeviceContext;
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif //_DEBUG
    HRESULT hResult = D3D11CreateDevice(
        0, D3D_DRIVER_TYPE_HARDWARE,
        0, creationFlags,
        featureLevels, ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION, &baseDevice,
        0, &baseDeviceContext
    );
    if (FAILED(hResult)) {
        MessageBoxA(0, "D3D11CreateDevice() failed", "Fatal Error", MB_OK);
        return GetLastError();
    }

    // Get 1.1 interface of D3D11 Device and Context
    hResult = baseDevice->QueryInterface(__uuidof(ID3D11Device1), (void**)&d3d11Device);
    assert(SUCCEEDED(hResult));
    baseDevice->Release();

    hResult = baseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&d3d11DeviceContext);
    assert(SUCCEEDED(hResult));
    baseDeviceContext->Release();

    SetupDebugLayer();
    return 0;
}

void SetupDebugLayer() {
#if defined(_DEBUG)
    // Set up debug layer to break on D3D11 errors
    ID3D11Debug* d3dDebug = nullptr;
    d3d11Device->QueryInterface(__uuidof(ID3D11Debug), (void**)&d3dDebug);
    if (d3dDebug)
    {
        ID3D11InfoQueue* d3dInfoQueue = nullptr;
        if (SUCCEEDED(d3dDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&d3dInfoQueue)))
        {
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
            d3dInfoQueue->Release();
        }
        d3dDebug->Release();
    }
#endif //_DEBUG
}

int CreateSwapChain(HWND hwnd) {
    // Get DXGI Factory (needed to create Swap Chain)
    IDXGIFactory2* dxgiFactory;
    {
        IDXGIDevice1* dxgiDevice;
        HRESULT hResult = d3d11Device->QueryInterface(__uuidof(IDXGIDevice1), (void**)&dxgiDevice);
        assert(SUCCEEDED(hResult));

        IDXGIAdapter* dxgiAdapter;
        hResult = dxgiDevice->GetAdapter(&dxgiAdapter);
        assert(SUCCEEDED(hResult));
        dxgiDevice->Release();

        DXGI_ADAPTER_DESC adapterDesc;
        dxgiAdapter->GetDesc(&adapterDesc);

        OutputDebugStringA("Graphics Device: ");
        OutputDebugStringW(adapterDesc.Description);
        OutputDebugStringA("\n");

        hResult = dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&dxgiFactory);
        assert(SUCCEEDED(hResult));
        dxgiAdapter->Release();
    }

    DXGI_SWAP_CHAIN_DESC1 d3d11SwapChainDesc = {};
    d3d11SwapChainDesc.Width = 0; // use window width
    d3d11SwapChainDesc.Height = 0; // use window height
    d3d11SwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    d3d11SwapChainDesc.SampleDesc.Count = 1;
    d3d11SwapChainDesc.SampleDesc.Quality = 0;
    d3d11SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    d3d11SwapChainDesc.BufferCount = 2;
    d3d11SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    d3d11SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    d3d11SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    d3d11SwapChainDesc.Flags = 0;

    HRESULT hResult = dxgiFactory->CreateSwapChainForHwnd(d3d11Device, hwnd, &d3d11SwapChainDesc, 0, 0, &d3d11SwapChain);
    assert(SUCCEEDED(hResult));

    dxgiFactory->Release();

    return 0;
}

int CreateFrameBuffer() {
    ID3D11Texture2D* d3d11FrameBuffer;
    HRESULT hResult = d3d11SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&d3d11FrameBuffer);
    assert(SUCCEEDED(hResult));

    hResult = d3d11Device->CreateRenderTargetView(d3d11FrameBuffer, 0, &d3d11FrameBufferView);
    assert(SUCCEEDED(hResult));
    d3d11FrameBuffer->Release();
    return 0;
}
