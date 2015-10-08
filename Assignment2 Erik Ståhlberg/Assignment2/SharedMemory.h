#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include <vector>
#include <string>
#include <iostream>
#include <windows.h>
#include <DirectXMath.h>
#include "Enumerations.h"

using namespace std;
using namespace DirectX;

class SharedMemory
{
public:

	SharedMemory();
	~SharedMemory();



	char* OpenMemory(size_t size);
	char* CloseMemory();

	HANDLE fmCB;
	HANDLE fmMain;

	struct CircBuffer
	{
		unsigned int freeMem;
		unsigned int head;
		unsigned int tail;
		unsigned int readersCount;
		unsigned int allRead;
	}*cb;
	unsigned int cbSize;

	struct MSGHeader
	{
		unsigned int type;
		unsigned int padding;
	}msgHeader;
	unsigned int msgHeaderSize;

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

	// Camera
	unsigned camDataSize;
};

#endif