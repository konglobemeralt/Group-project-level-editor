#include "D3D.h"

D3D::D3D() {}

D3D::D3D(HWND win)
{
	// create a struct to hold information about the swap chain
	DXGI_SWAP_CHAIN_DESC scd;

	// clear out the struct for use
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	// fill the swap chain description struct
	scd.BufferCount = 1;                                    // one back buffer
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
	scd.OutputWindow = win;                           // the window to be used
	scd.SampleDesc.Count = 1;                               // how many multisamples
	scd.SampleDesc.Quality = 0;
	scd.Windowed = TRUE;                                    // windowed/full-screen mode

	// create a device, device context and swap chain using the information in the scd struct
	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		NULL,
		NULL,
		NULL,
		D3D11_SDK_VERSION,
		&scd,
		&swapChain,
		&device,
		NULL,
		&devcon);

	if (SUCCEEDED(hr))
	{
		// get the address of the back buffer
		ID3D11Texture2D* pBackBuffer = nullptr;
		swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

		// use the back buffer address to create the render target
		device->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
		pBackBuffer->Release();

		D3D11_TEXTURE2D_DESC depthStencilDesc;
		ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
		depthStencilDesc.Width = 640;
		depthStencilDesc.Height = 480;
		depthStencilDesc.MipLevels = 0;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.MiscFlags = 0;
		depthStencilDesc.CPUAccessFlags = 0;

		ID3D11Texture2D* pStencilTexture;
		device->CreateTexture2D(&depthStencilDesc, NULL, &pStencilTexture);
		device->CreateDepthStencilView(pStencilTexture, NULL, &depthStencilView);
		pStencilTexture->Release();

		// set the render target as the back buffer
		devcon->OMSetRenderTargets(1, &backbuffer, depthStencilView);
	}

	viewPort.Width = (float)WINDOWSWIDTH;
	viewPort.Height = (float)WINDOWSHEIGHT;
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	devcon->RSSetViewports(1, &viewPort);

	CreateShaders();
	Create();
}

D3D::~D3D() {}

void D3D::Update()
{
	smType = ReadMSGHeader();

	if (smType == TMeshCreate)
	{
		ReadMemory(smType);
		meshes.back().meshesBuffer = CreateMesh(vertexSize * meshes.back().vertexCount, meshes.back().vertexData.data(), meshes.back().vertexCount);
		meshes.back().transformBuffer = CreateConstantBuffer(sizeof(XMFLOAT4X4), meshes.back().transform);
		meshes.back().colorBuffer = CreateConstantBuffer(sizeof(XMFLOAT4), meshes.back().materialColor);
	}
	else if (smType == TVertexUpdate)
	{
		ReadMemory(smType);

		devcon->Map(viewMatrix, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapSub);
		view = (XMFLOAT4X4*)mapSub.pData;

		for (size_t i = 0; i < meshes[localMesh].vertexCount; i++)
		{
			if (meshes[localMesh].idList[i] == localVertex)
			{

			}
		}

		devcon->Unmap(meshes[localMesh].meshesBuffer, 0);
	}
	else if (smType == TCameraUpdate)
	{
		// View
		devcon->Map(viewMatrix, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapSub);
		view = (XMFLOAT4X4*)mapSub.pData;
		ReadMemory(smType);

		XMStoreFloat4x4(view, XMMatrixTranspose(XMMatrixLookAtLH(
			XMVectorSet(cameraData->pos[0], cameraData->pos[1], -cameraData->pos[2], 0.0f),
			XMVectorSet(cameraData->view[0], cameraData->view[1], -cameraData->view[2], 0.0f),
			XMVectorSet(cameraData->up[0], cameraData->up[1], cameraData->up[2], 0.0f))));

		devcon->Unmap(viewMatrix, 0);

		//// Projection
		//devcon->Map(projectionMatrix, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapSub);
		//projection = (XMFLOAT4X4*)mapSub.pData;

		//projection = &projectionTemp;

		//devcon->Unmap(projectionMatrix, 0);

		//projectionMatrix = CreateConstantBuffer(sizeof(XMFLOAT4X4), &projectionTemp);
	}
	else if (smType == TtransformUpdate)
	{
		// View
		ReadMemory(smType);
		devcon->Map(meshes[localMesh].transformBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapSub);
		meshes[localMesh].transform = (XMFLOAT4X4*)mapSub.pData;

		memcpy(meshes[localMesh].transform, (char*)buffer + localTail, sizeof(XMFLOAT4X4));
		localTail += sizeof(XMFLOAT4X4);

		// Move tail
		cb->tail += slotSize;
		cb->freeMem += slotSize;

		devcon->Unmap(meshes[localMesh].transformBuffer, 0);
	}
	else if (smType == TLightCreate)
	{
		ReadMemory(smType);
		lights.back().lightBuffer = CreateConstantBuffer(sizeof(LightData), lights.back().lightData);
	}
	else if (smType == TLightUpdate)
	{
		// View
		ReadMemory(smType);
		devcon->Map(lights.back().lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapSub);

		if (localLight == 0)
		{
			memcpy(&lights.back().lightData->color, (char*)buffer + localTail, sizeof(XMFLOAT4));
			localTail += sizeof(XMFLOAT4);
		}
		else if (localLight == 1)
		{
			memcpy(&lights.back().lightData->pos, (char*)buffer + localTail, sizeof(XMFLOAT3));
			localTail += sizeof(XMFLOAT3);
		}

		cb->tail += slotSize;
		cb->freeMem += slotSize;

		devcon->Unmap(lights.back().lightBuffer, 0);
	}
}

void D3D::Render()
{
	// clear the back buffer to a deep blue
	float clearColor[] = { 0, 0, 1, 1 };
	devcon->ClearRenderTargetView(backbuffer, clearColor);
	devcon->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//devcon->VSSetConstantBuffers(0, 1, &worldTempBuffer);
	devcon->VSSetConstantBuffers(1, 1, &viewMatrix);
	devcon->VSSetConstantBuffers(2, 1, &projectionMatrix);
	devcon->IASetInputLayout(inputLayout);
	devcon->VSSetShader(vertexShader, NULL, 0);
	devcon->PSSetShader(pixelShader, NULL, 0);

	//Light do a 'for' later:
	if (lights.size() > 0)
		devcon->PSSetConstantBuffers(0, 1, &lights.back().lightBuffer);

	for (size_t i = 0; i < meshes.size(); i++)
	{
		devcon->VSSetConstantBuffers(0, 1, &meshes[i].transformBuffer);
		devcon->PSSetConstantBuffers(1, 1, &meshes[i].colorBuffer);
		devcon->PSSetShaderResources(0, 1, &meshTextures[i]);
		devcon->IASetVertexBuffers(0, 1, &meshes[i].meshesBuffer, &vertexSize, &offset);
		devcon->Draw(meshes[0].vertexCount, 0);
	}
	swapChain->Present(0, 0);
}

void D3D::Create()
{
	// CAMERA
	XMStoreFloat4x4(view, XMMatrixTranspose(XMMatrixLookAtLH(XMVectorSet(0.0f, 2.0f, -2.0f, 0.0f), XMVectorSet(0.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f))));
	XMStoreFloat4x4(projection, XMMatrixTranspose(XMMatrixPerspectiveFovLH(XM_PI * 0.45f, 640.0f / 480.0f, 0.1f, 500.0f)));
	viewMatrix = CreateConstantBuffer(sizeof(XMFLOAT4X4), view);
	projectionMatrix = CreateConstantBuffer(sizeof(XMFLOAT4X4), projection);

	XMStoreFloat4x4(&worldTemp, XMMatrixIdentity());
	worldTempBuffer = CreateConstantBuffer(sizeof(XMFLOAT4X4), &worldTemp);

	// MESH
	//TempMesh();
	//meshesBuffer.resize(1);
	//smIndex = sm.ReadMemory();
	//meshesBuffer[0] = CreateMesh(vertexSize * meshes[0].vertexCount, meshes[0].vertexData.data(), meshes[0].vertexCount);
	CreateTexture();
}

ID3D11Buffer* D3D::CreateMesh(size_t size, const void* data, size_t vertexCount)
{
	ID3D11Buffer* vertexBuffer;

	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.ByteWidth = size;

	D3D11_SUBRESOURCE_DATA VBdata;
	VBdata.pSysMem = data;
	VBdata.SysMemPitch = 0;
	VBdata.SysMemSlicePitch = 0;

	device->CreateBuffer(&vertexBufferDesc, &VBdata, &vertexBuffer);
	return vertexBuffer;
}

void D3D::CreateTexture()
{
	meshTextures.resize(1);
	CoInitialize(NULL);
	wstring textureName = L"CubeTexture.png";
	CreateWICTextureFromFile(device, textureName.c_str(), NULL, &meshTextures[0]);
}

ID3D11Buffer* D3D::CreateConstantBuffer(size_t size, const void* data)
{
	ID3D11Buffer* constantBuffer;

	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = size;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA cbInitData;
	cbInitData.pSysMem = data;
	cbInitData.SysMemPitch = 0;
	cbInitData.SysMemSlicePitch = 0;

	device->CreateBuffer(&cbDesc, &cbInitData, &constantBuffer);
	return constantBuffer;
}

void D3D::CreateShaders()
{
	//create vertex shader
	ID3DBlob* pVS = nullptr;
	D3DCompileFromFile(L"VertexShader.hlsl", NULL, NULL, "main", "vs_5_0", 0, NULL, &pVS, NULL);
	device->CreateVertexShader(pVS->GetBufferPointer(), pVS->GetBufferSize(), nullptr, &vertexShader);

	//create input layout (verified using vertex shader)
	D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
		{ "SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	device->CreateInputLayout(inputDesc, 3, pVS->GetBufferPointer(), pVS->GetBufferSize(), &inputLayout);
	pVS->Release();

	//create pixel shader
	ID3DBlob* pPS = nullptr;
	D3DCompileFromFile(L"PixelShader.hlsl", NULL, NULL, "main", "ps_5_0", 0, NULL, &pPS, NULL);
	device->CreatePixelShader(pPS->GetBufferPointer(), pPS->GetBufferSize(), nullptr, &pixelShader);
	pPS->Release();
}