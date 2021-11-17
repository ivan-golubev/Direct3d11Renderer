#if !D3D11_ON_12

/* Functions specific to D3D11 only */

#include "D3DRenderer.h"
#include <d3d11_1.h>
#include <cassert>

namespace awesome {

int D3DRenderer::CreateD3D11Device() {
	ID3D11DeviceContext* baseDeviceContext;
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif //_DEBUG

	ID3D11Device* baseDevice;
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
	return 0;
}

int D3DRenderer::CreateD3D11SwapChain() {
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

	HRESULT hResult = dxgiFactory->CreateSwapChainForHwnd(
		d3d11Device,
		windowHandle,
		&d3d11SwapChainDesc,
		0,
		0,
		&m_swapChain
	);
	assert(SUCCEEDED(hResult));

	dxgiFactory->Release();

	return 0;
}

int D3DRenderer::CreateRenderTarget() {
	ID3D11Texture2D* d3d11FrameBuffer;
	HRESULT hResult = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&d3d11FrameBuffer);
	assert(SUCCEEDED(hResult));

	hResult = d3d11Device->CreateRenderTargetView(d3d11FrameBuffer, 0, &m_d3d11RenderTargetView);
	assert(SUCCEEDED(hResult));
	d3d11FrameBuffer->Release();
	return 0;
}

void D3DRenderer::BeforeDraw() {}
void D3DRenderer::AfterDraw() {}

} // namespace awesome

#endif //!D3D11_ON_12