#if D3D11_ON_12

/* Functions specific to D3D11on12 only */

#include "D3DRenderer.h"
#include <d3d11_1.h>
#include <cassert>

namespace awesome {

// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, *ppAdapter will be set to nullptr.
	void GetHardwareAdapter(
		IDXGIFactory1* pFactory,
		IDXGIAdapter1** ppAdapter,
		bool requestHighPerformanceAdapter = false)
	{
		*ppAdapter = nullptr;

		ComPtr<IDXGIAdapter1> adapter;
		ComPtr<IDXGIFactory6> factory6;
		if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
		{
			for (
				UINT adapterIndex = 0;
				SUCCEEDED(factory6->EnumAdapterByGpuPreference(
					adapterIndex,
					requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
					IID_PPV_ARGS(&adapter)));
				++adapterIndex)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					// Don't select the Basic Render Driver adapter.
					// If you want a software adapter, pass in "/warp" on the command line.
					continue;
				}

				// Check to see whether the adapter supports Direct3D 12, but don't create the
				// actual device yet.
				if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
				{
					break;
				}
			}
		}

		if (adapter.Get() == nullptr)
		{
			for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					// Don't select the Basic Render Driver adapter.
					// If you want a software adapter, pass in "/warp" on the command line.
					continue;
				}

				// Check to see whether the adapter supports Direct3D 12, but don't create the
				// actual device yet.
				if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
				{
					break;
				}
			}
		}
		*ppAdapter = adapter.Detach();
	}

	int D3DRenderer::CreateD3D12Device() {
		UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
		// Enable the debug layer (requires the Graphics Tools "optional feature").
		// NOTE: Enabling the debug layer after device creation will invalidate the active device.
		{
			// Enable the Direct3D 12 debug layer.
			ComPtr<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
				// Enable additional debug layers.
				dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
			}
		}
#endif
		/* Create the D3D12 device */
		HRESULT hResult = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory));

		if (FAILED(hResult)) {
			MessageBoxA(0, "CreateDXGIFactory1() failed", "Fatal Error", MB_OK);
			return GetLastError();
		}

		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(m_dxgiFactory.Get(), &hardwareAdapter);

		hResult = D3D12CreateDevice(
			hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_d3d12Device)
		);

		if (FAILED(hResult)) {
			MessageBoxA(0, "D3D12CreateDevice() failed", "Fatal Error", MB_OK);
			return GetLastError();
		}
		return 0;
	}

	int D3DRenderer::CreateD3D12CmdQueueAndSwapChain()
	{
		// Describe and create the command queue.
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		HRESULT hResult = m_d3d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
		if (FAILED(hResult)) {
			MessageBoxA(0, "CreateCommandQueue() failed", "Fatal Error", MB_OK);
			return GetLastError();
		}

		// Describe the swap chain.
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferCount = FrameCount;
		swapChainDesc.BufferDesc.Width = 0; // use window width
		swapChainDesc.BufferDesc.Height = 0; // use window height
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.OutputWindow = windowHandle;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.Windowed = TRUE;

		ComPtr<IDXGISwapChain> swapChain;
		hResult = m_dxgiFactory->CreateSwapChain(
			m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
			&swapChainDesc,
			&swapChain
		);
		assert(SUCCEEDED(hResult));
		hResult = swapChain.As(&m_swapChain);
		assert(SUCCEEDED(hResult));
		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
		return 0;
	}

	int D3DRenderer::CreateD3D11Device() {
		ID3D11DeviceContext* baseDeviceContext;
		D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
		UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif //_DEBUG

		ComPtr<ID3D11Device> baseDevice;
		HRESULT hResult = D3D11On12CreateDevice(
			m_d3d12Device.Get(),
			creationFlags,
			nullptr,
			0,
			reinterpret_cast<IUnknown**>(m_commandQueue.GetAddressOf()),
			1,
			0,
			&baseDevice,
			&baseDeviceContext,
			nullptr
		);
		assert(SUCCEEDED(hResult));

		// Query the 11On12 device from the 11 device.
		hResult = baseDevice.As(&m_d3d11On12Device);
		assert(SUCCEEDED(hResult));

		// Create descriptor heaps.
		{
			// Describe and create a render target view (RTV) descriptor heap.
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.NumDescriptors = FrameCount;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			HRESULT hResult = m_d3d12Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
			assert(SUCCEEDED(hResult));
			m_rtvDescriptorSize = m_d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
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

	int D3DRenderer::CreateRenderTarget() {
		// Create frame resources.
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
			ComPtr<ID3D12Resource> buffers[FrameCount];
			// Create a RTV for each frame.
			for (UINT n = 0; n < FrameCount; n++)
			{
				HRESULT hResult = m_swapChain->GetBuffer(n, IID_PPV_ARGS(&buffers[n]));
				assert(SUCCEEDED(hResult));

				// Create a wrapped 11On12 resource of this buffer.
				D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
				hResult = m_d3d11On12Device->CreateWrappedResource(
					buffers[n].Get(),
					&d3d11Flags,
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PRESENT,
					IID_PPV_ARGS(&m_wrappedBackBuffers[n])
				);
				assert(SUCCEEDED(hResult));

				hResult = d3d11Device->CreateRenderTargetView(m_wrappedBackBuffers[n].Get(), 0, &m_d3d11RenderTargetViews[n]);
				assert(SUCCEEDED(hResult));

				rtvHandle.Offset(1, m_rtvDescriptorSize);
			}
		}
		return 0;
	}

	void D3DRenderer::BeforeDraw() {
		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
		// Acquire our wrapped render target resource for the current back buffer.
		m_d3d11On12Device->AcquireWrappedResources(m_wrappedBackBuffers[m_frameIndex].GetAddressOf(), 1);
	}

	void D3DRenderer::AfterDraw() {
		// Release our wrapped render target resource. Releasing 
		// transitions the back buffer resource to the state specified
		// as the OutState when the wrapped resource was created.
		m_d3d11On12Device->ReleaseWrappedResources(m_wrappedBackBuffers[m_frameIndex].GetAddressOf(), 1);

		// Flush to submit the 11 command list to the shared command queue.
		d3d11DeviceContext->Flush();
	}

} // namespace awesome

#endif // D3D11_ON_12
