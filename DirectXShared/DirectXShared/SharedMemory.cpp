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

		//// Move tail
		//cb->tail += sizeof(MSGHeader);
		//cb->freeMem += sizeof(MSGHeader);

		return msgHeader.type;
	}
	return -1;
}

void SharedMemory::ReadMemory(unsigned int type)
{
	if (type == TMeshCreate)
	{
		// Read and store whole mesh data

		meshes.push_back(MeshData());
		meshes.back().transform = new XMFLOAT4X4();
		meshes.back().materialColor = new XMFLOAT4();

		// Size of mesh
		memcpy(&meshes.back().vertexCount, (char*)buffer + localTail, sizeof(int));
		localTail += sizeof(int);

		// Rezise to hold every vertex
		meshes.back().pos = new XMFLOAT3[meshes.back().vertexCount];
		meshes.back().uv = new XMFLOAT2[meshes.back().vertexCount];
		meshes.back().normal = new XMFLOAT3[meshes.back().vertexCount];

		// Vertex data
		memcpy(meshes.back().pos, (char*)buffer + localTail, sizeof(XMFLOAT3) * meshes.back().vertexCount);
		localTail += sizeof(XMFLOAT3) * meshes.back().vertexCount;
		memcpy(meshes.back().uv, (char*)buffer + localTail, sizeof(XMFLOAT2) * meshes.back().vertexCount);
		localTail += sizeof(XMFLOAT2) * meshes.back().vertexCount;
		memcpy(meshes.back().normal, (char*)buffer + localTail, sizeof(XMFLOAT3) * meshes.back().vertexCount);
		localTail += sizeof(XMFLOAT3) * meshes.back().vertexCount;

		// Matrix data
		memcpy(meshes.back().transform, (char*)buffer + localTail, sizeof(XMFLOAT4X4));
		localTail += sizeof(XMFLOAT4X4);

		// Material data
		memcpy(meshes.back().materialColor, (char*)buffer + localTail, sizeof(XMFLOAT4));
		localTail += sizeof(XMFLOAT4);

		// Move tail
		cb->tail += (meshes.back().vertexCount * sizeof(float) * 8) + sizeof(MSGHeader) + msgHeader.padding + sizeof(XMFLOAT4X4) + sizeof(int) + sizeof(XMFLOAT4);
		cb->freeMem += (meshes.back().vertexCount * sizeof(float)* 8) + sizeof(MSGHeader)+msgHeader.padding + sizeof(XMFLOAT4X4)+sizeof(int)+sizeof(XMFLOAT4);
	}
	else if (type == TMeshUpdate)
	{
		// Read updated data and store vertexbuffer
	}
	else if (type == TVertexUpdate)
	{
		// Mesh index
		memcpy(&localMesh, (char*)buffer + localTail, sizeof(int));
		localTail += sizeof(int);
	}
	else if (type == TCameraUpdate)
	{
		// Read and store camera

		// View matrix
		memcpy(&cameraData->pos, (char*)buffer + localTail, sizeof(double)* 3);
		localTail += sizeof(double)* 3;
		memcpy(&cameraData->view, (char*)buffer + localTail, sizeof(double)* 3);
		localTail += sizeof(double)* 3;
		memcpy(&cameraData->up, (char*)buffer + localTail, sizeof(double)* 3);
		localTail += sizeof(double)* 3;

		// View matrix
		memcpy(testViewMatrix, (char*)buffer + localTail, sizeof(XMFLOAT4X4));
		localTail += sizeof(XMFLOAT4X4);

		// Projection matrix
		memcpy(&projectionTemp, (char*)buffer + localTail, sizeof(XMFLOAT4X4));

		// Move tail
		cb->tail += slotSize;
		cb->freeMem += slotSize;
	}
	else if (type == TtransformUpdate)
	{
		memcpy(&localMesh, (char*)buffer + localTail, sizeof(int));
		localTail += sizeof(int);
	}
	else if (type == TLightCreate)
	{
		lights.push_back(Lights());

		memcpy (&lights.back ().lightData.pos, (char*) buffer + localTail, sizeof(XMFLOAT3));
		localTail += sizeof(XMFLOAT3);
		memcpy (&lights.back ().lightData.color, (char*) buffer + localTail, sizeof(XMFLOAT4));
		localTail += sizeof(XMFLOAT4);

		cb->tail += slotSize;
		cb->freeMem += slotSize;
	}
	else if (type == TLightUpdate)
	{
		memcpy(&localLight, (char*)buffer + localTail, sizeof(int));
		localTail += sizeof(int);
	}
	else if (type == TNodeDestroyed)
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

void SharedMemory::TempMesh()
{
	// CUBE
	meshes.push_back(MeshData());
	//meshes[0].vertexData.resize(6);

	//meshes[0].vertexData[0].pos = XMFLOAT3(-0.5, -0.5, 0.0);
	//meshes[0].vertexData[1].pos = XMFLOAT3(-0.5, 0.5, 0.0);
	//meshes[0].vertexData[2].pos = XMFLOAT3(0.5, -0.5, 0.0);
	//meshes[0].vertexData[3].pos = XMFLOAT3(0.5, -0.5, 0.0);
	//meshes[0].vertexData[4].pos = XMFLOAT3(-0.5, 0.5, 0.0);
	//meshes[0].vertexData[5].pos = XMFLOAT3(0.5, 0.5, 0.0);
	//meshes[0].vertexCount = 6;
}