#include "D3DRenderer.h"

#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <assert.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "TimeManager.h"
#include "InputManager.h"
#include "Camera.h"

namespace awesome {

    /* elements need to be 16-byte aligned */
    struct Constants
    {
        float4x4 modelViewProj;
    };

    void D3DRenderer::Init(HWND windowHandle, TimeManager* timeManager, InputManager* inputManager, Camera* camera) {
        this->windowHandle = windowHandle;
        this->timeManager = timeManager;
        this->inputManager = inputManager;
        this->camera = camera;
#if D3D11_ON_12
        CreateD3D12Device();
        CreateD3D12CmdQueueAndSwapChain();
        CreateD3D11Device();
#else
		CreateD3D11Device();
		SetupDebugLayer();
        CreateD3D11SwapChain();
#endif
        CreateRenderTarget();
        ID3DBlob* vsBlob = CreateVertexShader();
        CreatePixelShader();
        CreateInputLayout(vsBlob);
        CreateVertexBuffer();
        LoadTextures();
        CreateSamplerState();
        CreateConstantBuffer();
        CreateRasterizerState();
    }

    void D3DRenderer::Render(unsigned long long deltaTimeMs) {
        CheckWindowResize();
        camera->UpdateCamera(deltaTimeMs);

#if D3D11_ON_12
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
		// Acquire our wrapped render target resource for the current back buffer.
		m_d3d11On12Device->AcquireWrappedResources(m_wrappedBackBuffers[m_frameIndex].GetAddressOf(), 1);
#endif
        
        float4x4 modelMat = rotateYMat(0.0002f * static_cast<float>(M_PI * timeManager->GetCurrentTimeMs())); // Spin the quad
        float4x4 modelViewProj = modelMat * camera->GetViewMatrix() * camera->GetPerspectiveMatrix();
        {
            D3D11_MAPPED_SUBRESOURCE mappedSubresource;
            d3d11DeviceContext->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
            Constants* constants = (Constants*)(mappedSubresource.pData);
            constants->modelViewProj = modelViewProj;
            d3d11DeviceContext->Unmap(constantBuffer, 0);
        }
        FLOAT backgroundColor[4] = { 0.1f, 0.2f, 0.6f, 1.0f };
#if D3D11_ON_12
        {
            ID3D11RenderTargetView* RTV = m_d3d11RenderTargetViews[m_frameIndex].Get();
			assert(RTV);
			d3d11DeviceContext->OMSetRenderTargets(1, &RTV, nullptr);
			d3d11DeviceContext->ClearRenderTargetView(RTV, backgroundColor);
        }
#else
		d3d11DeviceContext->OMSetRenderTargets(1, &m_d3d11RenderTargetView, nullptr);
		d3d11DeviceContext->ClearRenderTargetView(m_d3d11RenderTargetView, backgroundColor);
#endif

        RECT winRect;
        GetClientRect(windowHandle, &winRect);
        D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (FLOAT)(winRect.right - winRect.left), (FLOAT)(winRect.bottom - winRect.top), 0.0f, 1.0f };

        d3d11DeviceContext->RSSetViewports(1, &viewport);
        d3d11DeviceContext->RSSetState(rasterizerState);

        d3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        d3d11DeviceContext->IASetInputLayout(inputLayout);

        d3d11DeviceContext->VSSetShader(vertexShader, nullptr, 0);
        d3d11DeviceContext->PSSetShader(pixelShader, nullptr, 0);

        d3d11DeviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);
        d3d11DeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

        d3d11DeviceContext->PSSetShaderResources(0, 1, &textureView);
        d3d11DeviceContext->PSSetSamplers(0, 1, &samplerState);

        d3d11DeviceContext->Draw(numVerts, 0);

#if D3D11_ON_12
		// Release our wrapped render target resource. Releasing 
        // transitions the back buffer resource to the state specified
        // as the OutState when the wrapped resource was created.
		m_d3d11On12Device->ReleaseWrappedResources(m_wrappedBackBuffers[m_frameIndex].GetAddressOf(), 1);

		// Flush to submit the 11 command list to the shared command queue.
        d3d11DeviceContext->Flush();
#endif

        HRESULT hResult = m_swapChain->Present(1, 0);
        assert(SUCCEEDED(hResult));
    }    

    void D3DRenderer::CheckWindowResize() {
        if (windowResized)
        {
            d3d11DeviceContext->OMSetRenderTargets(0, 0, 0);
#if D3D11_ON_12
            CreateRenderTarget();
#else
            m_d3d11RenderTargetView->Release();
			HRESULT res = m_swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
			assert(SUCCEEDED(res));

			ID3D11Texture2D* d3d11FrameBuffer;
			res = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&d3d11FrameBuffer);
			assert(SUCCEEDED(res));

			res = d3d11Device->CreateRenderTargetView(d3d11FrameBuffer, NULL, &m_d3d11RenderTargetView);
			assert(SUCCEEDED(res));
			d3d11FrameBuffer->Release();
#endif
            // Get window dimensions
            int windowWidth, windowHeight;
            float windowAspectRatio;
            {
                RECT clientRect;
                GetClientRect(windowHandle, &clientRect);
                windowWidth = clientRect.right - clientRect.left;
                windowHeight = clientRect.bottom - clientRect.top;
                windowAspectRatio = (float)windowWidth / (float)windowHeight;
                camera->UpdatePerspectiveMatrix(windowAspectRatio);
            } 
            windowResized = false;
        }
    }

#if D3D11_ON_12
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

#endif

    int D3DRenderer::CreateD3D11Device() {
        ID3D11DeviceContext* baseDeviceContext;
        D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif //_DEBUG

#if D3D11_ON_12
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
#else
        ID3D11Device* baseDevice;
        HRESULT hResult = D3D11CreateDevice(
            0, D3D_DRIVER_TYPE_HARDWARE,
            0, creationFlags,
            featureLevels, ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION, &baseDevice,
            0, &baseDeviceContext
        );
#endif // !D3D11_ON_12

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

    void D3DRenderer::SetupDebugLayer() {
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

#if !D3D11_ON_12
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
#endif

    int D3DRenderer::CreateRenderTarget() {
#if D3D11_ON_12
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
#else
        ID3D11Texture2D* d3d11FrameBuffer;
        HRESULT hResult = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&d3d11FrameBuffer);
        assert(SUCCEEDED(hResult));

        hResult = d3d11Device->CreateRenderTargetView(d3d11FrameBuffer, 0, &m_d3d11RenderTargetView);
        assert(SUCCEEDED(hResult));
        d3d11FrameBuffer->Release();
#endif
        return 0;
    }

    ID3DBlob* D3DRenderer::CreateVertexShader() {
        ID3DBlob* vsBlob;
        ID3DBlob* shaderCompileErrorsBlob;
        HRESULT hResult = D3DCompileFromFile(L"Shaders/textured_surface.hlsl", nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &vsBlob, &shaderCompileErrorsBlob);
        if (FAILED(hResult))
        {
            const char* errorString = NULL;
            if (hResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                errorString = "Could not compile shader; file not found";
            else if (shaderCompileErrorsBlob) {
                errorString = (const char*)shaderCompileErrorsBlob->GetBufferPointer();
                shaderCompileErrorsBlob->Release();
            }
            MessageBoxA(0, errorString, "Shader Compiler Error", MB_ICONERROR | MB_OK);
            return nullptr;
        }

        hResult = d3d11Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
        assert(SUCCEEDED(hResult));
        return vsBlob;
    }

    int D3DRenderer::CreatePixelShader() {
        ID3DBlob* psBlob;
        ID3DBlob* shaderCompileErrorsBlob;
        HRESULT hResult = D3DCompileFromFile(L"Shaders/textured_surface.hlsl", nullptr, nullptr, "ps_main", "ps_5_0", 0, 0, &psBlob, &shaderCompileErrorsBlob);
        if (FAILED(hResult))
        {
            const char* errorString = NULL;
            if (hResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                errorString = "Could not compile shader; file not found";
            else if (shaderCompileErrorsBlob) {
                errorString = (const char*)shaderCompileErrorsBlob->GetBufferPointer();
                shaderCompileErrorsBlob->Release();
            }
            MessageBoxA(0, errorString, "Shader Compiler Error", MB_ICONERROR | MB_OK);
            return 1;
        }

        hResult = d3d11Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
        assert(SUCCEEDED(hResult));
        psBlob->Release();
        return 0;
    }

    int D3DRenderer::CreateInputLayout(ID3DBlob* vsBlob) {
        D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
        {
            { "POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        HRESULT hResult = d3d11Device->CreateInputLayout(
            inputElementDesc,
            ARRAYSIZE(inputElementDesc),
            vsBlob->GetBufferPointer(),
            vsBlob->GetBufferSize(),
            &inputLayout
        );
        assert(SUCCEEDED(hResult));
        vsBlob->Release();
        return 0;
    }

    int D3DRenderer::CreateVertexBuffer() {
        float vertexData[] = { // x, y, u, v
            -0.5f,  0.5f, 0.f, 0.f,
            0.5f, -0.5f, 1.f, 1.f,
            -0.5f, -0.5f, 0.f, 1.f,
            -0.5f,  0.5f, 0.f, 0.f,
            0.5f,  0.5f, 1.f, 0.f,
            0.5f, -0.5f, 1.f, 1.f
        };
        stride = 4 * sizeof(float);
        offset = 0;
        numVerts = sizeof(vertexData) / stride;

        D3D11_BUFFER_DESC vertexBufferDesc = {};
        vertexBufferDesc.ByteWidth = sizeof(vertexData);
        vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vertexSubresourceData = { vertexData };

        HRESULT hResult = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexSubresourceData, &vertexBuffer);
        assert(SUCCEEDED(hResult));

        return 0;
    }

    int D3DRenderer::CreateSamplerState() {
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.BorderColor[0] = 1.0f;
        samplerDesc.BorderColor[1] = 1.0f;
        samplerDesc.BorderColor[2] = 1.0f;
        samplerDesc.BorderColor[3] = 1.0f;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

        HRESULT hResult = d3d11Device->CreateSamplerState(&samplerDesc, &samplerState);
        assert(SUCCEEDED(hResult));
        return 0;
    }

    int D3DRenderer::LoadTextures() {
        // Load Image
        int texWidth, texHeight, texNumChannels;
        int texForceNumChannels = 4;
        unsigned char* inputTextureBytes = stbi_load(
            "Textures/texture1.png",
            &texWidth,
            &texHeight,
            &texNumChannels,
            texForceNumChannels
        );
        assert(inputTextureBytes);
        int texBytesPerRow = 4 * texWidth;

        // Create Texture
        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = texWidth;
        textureDesc.Height = texHeight;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA textureSubresourceData = {};
        textureSubresourceData.pSysMem = inputTextureBytes;
        textureSubresourceData.SysMemPitch = texBytesPerRow;

        ID3D11Texture2D* texture;
        HRESULT hResult = d3d11Device->CreateTexture2D(&textureDesc, &textureSubresourceData, &texture);
        assert(SUCCEEDED(hResult));

        hResult = d3d11Device->CreateShaderResourceView(texture, nullptr, &textureView);
        assert(SUCCEEDED(hResult));
        free(inputTextureBytes);
        return 0;
    }

    int D3DRenderer::CreateConstantBuffer() {
        D3D11_BUFFER_DESC constantBufferDesc = {};
        constantBufferDesc.ByteWidth = sizeof(Constants);
        constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hResult = d3d11Device->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
        assert(SUCCEEDED(hResult));
        return 0;
    }

    int D3DRenderer::CreateRasterizerState() {
        D3D11_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;
        rasterizerDesc.FrontCounterClockwise = TRUE;

        HRESULT hResult = d3d11Device->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
        assert(SUCCEEDED(hResult));
        return 0;
    }

} // namespace awesome
