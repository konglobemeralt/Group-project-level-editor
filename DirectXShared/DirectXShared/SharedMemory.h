#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <iostream>
#include <windows.h>

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
	int ReadMemory();
	void CreateMesh();

	HANDLE fmCB;
	HANDLE fmMain;

	struct CircBuffer
	{
		size_t freeMem;
		size_t head;
		size_t tail;
		size_t readersCount;
		size_t allRead;
	}* cb;

	size_t memSize;
	void* buffer;

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
		size_t vertexCount;
	};
	vector<MeshData> meshes;
	vector<ID3D11Buffer*> meshesBuffer;
	vector<string> meshNames;

	// TRANSFORM
	vector<string> tranformNames;
	vector<XMFLOAT4X4> transforms;
	vector<ID3D11Buffer*> transformBuffers;

	// CAMERA
	XMFLOAT4X4 view;
	XMFLOAT4X4 projection;
	ID3D11Buffer* viewMatrix;
	ID3D11Buffer* projectionMatrix;

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