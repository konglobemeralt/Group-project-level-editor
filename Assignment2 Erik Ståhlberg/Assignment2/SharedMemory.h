#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include <vector>
#include <string>
#include <iostream>
#include <windows.h>
#include <DirectXMath.h>
#include "Enumerations.h"
#include <fstream>

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
	vector<XMFLOAT3> pos;
	vector<XMFLOAT2> uv;
	vector<XMFLOAT3> normal;
	vector<XMFLOAT3> vertices;

	// Camera
	unsigned camDataSize;
};

#endif