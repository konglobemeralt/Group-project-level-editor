#ifndef MESHES_H
#define MESHES_H

#include "Includes.h"

class Meshes
{
public:

	Meshes();
	~Meshes();

	void Update();

private:

	struct VertexData
	{
		XMFLOAT3 pos;
		XMFLOAT2 uv;
		XMFLOAT3 normal;
	};
	struct MeshData
	{
		vector<VertexData> vertexData;
	};
	vector<MeshData> meshes;
	vector<ID3D11Buffer*> meshesBuffer;

	vector<string> names;
};

#endif