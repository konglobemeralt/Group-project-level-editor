#include "MayaHeaders.h"
#include <iostream>
#include "SharedMemory.h"

void NodeCreationCB(MObject& node, void* clientData);
void MeshChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);
void NameChangedCB(MObject& node, const MString& prevName, void* clientData);
void TimerCB(float elapsedTime, float lastTime, void* clientData);
void TransformCreationCB(MObject& node, void* clientData);
void TransformChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);

// Timer callback
float period = 5.0f;
int totalTime = 0.0f;

MCallbackIdArray callbackIds;

SharedMemory sm;

void GetSceneData();
void GetMeshInformation(MFnMesh& mesh);
void GetTransformInformation(MFnTransform& transform);

EXPORT MStatus initializePlugin(MObject obj)
{
	MStatus res = MS::kSuccess;

	MFnPlugin myPlugin(obj, "Maya plugin", "1.0", "any", &res);
	if (MFAIL(res))
		CHECK_MSTATUS(res);

	MGlobal::displayInfo(sm.OpenMemory(10));

	GetSceneData();

	callbackIds.append(MDGMessage::addNodeAddedCallback(NodeCreationCB, "mesh", &res));
	callbackIds.append(MDGMessage::addNodeAddedCallback(TransformCreationCB, "transform", &res));
	callbackIds.append(MTimerMessage::addTimerCallback(period, TimerCB, &res));

	if(res == MS::kSuccess)
		MGlobal::displayInfo("Maya plugin loaded!");

	return res;
}

EXPORT MStatus uninitializePlugin(MObject obj)
{
	MFnPlugin plugin(obj);

	if(callbackIds.length() != 0)
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

void NodeCreationCB(MObject& object, void* clientData)
{
	// Add a callback specifik to every new mesh that are created
	callbackIds.append(MNodeMessage::addAttributeChangedCallback(object, MeshChangedCB));
	callbackIds.append(MNodeMessage::addNameChangedCallback(object, NameChangedCB));
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
		int id = 0;
		//vertexIDList.setLength(mesh.numPolygons());
		int vertexPoint[3];
		MIntArray triangleId;
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
			triangleId.append(triangleCounts[i]);
			for (size_t j = 0; j < triangleCounts[i]; j++)
			{
				mesh.getPolygonTriangleVertices(i, j, vertexPoint);
				vertexIDList.append(vertexPoint[0]);
				vertexIDList.append(vertexPoint[1]);
				vertexIDList.append(vertexPoint[2]);
				//vertexIDList[id] = vertexPoint[0];
				//vertexIDList[id+1] = vertexPoint[1];
				//vertexIDList[id+2] = vertexPoint[2];

				//Vertex:
				mesh.getPoint(vertexPoint[0], point);
				vertexList.append(MVector());
				vertexList[id].x = point.x;
				vertexList[id].y = point.y;
				vertexList[id].z = point.z;

				//Uv:
				mesh.getUVAtPoint(point, uvPoint);
				uList.append(uvPoint[0]);
				vList.append(uvPoint[1]);

				mesh.getPoint(vertexPoint[1], point);
				vertexList.append(MVector());
				vertexList[id+1].x = point.x;
				vertexList[id+1].y = point.y;
				vertexList[id+1].z = point.z;

				mesh.getUVAtPoint(point, uvPoint);
				uList.append(uvPoint[0]);
				vList.append(uvPoint[1]);

				mesh.getPoint(vertexPoint[2], point);
				vertexList.append(MVector());
				vertexList[id+2].x = point.x;
				vertexList[id+2].y = point.y;
				vertexList[id+2].z = point.z;

				mesh.getUVAtPoint(point, uvPoint);
				uList.append(uvPoint[0]);
				vList.append(uvPoint[1]);

				//Normal:
				mesh.getVertexNormal(vertexIDList[0], true, normal);
				normalList.append(normal);
				mesh.getVertexNormal(vertexIDList[1], true, normal);
				normalList.append(normal);
				mesh.getVertexNormal(vertexIDList[2], true, normal);
				normalList.append(normal);

				id += 3;
			}
		}


		//MGlobal::displayInfo(MString("Number of vertices: ") + vtxArray.length());
		//MGlobal::displayInfo(MString("HEJ ") + vertexList[2]);
		MPointArray vts;
		mesh.getPoints(vts);
		int hej = vertexIDList.length();
		sm.vertexData.resize(vertexList.length());
		if (vertexIDList.length() > 0)
		{
			for (size_t i = 0; i < vertexList.length(); i++)
			{
				MGlobal::displayInfo(MString("V: ") + vertexIDList[i] + ": " + vertexList[i].x + " " + vertexList[i].y + " " + vertexList[i].z + " "  + i);
				//MGlobal::displayInfo(MString("V: ") + vtxArray[i] +  ": " + vts[vtxArray[i]].x + " " + vts[vtxArray[i]].y + " " + vts[vtxArray[i]].z);
				sm.vertexData[i].pos = XMFLOAT3(vertexList[i].x, vertexList[i].y, vertexList[i].z);
				sm.vertexData[i].uv = XMFLOAT2(uList[i], vList[i]);
				MGlobal::displayInfo(MString("UV ") + i + ": " + uList[i] + " " + vList[i]);
				MGlobal::displayInfo(MString("N ") + i + ": " + normalList[i].x + " " + normalList[i].y + " " + normalList[i].z);
				//MGlobal::displayInfo(MString("V: ") + vtxArray[i] + ": " + sm.vertexData[i].pos.x + " " + sm.vertexData[i].pos.y + " " + sm.vertexData[i].pos.z);
			}
		}

		//MIntArray vertexIDList;
		//MVectorArray vertexList;
		//int id = 0;
		////vertexIDList.setLength(mesh.numPolygons());
		//int vertexPoint[3];
		//MIntArray triangleId;
		//MIntArray triangleCounts;
		//MIntArray triangleVertices;
		//MPoint point;
		//mesh.getTriangles(triangleCounts, triangleVertices);
		//for (size_t i = 0; i < mesh.numPolygons(); i++)
		//{
		//	triangleId.append(triangleCounts[i]);
		//	for (size_t j = 0; j < triangleCounts[i]; j++)
		//	{
		//		mesh.getPolygonTriangleVertices(i, j, vertexPoint);
		//		vertexIDList.append(vertexPoint[0]);
		//		vertexIDList.append(vertexPoint[1]);
		//		vertexIDList.append(vertexPoint[2]);
		//		//vertexIDList[id] = vertexPoint[0];
		//		//vertexIDList[id+1] = vertexPoint[1];
		//		//vertexIDList[id+2] = vertexPoint[2];

		//		mesh.getPoint(vertexPoint[0], point);
		//		vertexList.append(MVector());
		//		vertexList[id].x = point.x;
		//		vertexList[id].y = point.y;
		//		vertexList[id].z = point.z;
		//		mesh.getPoint(vertexPoint[1], point);
		//		vertexList.append(MVector());
		//		vertexList[id + 1].x = point.x;
		//		vertexList[id + 1].y = point.y;
		//		vertexList[id + 1].z = point.z;
		//		mesh.getPoint(vertexPoint[2], point);
		//		vertexList.append(MVector());
		//		vertexList[id + 2].x = point.x;
		//		vertexList[id + 2].y = point.y;
		//		vertexList[id + 2].z = point.z;
		//		id += 3;
		//	}
		//}

		// UV:s
		//MFloatArray u, v;
		//mesh.getUVs(u, v);
		////mesh.getPolygonUV;
		//MGlobal::displayInfo(MString("Number of uv: ") + u.length());
		//for (size_t i = 0; i < u.length(); i++)
		//{
		//	MGlobal::displayInfo(MString("UV ") + i + ": " + u[i] + " " + v[i]);
		//}

		//// Normals
		//MFloatVectorArray nArray;
		//mesh.getNormals(nArray);
		//MGlobal::displayInfo(MString("Number of normals: ") + nArray.length());
		//for (size_t i = 0; i < nArray.length(); i++)
		//{
		//	MGlobal::displayInfo(MString("N ") + i + ": " + nArray[i][0] + " " + nArray[i][1] + " " + nArray[i][2]);
		//}

		// Send data to shared memory
		int size = vertexList.length();
		memcpy((int*)sm.buffer, &size, sizeof(int));
		for (size_t i = 0; i < vertexList.length(); i++)
		{
			memcpy((XMFLOAT3*)sm.buffer + i + 1, &sm.vertexData[i].pos, sizeof(XMFLOAT3));
		}
		int asd = 0;
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

void TransformChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData)
{
	if (msg & MNodeMessage::kAttributeSet)
	{
		MStatus res;		
		MObject object = plug.node();
		MFnTransform transform(object, &res);

		// Translation
		MVector translation = transform.getTranslation(MSpace::kObject);
		//// Rotation
		//double rotation[3];
		//transform.getRotation(rotation, MTransformationMatrix::kXYZ, MSpace::kObject);
		// Scale
		double scale[3];
		transform.getScale(scale);

		MGlobal::displayInfo("Transform: " + transform.fullPathName() + " has changed!");
		MPlug plugValue;

		// Rotation
		float rx, ry, rz;
		plugValue = transform.findPlug("rx");
		plugValue.getValue(rx);
		plugValue = transform.findPlug("ry");
		plugValue.getValue(ry);
		plugValue = transform.findPlug("rz");
		plugValue.getValue(rz);

		// Print to script editor
		MGlobal::displayInfo(MString("Translate: ") + translation.x + " " + translation.y + " " + translation.z);
		MGlobal::displayInfo(MString("Rotation:  ") + rx + " " + ry + " " + rz);
		MGlobal::displayInfo(MString("Scale:     ") + scale[0] + " " + scale[1] + " " + scale[2]);
	}
}