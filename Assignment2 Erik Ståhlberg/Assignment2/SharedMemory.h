#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include <vector>
#include <string>
#include <iostream>
#include <windows.h>
#include <DirectXMath.h>

using namespace std;
using namespace DirectX;

class SharedMemory
{
public:

	SharedMemory();
	~SharedMemory();



	char* OpenMemory(size_t size);
	char* CloseMemory();
	void SetVertex(XMFLOAT3 pos);

	HANDLE fmCB;
	HANDLE fmMain;

	struct CircBuffer
	{
		size_t freeMem;
		size_t head;
		size_t tail;
		size_t readersCount;
		size_t allRead;
	}*cb;

	struct MainHeader
	{
		size_t id;
		size_t length;
		size_t padder;

		size_t objectType;
		size_t nameLength;
		size_t type;
	};

	size_t memSize;
	void* buffer;

	// MESHES
	struct VertexData
	{
		XMFLOAT3 pos;
		XMFLOAT2 uv;
		XMFLOAT3 normal;
	};
	vector<VertexData> vertexData;
};

#endif