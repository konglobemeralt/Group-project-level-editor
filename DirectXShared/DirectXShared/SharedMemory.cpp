#include "SharedMemory.h"

SharedMemory::SharedMemory()
{
	OpenMemory(10);
}

SharedMemory::~SharedMemory()
{
	if (UnmapViewOfFile(cb) == 0)
		OutputDebugStringA("Failed unmap CircBuffer!");
	if (CloseHandle(fmCB) == 0)
		OutputDebugStringA("Failed close fmCB!");
	if (UnmapViewOfFile(buffer) == 0)
		OutputDebugStringA("Failed unmap buffer!");
	if (CloseHandle(fmMain) == 0)
		OutputDebugStringA("Failed unmap fmMain!");
}

void SharedMemory::OpenMemory(size_t size)
{
	size *= 1024 * 1024;
	// Circular buffer data
	fmCB = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		(DWORD)0,
		size,
		L"Global/CircularBuffer4");
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		OutputDebugStringA("CircularBuffer allready exist\n");

	if (fmCB == NULL)
		OutputDebugStringA("Could not open file mapping object! -> CircularBuffer\n");

	cb = (CircBuffer*)MapViewOfFile(fmCB, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (cb == NULL)
	{
		OutputDebugStringA("Could not map view of file!\n");
		CloseHandle(cb);
	}

	cb->head = 0;
	cb->tail = 0;
	cb->freeMem = size;
	cb->readersCount = 0;
	cb->allRead = 0;

	// Main data
	fmMain = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		(DWORD)0,
		size,
		L"Global/MainData4");
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		OutputDebugStringA("MainData allready exist\n");

	if (fmMain == NULL)
		OutputDebugStringA("Could not open file mapping object! -> MainData\n");

	buffer = MapViewOfFile(fmMain, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (buffer == NULL)
	{
		OutputDebugStringA("Could not map view of file!\n");
		CloseHandle(buffer);
	}
}

int SharedMemory::ReadMemory()
{
	meshes.push_back(MeshData());
	head = 4;

	memcpy(&meshes[0].vertexCount, (int*)buffer, sizeof(int));
	meshes[0].vertexData.resize(meshes[0].vertexCount);
	// Retrieve a cube from maya
	for (size_t i = 0; i < meshes[0].vertexCount; i++)
	{
		memcpy(&meshes[0].vertexData[i].pos, (XMFLOAT3*)buffer + head, sizeof(XMFLOAT3));
	}
	//meshes[0].vertexCount = 36;

	return 0;
}

void SharedMemory::CreateMesh()
{
	// CUBE
	meshes.push_back(MeshData());
	meshes[0].vertexData.resize(6);

	//meshes[0].vertexData[0].pos = XMFLOAT3(-0.5, -5.0, 0.0);
	//meshes[0].vertexData[1].pos = XMFLOAT3(0.0, 5.0, 0.0);
	//meshes[0].vertexData[2].pos = XMFLOAT3(5.0, -5.0, 0.0);
	//meshes[0].vertexData[3].pos = XMFLOAT3(10.0, 5.0, 0.0);

	meshes[0].vertexData[0].pos = XMFLOAT3(-0.5, -0.5, 0.0);
	meshes[0].vertexData[1].pos = XMFLOAT3(-0.5, 0.5, 0.0);
	meshes[0].vertexData[2].pos = XMFLOAT3(0.5, -0.5, 0.0);
	meshes[0].vertexData[3].pos = XMFLOAT3(0.5, -0.5, 0.0);
	meshes[0].vertexData[4].pos = XMFLOAT3(-0.5, 0.5, 0.0);
	meshes[0].vertexData[5].pos = XMFLOAT3(0.5, 0.5, 0.0);
	meshes[0].vertexCount = 6;
}

void SharedMemory::Update()
{

}