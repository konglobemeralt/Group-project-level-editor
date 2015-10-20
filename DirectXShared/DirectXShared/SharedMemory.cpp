#include "SharedMemory.h"

SharedMemory::SharedMemory()
{
	OpenMemory(100);
	slotSize = 256;
	cameraData = new CameraData();
	view = new XMFLOAT4X4();
	projection = new XMFLOAT4X4();
	testViewMatrix = new XMFLOAT4X4();
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
		L"Global/CircularBuffer2");
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
		L"Global/MainData2");
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

		return msgHeader.type;
	}
	return -1;
}

void SharedMemory::ReadMemory(unsigned int type)
{
	if (type == TMeshCreate)
	{
		// Read and store whole mesh data

		// Size of mesh
		memcpy(&meshSize, (char*)buffer + localTail, sizeof(int));
		localTail += sizeof(int);

		meshes.push_back(MeshData());
		meshes.back().transform = new XMFLOAT4X4();

		if (meshSize > 0)
		{
			meshes.back().vertexCount = meshSize;

			// Rezise to hold every vertex
			meshes.back().pos = new XMFLOAT3[meshes.back().vertexCount];
			meshes.back().uv = new XMFLOAT2[meshes.back().vertexCount];
			meshes.back().normal = new XMFLOAT3[meshes.back().vertexCount];

			// Vertex data
			memcpy(meshes.back().pos, (char*)buffer + localTail, sizeof(XMFLOAT3)* meshes.back().vertexCount);
			localTail += sizeof(XMFLOAT3)* meshes.back().vertexCount;
			memcpy(meshes.back().uv, (char*)buffer + localTail, sizeof(XMFLOAT2)* meshes.back().vertexCount);
			localTail += sizeof(XMFLOAT2)* meshes.back().vertexCount;
			memcpy(meshes.back().normal, (char*)buffer + localTail, sizeof(XMFLOAT3)* meshes.back().vertexCount);
			localTail += sizeof(XMFLOAT3)* meshes.back().vertexCount;

			// Matrix data
			memcpy(meshes.back().transform, (char*)buffer + localTail, sizeof(XMFLOAT4X4));
			localTail += sizeof(XMFLOAT4X4);

			// Material data
			memcpy(&meshes.back().meshTex.materialColor, (char*)buffer + localTail, sizeof(XMFLOAT4));
			localTail += sizeof(XMFLOAT4);

			//Texture true or false
			memcpy(&meshes.back().meshTex.textureExist.x, (char*)buffer + localTail, sizeof(int));
			localTail += sizeof(int);

			if (meshes.back().meshTex.textureExist.x == 1)
			{
				//Texture path size
				memcpy(&meshes.back().textureSize, (char*)buffer + localTail, sizeof(int));
				localTail += sizeof(int);

				meshes.back().texturePath = new char[meshes.back().textureSize + 1];

				//Texture data
				memcpy(meshes.back().texturePath, (char*)buffer + localTail, meshes.back().textureSize);
				localTail += meshes.back().textureSize;

				meshes.back().texturePath[meshes.back().textureSize] = '\0';

			}

			// Move tail
			cb->freeMem += (localTail - cb->tail) + msgHeader.padding;
			cb->tail += (localTail - cb->tail) + msgHeader.padding;
		}
	}
	else if (type == TMeshUpdate)
	{
		// Read updated data and store vertexbuffer
	}
	else if (type == TMaterialUpdate)
	{
		memcpy(&localMesh, (char*)buffer + localTail, sizeof(int));
		localTail += sizeof(int);

		// Material data
		memcpy(&meshes[localMesh].meshTex.materialColor, (char*)buffer + localTail, sizeof(XMFLOAT4));
		localTail += sizeof(XMFLOAT4);

		//Texture true or false
		memcpy(&meshes[localMesh].meshTex.textureExist.x, (char*)buffer + localTail, sizeof(int));
		localTail += sizeof(int);

		if (meshes[localMesh].meshTex.textureExist.x == 1)
		{
			//Texture path size
			memcpy(&meshes[localMesh].textureSize, (char*)buffer + localTail, sizeof(int));
			localTail += sizeof(int);

			meshes[localMesh].texturePath = new char[meshes[localMesh].textureSize + 1];

			//Texture data
			memcpy(meshes[localMesh].texturePath, (char*)buffer + localTail, meshes[localMesh].textureSize);
			localTail += meshes[localMesh].textureSize;

			meshes[localMesh].texturePath[meshes[localMesh].textureSize] = '\0';

		}

		// Move tail
		cb->freeMem += (localTail - cb->tail) + msgHeader.padding;
		cb->tail += (localTail - cb->tail) + msgHeader.padding;
	}
	else if (type == TtransformUpdate)
	{
		memcpy(&localMesh, (char*)buffer + localTail, sizeof(int));
		localTail += sizeof(int);
	}
	else if (type == TLightCreate)
	{
		lights.push_back(Lights());
		lights.back().lightData = new LightData();

		// Light data
		memcpy(&lights.back().lightData->pos, (char*)buffer + localTail, sizeof(XMFLOAT3));
		localTail += sizeof(XMFLOAT3);
		memcpy(&lights.back().lightData->color, (char*)buffer + localTail, sizeof(XMFLOAT4));
		localTail += sizeof(XMFLOAT4);

		cb->tail += slotSize;
		cb->freeMem += slotSize;
	}
	else if (type == TMeshDestroyed)
	{
		localTail = cb->tail + sizeof(MSGHeader);
		// Read mesh index
		memcpy(&localMesh, (char*)buffer + localTail, sizeof(int));
		localTail += sizeof(int);

		// Move tail
		cb->tail += slotSize;
		cb->freeMem += slotSize;
	}
}