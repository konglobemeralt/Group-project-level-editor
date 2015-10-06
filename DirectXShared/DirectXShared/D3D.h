#ifndef D3D_H
#define D3D_H

#include "SharedMemory.h"
#include "WICTextureLoader.h"

#define WINDOWSWIDTH 640
#define WINDOWSHEIGHT 480

class D3D
{
public:

	D3D();
	D3D(HWND win);
	~D3D();

	void Update();
	void Render();

private:
	// TEMP
	XMFLOAT4X4 worldTemp;
	ID3D11Buffer* worldTempBuffer;

	SharedMemory sm;

	// Creators
	void Create();
	ID3D11Buffer* CreateMesh(size_t size, const void* data, size_t vertexCount);
	void CreateTexture();
	ID3D11Buffer* D3D::CreateConstantBuffer(size_t size, const void* data);
	void CreateShaders();

	// D3D
	IDXGISwapChain* swapChain;
	ID3D11Device* device;
	ID3D11DeviceContext* devcon;
	ID3D11RenderTargetView* backbuffer;
	ID3D11DepthStencilView* depthStencilView;
	D3D11_VIEWPORT viewPort;

	// Shaders
	ID3D11InputLayout* inputLayout;
	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShader;

	UINT32 vertexSize = sizeof(float) * 8;
	UINT32 offset = 0;

	// Shared memory
	int smIndex;
};

#endif