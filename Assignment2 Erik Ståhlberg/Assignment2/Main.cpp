#include "MayaHeaders.h"
#include <iostream>
#include "SharedMemory.h"

void GetSceneData();

void GetMeshInformation(MFnMesh& mesh);
void MeshCreationCB(MObject& node, void* clientData);
void MeshChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);
void VertexChanged(MPlug& plug);
void NormalChanged(MPlug& plug);
void UVChanged(MPlug& plug);
void MaterialChanged(MFnMesh& mesh);

void ShaderChangedCB(MObject& object, void* clientData);
void test(MObject& object, void* clientData);
void shaderchanged(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);

void TransformCreationCB(MObject& object, void* clientData);
void TransformChangedCB(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);

void CameraCreationCB(MObject& object, void* clientData);
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

MVector lastCamPos;
unsigned int i = 0;
MVector diff;

SharedMemory sm;
unsigned int localHead;
unsigned int slotSize;
MStringArray meshNames;
MStringArray lightNames;
MStringArray matNames;
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

	MGlobal::displayInfo(sm.OpenMemory(100));

	GetSceneData();

	callbackIds.append(MDGMessage::addNodeAddedCallback(MeshCreationCB, "mesh", &res));
	callbackIds.append(MDGMessage::addNodeAddedCallback(TransformCreationCB, "transform", &res));
	callbackIds.append(MDGMessage::addNodeAddedCallback(LightCreationCB, "light", &res));
	callbackIds.append(MDGMessage::addNodeAddedCallback(ShaderChangedCB, "lambert", &res));
	callbackIds.append(MDGMessage::addNodeAddedCallback(ShaderChangedCB, "blinn", &res));
	callbackIds.append(MDGMessage::addNodeAddedCallback(ShaderChangedCB, "phong", &res));
	callbackIds.append(MTimerMessage::addTimerCallback(period, TimerCB, &res));

	//MFn::kSurfaceShader

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
		callbackIds.append(MNodeMessage::addNodeAboutToDeleteCallback(light.object(), NodeDestroyedCB));
		itLight.next();
	}

	MItDag itCamera(MItDag::kDepthFirst, MFn::kCamera);
	while (!itCamera.isDone())
	{
		MFnCamera camera(itCamera.item());
		//GetCameraInformation(camera);

		callbackIds.append(MNodeMessage::addNameChangedCallback(camera.object(), NameChangedCB));
		itCamera.next();
	}

	MItDependencyNodes itLambert(MFn::kLambert);
	while (!itLambert.isDone())
	{
		MFnLambertShader lambertShader(itLambert.item());

		matNames.append(lambertShader.name());
		callbackIds.append(MNodeMessage::addAttributeChangedCallback(lambertShader.object(), shaderchanged));
		itLambert.next();
	}

	MItDependencyNodes itBlinn(MFn::kBlinn);
	while (!itBlinn.isDone())
	{
		MFnBlinnShader blinnShader(itBlinn.item());

		matNames.append(blinnShader.name());
		callbackIds.append(MNodeMessage::addAttributeChangedCallback(blinnShader.object(), shaderchanged));
		itBlinn.next();
	}

	MItDependencyNodes itPhong(MFn::kPhong);
	while (!itPhong.isDone())
	{
		MFnPhongShader phongShader(itPhong.item());

		matNames.append(phongShader.name());
		callbackIds.append(MNodeMessage::addAttributeChangedCallback(phongShader.object(), shaderchanged));
		itPhong.next();
	}
	i = 0;

	//M3dView view = M3dView::active3dView();
	//MDagPath camPath;
	//view.getCamera(camPath);
	//MFnCamera camera(camPath);

	//MPoint eye = camera.eyePoint(MSpace::kWorld);
	//MVector camTranslations(eye);
	//lastCamPos = camTranslations;
}

// Försök slå ihop GetMeshInformation och MeshChangedCB
void GetMeshInformation(MFnMesh& mesh)
{
	unsigned int index = 0;
	MItMeshPolygon itPoly(mesh.object());
	MIntArray vtxIDArray;
	MIntArray uvIDArray;
	MIntArray norIDArray;
	int uvIndex;
	unsigned id = 0;
	unsigned int polyVtxCount = 0;
	while (!itPoly.isDone())
	{
		// Vertex ID:s array
		vtxIDArray.append(itPoly.vertexIndex(0));
		vtxIDArray.append(itPoly.vertexIndex(3));
		vtxIDArray.append(itPoly.vertexIndex(1));
		vtxIDArray.append(itPoly.vertexIndex(1));
		vtxIDArray.append(itPoly.vertexIndex(3));
		vtxIDArray.append(itPoly.vertexIndex(2));

		// UV ID:s array
		itPoly.getUVIndex(0, uvIndex, 0);
		uvIDArray.append(uvIndex);
		itPoly.getUVIndex(3, uvIndex, 0);
		uvIDArray.append(uvIndex);
		itPoly.getUVIndex(1, uvIndex, 0);
		uvIDArray.append(uvIndex);
		itPoly.getUVIndex(1, uvIndex, 0);
		uvIDArray.append(uvIndex);
		itPoly.getUVIndex(3, uvIndex, 0);
		uvIDArray.append(uvIndex);
		itPoly.getUVIndex(2, uvIndex, 0);
		uvIDArray.append(uvIndex);

		// Normal ID:s array
		norIDArray.append(itPoly.normalIndex(0));
		norIDArray.append(itPoly.normalIndex(3));
		norIDArray.append(itPoly.normalIndex(1));
		norIDArray.append(itPoly.normalIndex(1));
		norIDArray.append(itPoly.normalIndex(3));
		norIDArray.append(itPoly.normalIndex(2));

		//	MGlobal::displayInfo(MString("VERTICES: ") + itPoly.polygonVertexCount());
		//	polyVtxCount = itPoly.polygonVertexCount();
		//	if (polyVtxCount % 2 == 0)
		//	{
		//		for (size_t i = 0; i < polyVtxCount - 1; i += 3)
		//		{
		//			// Vertex ID:s array
		//			vtxIDArray.append(itPoly.vertexIndex(id));
		//			vtxIDArray.append(itPoly.vertexIndex(id + 1));
		//			vtxIDArray.append(itPoly.vertexIndex(id + 2));
		//			vtxIDArray.append(itPoly.vertexIndex(id + 2));

		//			// UV ID:s array
		//			itPoly.getUVIndex(id, uvIndex, 0);
		//			uvIDArray.append(uvIndex);
		//			itPoly.getUVIndex(id + 1, uvIndex, 0);
		//			uvIDArray.append(uvIndex);
		//			itPoly.getUVIndex(id + 2, uvIndex, 0);
		//			uvIDArray.append(uvIndex);
		//			itPoly.getUVIndex(id + 2, uvIndex, 0);
		//			uvIDArray.append(uvIndex);

		//			// Normal ID:s array
		//			norIDArray.append(itPoly.normalIndex(id));
		//			norIDArray.append(itPoly.normalIndex(id + 1));
		//			norIDArray.append(itPoly.normalIndex(id + 2));
		//			norIDArray.append(itPoly.normalIndex(id + 2));

		//			MGlobal::displayInfo(MString() + itPoly.vertexIndex(id));
		//			MGlobal::displayInfo(MString() + itPoly.vertexIndex(id + 1));
		//			MGlobal::displayInfo(MString() + itPoly.vertexIndex(id + 2));
		//			MGlobal::displayInfo(MString() + itPoly.vertexIndex(id + 2));

		//			id += 3;
		//		}

		//		// Last vertex uv and normal triangle
		//		vtxIDArray.append(itPoly.vertexIndex(polyVtxCount - 1));
		//		itPoly.getUVIndex(polyVtxCount - 1, uvIndex, 0);
		//		uvIDArray.append(uvIndex);
		//		norIDArray.append(itPoly.normalIndex(polyVtxCount - 1));

		//		vtxIDArray.append(itPoly.vertexIndex(0));
		//		itPoly.getUVIndex(0, uvIndex, 0);
		//		uvIDArray.append(uvIndex);
		//		norIDArray.append(itPoly.normalIndex(0));

		//		MGlobal::displayInfo(MString() + itPoly.vertexIndex(polyVtxCount - 1));
		//		MGlobal::displayInfo(MString() + itPoly.vertexIndex(0));

		//		// Every other vertex point
		//		if (polyVtxCount > 5)
		//		{
		//			for (size_t i = 0; i < 3; i++)
		//			{
		//				vtxIDArray.append(itPoly.vertexIndex(i*2));
		//				itPoly.getUVIndex(i*2, uvIndex, 0);
		//				uvIDArray.append(uvIndex);
		//				norIDArray.append(itPoly.normalIndex(i*2));

		//				MGlobal::displayInfo(MString() + itPoly.vertexIndex(i*2));
		//			}
		//		}
		//	}

		//	//// TESTING
		//	//MGlobal::displayInfo(MString("VERTICES: ") + itPoly.polygonVertexCount());
		//	//for (size_t i = 0; i < itPoly.polygonVertexCount(); i++)
		//	//{
		//	//	MGlobal::displayInfo(MString() + itPoly.vertexIndex(i));
		//	//}

		//	id = 0;
		itPoly.next();
	}

	MPointArray points;
	MFloatArray us;
	MFloatArray vs;
	MFloatVectorArray normals;
	mesh.getPoints(points);
	mesh.getUVs(us, vs);
	mesh.getNormals(normals);

	sm.pos.resize(vtxIDArray.length());
	sm.uv.resize(vtxIDArray.length());
	sm.normal.resize(vtxIDArray.length());
	if (vtxIDArray.length() > 0)
	{
		MGlobal::displayInfo("Mesh: " + mesh.fullPathName() + " loaded!");
		meshNames.append(mesh.name());

		for (size_t i = 0; i < vtxIDArray.length(); i++)
		{
			sm.pos[i] = XMFLOAT3(points[vtxIDArray[i]].x, points[vtxIDArray[i]].y, -points[vtxIDArray[i]].z);
			sm.uv[i] = XMFLOAT2(us[uvIDArray[i]], 1 - vs[uvIDArray[i]]);
			sm.normal[i] = XMFLOAT3(normals[norIDArray[i]].x, normals[norIDArray[i]].y, -normals[norIDArray[i]].z);
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

		// MATERIAL:
		MGlobal::displayInfo(mesh.name());

		unsigned int instanceNumber = 0;
		MObjectArray shaders;
		MIntArray indices;
		MPlugArray connections;
		MColor color;
		int whatMaterial = 0;
		MFnLambertShader lambertShader;
		MFnBlinnShader blinnShader;
		MFnPhongShader phongShader;

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
			//MFnLambertShader lambertShader(connections[0].node());
			lambertShader.setObject(connections[0].node());
			MGlobal::displayInfo(lambertShader.name());
			color = lambertShader.color();
			whatMaterial = 1;
			meshMatNames.append(lambertShader.name());
		}
		else if (connections[0].node().hasFn(MFn::kBlinn))
		{
			//MFnBlinnShader blinnShader(connections[0].node());
			blinnShader.setObject(connections[0].node());
			MGlobal::displayInfo(blinnShader.name());
			color = blinnShader.color();
			whatMaterial = 2;
			meshMatNames.append(blinnShader.name());
		}
		else if (connections[0].node().hasFn(MFn::kPhong))
		{
			//MFnPhongShader phongShader(connections[0].node());
			phongShader.setObject(connections[0].node());
			MGlobal::displayInfo(phongShader.name());
			color = phongShader.color();
			whatMaterial = 3;
			meshMatNames.append(phongShader.name());
		}

		MObjectArray Files;
		MString filename;
		int pathSize;
		int texExist = 0;

		//Texture:
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
		unsigned int meshSize = vtxIDArray.length();
		if (meshSize > 0)
		{
			sm.msgHeader.type = TMeshCreate;
			if (texExist == 1)
			{
				sm.msgHeader.padding = slotSize - ((meshSize * sizeof(float)* 8) + sm.msgHeaderSize + sizeof(XMFLOAT4X4)+sizeof(int)+sizeof(MColor)+sizeof(int)+sizeof(int)+pathSize) % slotSize;
			}
			else
			{
				sm.msgHeader.padding = slotSize - ((meshSize * sizeof(float)* 8) + sm.msgHeaderSize + sizeof(XMFLOAT4X4)+sizeof(int)+sizeof(MColor)+sizeof(int)) % slotSize;
			}
			do
			{
				if (sm.cb->freeMem > ((meshSize * sizeof(float)* 8) - sm.msgHeaderSize - sizeof(XMFLOAT4X4)-sizeof(int)) + sm.msgHeader.padding)
				{
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

					//Texture do not exist
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
					sm.cb->freeMem -= (localHead - sm.cb->head) + sm.msgHeader.padding;
					sm.cb->head += (localHead - sm.cb->head) + sm.msgHeader.padding;

					break;
				}
			} while (sm.cb->freeMem > !((meshSize * sizeof(float)* 8) - sm.msgHeaderSize - sizeof(XMFLOAT4X4)-sizeof(int)) + sm.msgHeader.padding);
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
	else if (strstr(plug.partialName().asChar(), "pt["))
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
}

void VertexChanged(MPlug& plug)
{
	MFnMesh mesh(plug.node());
	MItMeshPolygon itPoly(mesh.object());

	MPointArray points;
	mesh.getPoints(points);
	sm.vertices.clear();

	MFloatVectorArray normals;
	mesh.getNormals(normals);
	sm.normal.clear();

	while (!itPoly.isDone())
	{
		// Vertices
		sm.vertices.push_back(XMFLOAT3(points[itPoly.vertexIndex(0)].x, points[itPoly.vertexIndex(0)].y, -points[itPoly.vertexIndex(0)].z));
		sm.vertices.push_back(XMFLOAT3(points[itPoly.vertexIndex(3)].x, points[itPoly.vertexIndex(3)].y, -points[itPoly.vertexIndex(3)].z));
		sm.vertices.push_back(XMFLOAT3(points[itPoly.vertexIndex(1)].x, points[itPoly.vertexIndex(1)].y, -points[itPoly.vertexIndex(1)].z));
		sm.vertices.push_back(XMFLOAT3(points[itPoly.vertexIndex(1)].x, points[itPoly.vertexIndex(1)].y, -points[itPoly.vertexIndex(1)].z));
		sm.vertices.push_back(XMFLOAT3(points[itPoly.vertexIndex(3)].x, points[itPoly.vertexIndex(3)].y, -points[itPoly.vertexIndex(3)].z));
		sm.vertices.push_back(XMFLOAT3(points[itPoly.vertexIndex(2)].x, points[itPoly.vertexIndex(2)].y, -points[itPoly.vertexIndex(2)].z));

		// Normals
		sm.normal.push_back(XMFLOAT3(normals[itPoly.normalIndex(0)].x, normals[itPoly.normalIndex(0)].y, -normals[itPoly.normalIndex(0)].z));
		sm.normal.push_back(XMFLOAT3(normals[itPoly.normalIndex(3)].x, normals[itPoly.normalIndex(3)].y, -normals[itPoly.normalIndex(3)].z));
		sm.normal.push_back(XMFLOAT3(normals[itPoly.normalIndex(1)].x, normals[itPoly.normalIndex(1)].y, -normals[itPoly.normalIndex(1)].z));
		sm.normal.push_back(XMFLOAT3(normals[itPoly.normalIndex(1)].x, normals[itPoly.normalIndex(1)].y, -normals[itPoly.normalIndex(1)].z));
		sm.normal.push_back(XMFLOAT3(normals[itPoly.normalIndex(3)].x, normals[itPoly.normalIndex(3)].y, -normals[itPoly.normalIndex(3)].z));
		sm.normal.push_back(XMFLOAT3(normals[itPoly.normalIndex(2)].x, normals[itPoly.normalIndex(2)].y, -normals[itPoly.normalIndex(2)].z));

		itPoly.next();
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
			sm.msgHeader.padding = slotSize - ((sizeof(XMFLOAT3)* sm.vertices.size()) * 2 + sm.msgHeaderSize + sizeof(int)) % slotSize;
			memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
			localHead += sm.msgHeaderSize;

			// Mesh index
			memcpy((char*)sm.buffer + localHead, &meshIndex, sizeof(int));
			localHead += sizeof(int);

			// Vertex data
			memcpy((char*)sm.buffer + localHead, sm.vertices.data(), sizeof(XMFLOAT3)* sm.vertices.size());
			localHead += sizeof(XMFLOAT3)* sm.vertices.size();

			// Normal data
			memcpy((char*)sm.buffer + localHead, sm.normal.data(), sizeof(XMFLOAT3)* sm.normal.size());
			localHead += sizeof(XMFLOAT3)* sm.normal.size();

			// Move header
			sm.cb->freeMem -= (localHead - sm.cb->head) + sm.msgHeader.padding;
			sm.cb->head += (localHead - sm.cb->head) + sm.msgHeader.padding;
			break;
		}
	} while (sm.cb->freeMem > !slotSize);
}

void NormalChanged(MPlug& plug)
{
	MFnMesh mesh(plug.node());
	MItMeshPolygon itPoly(mesh.object());
	MFloatVectorArray normals;
	mesh.getNormals(normals);
	sm.normal.clear();

	while (!itPoly.isDone())
	{
		// Normals
		sm.normal.push_back(XMFLOAT3(normals[itPoly.normalIndex(0)].x, normals[itPoly.normalIndex(0)].y, -normals[itPoly.normalIndex(0)].z));
		sm.normal.push_back(XMFLOAT3(normals[itPoly.normalIndex(3)].x, normals[itPoly.normalIndex(3)].y, -normals[itPoly.normalIndex(3)].z));
		sm.normal.push_back(XMFLOAT3(normals[itPoly.normalIndex(1)].x, normals[itPoly.normalIndex(1)].y, -normals[itPoly.normalIndex(1)].z));
		sm.normal.push_back(XMFLOAT3(normals[itPoly.normalIndex(1)].x, normals[itPoly.normalIndex(1)].y, -normals[itPoly.normalIndex(1)].z));
		sm.normal.push_back(XMFLOAT3(normals[itPoly.normalIndex(3)].x, normals[itPoly.normalIndex(3)].y, -normals[itPoly.normalIndex(3)].z));
		sm.normal.push_back(XMFLOAT3(normals[itPoly.normalIndex(2)].x, normals[itPoly.normalIndex(2)].y, -normals[itPoly.normalIndex(2)].z));

		itPoly.next();
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
			sm.msgHeader.type = TNormalUpdate;
			sm.msgHeader.padding = slotSize - ((sizeof(XMFLOAT3)* sm.vertices.size()) + sm.msgHeaderSize + sizeof(int)) % slotSize;
			memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
			localHead += sm.msgHeaderSize;

			// Mesh index
			memcpy((char*)sm.buffer + localHead, &meshIndex, sizeof(int));
			localHead += sizeof(int);

			// Normal data
			memcpy((char*)sm.buffer + localHead, sm.vertices.data(), sizeof(XMFLOAT3)* sm.vertices.size());
			localHead += sizeof(XMFLOAT3)* sm.vertices.size();

			// Move header
			sm.cb->freeMem -= (localHead - sm.cb->head) + sm.msgHeader.padding;
			sm.cb->head += (localHead - sm.cb->head) + sm.msgHeader.padding;
			break;
		}
	} while (sm.cb->freeMem > !slotSize);
}

void UVChanged(MPlug& plug)
{
	MFnMesh mesh(plug.node());
	MItMeshPolygon itPoly(mesh.object());
	float2 uv;
	int uvIndex;
	sm.uv.clear();

	while (!itPoly.isDone())
	{
		// UV:s
		itPoly.getUV(0, uv);
		sm.uv.push_back(XMFLOAT2(uv[0], 1 - uv[1]));
		itPoly.getUV(3, uv);
		sm.uv.push_back(XMFLOAT2(uv[0], 1 - uv[1]));
		itPoly.getUV(1, uv);
		sm.uv.push_back(XMFLOAT2(uv[0], 1 - uv[1]));
		itPoly.getUV(1, uv);
		sm.uv.push_back(XMFLOAT2(uv[0], 1 - uv[1]));
		itPoly.getUV(3, uv);
		sm.uv.push_back(XMFLOAT2(uv[0], 1 - uv[1]));
		itPoly.getUV(2, uv);
		sm.uv.push_back(XMFLOAT2(uv[0], 1 - uv[1]));

		itPoly.next();
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
			sm.msgHeader.type = TUVUpdate;
			sm.msgHeader.padding = slotSize - ((sizeof(XMFLOAT2)* sm.uv.size()) + sm.msgHeaderSize + sizeof(int)) % slotSize;
			memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
			localHead += sm.msgHeaderSize;

			// Mesh index
			memcpy((char*)sm.buffer + localHead, &meshIndex, sizeof(int));
			localHead += sizeof(int);

			// UV data
			memcpy((char*)sm.buffer + localHead, sm.uv.data(), sizeof(XMFLOAT2)* sm.uv.size());
			localHead += sizeof(XMFLOAT2)* sm.uv.size();

			// Move header
			sm.cb->freeMem -= (localHead - sm.cb->head) + sm.msgHeader.padding;
			sm.cb->head += (localHead - sm.cb->head) + sm.msgHeader.padding;
			break;
		}
	} while (sm.cb->freeMem > !slotSize);
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
			//MFnLambertShader lambertShader(connections[0].node());
			lambertShader.setObject(connections[0].node());
			MGlobal::displayInfo(lambertShader.name());
			color = lambertShader.color();
			whatMaterial = 1;
			meshMatNames[localMesh] = lambertShader.name();
		}
		else if (connections[0].node().hasFn(MFn::kBlinn))
		{
			//MFnBlinnShader blinnShader(connections[0].node());
			blinnShader.setObject(connections[0].node());
			MGlobal::displayInfo(blinnShader.name());
			color = blinnShader.color();
			whatMaterial = 2;
			meshMatNames[localMesh] = blinnShader.name();
		}
		else if (connections[0].node().hasFn(MFn::kPhong))
		{
			//MFnPhongShader phongShader(connections[0].node());
			phongShader.setObject(connections[0].node());
			MGlobal::displayInfo(phongShader.name());
			color = phongShader.color();
			whatMaterial = 3;
			meshMatNames[localMesh] = phongShader.name();
		}

		MObjectArray Files;
		MString filename;
		int pathSize;
		int texExist = 0;

		//Texture:
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
			if (sm.cb->freeMem >= slotSize)
			{
				localHead = sm.cb->head;

				// Message header
				sm.msgHeader.type = TMaterialUpdate;
				if (texExist == 0)
				{
					sm.msgHeader.padding = slotSize - (sizeof(MColor)+sizeof(int)+sizeof(int)) % slotSize;
				}
				else
				{
					sm.msgHeader.padding = slotSize - (sizeof(MColor)+sizeof(int)+sizeof(int)+pathSize + sizeof(int)) % slotSize;
				}
				memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
				localHead += sm.msgHeaderSize;

				// Mesh ID
				memcpy((char*)sm.buffer + localHead, &localMesh, sizeof(int));
				localHead += sizeof(int);

				// Material data
				memcpy((char*)sm.buffer + localHead, &color, sizeof(MColor));
				localHead += sizeof(MColor);

				//Texture do not exist
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
				sm.cb->freeMem -= (localHead - sm.cb->head) + sm.msgHeader.padding;
				sm.cb->head += (localHead - sm.cb->head) + sm.msgHeader.padding;
				break;
			}
		} while (sm.cb->freeMem > !slotSize);
	}
}

void ShaderChangedCB(MObject& object, void* clientData)
{
	MFnDependencyNode Shadername(object);
	if (matNames[matNames.length() - 1] != Shadername.name())
	{
		matNames.append(Shadername.name());
		MGlobal::displayInfo("Shader name: " + matNames[matNames.length() - 1]);
	}
	callbackIds.append(MNodeMessage::addAttributeChangedCallback(object, shaderchanged));
}

void shaderchanged(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData)
{
	// WE GET FILE BUT WHEN WE ADD A IMAGE NAME IT WON'T REGISTER
	if ((msg & MNodeMessage::kAttributeSet) || msg & MNodeMessage::kConnectionMade)
	{
		//meshNames.append(mesh.name());
		MFnLambertShader lambertShader;
		MFnBlinnShader blinnShader;
		MFnPhongShader phongShader;
		MColor color;
		int whatMaterial = 0;

		//int localMesh = meshNames.length();
		//for (int i = 0; i < meshNames.length(); i++)
		//{
		//	if (mesh.name() == meshNames[i])
		//	{
		//		localMesh = i;
		//	}
		//}

		// Find the material and then color
		if (plug.node().hasFn(MFn::kLambert))
		{
			//MFnLambertShader lambertShader(connections[0].node());
			lambertShader.setObject(plug.node());
			MGlobal::displayInfo(lambertShader.name());
			color = lambertShader.color();
			whatMaterial = 1;
		}
		else if (plug.node().hasFn(MFn::kBlinn))
		{
			//MFnBlinnShader blinnShader(connections[0].node());
			blinnShader.setObject(plug.node());
			MGlobal::displayInfo(blinnShader.name());
			color = blinnShader.color();
			whatMaterial = 2;
		}
		else if (plug.node().hasFn(MFn::kPhong))
		{
			//MFnPhongShader phongShader(connections[0].node());
			phongShader.setObject(plug.node());
			MGlobal::displayInfo(phongShader.name());
			color = phongShader.color();
			whatMaterial = 3;
		}

		MFnDependencyNode matNamer(plug.node());
		MObjectArray Files;
		MString filename;
		int pathSize;
		int texExist = 0;

		//Texture:
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
						localHead = sm.cb->head;

						// Message header
						sm.msgHeader.type = TMaterialUpdate;
						if (texExist == 0)
						{
							sm.msgHeader.padding = slotSize - (sizeof(MColor)+sizeof(int)+sizeof(int)) % slotSize;
						}
						else
						{
							sm.msgHeader.padding = slotSize - (sizeof(MColor)+sizeof(int)+sizeof(int)+pathSize + sizeof(int)) % slotSize;
						}
						memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
						localHead += sm.msgHeaderSize;

						// Mesh ID
						memcpy((char*)sm.buffer + localHead, &i, sizeof(int));
						localHead += sizeof(int);

						// Material data
						memcpy((char*)sm.buffer + localHead, &color, sizeof(MColor));
						localHead += sizeof(MColor);

						//Texture do not exist
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
						sm.cb->freeMem -= (localHead - sm.cb->head) + sm.msgHeader.padding;
						sm.cb->head += (localHead - sm.cb->head) + sm.msgHeader.padding;
						break;
					}
				} while (sm.cb->freeMem > !slotSize);
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

		if (transform.isParentOf(camera.object()))
			CameraChanged(transform, camera);
		else
		{

			MStatus res;

			unsigned int localMesh = INT_MAX;

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

			DirectX::XMVECTOR translationV = XMVectorSet(translation.x, translation.y, -translation.z, 1.0f);
			DirectX::XMVECTOR rotationV = XMVectorSet(rotation[0], rotation[1], rotation[2], 0.0f);
			DirectX::XMVECTOR scaleV = XMVectorSet(scale[0], scale[1], scale[2], 0.0f);
			XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
			XMFLOAT4X4 matrixData;
			XMStoreFloat4x4(&matrixData, XMMatrixTranspose(XMMatrixAffineTransformation(scaleV, translationV, rotationV, translationV)));

			//DirectX::XMMATRIX translationM = DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z);
			//DirectX::XMMATRIX rotationXM = DirectX::XMMatrixRotationX(rx);
			//DirectX::XMMATRIX rotationYM = DirectX::XMMatrixRotationX(ry);
			//DirectX::XMMATRIX rotationZM = DirectX::XMMatrixRotationX(rz);
			//DirectX::XMMATRIX scalingM = DirectX::XMMatrixScaling(scale[0], scale[1], scale[2]);

			//XMFLOAT4X4 matrixData;
			//XMStoreFloat4x4(&matrixData, (scalingM * rotationXM * rotationYM * rotationZM * translationM));

			//do
			//{
			//	if (sm.cb->freeMem > slotSize)
			//	{
			localHead = sm.cb->head;
			// Message header
			sm.msgHeader.type = TtransformUpdate;
			sm.msgHeader.padding = slotSize - sm.msgHeaderSize - sizeof(int)-sizeof(XMFLOAT4X4);
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
	//MIRRORING CAMERA AND PROJECTION NOT REALLY WORKING AS INTENDED!
	MPoint eye = camera.eyePoint(MSpace::kWorld);
	MVector viewDirection = camera.viewDirection(MSpace::kWorld);
	MVector upDirection = camera.upDirection(MSpace::kWorld);
	eye.z = -eye.z;
	viewDirection.z = -viewDirection.z;
	viewDirection += eye;

	XMFLOAT4X4 viewMatrix;
	XMStoreFloat4x4(&viewMatrix, XMMatrixTranspose(DirectX::XMMatrixLookAtLH(
		XMVectorSet(eye.x, eye.y, eye.z, 1.0f),
		XMVectorSet(viewDirection.x, viewDirection.y, viewDirection.z, 0.0f),
		XMVectorSet(upDirection.x, upDirection.y, -upDirection.z, 0.0f))));

	//Projection Matrix
	MFloatMatrix floatProject = camera.projectionMatrix().transpose();
	float pm[4][4];
	floatProject.get(pm);
	XMFLOAT4X4 projectMatrix;
	projectMatrix._11 = pm[0][0];
	projectMatrix._12 = pm[0][1];
	projectMatrix._13 = pm[0][2];
	projectMatrix._14 = pm[0][3];

	projectMatrix._21 = pm[1][0];
	projectMatrix._22 = pm[1][1];
	projectMatrix._23 = pm[1][2];
	projectMatrix._24 = pm[1][3];

	projectMatrix._31 = pm[2][0];
	projectMatrix._32 = pm[2][1];
	projectMatrix._33 = pm[2][2];
	projectMatrix._34 = -pm[2][3];

	projectMatrix._41 = pm[3][0];
	projectMatrix._42 = pm[3][1];
	projectMatrix._43 = -pm[3][2];
	projectMatrix._44 = pm[3][3];

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

			//// View matrix
			//memcpy((char*)sm.buffer + localHead, &camTranslations, sizeof(MVector));
			//localHead += sizeof(MVector);
			//memcpy((char*)sm.buffer + localHead, &viewDirection, sizeof(MVector));
			//localHead += sizeof(MVector);
			//memcpy((char*)sm.buffer + localHead, &upDirection, sizeof(MVector));
			//localHead += sizeof(MVector);

			// View matrix
			memcpy((char*)sm.buffer + localHead, &viewMatrix, sizeof(XMFLOAT4X4));
			localHead += sizeof(XMFLOAT4X4);

			// Projection matrix
			memcpy((char*)sm.buffer + localHead, &projectMatrix, sizeof(XMFLOAT4X4));
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
	callbackIds.append(MNodeMessage::addAttributeChangedCallback(light.object(), LightChangedCB));
	MObject dad = light.parent(0);
	callbackIds.append(MNodeMessage::addAttributeChangedCallback(dad, LightChangedCB));

	MColor color;
	MVector position;

	float r, b, g, a;

	MStatus status;
	if (dad.hasFn(MFn::kTransform))
	{
		MFnTransform lightPoint(dad);
		position = lightPoint.getTranslation(MSpace::kObject, &status);
	}
	color = light.color();
	r = color.r;
	b = color.b;
	g = color.g;
	a = color.a;

	MGlobal::displayInfo(MString() + "Color: " + r + " " + b + " " + g + " " + a);
	MGlobal::displayInfo(MString() + "Position: " + position.x + " " + position.y + " " + position.z);
	XMFLOAT3 positionF(position.x, position.y, -position.z);

	do
	{
		if (sm.cb->freeMem > slotSize)
		{
			localHead = sm.cb->head;
			// Message header
			sm.msgHeader.type = TLightCreate;
			sm.msgHeader.padding = slotSize - sm.msgHeaderSize - sizeof(MColor)-sizeof(XMFLOAT3);
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
	} while (sm.cb->freeMem > !slotSize);
}

void LightCreationCB(MObject& lightObject, void* clientData)
{
	MFnLight light(lightObject);
	lightNames.append(light.name());

	callbackIds.append(MNodeMessage::addAttributeChangedCallback(lightObject, LightChangedCB));
	MObject dad = light.parent(0);
	callbackIds.append(MNodeMessage::addAttributeChangedCallback(dad, LightChangedCB));

	MColor color;
	MVector position;

	float r, b, g, a;

	MStatus status;
	if (dad.hasFn(MFn::kTransform))
	{
		MFnTransform lightPoint(dad);
		position = lightPoint.getTranslation(MSpace::kObject, &status);
	}
	color = light.color();
	r = color.r;
	b = color.b;
	g = color.g;
	a = color.a;

	MGlobal::displayInfo(MString() + "Color: " + r + " " + b + " " + g + " " + a);
	MGlobal::displayInfo(MString() + "Position: " + position.x + " " + position.y + " " + position.z);
	XMFLOAT3 positionF(position.x, position.y, -position.z);

	do
	{
		if (sm.cb->freeMem > slotSize)
		{
			localHead = sm.cb->head;
			// Message header
			sm.msgHeader.type = TLightCreate;
			sm.msgHeader.padding = slotSize - sm.msgHeaderSize - sizeof(MColor)-sizeof(XMFLOAT3);
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
	} while (sm.cb->freeMem > !slotSize);
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

			positionF = XMFLOAT3(position.x, position.y, -position.z);
			color = light.color();
		}

		MGlobal::displayInfo(MString() + "Position: " + positionF.x + " " + positionF.y + " " + positionF.z);
		MGlobal::displayInfo(MString() + "Color: " + color.r + " " + color.g + " " + color.b + " " + color.a);

		unsigned int lightIndex = INT_MAX;
		for (size_t i = 0; i < lightNames.length(); i++)
		{
			MGlobal::displayInfo(lightNames[i]);
			MGlobal::displayInfo(light.name());
			if (lightNames[i] == light.name())
			{
				lightIndex = i;
			}
		}
		lightIndex = 0; // Remove this later to support multiple lights

		do
		{
			if (sm.cb->freeMem > slotSize)
			{
				localHead = sm.cb->head;
				// Message header
				unsigned int localLight = 0;
				sm.msgHeader.type = TLightUpdate;
				sm.msgHeader.padding = slotSize - sm.msgHeaderSize - sizeof(MColor)-sizeof(XMFLOAT3);
				memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
				localHead += sm.msgHeaderSize;

				// Light index
				memcpy((char*)sm.buffer + localHead, &lightIndex, sizeof(int));
				localHead += sizeof(int);

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
		} while (sm.cb->freeMem > !slotSize);
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
	localHead = sm.cb->head;
	if (object.hasFn(MFn::kMesh))
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


		// Message header
		sm.msgHeader.type = TMeshDestroyed;
		sm.msgHeader.padding = slotSize - sm.msgHeaderSize - sizeof(int);
		memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
		localHead += sm.msgHeaderSize;

		// Node index
		memcpy((char*)sm.buffer + localHead, &meshIndex, sizeof(int));
		localHead += sizeof(int);
	}
	else if (object.hasFn(MFn::kLight))
	{
		MFnLight light(object);

		// Message header
		sm.msgHeader.type = TLightDestroyed;
		sm.msgHeader.padding = slotSize - sm.msgHeaderSize - sizeof(int);
		memcpy((char*)sm.buffer + localHead, &sm.msgHeader, sm.msgHeaderSize);
		localHead += sm.msgHeaderSize;
	}

	// Move header
	sm.cb->freeMem -= slotSize;
	sm.cb->head += slotSize;
}