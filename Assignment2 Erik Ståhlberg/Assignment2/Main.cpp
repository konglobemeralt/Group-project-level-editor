#include "MayaHeaders.h"
#include <iostream>
#include "SharedMemory.h"

void GetSceneData();

void GetMeshInformation(MFnMesh& mesh);
void MeshCreationCB(MObject& node, void* clientData);
void MeshChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);

void GetTransformInformation(MFnTransform& transform);
void TransformCreationCB(MObject& object, void* clientData);
void TransformChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);

void GetCameraInformation(MFnCamera& camera);
void CameraCreationCB(MObject& object, void* clientData);
void CameraChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);

void LightCreationCB(MObject& lightObject, void* clientData);
void LightChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);

void NameChangedCB(MObject& node, const MString& prevName, void* clientData);
void TimerCB(float elapsedTime, float lastTime, void* clientData);

// Timer callback
float period = 5.0f;
int totalTime = 0.0f;

MCallbackIdArray callbackIds;

SharedMemory sm;
unsigned int localHead;
unsigned int slotSize;

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
{	// Meshes
	MItDag itMeshes(MItDag::kDepthFirst, MFn::kMesh);
	while (!itMeshes.isDone())
	{
		MFnMesh mesh(itMeshes.item());
		GetMeshInformation(mesh);
		callbackIds.append(MNodeMessage::addAttributeChangedCallback(mesh.object(), MeshChangedCB));
		callbackIds.append(MNodeMessage::addNameChangedCallback(mesh.object(), NameChangedCB));
		itMeshes.next();
	}

	// Tranforms including cameras
	MItDag itTransform(MItDag::kDepthFirst, MFn::kTransform);
	while (!itTransform.isDone())
	{
		MFnTransform transform(itTransform.item());
		GetTransformInformation(transform);
		callbackIds.append(MNodeMessage::addAttributeChangedCallback(transform.object(), TransformChangedCB));
		callbackIds.append(MNodeMessage::addNameChangedCallback(transform.object(), NameChangedCB));
		itTransform.next();
	}

	MItDag itCamera(MItDag::kDepthFirst, MFn::kCamera);
	while (!itCamera.isDone())
	{
		MFnCamera camera(itCamera.item());
		GetCameraInformation(camera);
		callbackIds.append(MNodeMessage::addAttributeChangedCallback(camera.object(), CameraChangedCB));
		callbackIds.append(MNodeMessage::addNameChangedCallback(camera.object(), NameChangedCB));
		itCamera.next();
	}
}


void GetMeshInformation(MFnMesh& mesh)
{
	MGlobal::displayInfo("Mesh: " + mesh.fullPathName() + " loaded!");

	// Vertex position
	MIntArray vtxCount;
	MIntArray vtxArray;
	mesh.getVertices(vtxCount, vtxArray);
	MGlobal::displayInfo(MString("Number of vertices: ") + vtxArray.length());
	MPointArray vts;
	mesh.getPoints(vts);
	for (size_t i = 0; i < vtxArray.length(); i++)
	{
		MGlobal::displayInfo(MString("V: ") + vtxArray[i] + ": " + vts[vtxArray[i]].x + " " + vts[vtxArray[i]].y + " " + vts[vtxArray[i]].z);
	}

	// UV:s
	MFloatArray u, v;
	mesh.getUVs(u, v);
	MGlobal::displayInfo(MString("Number of uv: ") + u.length());
	for (size_t i = 0; i < u.length(); i++)
	{
		MGlobal::displayInfo(MString("UV ") + i + ": " + u[i] + " " + v[i]);
	}

	// Normals
	MFloatVectorArray nArray;
	mesh.getNormals(nArray);
	MGlobal::displayInfo(MString("Number of normals: ") + nArray.length());
	for (size_t i = 0; i < nArray.length(); i++)
	{
		MGlobal::displayInfo(MString("N ") + i + ": " + nArray[i][0] + " " + nArray[i][1] + " " + nArray[i][2]);
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
				vList.append(uvPoint[1]);

				// Vertex 1 in triangle:
				mesh.getPoint(vertexPoint[1], point);
				vertexList.append(MVector());
				vertexList[id + 1].x = point.x;
				vertexList[id + 1].y = point.y;
				vertexList[id + 1].z = point.z;

				// UV 1 in triangle:
				mesh.getUVAtPoint(point, uvPoint);
				uList.append(uvPoint[0]);
				vList.append(uvPoint[1]);

				// Vertex 2 in triangle:
				mesh.getPoint(vertexPoint[2], point);
				vertexList.append(MVector());
				vertexList[id + 2].x = point.x;
				vertexList[id + 2].y = point.y;
				vertexList[id + 2].z = point.z;

				// UV 2 in triangle:
				mesh.getUVAtPoint(point, uvPoint);
				uList.append(uvPoint[0]);
				vList.append(uvPoint[1]);

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
		}

		// Send data to shared memory
		int size = vertexList.length();
		int head = 4;
		memcpy((char*)sm.buffer, &size, sizeof(int));
		for (size_t i = 0; i < vertexList.length(); i++)
		{
			memcpy((char*)sm.buffer + head, &sm.vertexData[i].pos, sizeof(XMFLOAT3));
			head += sizeof(XMFLOAT3);
			memcpy((char*)sm.buffer + head, &sm.vertexData[i].uv, sizeof(XMFLOAT2));
			head += sizeof(XMFLOAT2);
			memcpy((char*)sm.buffer + head, &sm.vertexData[i].normal, sizeof(XMFLOAT3));
			head += sizeof(XMFLOAT3);
		}
	}

	// Vertex has changed
	else if (strstr(plug.partialName().asChar(), "pt["))
	{
		MFnMesh mesh(plug.node());
		MPoint point;
		mesh.getPoint(plug.logicalIndex(), point);
		MGlobal::displayInfo("Mesh: " + mesh.fullPathName() + " vertex changed!");
		MGlobal::displayInfo(MString("Vertex ID: ") + plug.logicalIndex() + " " + point.x + " " + point.y + " " + point.z);
	}
}


void GetTransformInformation(MFnTransform& transform)
{
	MGlobal::displayInfo("Transform: " + transform.fullPathName() + " loaded!");
	MPlug plugValue;

	// Translation
	float tx, ty, tz;
	plugValue = transform.findPlug("tx");
	plugValue.getValue(tx);
	plugValue = transform.findPlug("ty");
	plugValue.getValue(ty);
	plugValue = transform.findPlug("tz");
	plugValue.getValue(tz);

	// Rotation
	float rx, ry, rz;
	plugValue = transform.findPlug("rx");
	plugValue.getValue(rx);
	plugValue = transform.findPlug("ry");
	plugValue.getValue(ry);
	plugValue = transform.findPlug("rz");
	plugValue.getValue(rz);

	// Scale
	float sx, sy, sz;
	plugValue = transform.findPlug("sx");
	plugValue.getValue(sx);
	plugValue = transform.findPlug("sy");
	plugValue.getValue(sy);
	plugValue = transform.findPlug("sz");
	plugValue.getValue(sz);

	// Print to script editor
	MGlobal::displayInfo(MString("Translate: ") + tx + " " + ty + " " + tz);
	MGlobal::displayInfo(MString("Rotation:  ") + rx + " " + ry + " " + rz);
	MGlobal::displayInfo(MString("Scale:     ") + sx + " " + sy + " " + sz);
}

void TransformCreationCB(MObject& object, void* clientData)
{
	// Add a callback specifik to every new transform that are created
	callbackIds.append(MNodeMessage::addAttributeChangedCallback(object, TransformChangedCB));
	callbackIds.append(MNodeMessage::addNameChangedCallback(object, NameChangedCB));

	MStatus res;
	MFnTransform transform(object, &res);
	MGlobal::displayInfo("Transform: " + transform.fullPathName() + " created!");
	MPlug plugValue;

	// Translation
	float tx, ty, tz;
	plugValue = transform.findPlug("tx");
	plugValue.getValue(tx);
	plugValue = transform.findPlug("ty");
	plugValue.getValue(ty);
	plugValue = transform.findPlug("tz");
	plugValue.getValue(tz);

	// Rotation
	float rx, ry, rz;
	plugValue = transform.findPlug("rx");
	plugValue.getValue(rx);
	plugValue = transform.findPlug("ry");
	plugValue.getValue(ry);
	plugValue = transform.findPlug("rz");
	plugValue.getValue(rz);

	// Scale
	float sx, sy, sz;
	plugValue = transform.findPlug("sx");
	plugValue.getValue(sx);
	plugValue = transform.findPlug("sy");
	plugValue.getValue(sy);
	plugValue = transform.findPlug("sz");
	plugValue.getValue(sz);

	// Print to script editor
	MGlobal::displayInfo(MString("Translate: ") + tx + " " + ty + " " + tz);
	MGlobal::displayInfo(MString("Rotation:  ") + rx + " " + ry + " " + rz);
	MGlobal::displayInfo(MString("Scale:     ") + sx + " " + sy + " " + sz);

	MFnMatrixData matrix(object);
}

void TransformChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData)
{
	M3dView view = M3dView::active3dView();
	MDagPath camPath;
	view.getCamera(camPath);
	MFnCamera activeCamera(camPath);

	if (msg & MNodeMessage::kAttributeSet)
	{
		MFnTransform transform(plug.node());

		if (transform.isParentOf(activeCamera.object()))
		{
			MFnCamera camera(transform.child(0));
			MVector camTranslations = transform.getTranslation(MSpace::kTransform);
			MVector viewDirection = camera.viewDirection(MSpace::kWorld);
			MVector upDirection = camera.upDirection();
			//MFloatMatrix projectionMatrix(camera.projectionMatrix());

			// Send data to shared memory
			localHead = sm.cb->head;
			// Message header
			sm.msgHeader.type = TCameraUpdate;
			sm.msgHeader.padding = slotSize - sm.camDataSize - sm.msgHeaderSize;
			memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sizeof(sm.msgHeaderSize));
			localHead += sm.msgHeaderSize;

			// Data
			memcpy((char*)sm.buffer + localHead, &camTranslations, sizeof(MVector));
			localHead += sizeof(MVector);
			memcpy((char*)sm.buffer + localHead, &viewDirection, sizeof(MVector));
			localHead += sizeof(MVector);
			memcpy((char*)sm.buffer + localHead, &upDirection, sizeof(MVector));

			// Move header
			sm.cb->head += slotSize;
			sm.cb->freeMem -= slotSize;
		}
		else
		{
			MStatus res;
			//MObject object = plug.node();
			//MFnTransform transform(object, &res);
			////MMatrix matrix = transform.transformationMatrix();
			////float m[4][4];
			////matrix.get(m);

			// Translation
			MVector translation = transform.getTranslation(MSpace::kObject);
			//// Rotation
			//double rotation[3];
			//transform.getRotation(rotation, MTransformationMatrix::kXYZ, MSpace::kObject);
			// Scale
			double scale[3];
			transform.getScale(scale);

			//MGlobal::displayInfo("Transform: " + transform.fullPathName() + " has changed!");
			MPlug plugValue;

			// Rotation
			float rx, ry, rz;
			plugValue = transform.findPlug("rx");
			plugValue.getValue(rx);
			plugValue = transform.findPlug("ry");
			plugValue.getValue(ry);
			plugValue = transform.findPlug("rz");
			plugValue.getValue(rz);

			//// Print to script editor
			//MGlobal::displayInfo(MString("Translate: ") + translation.x + " " + translation.y + " " + translation.z);
			//MGlobal::displayInfo(MString("Rotation:  ") + rx + " " + ry + " " + rz);
			//MGlobal::displayInfo(MString("Scale:     ") + scale[0] + " " + scale[1] + " " + scale[2]);
		}
	}
}


void GetCameraInformation(MFnCamera& camera)
{
	M3dView view = M3dView::active3dView();
	MDagPath camPath;
	view.getCamera(camPath);
	MFnCamera activeCamera(camPath);

	if (camera.name() == activeCamera.name())
	{
		MFnTransform transform(camera.parent(0));

		//callbackIds.append(MNodeMessage::addAttributeChangedCallback(transform.object(), CameraChangedCB));
		callbackIds.append(MNodeMessage::addNameChangedCallback(transform.object(), NameChangedCB));

		MVector camTranslations = transform.getTranslation(MSpace::kTransform);
		MVector viewDirection = camera.viewDirection(MSpace::kWorld);
		MVector upDirection = camera.upDirection();
		//MFloatMatrix projectionMatrix(camera.projectionMatrix());

		// Send data to shared memory
		localHead = sm.cb->head;
		// Message header
		sm.msgHeader.type = TCameraUpdate;
		sm.msgHeader.padding = slotSize - sm.camDataSize - sm.msgHeaderSize;
		memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
		localHead += sm.msgHeaderSize;

		// Data
		memcpy((char*)sm.buffer + localHead, &camTranslations, sizeof(MVector));
		localHead += sizeof(MVector);
		memcpy((char*)sm.buffer + localHead, &viewDirection, sizeof(MVector));
		localHead += sizeof(MVector);
		memcpy((char*)sm.buffer + localHead, &upDirection, sizeof(MVector));
		
		// Move header
		sm.cb->head += slotSize;
		sm.cb->freeMem -= slotSize;
	}
}

void CameraCreationCB(MObject& object, void* clientData){}

void CameraChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData)
{
	M3dView view = M3dView::active3dView();
	MDagPath camPath;
	view.getCamera(camPath);
	MFnCamera activeCamera(camPath);

	MFnCamera camera(plug.child(0));
	//MFnCamera camera(plug.node());
	//MGlobal::displayInfo(camera.name());

	if (camera.name() == activeCamera.name())
	{
		MFnTransform transform(camera.parent(0));

		MVector camTranslations = transform.getTranslation(MSpace::kTransform);
		MVector viewDirection = camera.viewDirection(MSpace::kWorld);
		MVector upDirection = camera.upDirection();
		//MFloatMatrix projectionMatrix(camera.projectionMatrix());

		// Send data to shared memory
		localHead = sm.cb->head;
		// Message header
		sm.msgHeader.type = TCameraUpdate;
		sm.msgHeader.padding = slotSize - sm.camDataSize - sm.msgHeaderSize;
		memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sizeof(sm.msgHeaderSize));
		localHead += sm.msgHeaderSize;

		// Data
		memcpy((char*)sm.buffer + localHead, &camTranslations, sizeof(MVector));
		localHead += sizeof(MVector);
		memcpy((char*)sm.buffer + localHead, &viewDirection, sizeof(MVector));
		localHead += sizeof(MVector);
		memcpy((char*)sm.buffer + localHead, &upDirection, sizeof(MVector));

		// Move header
		sm.cb->head += slotSize;

		//// Send data to shared memory
		//int head = sm.msgHeaderSize;
		//sm.msgHeader.type = 2;
		//sm.msgHeader.padding = 0;
		//memcpy((char*)sm.buffer, &sm.msgHeader, sizeof(sm.msgHeaderSize));

		//memcpy((char*)sm.buffer + head, &camTranslations, sizeof(MVector));
		//head += sizeof(MVector);
		//memcpy((char*)sm.buffer + head, &viewDirection, sizeof(MVector));
		//head += sizeof(MVector);
		//memcpy((char*)sm.buffer + head, &upDirection, sizeof(MVector));
		//int hej = 0;
	}
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
	}
	else if (msg & MNodeMessage::kAttributeSet)
	{
		MFnTransform lightPoint(plug.node());

		MVector position;

		position = lightPoint.getTranslation(MSpace::kObject);

		MGlobal::displayInfo(MString() + "Position: " + position.x + " " + position.y + " " + position.z);
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