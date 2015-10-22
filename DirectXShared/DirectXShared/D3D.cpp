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

		ID3D11RasterizerState* p_RasterState;					// disables default backface culling
		D3D11_RASTERIZER_DESC descRaster;
		ZeroMemory(&descRaster, sizeof(D3D11_RASTERIZER_DESC));
		descRaster.FillMode = D3D11_FILL_SOLID;					// WIREFRAME;
		descRaster.CullMode = D3D11_CULL_NONE;
		descRaster.MultisampleEnable = TRUE;

		device->CreateRasterizerState(&descRaster, &p_RasterState);
		devcon->RSSetState(p_RasterState);
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
		//if (meshes.back().vertexCount > 60)
		//	int hej = 0;
		meshes.back().meshesBuffer[0] = CreateMesh(sizeof(XMFLOAT3) * meshes.back().vertexCount, meshes.back().pos, meshes.back().vertexCount);
		meshes.back().meshesBuffer[1] = CreateMesh(sizeof(XMFLOAT2) * meshes.back().vertexCount, meshes.back().uv, meshes.back().vertexCount);
		meshes.back().meshesBuffer[2] = CreateMesh(sizeof(XMFLOAT3) * meshes.back().vertexCount, meshes.back().normal, meshes.back().vertexCount);
		meshes.back().transformBuffer = CreateConstantBuffer(sizeof(XMFLOAT4X4), meshes.back().transform);
		meshes.back().colorBuffer = CreateConstantBuffer(sizeof(meshTexture), &meshes.back().meshTex);
		if (meshes.back().meshTex.textureExist.x == 1)
			CreateTexture();
	}
	else if (smType == TAddedVertex)
	{
		ReadMemory(smType);
		meshes[localMesh].meshesBuffer[0] = CreateMesh(sizeof(XMFLOAT3) * meshes[localMesh].vertexCount, meshes[localMesh].pos, meshes[localMesh].vertexCount);
		meshes[localMesh].meshesBuffer[1] = CreateMesh(sizeof(XMFLOAT2) * meshes[localMesh].vertexCount, meshes[localMesh].uv, meshes[localMesh].vertexCount);
		meshes[localMesh].meshesBuffer[2] = CreateMesh(sizeof(XMFLOAT3) * meshes[localMesh].vertexCount, meshes[localMesh].normal, meshes[localMesh].vertexCount);

	}
	else if (smType == TVertexUpdate)
	{
		// Mesh index
		memcpy(&localMesh, (char*)buffer + localTail, sizeof(int));
		localTail += sizeof(int);

		// Vertices
		devcon->Map(meshes[localMesh].meshesBuffer[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapSub);
		meshes[localMesh].pos = (XMFLOAT3*)mapSub.pData;

		// Vertex data
		memcpy(meshes[localMesh].pos, (char*)buffer + localTail, meshes[localMesh].vertexCount * sizeof(XMFLOAT3));
		localTail += meshes[localMesh].vertexCount * sizeof(XMFLOAT3);

		devcon->Unmap(meshes[localMesh].meshesBuffer[0], 0);

		// UV
		devcon->Map(meshes[localMesh].meshesBuffer[1], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapSub);
		meshes[localMesh].uv = (XMFLOAT2*)mapSub.pData;

		// UV data
		memcpy(meshes[localMesh].uv, (char*)buffer + localTail, meshes[localMesh].vertexCount * sizeof(XMFLOAT2));
		localTail += meshes[localMesh].vertexCount * sizeof(XMFLOAT2);

		devcon->Unmap(meshes[localMesh].meshesBuffer[1], 0);

		// Normals
		devcon->Map(meshes[localMesh].meshesBuffer[2], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapSub);
		meshes[localMesh].normal = (XMFLOAT3*)mapSub.pData;

		// Normal data
		memcpy(meshes[localMesh].normal, (char*)buffer + localTail, meshes[localMesh].vertexCount * sizeof(XMFLOAT3));
		localTail += meshes[localMesh].vertexCount * sizeof(XMFLOAT3);

		devcon->Unmap(meshes[localMesh].meshesBuffer[2], 0);

		// Move tail
		cb->freeMem += msgHeader.byteSize + localFreeMem;
		cb->tail += msgHeader.byteSize;
	}
	else if (smType == TNormalUpdate)
	{
		// Mesh index
		memcpy(&localMesh, (char*)buffer + localTail, sizeof(int));
		localTail += sizeof(int);

		// Normals
		devcon->Map(meshes[localMesh].meshesBuffer[2], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapSub);
		meshes[localMesh].normal = (XMFLOAT3*)mapSub.pData;

		// Normal data
		memcpy(meshes[localMesh].normal, (char*)buffer + localTail, meshes[localMesh].vertexCount * sizeof(XMFLOAT3));
		localTail += meshes[localMesh].vertexCount * sizeof(XMFLOAT3);

		devcon->Unmap(meshes[localMesh].meshesBuffer[2], 0);

		// Move tail
		cb->freeMem += msgHeader.byteSize + localFreeMem;
		cb->tail += msgHeader.byteSize;
	}
	else if (smType == TUVUpdate)
	{
		// Mesh index
		memcpy(&localMesh, (char*)buffer + localTail, sizeof(int));
		localTail += sizeof(int);

		// UV
		devcon->Map(meshes[localMesh].meshesBuffer[1], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapSub);
		meshes[localMesh].uv = (XMFLOAT2*)mapSub.pData;

		// UV data
		memcpy(meshes[localMesh].uv, (char*)buffer + localTail, meshes[localMesh].vertexCount * sizeof(XMFLOAT2));
		localTail += meshes[localMesh].vertexCount * sizeof(XMFLOAT2);

		devcon->Unmap(meshes[localMesh].meshesBuffer[1], 0);

		// Move tail
		cb->freeMem += msgHeader.byteSize + localFreeMem;
		cb->tail += msgHeader.byteSize;
	}
	else if (smType == TMaterialUpdate)
	{
		ReadMemory(smType);
		meshes[localMesh].colorBuffer = CreateConstantBuffer(sizeof(meshTexture), &meshes[localMesh].meshTex);
		if (meshes[localMesh].meshTex.textureExist.x == 1)
			CreateTexture();
	}
	else if (smType == TCameraUpdate)
	{
		// View
		devcon->Map(viewMatrix, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapSub);
		view = (XMFLOAT4X4*)mapSub.pData;
		//ReadMemory(smType);

		memcpy(view, (char*)buffer + localTail, sizeof(XMFLOAT4X4));
		localTail += sizeof(XMFLOAT4X4);

		devcon->Unmap(viewMatrix, 0);

		// Projection
		devcon->Map(projectionMatrix, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapSub);
		projection = (XMFLOAT4X4*)mapSub.pData;

		// Projection matrix
		memcpy(projection, (char*)buffer + localTail, sizeof(XMFLOAT4X4));
		localTail += sizeof(XMFLOAT4X4);

		devcon->Unmap(projectionMatrix, 0);

		// Move tail
		cb->tail += slotSize;
		cb->freeMem += slotSize;
	}
	else if (smType == TtransformUpdate)
	{
		// View
		ReadMemory(smType);
		if (localMesh < meshes.size())
		{
			devcon->Map(meshes[localMesh].transformBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapSub);
			meshes[localMesh].transform = (XMFLOAT4X4*)mapSub.pData;

			memcpy(meshes[localMesh].transform, (char*)buffer + localTail, sizeof(XMFLOAT4X4));
			localTail += sizeof(XMFLOAT4X4);

			devcon->Unmap(meshes[localMesh].transformBuffer, 0);
		}

		// Move tail
		cb->tail += slotSize;
		cb->freeMem += slotSize;
	}
	else if (smType == TLightCreate)
	{
		ReadMemory(smType);
		light.lightBuffer = CreateConstantBuffer(sizeof(LightData), light.lightData);
	}
	else if (smType == TLightUpdate)
	{
		devcon->Map(light.lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapSub);
		light.lightData = (LightData*)mapSub.pData;

		// Light data
		memcpy(&light.lightData->pos, (char*)buffer + localTail, sizeof(XMFLOAT3));
		localTail += sizeof(XMFLOAT3);
		memcpy(&light.lightData->color, (char*)buffer + localTail, sizeof(XMFLOAT4));
		localTail += sizeof(XMFLOAT4);

		devcon->Unmap(light.lightBuffer, 0);

		cb->tail += slotSize;
		cb->freeMem += slotSize;
	}
	else if (smType == TMeshDestroyed)
	{
		ReadMemory(smType);

		// Delete mesh with the given index
		meshes[localMesh].meshesBuffer[0]->Release();
		meshes[localMesh].meshesBuffer[1]->Release();
		meshes[localMesh].meshesBuffer[2]->Release();
		meshes[localMesh].transformBuffer->Release();
		meshes[localMesh].colorBuffer->Release();

		meshes[localMesh].meshesBuffer[0] = NULL;
		meshes[localMesh].meshesBuffer[1] = NULL;
		meshes[localMesh].meshesBuffer[2] = NULL;
		meshes[localMesh].transformBuffer = NULL;
		meshes[localMesh].colorBuffer = NULL;

		delete[] meshes[localMesh].pos;
		delete[] meshes[localMesh].uv;
		delete[] meshes[localMesh].normal;
		delete meshes[localMesh].transform;
	}
}

void D3D::Render()
{
	// clear the back buffer to a deep blue
	float clearColor[] = { 0, 0, 1, 1 };
	devcon->ClearRenderTargetView(backbuffer, clearColor);
	devcon->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	devcon->VSSetConstantBuffers(1, 1, &viewMatrix);
	devcon->VSSetConstantBuffers(2, 1, &projectionMatrix);
	devcon->IASetInputLayout(inputLayout);
	devcon->VSSetShader(vertexShader, NULL, 0);
	devcon->PSSetShader(pixelShader, NULL, 0);

	unsigned int strides[3] = { 12, 8, 12 };
	unsigned int offsets[3] = { 0, 0, 0 };

	if (light.lightBuffer != NULL)
		devcon->PSSetConstantBuffers(0, 1, &light.lightBuffer);

	for (size_t i = 0; i < meshes.size(); i++)
	{
		if (meshes[i].meshesBuffer[0] != NULL)
		{
			devcon->VSSetConstantBuffers(0, 1, &meshes[i].transformBuffer);
			devcon->PSSetConstantBuffers(1, 1, &meshes[i].colorBuffer);
			if (meshes.back().meshTex.textureExist.x == 1)
				devcon->PSSetShaderResources(0, 1, &meshTextures[i]);
			devcon->IASetVertexBuffers(0, 3, meshes[i].meshesBuffer, strides, offsets);
			devcon->Draw(meshes[0].vertexCount, 0);
		}
	}
	swapChain->Present(0, 0);
}

void D3D::Create()
{
	// CAMERA
	XMStoreFloat4x4(view, XMMatrixTranspose(XMMatrixLookAtRH(XMVectorSet(0.0f, 2.0f, 2.0f, 0.0f), XMVectorSet(0.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f))));
	XMStoreFloat4x4(projection, XMMatrixTranspose(XMMatrixPerspectiveFovRH(XM_PI * 0.45f, 640.0f / 480.0f, 1.0f, 1000.0f)));
	viewMatrix = CreateConstantBuffer(sizeof(XMFLOAT4X4), view);
	projectionMatrix = CreateConstantBuffer(sizeof(XMFLOAT4X4), projection);

	XMStoreFloat4x4(&worldTemp, XMMatrixIdentity());
	worldTempBuffer = CreateConstantBuffer(sizeof(XMFLOAT4X4), &worldTemp);
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
	meshTextures.resize(meshes.size());
	CoInitialize(NULL);
	string textureString = meshes.back().texturePath;
	wstring textureName(textureString.begin(), textureString.end());

	CreateWICTextureFromFile(device, textureName.c_str(), NULL, &meshTextures[meshes.size() - 1]);
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
	HRESULT hr;
	hr = device->CreateBuffer(&cbDesc, &cbInitData, &constantBuffer);
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
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	device->CreateInputLayout(inputDesc, 3, pVS->GetBufferPointer(), pVS->GetBufferSize(), &inputLayout);
	pVS->Release();

	//create pixel shader
	ID3DBlob* pPS = nullptr;
	D3DCompileFromFile(L"PixelShader.hlsl", NULL, NULL, "main", "ps_5_0", 0, NULL, &pPS, NULL);
	device->CreatePixelShader(pPS->GetBufferPointer(), pPS->GetBufferSize(), nullptr, &pixelShader);
	pPS->Release();
}