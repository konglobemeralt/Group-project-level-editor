#include "MayaHeaders.h"
#include <iostream>
#include "SharedMemory.h"

void GetSceneData();

void GetMeshInformation(MFnMesh& mesh);
void MeshCreationCB(MObject& node, void* clientData);
void MeshChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);

void TransformCreationCB(MObject& object, void* clientData);
void TransformChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);

void CameraCreationCB(MObject& object, void* clientData);
void CameraChanged(MFnTransform& transform, MFnCamera& camera);

void LightCreationCB(MObject& lightObject, void* clientData);
void LightChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);

void NameChangedCB(MObject& node, const MString& prevName, void* clientData);
void TimerCB(float elapsedTime, float lastTime, void* clientData);

void NodeDestroyedCB(MObject& object, MDGModifier& modifier, void* clientData);



// Timer callback
float period = 5.0f;
int totalTime = 0.0f;

MCallbackIdArray callbackIds;

MVector camFix;

SharedMemory sm;
unsigned int localHead;
unsigned int slotSize;
MStringArray meshNames;

EXPORT MStatus initializePlugin(MObject obj)
{
	MStatus res = MS::kSuccess;

	MFnPlugin myPlugin(obj, "Maya plugin", "1.0", "any", &res);
	if (MFAIL(res))
		CHECK_MSTATUS(res);

	sm.camDataSize = sizeof(MVector) * 3;
	localHead = 0;
	slotSize = 256;
	sm.cbSize = 20;
	sm.msgHeaderSize = 8;

	MGlobal::displayInfo(sm.OpenMemory(100));

	GetSceneData();

	callbackIds.append(MDGMessage::addNodeAddedCallback(MeshCreationCB, "mesh", &res));
	callbackIds.append(MDGMessage::addNodeAddedCallback(TransformCreationCB, "transform", &res));
	callbackIds.append(MDGMessage::addNodeAddedCallback(LightCreationCB, "light", &res));
	callbackIds.append(MTimerMessage::addTimerCallback(period, TimerCB, &res));

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

	MItDag itCamera(MItDag::kDepthFirst, MFn::kCamera);
	while (!itCamera.isDone())
	{
		MFnCamera camera(itCamera.item());
		//GetCameraInformation(camera);
		callbackIds.append(MNodeMessage::addNameChangedCallback(camera.object(), NameChangedCB));
		itCamera.next();
	}
}


void GetMeshInformation(MFnMesh& mesh)
{
	MGlobal::displayInfo("Mesh: " + mesh.fullPathName() + " loaded!");
	meshNames.append(mesh.name());

	// Vertex position
	MIntArray vtxCount;
	MIntArray vtxArray;
	mesh.getVertices(vtxCount, vtxArray);

	MIntArray vertexIDList;
	MVectorArray vertexList;
	size_t id = 0;
	int vertexPoint[3];
	MIntArray triangleCounts;
	MIntArray triangleVertices;
	MPoint point;
	float2 uvPoint;
	MFloatArray uList;
	MFloatArray vList;
	MVector normal;
	MVectorArray normalList;
	mesh.getTriangles(triangleCounts, triangleVertices);

	for (size_t i = 0; i < mesh.numPolygons(); i++)
	{
		for (size_t j = 0; j < triangleCounts[i]; j++)
		{
			mesh.getPolygonTriangleVertices(i, j, vertexPoint);
			vertexIDList.append(vertexPoint[0]);
			vertexIDList.append(vertexPoint[1]);
			vertexIDList.append(vertexPoint[2]);

			// Vertex 0 in triangle:
			mesh.getPoint(vertexPoint[0], point);
			vertexList.append(MVector());
			vertexList[id].x = point.x;
			vertexList[id].y = point.y;
			vertexList[id].z = point.z;

			// UV 0 in triangle:
			mesh.getUVAtPoint(point, uvPoint);
			uList.append(uvPoint[0]);
			vList.append(1 - uvPoint[1]);

			// Vertex 1 in triangle:
			mesh.getPoint(vertexPoint[1], point);
			vertexList.append(MVector());
			vertexList[id + 1].x = point.x;
			vertexList[id + 1].y = point.y;
			vertexList[id + 1].z = point.z;

			// UV 1 in triangle:
			mesh.getUVAtPoint(point, uvPoint);
			uList.append(uvPoint[0]);
			vList.append(1 - uvPoint[1]);

			// Vertex 2 in triangle:
			mesh.getPoint(vertexPoint[2], point);
			vertexList.append(MVector());
			vertexList[id + 2].x = point.x;
			vertexList[id + 2].y = point.y;
			vertexList[id + 2].z = point.z;

			// UV 2 in triangle:
			mesh.getUVAtPoint(point, uvPoint);
			uList.append(uvPoint[0]);
			vList.append(1 - uvPoint[1]);

			// Normal:
			mesh.getVertexNormal(vertexIDList[0], true, normal);
			normalList.append(normal);
			mesh.getVertexNormal(vertexIDList[1], true, normal);
			normalList.append(normal);
			mesh.getVertexNormal(vertexIDList[2], true, normal);
			normalList.append(normal);

			id += 3;
		}
	}

	sm.vertexData.resize(vertexList.length());
	if (vertexIDList.length() > 0)
	{
		for (size_t i = 0; i < vertexList.length(); i++)
		{
			MGlobal::displayInfo(MString("V: ") + vertexIDList[i] + ": " + vertexList[i].x + " " + vertexList[i].y + " " + vertexList[i].z + " " + i);
			MGlobal::displayInfo(MString("UV ") + i + ": " + uList[i] + " " + vList[i]);
			MGlobal::displayInfo(MString("N ") + i + ": " + normalList[i].x + " " + normalList[i].y + " " + normalList[i].z);
			sm.vertexData[i].pos = XMFLOAT3(vertexList[i].x, vertexList[i].y, vertexList[i].z);
			sm.vertexData[i].uv = XMFLOAT2(uList[i], vList[i]);
			sm.vertexData[i].pos = XMFLOAT3(vertexList[i].x, vertexList[i].y, vertexList[i].z);
			sm.vertexData[i].normal = XMFLOAT3(normalList[i].x, normalList[i].y, normalList[i].z);
		}

		MFnTransform transform(mesh.parent(0));
		MVector translation = transform.getTranslation(MSpace::kObject);
		double rotation[4];
		transform.getRotationQuaternion(rotation[0], rotation[1], rotation[2], rotation[3]);
		double scale[3];
		transform.getScale(scale);

		XMVECTOR translationV = XMVectorSet(translation.x, translation.y, translation.z, 0.0f);
		XMVECTOR rotationV = XMVectorSet(rotation[0], rotation[1], rotation[2], rotation[3]);
		XMVECTOR scaleV = XMVectorSet(scale[0], scale[1], scale[2], 0.0f);
		XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
		XMFLOAT4X4 matrixData;
		XMStoreFloat4x4(&matrixData, XMMatrixTranspose(XMMatrixAffineTransformation(scaleV, zero, rotationV, translationV)));

		// MATERIAL:
		unsigned int instanceNumber = 0;
		MObjectArray shaders;
		MIntArray indices;
		MPlugArray connections;
		MPlugArray plugs;
		MColor color;

		// Find the shadingReasourceGroup
		mesh.getConnectedShaders(instanceNumber, shaders, indices);
		MFnDependencyNode shaderGroup(shaders[0]);
		MGlobal::displayInfo(shaderGroup.name());
		MPlug shaderPlug = shaderGroup.findPlug("surfaceShader");
		MGlobal::displayInfo(shaderPlug.name());
		shaderPlug.connectedTo(connections, true, false);

		// Find the material and then color
		if (connections[0].node().hasFn(MFn::kLambert))
		{
			MFnLambertShader lambertShader(connections[0].node());
			MGlobal::displayInfo(lambertShader.name());
			color = lambertShader.color();
		}
		else if (connections[0].node().hasFn(MFn::kBlinn))
		{
			MFnBlinnShader blinnShader(connections[0].node());
			MGlobal::displayInfo(blinnShader.name());
			color = blinnShader.color();
		}
		else if (connections[0].node().hasFn(MFn::kPhong))
		{
			MFnPhongShader phongShader(connections[0].node());
			MGlobal::displayInfo(phongShader.name());
			color = phongShader.color();
		}

		// Send data to shared memory
		unsigned int meshSize = vertexList.length();
		if (meshSize > 0)
		{
			sm.msgHeader.type = TMeshCreate;
			sm.msgHeader.padding = ((meshSize * sizeof(float) * 8) - sm.msgHeaderSize - sizeof(XMFLOAT4X4) - sizeof(int) - sizeof(MColor)) % slotSize;
			//do
			//{
			//	if (sm.cb->freeMem > ((meshSize * sizeof(float) * 8) - sm.msgHeaderSize - sizeof(XMFLOAT4X4) - sizeof(int)) + sm.msgHeader.padding)
			//	{
			localHead = sm.cb->head;
			// Message header
			memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
			localHead += sm.msgHeaderSize;

			// Size of mesh
			memcpy((char*)sm.buffer + localHead, &meshSize, sizeof(int));
			localHead += sizeof(int);
			// Vertex data
			for (size_t i = 0; i < vertexList.length(); i++)
			{
				memcpy((char*)sm.buffer + localHead, &sm.vertexData[i].pos, sizeof(XMFLOAT3));
				localHead += sizeof(XMFLOAT3);
				memcpy((char*)sm.buffer + localHead, &sm.vertexData[i].uv, sizeof(XMFLOAT2));
				localHead += sizeof(XMFLOAT2);
				memcpy((char*)sm.buffer + localHead, &sm.vertexData[i].normal, sizeof(XMFLOAT3));
				localHead += sizeof(XMFLOAT3);
			}

			// Matrix data
			memcpy((char*)sm.buffer + localHead, &matrixData, sizeof(XMFLOAT4X4));
			localHead += sizeof(XMFLOAT4X4);

			// Material data
			memcpy((char*)sm.buffer + localHead, &color, sizeof(MColor));
			localHead += sizeof(MColor);

			// Move header
			sm.cb->freeMem -= (localHead - sm.cb->head) + sm.msgHeader.padding;
			sm.cb->head += (localHead - sm.cb->head) + sm.msgHeader.padding;

			//		break;
			//	}
			//}while (sm.cb->freeMem >! ((meshSize * sizeof(float) * 8) - sm.msgHeaderSize - sizeof(XMFLOAT4X4) - sizeof(int)) + sm.msgHeader.padding);
		}
	}
}

void MeshCreationCB(MObject& object, void* clientData)
{
	// Add a callback specifik to every new mesh that are created
	callbackIds.append(MNodeMessage::addAttributeChangedCallback(object, MeshChangedCB));
	callbackIds.append(MNodeMessage::addNameChangedCallback(object, NameChangedCB));
}

void MeshChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData)
{
	// check if the plug p_Plug has in its name "doubleSided", which is an attribute that when is set we know that the mesh is finally available.
	// Only used for the creation of the mesh
	if (strstr(plug.name().asChar(), "doubleSided") != NULL && MNodeMessage::AttributeMessage::kAttributeSet)
	{
		MStatus res;
		MFnMesh mesh(plug.node(), &res);
		MGlobal::displayInfo("Mesh: " + mesh.fullPathName() + " created!");

		// Vertex position
		MIntArray vtxCount;
		MIntArray vtxArray;
		mesh.getVertices(vtxCount, vtxArray);

		MIntArray vertexIDList;
		MVectorArray vertexList;
		size_t id = 0;
		int vertexPoint[3];
		MIntArray triangleCounts;
		MIntArray triangleVertices;
		MPoint point;
		float2 uvPoint;
		MFloatArray uList;
		MFloatArray vList;
		MVector normal;
		MVectorArray normalList;
		mesh.getTriangles(triangleCounts, triangleVertices);

		for (size_t i = 0; i < mesh.numPolygons(); i++)
		{
			for (size_t j = 0; j < triangleCounts[i]; j++)
			{
				mesh.getPolygonTriangleVertices(i, j, vertexPoint);
				vertexIDList.append(vertexPoint[0]);
				vertexIDList.append(vertexPoint[1]);
				vertexIDList.append(vertexPoint[2]);

				// Vertex 0 in triangle:
				mesh.getPoint(vertexPoint[0], point);
				vertexList.append(MVector());
				vertexList[id].x = point.x;
				vertexList[id].y = point.y;
				vertexList[id].z = point.z;

				// UV 0 in triangle:
				mesh.getUVAtPoint(point, uvPoint);
				uList.append(uvPoint[0]);
				vList.append(1 - uvPoint[1]);

				// Vertex 1 in triangle:
				mesh.getPoint(vertexPoint[1], point);
				vertexList.append(MVector());
				vertexList[id + 1].x = point.x;
				vertexList[id + 1].y = point.y;
				vertexList[id + 1].z = point.z;

				// UV 1 in triangle:
				mesh.getUVAtPoint(point, uvPoint);
				uList.append(uvPoint[0]);
				vList.append(1 - uvPoint[1]);

				// Vertex 2 in triangle:
				mesh.getPoint(vertexPoint[2], point);
				vertexList.append(MVector());
				vertexList[id + 2].x = point.x;
				vertexList[id + 2].y = point.y;
				vertexList[id + 2].z = point.z;

				// UV 2 in triangle:
				mesh.getUVAtPoint(point, uvPoint);
				uList.append(uvPoint[0]);
				vList.append(1 - uvPoint[1]);

				// Normal:
				mesh.getVertexNormal(vertexIDList[0], true, normal);
				normalList.append(normal);
				mesh.getVertexNormal(vertexIDList[1], true, normal);
				normalList.append(normal);
				mesh.getVertexNormal(vertexIDList[2], true, normal);
				normalList.append(normal);

				id += 3;
			}
		}

		sm.vertexData.resize(vertexList.length());
		if (vertexIDList.length() > 0)
		{
			for (size_t i = 0; i < vertexList.length(); i++)
			{
				MGlobal::displayInfo(MString("V: ") + vertexIDList[i] + ": " + vertexList[i].x + " " + vertexList[i].y + " " + vertexList[i].z + " " + i);
				MGlobal::displayInfo(MString("UV ") + i + ": " + uList[i] + " " + vList[i]);
				MGlobal::displayInfo(MString("N ") + i + ": " + normalList[i].x + " " + normalList[i].y + " " + normalList[i].z);
				sm.vertexData[i].pos = XMFLOAT3(vertexList[i].x, vertexList[i].y, vertexList[i].z);
				sm.vertexData[i].uv = XMFLOAT2(uList[i], vList[i]);
				sm.vertexData[i].pos = XMFLOAT3(vertexList[i].x, vertexList[i].y, vertexList[i].z);
				sm.vertexData[i].normal = XMFLOAT3(normalList[i].x, normalList[i].y, normalList[i].z);
			}

			MFnTransform transform(mesh.parent(0));
			MVector translation = transform.getTranslation(MSpace::kObject);
			double rotation[4];
			transform.getRotationQuaternion(rotation[0], rotation[1], rotation[2], rotation[3]);
			double scale[3];
			transform.getScale(scale);

			XMVECTOR translationV = XMVectorSet(translation.x, translation.y, translation.z, 0.0f);
			XMVECTOR rotationV = XMVectorSet(rotation[0], rotation[1], rotation[2], rotation[3]);
			XMVECTOR scaleV = XMVectorSet(scale[0], scale[1], scale[2], 0.0f);
			XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
			XMFLOAT4X4 matrixData;
			DirectX::XMStoreFloat4x4(&matrixData, XMMatrixTranspose(XMMatrixAffineTransformation(scaleV, zero, rotationV, translationV)));

			// MATERIAL
			unsigned int instanceNumber = 0;
			MObjectArray shaders;
			MIntArray indices;
			MPlugArray connections;
			MPlugArray plugs;
			MColor color;

			// Find the shadingReasourceGroup
			mesh.getConnectedShaders(instanceNumber, shaders, indices);
			MFnDependencyNode shaderGroup(shaders[0]);
			MGlobal::displayInfo(shaderGroup.name());
			MPlug shaderPlug = shaderGroup.findPlug("surfaceShader");
			MGlobal::displayInfo(shaderPlug.name());
			shaderPlug.connectedTo(connections, true, false);

			// Find the material and then color
			if (connections[0].node().hasFn(MFn::kLambert))
			{
				MFnLambertShader lambertShader(connections[0].node());
				MGlobal::displayInfo(lambertShader.name());
				color = lambertShader.color();
			}
			else if (connections[0].node().hasFn(MFn::kBlinn))
			{
				MFnBlinnShader blinnShader(connections[0].node());
				MGlobal::displayInfo(blinnShader.name());
				color = blinnShader.color();
			}
			else if (connections[0].node().hasFn(MFn::kPhong))
			{
				MFnPhongShader phongShader(connections[0].node());
				MGlobal::displayInfo(phongShader.name());
				color = phongShader.color();
			}

			// Send data to shared memory
			unsigned int meshSize = vertexList.length();
			if (meshSize > 0)
			{
				meshNames.append(mesh.name());
				sm.msgHeader.type = TMeshCreate;
				sm.msgHeader.padding = slotSize - ((meshSize * sizeof(float) * 8) + sm.msgHeaderSize + sizeof(XMFLOAT4X4) + sizeof(int) + sizeof(MColor) + sizeof(int)*meshSize) % slotSize;
				//do
				//{
				//	if (sm.cb->freeMem > ((meshSize * sizeof(float) * 8) - sm.msgHeaderSize - sizeof(XMFLOAT4X4) - sizeof(int)) + sm.msgHeader.padding)
				//	{
				localHead = sm.cb->head;
				// Message header
				memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
				localHead += sm.msgHeaderSize;

				// Size of mesh
				memcpy((char*)sm.buffer + localHead, &meshSize, sizeof(int));
				localHead += sizeof(int);

				// Vertex indices list
				memcpy((char*)sm.buffer + localHead, &vertexIDList, sizeof(int)*meshSize);
				localHead += sizeof(int)*meshSize;

				// Vertex data
				for (size_t i = 0; i < vertexList.length(); i++)
				{
					memcpy((char*)sm.buffer + localHead, &sm.vertexData[i].pos, sizeof(XMFLOAT3));
					localHead += sizeof(XMFLOAT3);
					memcpy((char*)sm.buffer + localHead, &sm.vertexData[i].uv, sizeof(XMFLOAT2));
					localHead += sizeof(XMFLOAT2);
					memcpy((char*)sm.buffer + localHead, &sm.vertexData[i].normal, sizeof(XMFLOAT3));
					localHead += sizeof(XMFLOAT3);
				}

				// Matrix data
				memcpy((char*)sm.buffer + localHead, &matrixData, sizeof(XMFLOAT4X4));
				localHead += sizeof(XMFLOAT4X4);

				// Material data
				memcpy((char*)sm.buffer + localHead, &color, sizeof(MColor));
				localHead += sizeof(MColor);

				// Move header
				sm.cb->freeMem -= (localHead - sm.cb->head) + sm.msgHeader.padding;
				sm.cb->head += (localHead - sm.cb->head) + sm.msgHeader.padding;

				//		break;
				//	}
				//} while (sm.cb->freeMem >! ((meshSize * sizeof(float) * 8) - sm.msgHeaderSize - sizeof(XMFLOAT4X4) - sizeof(int)) + sm.msgHeader.padding);
			}
		}
	}
	// Vertex has changed
	else if (strstr(plug.partialName().asChar(), "pt["))
	{
		MFnMesh mesh(plug.node());
		MPoint point;
		mesh.getPoint(plug.logicalIndex(), point);
		MGlobal::displayInfo(MString("Vertex ID: ") + plug.logicalIndex() + " " + point.x + " " + point.y + " " + point.z);
		unsigned int vtxIndex = plug.logicalIndex();
		XMFLOAT3 position(point.x, point.y, point.z);

		//MVectorArray vertexList;
		MFloatArray vertexList;
		int vertexPoint[3];
		MIntArray triangleCounts;
		MIntArray triangleVertices;
		mesh.getTriangles(triangleCounts, triangleVertices);

		for (size_t i = 0; i < mesh.numPolygons(); i++)
		{
			for (size_t j = 0; j < triangleCounts[i]; j++)
			{
				mesh.getPolygonTriangleVertices(i, j, vertexPoint);

				// Vertex 0 in triangle:
				mesh.getPoint(vertexPoint[0], point);
				vertexList.append(point.x);
				vertexList.append(point.y);
				vertexList.append(point.z);

				// Vertex 1 in triangle:
				mesh.getPoint(vertexPoint[1], point);
				vertexList.append(point.x);
				vertexList.append(point.y);
				vertexList.append(point.z);

				// Vertex 2 in triangle:
				mesh.getPoint(vertexPoint[2], point);
				vertexList.append(point.x);
				vertexList.append(point.y);
				vertexList.append(point.z);
			}
		}

		unsigned int meshIndex = meshNames.length();
		for (size_t i = 0; i < meshNames.length(); i++)
		{
			if (meshNames[i] == mesh.name())
				meshIndex = i;
		}

		// Send data to shared memory
		do
		{
			if (sm.cb->freeMem >= slotSize)
			{
				localHead = sm.cb->head;

				// Message header
				sm.msgHeader.type = TVertexUpdate;
				sm.msgHeader.padding = slotSize - ((sizeof(float)* vertexList.length()) + sm.msgHeaderSize + sizeof(int)) % slotSize;
				memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
				localHead += sm.msgHeaderSize;

				// Mesh index
				memcpy((char*)sm.buffer + localHead, &meshIndex, sizeof(int));
				localHead += sizeof(int);

				// Vertex data
				memcpy((char*)sm.buffer + localHead, &vertexList, sizeof(float) * vertexList.length());
				localHead += sizeof(float) * vertexList.length();

				// Move header
				sm.cb->freeMem -= (localHead - sm.cb->head) + sm.msgHeader.padding;
				sm.cb->head += (localHead - sm.cb->head) + sm.msgHeader.padding;
				break;
			}
		} while (sm.cb->freeMem >! slotSize);
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

		if (transform.isParentOf(camera.object()))
			CameraChanged(transform, camera);
		else
		{
			MStatus res;

			unsigned int localMesh;

			for (size_t i = 0; i < meshNames.length(); i++)
			{
				MFnMesh mesh(transform.child(0));
				if (meshNames[i] == mesh.name())
				{
					localMesh = i;
				}
			}

			MVector translation = transform.getTranslation(MSpace::kObject);
			double rotation[4];
			transform.getRotationQuaternion(rotation[0], rotation[1], rotation[2], rotation[3]);
			double scale[3];
			transform.getScale(scale);
			// Rotation
			float rx, ry, rz;
			MPlug plugValue;
			plugValue = transform.findPlug("rx");
			plugValue.getValue(rx);
			plugValue = transform.findPlug("ry");
			plugValue.getValue(ry);
			plugValue = transform.findPlug("rz");
			plugValue.getValue(rz);

			//DirectX::XMVECTOR translationV = XMVectorSet(translation.x, translation.y, -translation.z, 1.0f);
			//DirectX::XMVECTOR rotationV = XMVectorSet(rotation[0], rotation[1], rotation[2], 0.0f);
			//DirectX::XMVECTOR scaleV = XMVectorSet(scale[0], scale[1], scale[2], 0.0f);
			//XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
			//XMFLOAT4X4 matrixData;
			//XMStoreFloat4x4(&matrixData, XMMatrixTranspose(XMMatrixAffineTransformation(scaleV, translationV, rotationV, translationV)));

			DirectX::XMMATRIX translationM = DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z);
			DirectX::XMMATRIX rotationXM = DirectX::XMMatrixRotationX(rx);
			DirectX::XMMATRIX rotationYM = DirectX::XMMatrixRotationX(ry);
			DirectX::XMMATRIX rotationZM = DirectX::XMMatrixRotationX(rz);
			DirectX::XMMATRIX scalingM = DirectX::XMMatrixScaling(scale[0], scale[1], scale[2]);

			XMFLOAT4X4 matrixData;
			XMStoreFloat4x4(&matrixData, (scalingM * rotationXM * rotationYM * rotationZM * translationM));

			//do
			//{
			//	if (sm.cb->freeMem > slotSize)
			//	{
			localHead = sm.cb->head;
			// Message header
			sm.msgHeader.type = TtransformUpdate;
			sm.msgHeader.padding = slotSize - sm.msgHeaderSize - sizeof(int) - sizeof(XMFLOAT4X4);
			memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
			localHead += sm.msgHeaderSize;
			memcpy((char*)sm.buffer + localHead, &localMesh, sizeof(int));
			localHead += sizeof(int);
			memcpy((char*)sm.buffer + localHead, &matrixData, sizeof(XMFLOAT4X4));
			localHead += sizeof(XMFLOAT4X4);

			// Move header
			sm.cb->freeMem -= slotSize;
			sm.cb->head += slotSize;

			//		break;
			//	}
			//}while (sm.cb->freeMem >! slotSize);
		}
	}
}


void CameraCreationCB(MObject& object, void* clientData) {}

void CameraChanged(MFnTransform& transform, MFnCamera& camera)
{
	//MFnCamera camera(transform.child(0));
	MVector camTranslations = transform.getTranslation(MSpace::kTransform);
	//MPoint eye = camera.eyePoint(MSpace::kWorld);
	//MVector camTranslations(eye);
	MVector viewDirection = camera.viewDirection(MSpace::kWorld);
	MVector upDirection = camera.upDirection(MSpace::kWorld);
	//MFloatMatrix projection = camera.projectionMatrix();
	XMFLOAT4X4 projection;
	XMStoreFloat4x4(&projection, XMMatrixTranspose(XMMatrixPerspectiveFovLH(
		camera.verticalFieldOfView(),
		camera.aspectRatio(),
		camera.nearClippingPlane(),
		camera.farClippingPlane())));

	MGlobal::displayInfo(MString() + "Test: " + camTranslations[0] + " " + camTranslations[1] + " " + camTranslations[2] + " ");

	viewDirection += camTranslations;
	float x = fabs(camFix.x) - fabs(camTranslations.x);
	float y = fabs(camFix.y) - fabs(camTranslations.y);
	float z = fabs(camFix.z) - fabs(camTranslations.z);

	//if (x < 0.01 && y < 0.01, z < 0.01)
	//{
	camFix = camTranslations;

	do
	{
		if (sm.cb->freeMem > slotSize)
		{
			// Send data to shared memory
			localHead = sm.cb->head;
			// Message header
			sm.msgHeader.type = TCameraUpdate;
			sm.msgHeader.padding = slotSize - sm.camDataSize - sm.msgHeaderSize;
			memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sizeof(sm.msgHeaderSize));
			localHead += sm.msgHeaderSize;

			// View matrix
			memcpy((char*)sm.buffer + localHead, &camTranslations, sizeof(MVector));
			localHead += sizeof(MVector);
			memcpy((char*)sm.buffer + localHead, &viewDirection, sizeof(MVector));
			localHead += sizeof(MVector);
			memcpy((char*)sm.buffer + localHead, &upDirection, sizeof(MVector));
			localHead += sizeof(MVector);

			// Projection matrix
			memcpy((char*)sm.buffer + localHead, &projection, sizeof(XMFLOAT4X4));

			// Move header
			sm.cb->head += slotSize;
			sm.cb->freeMem -= slotSize;

			break;
		}
	} while (sm.cb->freeMem > !slotSize);
	//}
}


void LightCreationCB(MObject& lightObject, void* clientData)
{
	callbackIds.append(MNodeMessage::addAttributeChangedCallback(lightObject, LightChangedCB));

	MFnLight light(lightObject);

	//MMatrix test;

	//light.transformationMatrix();

	MObject dad = light.parent(0);

	MFnTransform lightPoint(dad);
	callbackIds.append(MNodeMessage::addAttributeChangedCallback(lightPoint.object(), LightChangedCB));

	MColor color;
	MVector position;

	float r, b, g, a;

	MStatus status;
	if (dad.hasFn(MFn::kTransform))
	{
		position = lightPoint.getTranslation(MSpace::kObject, &status);
		MGlobal::displayInfo(MString() + "HEJ");
	}
	color = light.color();
	r = color.r;
	b = color.b;
	g = color.g;
	a = color.a;

	MGlobal::displayInfo(MString() + "Color: " + r + " " + b + " " + g + " " + a);
	MGlobal::displayInfo(MString() + "Position: " + position.x + " " + position.y + " " + position.z);
	XMFLOAT3 positionF(position.x, position.y, position.z);

	//do
	//{
	//	if (sm.cb->freeMem > slotSize)
	//	{
	localHead = sm.cb->head;
	// Message header
	sm.msgHeader.type = TLightCreate;
	sm.msgHeader.padding = slotSize - sm.msgHeaderSize - sizeof(MColor) - sizeof(XMFLOAT3);
	memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
	localHead += sm.msgHeaderSize;
	memcpy((char*)sm.buffer + localHead, &positionF, sizeof(XMFLOAT3));
	localHead += sizeof(XMFLOAT3);
	memcpy((char*)sm.buffer + localHead, &color, sizeof(MColor));
	localHead += sizeof(MColor);

	// Move header
	sm.cb->freeMem -= slotSize;
	sm.cb->head += slotSize;

	//		break;
	//	}
	//} while (sm.cb->freeMem > !slotSize);
}

void LightChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData)
{
	MFnLight light(plug.node());

	if (plug.partialName() == "cl")
	{
		MColor color(light.color());
		float r, b, g, a;

		r = color.r;
		b = color.b;
		g = color.g;
		a = color.a;

		MGlobal::displayInfo(MString() + "Color: " + r + " " + b + " " + g + " " + a);

		//do
		//{
		//	if (sm.cb->freeMem > slotSize)
		//	{
		localHead = sm.cb->head;
		// Message header
		unsigned int localLight = 0;
		sm.msgHeader.type = TLightUpdate;
		sm.msgHeader.padding = slotSize - sm.msgHeaderSize - sizeof(MColor);
		memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
		localHead += sm.msgHeaderSize;
		memcpy((char*)sm.buffer + localHead, &localLight, sizeof(int));
		localHead += sizeof(int);
		memcpy((char*)sm.buffer + localHead, &color, sizeof(MColor));
		localHead += sizeof(MColor);

		// Move header
		sm.cb->freeMem -= slotSize;
		sm.cb->head += slotSize;

		//		break;
		//	}
		//} while (sm.cb->freeMem > !slotSize);
	}
	else if (msg & MNodeMessage::kAttributeSet)
	{
		MFnTransform lightPoint(plug.node());

		MVector position;

		position = lightPoint.getTranslation(MSpace::kObject);

		MGlobal::displayInfo(MString() + "Position: " + position.x + " " + position.y + " " + position.z);

		XMFLOAT3 positionF(position.x, position.y, position.z);

		//while (sm.cb->freeMem > !slotSize)
		//{
		//	if (sm.cb->freeMem > slotSize)
		//	{

		localHead = sm.cb->head;

		unsigned int localLight = 1;
		sm.msgHeader.type = TLightUpdate;
		sm.msgHeader.padding = slotSize - sm.msgHeaderSize - sizeof(XMFLOAT3);
		memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
		localHead += sm.msgHeaderSize;
		memcpy((char*)sm.buffer + localHead, &localLight, sizeof(int));
		localHead += sizeof(int);
		memcpy((char*)sm.buffer + localHead, &positionF, sizeof(XMFLOAT3));
		localHead += sizeof(XMFLOAT3);

		// Move header
		sm.cb->freeMem -= slotSize;
		sm.cb->head += slotSize;

		//		break;
		//	}
		//}
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

	localHead = sm.cb->head;
	// Message header
	sm.msgHeader.type = TNodeDestroyed;
	sm.msgHeader.padding = slotSize - sm.msgHeaderSize - sizeof(int);
	memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
	localHead += sm.msgHeaderSize;

	// Node index
	memcpy((char*)sm.buffer + localHead, &meshIndex, sizeof(int));
	localHead += sizeof(int);

	// Move header
	sm.cb->freeMem -= slotSize;
	sm.cb->head += slotSize;
}