#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <iostream>
#include <windows.h>
#include "Enumerations.h"

using namespace DirectX;
using namespace std;

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

using namespace DirectX;

class SharedMemory
{
public:

	SharedMemory();
	~SharedMemory();

	void Update();

	void OpenMemory(size_t size);
	int ReadMSGHeader();
	void ReadMemory(unsigned int type);
	void TempMesh();

	// SHARED MEOMRY
	HANDLE fmCB;
	HANDLE fmMain;
	unsigned int slotSize;
	unsigned int localTail;

	struct CircBuffer
	{
		unsigned int freeMem;
		unsigned int head;
		unsigned int tail;
		unsigned int readersCount;
		unsigned int allRead;
	}*cb;

	size_t memSize;
	void* buffer;

	// MESSAGE HEADER
	struct MSGHeader
	{
		unsigned int type;
		unsigned int padding;
	}msgHeader;

	// MESH
	struct VertexData
	{
		XMFLOAT3 pos;
		XMFLOAT2 uv;
		XMFLOAT3 normal;
	};
	struct MeshData
	{
		vector<VertexData> vertexData;
		unsigned int vertexCount;
		XMFLOAT4X4 transform;
	};
	vector<MeshData> meshes;
	vector<ID3D11Buffer*> meshesBuffer;
	vector<string> meshNames;

	// TEXTURES
	vector<ID3D11ShaderResourceView*> meshTextures;

	// TRANSFORM
	vector<string> tranformNames;
	vector<XMFLOAT4X4> transforms;
	vector<ID3D11Buffer*> transformBuffers;

	// CAMERA
	XMFLOAT4X4* view;
	XMFLOAT4X4* projection;
	XMFLOAT4X4 projectionTemp;
	ID3D11Buffer* viewMatrix;
	ID3D11Buffer* projectionMatrix;
	D3D11_MAPPED_SUBRESOURCE camMapSub;

	struct CameraData
	{
		double pos[3];
		double view[3];
		double up[3];
	}*cameraData;

	//LIGHT
	vector<string> lightNames;
	struct LightData
	{
		XMFLOAT3 pos;
		XMFLOAT3 color;
	};
	vector<LightData> lightData;

	// Material
};

#endif