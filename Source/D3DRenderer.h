#include <combaseapi.h>

class ID3D11Device1;
class ID3D11DeviceContext1;
class IDXGISwapChain1;
class ID3D11RenderTargetView;
class ID3D11InputLayout;
class ID3D11Buffer;
class ID3D11VertexShader;
class ID3D11PixelShader;
class ID3D11SamplerState;
class ID3D11ShaderResourceView;

// Forward declaring ID3DBlob
struct ID3D10Blob;
typedef interface ID3D10Blob* LPD3D10BLOB;
typedef ID3D10Blob ID3DBlob;

namespace awesome { 

	class D3DRenderer {
	public:
		void Init(HWND windowHandle);
		void SetWindowsResized(bool value) { windowResized = value; };
		void MainLoop();

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
	};
}