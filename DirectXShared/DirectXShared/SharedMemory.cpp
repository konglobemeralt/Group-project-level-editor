#include "SharedMemory.h"

SharedMemory::SharedMemory()
{
	OpenMemory(100);
	slotSize = 256;
	cameraData = new CameraData();
	view = new XMFLOAT4X4();
	projection = new XMFLOAT4X4();
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

	delete cameraData;
}

void SharedMemory::OpenMemory(size_t size)
{
	size *= 1024 * 1024;
	memSize = size;
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

	if (GetLastError() != ERROR_ALREADY_EXISTS)
	{
		cb->head = 0;
		cb->tail = 0;
		cb->freeMem = size;
		cb->readersCount = 0;
		cb->allRead = 0;
	}

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

int SharedMemory::ReadMSGHeader()
{
	if (cb->freeMem < memSize)
	{
		localTail = cb->tail;
		// Message header
		memcpy(&msgHeader, (char*)buffer + localTail, sizeof(MSGHeader));
		localTail += sizeof(MSGHeader);

		// Move tail
		cb->tail += sizeof(MSGHeader);
		cb->freeMem += sizeof(MSGHeader);

		return msgHeader.type;
	}
	return -1;
}

int SharedMemory::ReadMemory(unsigned int type)
{
	//meshes.push_back(MeshData());
	//tail = 4;

	// Test mesh
	//memcpy(&meshes[0].vertexCount, (int*)buffer, sizeof(int));
	//meshes[0].vertexData.resize(meshes[0].vertexCount);
	//// Retrieve a cube from maya
	//for (size_t i = 0; i < meshes[0].vertexCount; i++)
	//{
	//	memcpy(&meshes[0].vertexData[i].pos, (char*)buffer + tail, sizeof(XMFLOAT3));
	//	tail += sizeof(XMFLOAT3);
	//	memcpy(&meshes[0].vertexData[i].uv, (char*)buffer + tail, sizeof(XMFLOAT2));
	//	tail += sizeof(XMFLOAT2);
	//	memcpy(&meshes[0].vertexData[i].normal, (char*)buffer + tail, sizeof(XMFLOAT3));
	//	tail += sizeof(XMFLOAT3);
	//}

	localTail = cb->tail;

	if (cb->freeMem < memSize)
	{
		if (type == TMeshCreate)
		{
			// Read and store whole mesh data
		}
		else if (type == TMeshUpdate)
		{
			// Read updated data and store vertexbuffer
		}
		else if (type == TCameraUpdate)
		{
			// Read and store camera

			// Data
			memcpy(&cameraData->pos, (char*)buffer + localTail, sizeof(double) * 3);
			localTail += sizeof(double) * 3;
			memcpy(&cameraData->view, (char*)buffer + localTail, sizeof(double) * 3);
			localTail += sizeof(double) * 3;
			memcpy(&cameraData->up, (char*)buffer + localTail, sizeof(double) * 3);

			// Move tail
			cb->tail += slotSize;
			cb->freeMem += slotSize;

			return TCameraUpdate;
		}
	}

	return -1;
}

void SharedMemory::TempMesh()
{
	// CUBE
	meshes.push_back(MeshData());
	meshes[0].vertexData.resize(6);

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