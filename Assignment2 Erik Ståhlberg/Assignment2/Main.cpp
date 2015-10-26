#include "MayaHeaders.h"
#include <iostream>
#include "SharedMemory.h"

void GetSceneData();

void GetMeshInformation(MFnMesh& mesh);
void MeshCreationCB(MObject& node, void* clientData);
void MeshChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);
void AddedVertex(MPlug& plug);
void VertexChanged(MPlug& plug);
void NormalChanged(MPlug& plug);
void UVChanged(MPlug& plug);

void MaterialChanged(MFnMesh& mesh);
void ShaderChangedCB(MObject& object, void* clientData);
void shaderchanged(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);

void TransformCreationCB(MObject& object, void* clientData);
void TransformChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);

void GetCameraInformation();
void CameraChanged(MFnTransform& transform, MFnCamera& camera);

void GetLightInformation(MFnLight& light);
void LightCreationCB(MObject& lightObject, void* clientData);
void LightChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);

void NameChangedCB(MObject& node, const MString& prevName, void* clientData);
void TimerCB(float elapsedTime, float lastTime, void* clientData);

void NodeDestroyedCB(MObject& object, MDGModifier& modifier, void* clientData);

// Timer callback
float period = 5.0f;
int totalTime = 0.0f;

MCallbackIdArray callbackIds;

SharedMemory sm;
unsigned int localHead;
unsigned int slotSize;
MStringArray meshNames;
MStringArray meshMatNames;

EXPORT MStatus initializePlugin(MObject obj)
{
	MStatus res = MS::kSuccess;

	MFnPlugin myPlugin(obj, "Maya plugin", "1.0", "any", &res);
	if (MFAIL(res))
		CHECK_MSTATUS(res);

	sm.camDataSize = sizeof(MVector)* 3;
	localHead = 0;
	slotSize = 256;
	sm.cbSize = 20;
	sm.msgHeaderSize = 8;

	MString memoryString;
	//sm.OpenMemory(1.0f / 256.0f)
	memoryString = sm.OpenMemory(100);
	if (memoryString != "Shared memory open success!")
	{
		MGlobal::displayInfo(memoryString);
		sm.CloseMemory();
		return MStatus::kFailure;
	}
	else
	{
		MGlobal::displayInfo(memoryString);
	}

	GetSceneData();

	callbackIds.append(MDGMessage::addNodeAddedCallback(MeshCreationCB, "mesh", &res));
	callbackIds.append(MDGMessage::addNodeAddedCallback(TransformCreationCB, "transform", &res));
	callbackIds.append(MDGMessage::addNodeAddedCallback(LightCreationCB, "light", &res));
	callbackIds.append(MDGMessage::addNodeAddedCallback(ShaderChangedCB, "lambert", &res));
	callbackIds.append(MDGMessage::addNodeAddedCallback(ShaderChangedCB, "blinn", &res));
	callbackIds.append(MDGMessage::addNodeAddedCallback(ShaderChangedCB, "phong", &res));
	callbackIds.append(MTimerMessage::addTimerCallback(period, TimerCB, &res));

	GetCameraInformation();

	if (res == MS::kSuccess)
		MGlobal::displayInfo("Maya plugin loaded!");

	return res;
}

EXPORT MStatus uninitializePlugin(MObject obj)
{
	MFnPlugin plugin(obj);

	if (callbackIds.length() != 0)
		MMessage::removeCallbacks(callbackIds);

	MGlobal::displayInfo(sm.CloseMemory());
	MGlobal::displayInfo("Maya plugin unloaded!");

	return MS::kSuccess;
}


void GetSceneData()
{
	// Meshes
	MItDag itMeshes(MItDag::kDepthFirst, MFn::kMesh);
	while (!itMeshes.isDone())
	{
		MFnMesh mesh(itMeshes.item());
		GetMeshInformation(mesh);
		callbackIds.append(MNodeMessage::addAttributeChangedCallback(mesh.object(), MeshChangedCB));
		callbackIds.append(MNodeMessage::addNameChangedCallback(mesh.object(), NameChangedCB));
		callbackIds.append(MNodeMessage::addNodeAboutToDeleteCallback(mesh.object(), NodeDestroyedCB));
		itMeshes.next();
	}

	// Tranforms including cameras
	MItDag itTransform(MItDag::kDepthFirst, MFn::kTransform);
	while (!itTransform.isDone())
	{
		MFnTransform transform(itTransform.item());
		callbackIds.append(MNodeMessage::addAttributeChangedCallback(transform.object(), TransformChangedCB));
		callbackIds.append(MNodeMessage::addNameChangedCallback(transform.object(), NameChangedCB));
		itTransform.next();
	}

	// Lights
	MItDag itLight(MItDag::kDepthFirst, MFn::kLight);
	while (!itLight.isDone())
	{
		MFnLight light(itLight.item());
		GetLightInformation(light);
		callbackIds.append(MNodeMessage::addAttributeChangedCallback(light.object(), LightChangedCB));
		callbackIds.append(MNodeMessage::addAttributeChangedCallback(light.parent(0), LightChangedCB));
		itLight.next();
	}

	// Cameras
	MItDag itCamera(MItDag::kDepthFirst, MFn::kCamera);
	while (!itCamera.isDone())
	{
		MFnCamera camera(itCamera.item());
		callbackIds.append(MNodeMessage::addNameChangedCallback(camera.object(), NameChangedCB));
		itCamera.next();
	}

	// Lambert Materials
	MItDependencyNodes itLambert(MFn::kLambert);
	while (!itLambert.isDone())
	{
		MFnLambertShader lambertShader(itLambert.item());

		callbackIds.append(MNodeMessage::addAttributeChangedCallback(lambertShader.object(), shaderchanged));
		itLambert.next();
	}

	// Blinn Materials
	MItDependencyNodes itBlinn(MFn::kBlinn);
	while (!itBlinn.isDone())
	{
		MFnBlinnShader blinnShader(itBlinn.item());

		callbackIds.append(MNodeMessage::addAttributeChangedCallback(blinnShader.object(), shaderchanged));
		itBlinn.next();
	}

	// Phong Materials
	MItDependencyNodes itPhong(MFn::kPhong);
	while (!itPhong.isDone())
	{
		MFnPhongShader phongShader(itPhong.item());

		callbackIds.append(MNodeMessage::addAttributeChangedCallback(phongShader.object(), shaderchanged));
		itPhong.next();
	}
}


void GetMeshInformation(MFnMesh& mesh)
{
	MItMeshPolygon itPoly(mesh.object());

	float2 uv;
	MVector normal;
	MPointArray points;
	MIntArray vertexList;

	sm.pos.clear();
	sm.uv.clear();
	sm.normal.clear();

	// Normals are per face
	while (!itPoly.isDone())
	{
		itPoly.getTriangles(points, vertexList);

		for (size_t i = 0; i < points.length(); i++)
		{
			itPoly.getUVAtPoint(points[i], uv);
			//itPoly.getNormal(vertexList[i], normal);
			itPoly.getNormal(normal);

			sm.pos.push_back(XMFLOAT3(points[i].x, points[i].y, points[i].z));
			sm.uv.push_back(XMFLOAT2(uv[0], 1 - uv[1]));
			sm.normal.push_back(XMFLOAT3(normal.x, normal.y, normal.z));
		}
		itPoly.next();
	}

	if (sm.pos.size() > 0)
	{
		MGlobal::displayInfo("Mesh: " + mesh.fullPathName() + " loaded!");
		meshNames.append(mesh.name());

		MFnTransform transform(mesh.parent(0));
		MMatrix transMatrix = transform.transformationMatrix().transpose();
		float pm[4][4];
		transMatrix.get(pm);
		XMFLOAT4X4 matrixData(
			pm[0][0], pm[0][1], pm[0][2], pm[0][3],
			pm[1][0], pm[1][1], pm[1][2], pm[1][3],
			pm[2][0], pm[2][1], pm[2][2], pm[2][3],
			pm[3][0], pm[3][1], pm[3][2], pm[3][3]);

		// MATERIAL:
		MObjectArray shaders;
		MIntArray indices;
		MPlugArray connections;
		MColor color;
		int whatMaterial = 0;
		MFnLambertShader lambertShader;
		MFnBlinnShader blinnShader;
		MFnPhongShader phongShader;

		// Find the shadingReasourceGroup
		mesh.getConnectedShaders(0, shaders, indices);
		MFnDependencyNode shaderGroup(shaders[0]);
		MGlobal::displayInfo(shaderGroup.name());
		MPlug shaderPlug = shaderGroup.findPlug("surfaceShader");
		MGlobal::displayInfo(shaderPlug.name());
		shaderPlug.connectedTo(connections, true, false);

		// Find the material and then color
		if (connections[0].node().hasFn(MFn::kLambert))
		{
			lambertShader.setObject(connections[0].node());
			MGlobal::displayInfo(lambertShader.name());
			color = lambertShader.color();
			whatMaterial = 1;
			meshMatNames.append(lambertShader.name());
		}
		else if (connections[0].node().hasFn(MFn::kBlinn))
		{
			blinnShader.setObject(connections[0].node());
			MGlobal::displayInfo(blinnShader.name());
			color = blinnShader.color();
			whatMaterial = 2;
			meshMatNames.append(blinnShader.name());
		}
		else if (connections[0].node().hasFn(MFn::kPhong))
		{
			phongShader.setObject(connections[0].node());
			MGlobal::displayInfo(phongShader.name());
			color = phongShader.color();
			whatMaterial = 3;
			meshMatNames.append(phongShader.name());
		}

		//Texture:
		MObjectArray Files;
		MString filename;
		int pathSize;
		int texExist = 0;
		MPlugArray textureConnect;
		MPlug texturePlug;

		if (whatMaterial == 1)
		{
			texturePlug = lambertShader.findPlug("color");
		}
		else if (whatMaterial == 2)
		{
			texturePlug = blinnShader.findPlug("color");
		}
		else if (whatMaterial == 3)
		{
			texturePlug = phongShader.findPlug("color");
		}

		MGlobal::displayInfo(texturePlug.name());
		texturePlug.connectedTo(textureConnect, true, false);

		if (textureConnect.length() != 0)
		{
			MGlobal::displayInfo(textureConnect[0].name());

			MFnDependencyNode fn(textureConnect[0].node());
			MGlobal::displayInfo(fn.name());

			MPlug ftn = fn.findPlug("ftn");

			ftn.getValue(filename);

			MGlobal::displayInfo(filename);

			pathSize = filename.numChars();

			texExist = 1;
		}

		// Send data to shared memory
		unsigned int meshSize = sm.pos.size();
		if (sm.pos.size() > 60)
			int hej = 0;
		if (meshSize > 0)
		{
			sm.msgHeader.type = TMeshCreate;
			unsigned int lastFreeMem = 0;
			if (texExist == 1)
			{
				sm.msgHeader.byteSize = (meshSize * sizeof(float) * 8) + sm.msgHeaderSize + sizeof(XMFLOAT4X4) + sizeof(int) + sizeof(MColor) + sizeof(int) + sizeof(int) + pathSize;
				sm.msgHeader.byteSize += slotSize - ((meshSize * sizeof(float) * 8) + sm.msgHeaderSize + sizeof(XMFLOAT4X4) + sizeof(int) + sizeof(MColor) + sizeof(int) + sizeof(int) + pathSize) % slotSize;
			}
			else
			{
				sm.msgHeader.byteSize = (meshSize * sizeof(float) * 8) + sm.msgHeaderSize + sizeof(XMFLOAT4X4) + sizeof(int) + sizeof(MColor) + sizeof(int);
				sm.msgHeader.byteSize += slotSize - ((meshSize * sizeof(float) * 8) + sm.msgHeaderSize + sizeof(XMFLOAT4X4) + sizeof(int) + sizeof(MColor) + sizeof(int)) % slotSize;
			}
			do
			{
				if (sm.cb->freeMem > sm.msgHeader.byteSize)
				{
					// Sets head to 0 if there are no place to write
					if (sm.cb->head > sm.memSize - sm.msgHeader.byteSize)
					{
						// The place over will set a number to indicate that the head was moved to 0
						unsigned int type = TAmount + 1;
						memcpy((char*)sm.buffer + sm.cb->head, &type, sizeof(int));
						lastFreeMem = sm.memSize - sm.cb->head;
						sm.cb->head = 0;
					}
					localHead = sm.cb->head;

					// Message header
					memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
					localHead += sm.msgHeaderSize;

					// Size of mesh
					memcpy((char*)sm.buffer + localHead, &meshSize, sizeof(int));
					localHead += sizeof(int);

					// Vertex data
					memcpy((char*)sm.buffer + localHead, sm.pos.data(), sizeof(XMFLOAT3)* sm.pos.size());
					localHead += sizeof(XMFLOAT3)* sm.pos.size();
					memcpy((char*)sm.buffer + localHead, sm.uv.data(), sizeof(XMFLOAT2)* sm.uv.size());
					localHead += sizeof(XMFLOAT2)* sm.uv.size();
					memcpy((char*)sm.buffer + localHead, sm.normal.data(), sizeof(XMFLOAT3)* sm.normal.size());
					localHead += sizeof(XMFLOAT3)* sm.normal.size();

					// Matrix data
					memcpy((char*)sm.buffer + localHead, &matrixData, sizeof(XMFLOAT4X4));
					localHead += sizeof(XMFLOAT4X4);

					// Material data
					memcpy((char*)sm.buffer + localHead, &color, sizeof(MColor));
					localHead += sizeof(MColor);

					//Bool for Texture
					XMINT4 ha = XMINT4(texExist, 0, 0, 0);
					memcpy((char*)sm.buffer + localHead, &ha.x, sizeof(int));
					localHead += sizeof(int);

					if (texExist == 1)
					{
						//Texture Size
						memcpy((char*)sm.buffer + localHead, &pathSize, sizeof(int));
						localHead += sizeof(int);

						//Texture data
						memcpy((char*)sm.buffer + localHead, filename.asChar(), pathSize);
						localHead += pathSize;
					}

					// Move header
					sm.cb->freeMem -= (sm.msgHeader.byteSize + lastFreeMem);
					sm.cb->head += sm.msgHeader.byteSize;

					break;
				}
			} while (sm.cb->freeMem >!sm.msgHeader.byteSize);
		}
	}
}

void MeshCreationCB(MObject& object, void* clientData)
{
	// Add a callback specifik to every new mesh that are created
	callbackIds.append(MNodeMessage::addAttributeChangedCallback(object, MeshChangedCB));
	callbackIds.append(MNodeMessage::addNameChangedCallback(object, NameChangedCB));
	callbackIds.append(MNodeMessage::addNodeAboutToDeleteCallback(object, NodeDestroyedCB));
}

void MeshChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData)
{
	// check if the plug p_Plug has in its name "doubleSided", which is an attribute that when is set we know that the mesh is finally available.
	// Only used for the creation of the mesh
	if (strstr(plug.name().asChar(), "doubleSided") != NULL && MNodeMessage::AttributeMessage::kAttributeSet)
	{
		MFnMesh mesh(plug.node());
		MGlobal::displayInfo("Mesh: " + mesh.fullPathName() + " created!");
		GetMeshInformation(mesh);
	}
	// Vertex has changed
	else if (strstr(plug.partialName().asChar(), "pt["))
		VertexChanged(plug);
	// Normal has changed
	else if (strstr(plug.partialName().asChar(), "NONAME"))
		NormalChanged(plug);
	// Uv has changed
	else if (strstr(plug.partialName().asChar(), "pv"))
		UVChanged(plug);
	// Material has changed
	else if (strstr(plug.partialName().asChar(), "iog"))
	{
		MFnMesh mesh(plug.node());
		MaterialChanged(mesh);
	}
	// Vertex added to mesh
	else if (strstr(otherPlug.name().asChar(), "polyExtrude") && strstr(otherPlug.name().asChar(), "manipMatrix"))
	{
		AddedVertex(plug);
	}
}

void AddedVertex(MPlug& plug)
{
	MFnMesh mesh(plug.node());

	MItMeshPolygon itPoly(mesh.object());

	float2 uv;
	MVector normal;
	MPointArray points;
	MIntArray vertexList;

	sm.pos.clear();
	sm.uv.clear();
	sm.normal.clear();

	// Normals are per face
	while (!itPoly.isDone())
	{
		itPoly.getTriangles(points, vertexList);

		for (size_t i = 0; i < points.length(); i++)
		{
			itPoly.getUVAtPoint(points[i], uv);
			//itPoly.getNormal(vertexList[i], normal);
			itPoly.getNormal(normal);

			sm.pos.push_back(XMFLOAT3(points[i].x, points[i].y, points[i].z));
			sm.uv.push_back(XMFLOAT2(uv[0], 1 - uv[1]));
			sm.normal.push_back(XMFLOAT3(normal.x, normal.y, normal.z));
		}
		itPoly.next();
	}

	if (sm.pos.size() > 0)
	{
		unsigned int meshIndex = meshNames.length();
		for (size_t i = 0; i < meshNames.length(); i++)
		{
			if (meshNames[i] == mesh.name())
				meshIndex = i;
		}

		// Send data to shared memory
		unsigned int meshSize = sm.pos.size();
		sm.msgHeader.type = TAddedVertex;
		sm.msgHeader.byteSize = sm.msgHeaderSize + (meshSize * sizeof(float) * 8) + sizeof(int) * 2;
		sm.msgHeader.byteSize += slotSize - (sm.msgHeaderSize + (meshSize * sizeof(float) * 8) + sizeof(int) * 2) % slotSize;
		unsigned int lastFreeMem = 0;
		do
		{
			if (sm.cb->freeMem > sm.msgHeader.byteSize)
			{
				// Sets head to 0 if there are no place to write
				if (sm.cb->head > sm.memSize - sm.msgHeader.byteSize)
				{
					// The place over will set a number to indicate that the head was moved to 0
					unsigned int type = TAmount + 1;
					memcpy((char*)sm.buffer + sm.cb->head, &type, sizeof(int));
					lastFreeMem = sm.memSize - sm.cb->head;
					sm.cb->head = 0;
				}
				localHead = sm.cb->head;

				// Message header
				memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
				localHead += sm.msgHeaderSize;

				// Mesh index
				memcpy((char*)sm.buffer + localHead, &meshIndex, sizeof(int));
				localHead += sizeof(int);

				// Size of mesh
				memcpy((char*)sm.buffer + localHead, &meshSize, sizeof(int));
				localHead += sizeof(int);

				// Vertex data
				memcpy((char*)sm.buffer + localHead, sm.pos.data(), sizeof(XMFLOAT3)* sm.pos.size());
				localHead += sizeof(XMFLOAT3)* sm.pos.size();
				memcpy((char*)sm.buffer + localHead, sm.uv.data(), sizeof(XMFLOAT2)* sm.uv.size());
				localHead += sizeof(XMFLOAT2)* sm.uv.size();
				memcpy((char*)sm.buffer + localHead, sm.normal.data(), sizeof(XMFLOAT3)* sm.normal.size());
				localHead += sizeof(XMFLOAT3)* sm.normal.size();

				// Move header
				sm.cb->freeMem -= (sm.msgHeader.byteSize + lastFreeMem);
				sm.cb->head += sm.msgHeader.byteSize;

				break;
			}
		} while (sm.cb->freeMem >! sm.msgHeader.byteSize);
	}
}

void VertexChanged(MPlug& plug)
{
	MFnMesh mesh(plug.node());
	MItMeshPolygon itPoly(mesh.object());

	float2 uv;
	MVector normal;
	MPointArray points;
	MIntArray vertexList;

	sm.pos.clear();
	sm.uv.clear();
	sm.normal.clear();

	// Normals are per face
	while (!itPoly.isDone())
	{
		itPoly.getTriangles(points, vertexList);

		for (size_t i = 0; i < points.length(); i++)
		{
			itPoly.getUVAtPoint(points[i], uv);
			//itPoly.getNormal(vertexList[i], normal);
			itPoly.getNormal(normal);

			sm.pos.push_back(XMFLOAT3(points[i].x, points[i].y, points[i].z));
			sm.uv.push_back(XMFLOAT2(uv[0], 1 - uv[1]));
			sm.normal.push_back(XMFLOAT3(normal.x, normal.y, normal.z));
		}
		itPoly.next();
	}

	unsigned int meshIndex = meshNames.length();
	for (size_t i = 0; i < meshNames.length(); i++)
	{
		if (meshNames[i] == mesh.name())
			meshIndex = i;
	}

	// Send data to shared memory
	sm.msgHeader.type = TVertexUpdate;
	sm.msgHeader.byteSize = (sizeof(XMFLOAT3) * 2 * sm.pos.size()) + sm.msgHeaderSize + sizeof(int) + (sizeof(XMFLOAT2) * sm.uv.size());
	sm.msgHeader.byteSize += slotSize - ((sizeof(XMFLOAT3) * 2 * sm.pos.size()) + sm.msgHeaderSize + sizeof(int) + (sizeof(XMFLOAT2) * sm.uv.size())) % slotSize;
	unsigned int lastFreeMem = 0;
	do
	{
		if (sm.cb->freeMem >= sm.msgHeader.byteSize)
		{
			// Sets head to 0 if there are no place to write
			if (sm.cb->head == sm.memSize)
				sm.cb->head = 0;
			else
			{
				// The place over will set a number to indicate that the head was moved to 0
				unsigned int type = TAmount + 1;
				memcpy((char*)sm.buffer + sm.cb->head, &type, sizeof(int));
				lastFreeMem = sm.memSize - sm.cb->head;
				sm.cb->head = 0;
			}
			localHead = sm.cb->head;

			// Message header
			memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
			localHead += sm.msgHeaderSize;

			// Mesh index
			memcpy((char*)sm.buffer + localHead, &meshIndex, sizeof(int));
			localHead += sizeof(int);

			// Vertex data
			memcpy((char*)sm.buffer + localHead, sm.pos.data(), sizeof(XMFLOAT3) * sm.pos.size());
			localHead += sizeof(XMFLOAT3) * sm.pos.size();

			// UV data
			memcpy((char*)sm.buffer + localHead, sm.uv.data(), sizeof(XMFLOAT2) * sm.uv.size());
			localHead += sizeof(XMFLOAT2) * sm.uv.size();

			// Normal data
			memcpy((char*)sm.buffer + localHead, sm.normal.data(), sizeof(XMFLOAT3) * sm.normal.size());
			localHead += sizeof(XMFLOAT3) * sm.normal.size();

			// Move header
			sm.cb->freeMem -= (sm.msgHeader.byteSize + lastFreeMem);
			sm.cb->head += sm.msgHeader.byteSize;
			break;
		}
	} while (sm.cb->freeMem >! sm.msgHeader.byteSize);
}

void NormalChanged(MPlug& plug)
{
	MFnMesh mesh(plug.node());

	MItMeshPolygon itPoly(mesh.object());
	MVector normal;
	MPointArray points;
	MIntArray vertexList;
	sm.normal.clear();

	// Normals are per face
	while (!itPoly.isDone())
	{
		itPoly.getTriangles(points, vertexList);

		for (size_t i = 0; i < points.length(); i++)
		{
			//itPoly.getNormal(vertexList[i], normal);
			itPoly.getNormal(normal);
			sm.normal.push_back(XMFLOAT3(normal.x, normal.y, normal.z));
		}
		itPoly.next();
	}

	unsigned int meshIndex = meshNames.length();
	for (size_t i = 0; i < meshNames.length(); i++)
	{
		if (meshNames[i] == mesh.name())
			meshIndex = i;
	}

	// Send data to shared memory
	sm.msgHeader.type = TNormalUpdate;
	sm.msgHeader.byteSize = (sizeof(XMFLOAT3) * sm.normal.size()) + sm.msgHeaderSize + sizeof(int);
	sm.msgHeader.byteSize += slotSize - ((sizeof(XMFLOAT3) * sm.normal.size()) + sm.msgHeaderSize + sizeof(int)) % slotSize;
	unsigned int lastFreeMem = 0;
	do
	{
		if (sm.cb->freeMem >= sm.msgHeader.byteSize)
		{
			// Sets head to 0 if there are no place to write
			if (sm.cb->head > sm.memSize - sm.msgHeader.byteSize)
			{
				// The place over will set a number to indicate that the head was moved to 0
				unsigned int type = TAmount + 1;
				memcpy((char*)sm.buffer + sm.cb->head, &type, sizeof(int));
				lastFreeMem = sm.memSize - sm.cb->head;
				sm.cb->head = 0;
			}
			localHead = sm.cb->head;

			// Message header
			memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
			localHead += sm.msgHeaderSize;

			// Mesh index
			memcpy((char*)sm.buffer + localHead, &meshIndex, sizeof(int));
			localHead += sizeof(int);

			// Normal data
			memcpy((char*)sm.buffer + localHead, sm.normal.data(), sizeof(XMFLOAT3)* sm.normal.size());
			localHead += sizeof(XMFLOAT3)* sm.normal.size();

			// Move header
			sm.cb->freeMem -= (sm.msgHeader.byteSize + lastFreeMem);
			sm.cb->head += sm.msgHeader.byteSize;
			break;
		}
	} while (sm.cb->freeMem >! sm.msgHeader.byteSize);
}

void UVChanged(MPlug& plug)
{
	MFnMesh mesh(plug.node());

	MItMeshPolygon itPoly(mesh.object());

	float2 uv;
	MPointArray points;
	MIntArray vertexList;
	sm.uv.clear();

	// Normals are per face
	while (!itPoly.isDone())
	{
		itPoly.getTriangles(points, vertexList);

		for (size_t i = 0; i < points.length(); i++)
		{
			itPoly.getUVAtPoint(points[i], uv);
			sm.uv.push_back(XMFLOAT2(uv[0], 1 - uv[1]));
		}
		itPoly.next();
	}

	unsigned int meshIndex = meshNames.length();
	for (size_t i = 0; i < meshNames.length(); i++)
	{
		if (meshNames[i] == mesh.name())
			meshIndex = i;
	}

	// Send data to shared memory
	sm.msgHeader.type = TUVUpdate;
	sm.msgHeader.byteSize = (sizeof(XMFLOAT2)* sm.uv.size()) + sm.msgHeaderSize + sizeof(int);
	sm.msgHeader.byteSize += slotSize - ((sizeof(XMFLOAT2)* sm.uv.size()) + sm.msgHeaderSize + sizeof(int)) % slotSize;
	unsigned int lastFreeMem = 0;
	do
	{
		if (sm.cb->freeMem >= sm.msgHeader.byteSize)
		{
			// Sets head to 0 if there are no place to write
			if (sm.cb->head > sm.memSize - sm.msgHeader.byteSize)
			{
				// The place over will set a number to indicate that the head was moved to 0
				unsigned int type = TAmount + 1;
				memcpy((char*)sm.buffer + sm.cb->head, &type, sizeof(int));
				lastFreeMem = sm.memSize - sm.cb->head;
				sm.cb->head = 0;
			}

			localHead = sm.cb->head;

			// Message header
			memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
			localHead += sm.msgHeaderSize;

			// Mesh index
			memcpy((char*)sm.buffer + localHead, &meshIndex, sizeof(int));
			localHead += sizeof(int);

			// UV data
			memcpy((char*)sm.buffer + localHead, sm.uv.data(), sizeof(XMFLOAT2)* sm.uv.size());
			localHead += sizeof(XMFLOAT2)* sm.uv.size();

			// Move header
			sm.cb->freeMem -= (sm.msgHeader.byteSize + lastFreeMem);
			sm.cb->head += sm.msgHeader.byteSize;
			break;
		}
	} while (sm.cb->freeMem >! sm.msgHeader.byteSize);
}


void MaterialChanged(MFnMesh& mesh)
{
	// MATERIAL:
	unsigned int instanceNumber = 0;
	MObjectArray shaders;
	MPlugArray test;
	MIntArray indices;
	MPlugArray connections;
	MColor color;
	int whatMaterial = 0;
	MFnLambertShader lambertShader;
	MFnBlinnShader blinnShader;
	MFnPhongShader phongShader;

	int localMesh = meshNames.length();
	for (int i = 0; i < meshNames.length(); i++)
	{
		if (mesh.name() == meshNames[i])
		{
			localMesh = i;
		}
	}

	// Find the shadingReasourceGroup
	mesh.getConnectedShaders(instanceNumber, shaders, indices);
	if (shaders.length() != 0)
	{
		MFnDependencyNode shaderGroup(shaders[0]);
		MGlobal::displayInfo(shaderGroup.name());
		MPlug shaderPlug = shaderGroup.findPlug("surfaceShader");
		MGlobal::displayInfo(shaderPlug.name());
		shaderPlug.connectedTo(connections, true, false);

		// Find the material and then color
		if (connections[0].node().hasFn(MFn::kLambert))
		{
			lambertShader.setObject(connections[0].node());
			MGlobal::displayInfo(lambertShader.name());
			color = lambertShader.color();
			whatMaterial = 1;
			meshMatNames[localMesh] = lambertShader.name();
		}
		else if (connections[0].node().hasFn(MFn::kBlinn))
		{
			blinnShader.setObject(connections[0].node());
			MGlobal::displayInfo(blinnShader.name());
			color = blinnShader.color();
			whatMaterial = 2;
			meshMatNames[localMesh] = blinnShader.name();
		}
		else if (connections[0].node().hasFn(MFn::kPhong))
		{
			phongShader.setObject(connections[0].node());
			MGlobal::displayInfo(phongShader.name());
			color = phongShader.color();
			whatMaterial = 3;
			meshMatNames[localMesh] = phongShader.name();
		}

		//Texture:
		MObjectArray Files;
		MString filename;
		int pathSize;
		int texExist = 0;
		MPlugArray textureConnect;
		MPlug texturePlug;

		if (whatMaterial == 1)
		{
			texturePlug = lambertShader.findPlug("color");
		}
		else if (whatMaterial == 2)
		{
			texturePlug = blinnShader.findPlug("color");
		}
		else if (whatMaterial == 3)
		{
			texturePlug = phongShader.findPlug("color");
		}

		MGlobal::displayInfo(texturePlug.name());
		texturePlug.connectedTo(textureConnect, true, false);

		if (textureConnect.length() != 0)
		{
			MGlobal::displayInfo(textureConnect[0].name());

			MFnDependencyNode fn(textureConnect[0].node());
			MGlobal::displayInfo(fn.name());

			MPlug ftn = fn.findPlug("ftn");

			ftn.getValue(filename);

			MGlobal::displayInfo(filename);

			pathSize = filename.numChars();

			texExist = 1;
		}

		// Send data to shared memory
		do
		{
			if (sm.cb->freeMem >= slotSize/* && sm.cb->head < sm.memSize - sm.cb->freeMem*/)
			{
				// Sets head to 0 if there are no place to write
				if (sm.cb->head == sm.memSize)
					sm.cb->head = 0;

				localHead = sm.cb->head;

				// Message header
				sm.msgHeader.type = TMaterialUpdate;
				if (texExist == 0)
				{
					sm.msgHeader.byteSize = sm.msgHeaderSize + sizeof(MColor) + sizeof(int) + sizeof(int);
					sm.msgHeader.byteSize += slotSize - sm.msgHeader.byteSize;
				}
				else
				{
					sm.msgHeader.byteSize = sm.msgHeaderSize + sizeof(MColor) + sizeof(int) + sizeof(int) + pathSize + sizeof(int);
					sm.msgHeader.byteSize += slotSize - sm.msgHeader.byteSize;
				}
				memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
				localHead += sm.msgHeaderSize;

				// Mesh ID
				memcpy((char*)sm.buffer + localHead, &localMesh, sizeof(int));
				localHead += sizeof(int);

				// Material data
				memcpy((char*)sm.buffer + localHead, &color, sizeof(MColor));
				localHead += sizeof(MColor);

				//Bool for Texture
				XMINT4 ha = XMINT4(texExist, 0, 0, 0);
				memcpy((char*)sm.buffer + localHead, &ha.x, sizeof(int));
				localHead += sizeof(int);

				if (texExist == 1)
				{
					//Texture Size
					memcpy((char*)sm.buffer + localHead, &pathSize, sizeof(int));
					localHead += sizeof(int);

					//Texture data
					memcpy((char*)sm.buffer + localHead, filename.asChar(), pathSize);
					localHead += pathSize;
				}

				// Move header
				sm.cb->freeMem -= slotSize;
				sm.cb->head += slotSize;
				break;
			}
		} while (sm.cb->freeMem >! slotSize);
	}
}

void ShaderChangedCB(MObject& object, void* clientData)
{
	MFnDependencyNode Shadername(object);
	callbackIds.append(MNodeMessage::addAttributeChangedCallback(object, shaderchanged));
}

void shaderchanged(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData)
{
	if ((msg & MNodeMessage::kAttributeSet) || msg & MNodeMessage::kConnectionMade)
	{
		//meshNames.append(mesh.name());
		MFnLambertShader lambertShader;
		MFnBlinnShader blinnShader;
		MFnPhongShader phongShader;
		MColor color;
		int whatMaterial = 0;

		// Find the material and then color
		if (plug.node().hasFn(MFn::kLambert))
		{
			lambertShader.setObject(plug.node());
			MGlobal::displayInfo(lambertShader.name());
			color = lambertShader.color();
			whatMaterial = 1;
		}
		else if (plug.node().hasFn(MFn::kBlinn))
		{
			blinnShader.setObject(plug.node());
			MGlobal::displayInfo(blinnShader.name());
			color = blinnShader.color();
			whatMaterial = 2;
		}
		else if (plug.node().hasFn(MFn::kPhong))
		{
			phongShader.setObject(plug.node());
			MGlobal::displayInfo(phongShader.name());
			color = phongShader.color();
			whatMaterial = 3;
		}

		//Texture:
		MFnDependencyNode matNamer(plug.node());
		MObjectArray Files;
		MString filename;
		int pathSize;
		int texExist = 0;
		MPlugArray textureConnect;
		MPlug texturePlug;

		if (whatMaterial == 1)
		{
			texturePlug = lambertShader.findPlug("color");
		}
		else if (whatMaterial == 2)
		{
			texturePlug = blinnShader.findPlug("color");
		}
		else if (whatMaterial == 3)
		{
			texturePlug = phongShader.findPlug("color");
		}

		MGlobal::displayInfo(texturePlug.name());
		texturePlug.connectedTo(textureConnect, true, false);

		if (textureConnect.length() != 0)
		{
			MGlobal::displayInfo(textureConnect[0].name());

			MFnDependencyNode fn(textureConnect[0].node());
			MGlobal::displayInfo(fn.name());

			MPlug ftn = fn.findPlug("ftn");

			ftn.getValue(filename);

			MGlobal::displayInfo(filename);

			pathSize = filename.numChars();

			texExist = 1;
		}
		for (int i = 0; i < meshMatNames.length(); i++)
		{
			if (meshMatNames[i] == matNamer.name())
			{
				// Send data to shared memory
				do
				{
					if (sm.cb->freeMem >= slotSize)
					{
						// Sets head to 0 if there are no place to write
						if (sm.cb->head == sm.memSize)
							sm.cb->head = 0;

						localHead = sm.cb->head;

						// Message header
						sm.msgHeader.type = TMaterialUpdate;
						if (texExist == 0)
						{
							sm.msgHeader.byteSize = sm.msgHeaderSize + sizeof(MColor) + sizeof(int) + sizeof(int);
							sm.msgHeader.byteSize += slotSize - sm.msgHeader.byteSize;
						}
						else
						{
							sm.msgHeader.byteSize = sm.msgHeaderSize + sizeof(MColor) + sizeof(int) + sizeof(int) + pathSize + sizeof(int);
							sm.msgHeader.byteSize += slotSize - sm.msgHeader.byteSize;
						}
						memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
						localHead += sm.msgHeaderSize;

						// Mesh ID
						memcpy((char*)sm.buffer + localHead, &i, sizeof(int));
						localHead += sizeof(int);

						// Material data
						memcpy((char*)sm.buffer + localHead, &color, sizeof(MColor));
						localHead += sizeof(MColor);

						//Bool for Texture
						XMINT4 ha = XMINT4(texExist, 0, 0, 0);
						memcpy((char*)sm.buffer + localHead, &ha.x, sizeof(int));
						localHead += sizeof(int);

						if (texExist == 1)
						{
							//Texture Size
							memcpy((char*)sm.buffer + localHead, &pathSize, sizeof(int));
							localHead += sizeof(int);

							//Texture data
							memcpy((char*)sm.buffer + localHead, filename.asChar(), pathSize);
							localHead += pathSize;
						}

						// Move header
						sm.cb->freeMem -= slotSize;
						sm.cb->head += slotSize;
						break;
					}
				} while (sm.cb->freeMem >! slotSize);
			}
		}
	}
}


void TransformCreationCB(MObject& object, void* clientData)
{
	// Add a callback specifik to every new transform that are created
	callbackIds.append(MNodeMessage::addAttributeChangedCallback(object, TransformChangedCB));
	callbackIds.append(MNodeMessage::addNameChangedCallback(object, NameChangedCB));
}

void TransformChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData)
{
	M3dView view = M3dView::active3dView();
	MDagPath camPath;
	view.getCamera(camPath);
	MFnCamera camera(camPath);

	if (msg & MNodeMessage::kAttributeSet)
	{
		MFnTransform transform(plug.node());

		// Checks for Camera else normal transforms
		if (transform.isParentOf(camera.object()))
			CameraChanged(transform, camera);
		else
		{
			unsigned int localMesh = INT_MAX;

			for (size_t i = 0; i < meshNames.length(); i++)
			{
				MFnMesh mesh(transform.child(0));
				if (meshNames[i] == mesh.name())
				{
					localMesh = i;
				}
			}

			MMatrix transMatrix = transform.transformationMatrix().transpose();
			float pm[4][4];
			transMatrix.get(pm);
			XMFLOAT4X4 matrixData(
				pm[0][0], pm[0][1], pm[0][2], pm[0][3],
				pm[1][0], pm[1][1], pm[1][2], pm[1][3],
				pm[2][0], pm[2][1], pm[2][2], pm[2][3],
				pm[3][0], pm[3][1], pm[3][2], pm[3][3]);

			do
			{
				if (sm.cb->freeMem > slotSize)
				{
					// Sets head to 0 if there are no place to write
					if (sm.cb->head == sm.memSize)
						sm.cb->head = 0;

					localHead = sm.cb->head;
					// Message header
					sm.msgHeader.type = TtransformUpdate;
					sm.msgHeader.byteSize = sm.msgHeaderSize + sizeof(int) + sizeof(XMFLOAT4X4);
					sm.msgHeader.byteSize += slotSize - sm.msgHeader.byteSize;

					// Message header
					memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
					localHead += sm.msgHeaderSize;

					// Mesh index
					memcpy((char*)sm.buffer + localHead, &localMesh, sizeof(int));
					localHead += sizeof(int);

					// Matrix data
					memcpy((char*)sm.buffer + localHead, &matrixData, sizeof(XMFLOAT4X4));
					localHead += sizeof(XMFLOAT4X4);

					// Move header
					sm.cb->freeMem -= slotSize;
					sm.cb->head += slotSize;

					break;
				}
			}while (sm.cb->freeMem >! slotSize);
		}
	}
}


void GetCameraInformation()
{
	M3dView view = M3dView::active3dView();
	MDagPath camPath;
	view.getCamera(camPath);
	MFnCamera camera(camPath);

	// Projection matrix
	MFloatMatrix projectionMatrix = camera.projectionMatrix();
	projectionMatrix[2][2] = -projectionMatrix[2][2];
	projectionMatrix[3][2] = -projectionMatrix[3][2];

	// View matrix
	MFnTransform transform(camera.parent(0));
	MMatrix transMatrix = transform.transformationMatrix().transpose().inverse();
	float pm[4][4];
	transMatrix.get(pm);
	XMFLOAT4X4 viewMatrix(
		pm[0][0], pm[0][1], pm[0][2], pm[0][3],
		pm[1][0], pm[1][1], pm[1][2], pm[1][3],
		pm[2][0], pm[2][1], pm[2][2], pm[2][3],
		pm[3][0], pm[3][1], pm[3][2], pm[3][3]);

	do
	{
		if (sm.cb->freeMem > slotSize)
		{
			// Sets head to 0 if there are no place to write
			if (sm.cb->head == sm.memSize)
				sm.cb->head = 0;

			// Send data to shared memory
			localHead = sm.cb->head;
			// Message header
			sm.msgHeader.type = TCameraUpdate;
			sm.msgHeader.byteSize = sizeof(XMFLOAT4X4) * 2 + sm.msgHeaderSize;
			sm.msgHeader.byteSize += slotSize - sm.msgHeader.byteSize;
			memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
			localHead += sm.msgHeaderSize;

			// View matrix
			memcpy((char*)sm.buffer + localHead, &viewMatrix, sizeof(XMFLOAT4X4));
			localHead += sizeof(XMFLOAT4X4);

			// Projection matrix
			memcpy((char*)sm.buffer + localHead, &projectionMatrix.transpose(), sizeof(MFloatMatrix));
			localHead += sizeof(XMFLOAT4X4);

			// Move header
			sm.cb->head += slotSize;
			sm.cb->freeMem -= slotSize;

			break;
		}
	} while (sm.cb->freeMem >!slotSize);
}

void CameraChanged(MFnTransform& transform, MFnCamera& camera)
{
	// Projection matrix
	MFloatMatrix projectionMatrix = camera.projectionMatrix();
	projectionMatrix[2][2] = -projectionMatrix[2][2];
	projectionMatrix[3][2] = -projectionMatrix[3][2];

	// View matrix
	float pm[4][4];
	MMatrix transMatrix = transform.transformationMatrix().transpose().inverse();
	transMatrix.get(pm);
	XMFLOAT4X4 viewMatrix(
		pm[0][0], pm[0][1], pm[0][2], pm[0][3],
		pm[1][0], pm[1][1], pm[1][2], pm[1][3],
		pm[2][0], pm[2][1], pm[2][2], pm[2][3],
		pm[3][0], pm[3][1], pm[3][2], pm[3][3]);

	MGlobal::displayInfo(MString("KOLLA: ") + transform.transformationMatrix()[3][1]);

	do
	{
		if (sm.cb->freeMem > slotSize)
		{
			// Sets head to 0 if there are no place to write
			if (sm.cb->head == sm.memSize)
				sm.cb->head = 0;

			// Send data to shared memory
			localHead = sm.cb->head;
			// Message header
			sm.msgHeader.type = TCameraUpdate;
			sm.msgHeader.byteSize = sizeof(XMFLOAT4X4) * 2 + sm.msgHeaderSize;
			sm.msgHeader.byteSize += slotSize - sm.msgHeader.byteSize;
			memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
			localHead += sm.msgHeaderSize;

			// View matrix
			memcpy((char*)sm.buffer + localHead, &viewMatrix, sizeof(XMFLOAT4X4));
			localHead += sizeof(XMFLOAT4X4);

			// Projection matrix
			memcpy((char*)sm.buffer + localHead, &projectionMatrix.transpose(), sizeof(MFloatMatrix));
			localHead += sizeof(XMFLOAT4X4);

			// Move header
			sm.cb->head += slotSize;
			sm.cb->freeMem -= slotSize;

			break;
		}
	} while (sm.cb->freeMem > !slotSize);
}


void GetLightInformation(MFnLight& light)
{
	MObject dad = light.parent(0);
	MColor color;
	MVector position;

	MStatus status;
	if (dad.hasFn(MFn::kTransform))
	{
		MFnTransform lightPoint(dad);
		position = lightPoint.getTranslation(MSpace::kObject, &status);
	}
	color = light.color();

	MGlobal::displayInfo(MString() + "Color: " + color.r + " " + color.g + " " + color.b + " " + color.a);
	MGlobal::displayInfo(MString() + "Position: " + position.x + " " + position.y + " " + position.z);
	XMFLOAT3 positionF(position.x, position.y, position.z);

	do
	{
		if (sm.cb->freeMem > slotSize)
		{
			// Sets head to 0 if there are no place to write
			if (sm.cb->head == sm.memSize)
				sm.cb->head = 0;

			localHead = sm.cb->head;
			// Message header
			sm.msgHeader.type = TLightCreate;
			sm.msgHeader.byteSize = sm.msgHeaderSize + sizeof(MColor) + sizeof(XMFLOAT3);
			sm.msgHeader.byteSize += slotSize - sm.msgHeader.byteSize;
			memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
			localHead += sm.msgHeaderSize;

			// Light data
			memcpy((char*)sm.buffer + localHead, &positionF, sizeof(XMFLOAT3));
			localHead += sizeof(XMFLOAT3);
			memcpy((char*)sm.buffer + localHead, &color, sizeof(MColor));
			localHead += sizeof(MColor);

			// Move header
			sm.cb->freeMem -= slotSize;
			sm.cb->head += slotSize;

			break;
		}
	} while (sm.cb->freeMem >! slotSize);
}

void LightCreationCB(MObject& lightObject, void* clientData)
{
	MFnLight light(lightObject);

	callbackIds.append(MNodeMessage::addAttributeChangedCallback(lightObject, LightChangedCB));
	MObject dad = light.parent(0);
	callbackIds.append(MNodeMessage::addAttributeChangedCallback(dad, LightChangedCB));

	GetLightInformation(light);
}

void LightChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData)
{
	if (msg & MNodeMessage::kAttributeSet)
	{
		MFnLight light;
		MFnTransform transform;
		XMFLOAT3 positionF;
		MColor color;

		if (plug.partialName() == "cl")
		{
			light.setObject(plug.node());
			transform.setObject(light.parent(0));

			MVector position;
			position = transform.getTranslation(MSpace::kObject);

			positionF = XMFLOAT3(position.x, position.y, position.z);
			color = light.color();
		}
		else if (strstr(plug.partialName().asChar(), "t") ||
			strstr(plug.partialName().asChar(), "r") ||
			strstr(plug.partialName().asChar(), "s"))
		{
			transform.setObject(plug.node());
			light.setObject(transform.child(0));

			MVector position;
			position = transform.getTranslation(MSpace::kObject);

			positionF = XMFLOAT3(position.x, position.y, position.z);
			color = light.color();
		}

		MGlobal::displayInfo(MString() + "Position: " + positionF.x + " " + positionF.y + " " + positionF.z);
		MGlobal::displayInfo(MString() + "Color: " + color.r + " " + color.g + " " + color.b + " " + color.a);

		do
		{
			if (sm.cb->freeMem > slotSize)
			{
				// Sets head to 0 if there are no place to write
				if (sm.cb->head == sm.memSize)
					sm.cb->head = 0;

				localHead = sm.cb->head;
				// Message header
				unsigned int localLight = 0;
				sm.msgHeader.type = TLightUpdate;
				sm.msgHeader.byteSize = sm.msgHeaderSize + sizeof(MColor) + sizeof(XMFLOAT3);
				sm.msgHeader.byteSize += slotSize - sm.msgHeader.byteSize;
				memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
				localHead += sm.msgHeaderSize;

				// Light data
				memcpy((char*)sm.buffer + localHead, &positionF, sizeof(XMFLOAT3));
				localHead += sizeof(XMFLOAT3);
				memcpy((char*)sm.buffer + localHead, &color, sizeof(MColor));
				localHead += sizeof(MColor);

				// Move header
				sm.cb->freeMem -= slotSize;
				sm.cb->head += slotSize;

				break;
			}
		} while (sm.cb->freeMem >! slotSize);
	}
}


void NameChangedCB(MObject& node, const MString& prevName, void* clientData)
{
	if (node.hasFn(MFn::kMesh))
	{
		MFnMesh mesh(node);
		MGlobal::displayInfo("Name changed: " + prevName + " -> " + mesh.fullPathName());
	}
	else if (node.hasFn(MFn::kTransform))
	{
		MFnTransform transform(node);
		MGlobal::displayInfo("Name changed: " + prevName + " -> " + transform.fullPathName());
	}
}

void TimerCB(float elapsedTime, float lastTime, void* clientData)
{
	totalTime += elapsedTime;
	MGlobal::displayInfo(MString() + totalTime);
}


void NodeDestroyedCB(MObject& object, MDGModifier& modifier, void* clientData)
{
	MFnMesh mesh(object);

	unsigned int meshIndex = meshNames.length();
	for (size_t i = 0; i < meshNames.length(); i++)
	{
		if (meshNames[i] == mesh.name())
		{
			meshIndex = i;
		}
	}

	do
	{
		if (sm.cb->freeMem > slotSize)
		{
			// Sets head to 0 if there are no place to write
			if (sm.cb->head == sm.memSize)
				sm.cb->head = 0;
			localHead = sm.cb->head;

			// Message header
			sm.msgHeader.type = TMeshDestroyed;
			sm.msgHeader.byteSize = sm.msgHeaderSize - sizeof(int);
			sm.msgHeader.byteSize += slotSize - sm.msgHeader.byteSize;
			memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
			localHead += sm.msgHeaderSize;

			// Mesh index
			memcpy((char*)sm.buffer + localHead, &meshIndex, sizeof(int));
			localHead += sizeof(int);

			// Move header
			sm.cb->freeMem -= slotSize;
			sm.cb->head += slotSize;

			break;
		}
	} while (sm.cb->freeMem >! slotSize);
}