#include <combaseapi.h>
#include <corecrt_math_defines.h>
#include "3DMaths.h"

#if D3D11_ON_12
#include <d3d11on12.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include "d3dx12.h"
using Microsoft::WRL::ComPtr;
#endif

struct ID3D11Device1;
struct ID3D11DeviceContext1;
struct IDXGISwapChain1;
struct ID3D11RenderTargetView;
struct ID3D11InputLayout;
struct ID3D11Buffer;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11SamplerState;
struct ID3D11ShaderResourceView;
struct ID3D11RasterizerState;

// Forward declaring ID3DBlob
struct ID3D10Blob;
typedef interface ID3D10Blob* LPD3D10BLOB;
typedef ID3D10Blob ID3DBlob;

namespace awesome { 

	class TimeManager;
	class InputManager;
	class Camera;

	class D3DRenderer {
	public:
		void Init(HWND windowHandle, TimeManager* gm, InputManager* im, Camera* c);
		void SetWindowsResized(bool value) { windowResized = value; };
		void BeforeDraw();
		void AfterDraw();
		void Render(unsigned long long deltaTimeMs);

	private:
		int CreateD3D11Device();
		void SetupDebugLayer();
		int CreateRenderTarget();
		ID3DBlob* CreateVertexShader();
		int CreatePixelShader();
		int CreateInputLayout(ID3DBlob* vsBlob);
		int CreateVertexBuffer();
		int LoadTextures();
		int CreateSamplerState();
		int CreateConstantBuffer();
		int CreateRasterizerState();
		void CheckWindowResize();

		HWND windowHandle{ nullptr };
		static const UINT FrameCount = 2;

#if D3D11_ON_12
		int CreateD3D12Device();
		int CreateD3D12CmdQueueAndSwapChain();
		ComPtr<IDXGIFactory4> m_dxgiFactory;
		ComPtr<ID3D12Device> m_d3d12Device;
		ComPtr<ID3D12CommandQueue> m_commandQueue;
		ComPtr<IDXGISwapChain3> m_swapChain;
		ComPtr<ID3D11On12Device2> m_d3d11On12Device;
		ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		ComPtr<ID3D11Resource> m_wrappedBackBuffers[FrameCount];
		ComPtr<ID3D11RenderTargetView> m_d3d11RenderTargetViews[FrameCount];

		UINT m_frameIndex;
		UINT m_rtvDescriptorSize;
#else
		int CreateD3D11SwapChain();
		IDXGISwapChain1* m_swapChain{ nullptr };
		ID3D11RenderTargetView* m_d3d11RenderTargetView{nullptr};
#endif

		ID3D11Device1* d3d11Device{ nullptr };
		ID3D11DeviceContext1* d3d11DeviceContext{ nullptr };
		ID3D11RasterizerState* rasterizerState{ nullptr };

		ID3D11InputLayout* inputLayout{ nullptr };
		ID3D11Buffer* vertexBuffer{ nullptr };
		UINT stride{ 0 };
		UINT offset{ 0 };
		UINT numVerts{ 0 };

		ID3D11VertexShader* vertexShader{ nullptr };
		ID3D11PixelShader* pixelShader{ nullptr };

		ID3D11SamplerState* samplerState{ nullptr };
		ID3D11ShaderResourceView* textureView{ nullptr };
		ID3D11Buffer* constantBuffer{ nullptr };

		bool windowResized{ true };

		TimeManager* timeManager{ nullptr };
		InputManager* inputManager{ nullptr };
		Camera* camera{ nullptr };
	};
}