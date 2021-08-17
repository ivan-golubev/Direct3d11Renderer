#include <combaseapi.h>
#include <corecrt_math_defines.h>
#include "3DMaths.h"

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

// Forward declaring ID3DBlob
struct ID3D10Blob;
typedef interface ID3D10Blob* LPD3D10BLOB;
typedef ID3D10Blob ID3DBlob;

namespace awesome { 

	class TimeManager;
	class InputManager;

	class D3DRenderer {
	public:
		void Init(HWND windowHandle, TimeManager* gm, InputManager* im);
		void SetWindowsResized(bool value) { windowResized = value; };
		void Render(unsigned long long deltaTimeMs);

	private:
		int RegisterDirect3DDevice();
		void SetupDebugLayer();
		int CreateSwapChain();
		int CreateFrameBuffer();
		ID3DBlob* CreateVertexShader();
		int CreatePixelShader();
		int CreateInputLayout(ID3DBlob* vsBlob);
		int CreateVertexBuffer();
		int CreateSamplerState();
		int CreateConstantBuffer();
		void CheckWindowResize();

		HWND windowHandle{ nullptr };
		ID3D11Device1* d3d11Device{ nullptr };
		ID3D11DeviceContext1* d3d11DeviceContext{ nullptr };
		IDXGISwapChain1* d3d11SwapChain{ nullptr };
		ID3D11RenderTargetView* d3d11FrameBufferView{ nullptr };

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

		bool windowResized{ false };
		
		/* TODO: this is definitely not the right place for these members: */
		float const colorCycleFreq { 2000.0f };

		TimeManager* timeManager;
		InputManager* inputManager;
		float2 playerPos{ 0.0f, 0.0f };
		float4 playerColor{};
	};
}