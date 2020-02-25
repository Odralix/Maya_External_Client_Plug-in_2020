// Original Example expanded upon by Ossian Edstr�m
#include "maya_includes.h"
#include <maya/MTimer.h>
#include <iostream>
#include <algorithm>
#include <queue>
//#include <string>
#include <map>
#include "ComLib_reference.h"
#include <maya/MCameraMessage.h>
//#include "../Structs.h"
#include "MayaBatchOutput.h"
#include <maya/MItSelectionList.h>

using namespace std;
MCallbackIdArray callbackIdArray;
MObject m_node;
MStatus status = MS::kSuccess;
bool initBool = false;

enum NODE_TYPE { TRANSFORM, MESH };
MTimer gTimer;

ComLib producer((std::string)"stuff", 200*1024*1024, ComLib::PRODUCER);
MString lastName = "0";

// keep track of created meshes to maintain them
std::map<std::string, int> mapOfVertexArrays;
std::map<std::string, MCallbackId> deletedCallbackArray;
MayaBatchOutput batch;

bool isExtruding = false;

MString getParentDagNodeName(MObject obj)
{
	//Note, the name is technically the transform and not the mesh.
	MFnDagNode dagSearch(obj);
	MObject handle = dagSearch.parent(0);
	MFnDagNode parent(handle);

	return parent.name();
}

void SendTransform(MObject transformNode)
{
	MFnDependencyNode nameFetch(transformNode);

	//Global Transform:
	MFnDagNode path(transformNode);
	MDagPath tNodeDag;

	path.getPath(tNodeDag);
	//Get the matrix with respect to parent transforms.
	MMatrix worldMat = tNodeDag.inclusiveMatrix();

	const MTransformationMatrix aMat(worldMat);

	MVector trans = aMat.getTranslation(MSpace::kWorld, &status);
	double transDouble[3];
	trans.get(transDouble);

	double scaleDouble[3];
	aMat.getScale(scaleDouble, MSpace::kWorld);

	double quatDouble[4];
	aMat.getRotationQuaternion(quatDouble[0], quatDouble[1], quatDouble[2], quatDouble[3]);

	//Prepare to store the whole transform somewhere.
	double transform[10];

	for (int i = 0; i < 3; i++)
	{
		transform[i] = transDouble[i];
		transform[i + 3] = scaleDouble[i];
	}

	for (int i = 6; i < 10; i++)
	{
		transform[i] = quatDouble[i - 6];
	}

	//Prepare name for sending.
	int len = 0;
	const char* name = nameFetch.name().asChar(len);

	TransHeader head;
	for (int i = 0; i < len; i++)
	{
		head.name[i] = name[i];
	}
	//Send transform to batch.
	batch.SetTransform(head, transform);

	//Ensure Children in hierarchy gets updated too.
	for (int i = 0; i < tNodeDag.childCount(); i++)
	{
		MObject child = tNodeDag.child(i);

		/*cout << "trans child: " << child.apiTypeStr() << endl;*/

		if (child.hasFn(MFn::kTransform))
		{
			SendTransform(child);
		}
	}
}

void nodeTransformChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	MFnDependencyNode nameFetch(plug.node());

	if (msg & MNodeMessage::kAttributeSet)
	{
		MObject transformNode = plug.node();

		if (transformNode.hasFn(MFn::kTransform))
		{
			SendTransform(transformNode);
		}
	}
}

void SendMaterial(MObject &node)
{
	MPlug colPlug;
	MPlug ambColPlug;
	MFnDependencyNode funcMat(node);

	//cout  << "Name: " << funcMat.name() << endl;
	colPlug = funcMat.findPlug("color", true, &status);
	ambColPlug = funcMat.findPlug("ambc", true, &status);
	//cout  << colPlug.name() << endl;
	//cout  << "" << endl;
	//cout  << ambColPlug.name() << endl;
	//cout  << endl;

	//Check if a plug has textures
	MPlugArray plugs;
	colPlug.connectedTo(plugs, true, false);
	if (plugs.length() != 0)
	{
		MString textureName = "";
		for (int j = 0; j < plugs.length(); j++)
		{
			if (plugs[j].node().apiType() == MFn::kFileTexture)
			{
				//cout  << "hit " << j << endl;
				MFnDependencyNode dep(plugs[j].node());
				MPlug ftn = dep.findPlug("ftn");
				//cout  << "plugName: " << ftn.name().asChar() << endl;
				ftn.getValue(textureName);
				//cout  << textureName << endl;
				if (textureName != "")
				{
					batch.SetMaterial((std::string)funcMat.name().asChar(), (std::string)textureName.asChar());
				}
				else
				{
					MColor color;
					for (int j = 0; j < colPlug.numChildren(); j++)
					{
						/*//cout  << colPlug.child(j).name().asChar() << endl;*/
						colPlug.child(j, &status).getValue(color[j]);
					}

					//cout  << "R: " << color.r << endl;
					//cout  << "G: " << color.g << endl;
					//cout  << "B: " << color.b << endl;

					float rgb[3];
					rgb[0] = color.r;
					rgb[1] = color.g;
					rgb[2] = color.b;

					batch.SetMaterial((std::string)funcMat.name().asChar(), rgb, 3);
				}
			}
		}
	}
	else
	{
		MColor color;
		for (int j = 0; j < colPlug.numChildren(); j++)
		{
			/*//cout  << colPlug.child(j).name().asChar() << endl;*/
			colPlug.child(j, &status).getValue(color[j]);
		}

		//cout  << "R: " << color.r << endl;
		//cout  << "G: " << color.g << endl;
		//cout  << "B: " << color.b << endl;
		
		float rgb[3];
		rgb[0] = color.r;
		rgb[1] = color.g;
		rgb[2] = color.b;

		/*batch.SetMaterial((std::string)funcMat.name().asChar(), rgb, 3);*/
		/*//cout  << "A: " << color.a << endl;*/
	}

	//MColor color;
	//for (int j = 0; j < ambColPlug.numChildren(); j++)
	//{
	//	//cout  << ambColPlug.child(j).name().asChar() << endl;
	//	ambColPlug.child(j, &status).getValue(color[j]);
	//}

	////cout  << "ambcR: " << color.r << endl;
	////cout  << "ambcG: " << color.g << endl;
	////cout  << "ambcB: " << color.b << endl;
}

// Must be sent with node as a mesh that is already in the MayaBatchOutput.
// Thus should be used ASAP after SendMesh function
void SendMaterialName(MObject &node)
{
	MFnMesh mesh(node);

	//nr of parents
	int numParents = mesh.parentCount();
	/*//cout  << "Number of Parents: " << numParents << endl;*/
	//cout  << "Entered SendMaterialName" << endl;

	//Loop through
	for (int i = 0; i < numParents; i++)
	{
		/*MFnDependencyNode pNode(mesh.parent(i));*/

		MObjectArray shaders;

		MIntArray indiciesPerFace;

		mesh.getConnectedShaders(i, shaders, indiciesPerFace);

		if (shaders.length() == 0)
		{
			//cout  << "MESH " << mesh.name().asChar() << " HAS NO SHADER" << endl;
		}
		else
		{
			MFnDependencyNode func(shaders[0]);

			MPlug surfShader = func.findPlug("surfaceShader");

			MPlugArray mats;

			//Get the actual materials
			surfShader.connectedTo(mats, true, false);

			if (mats.length())
			{
				MFnDependencyNode funcMat(mats[0].node());
				//cout  << "Material Name: " << funcMat.name().asChar() << endl;

				batch.SetMatSwitched((std::string)getParentDagNodeName(node).asChar(), (std::string) funcMat.name().asChar());
			}
		}
	}
}

void SendMaterialNameMatInput(MObject &node)
{
	MFnDependencyNode mat(node);
	//cout  << "ENTERED MATERIAL NAME INPUT" << endl;
	//cout  << mat.name() << endl;

	MPlugArray arr;
	mat.findPlug("oc").connectedTo(arr, false, true);

	if (arr.length() != 0)
	{
		//cout  << arr.length() << endl;
		//cout  << "Entered" << endl;
		for (int i = 0; i < arr.length(); i++)
		{
			MFnDependencyNode shadingGroup(arr[i].node());

			MPlug dagSetMembers;
			dagSetMembers = shadingGroup.findPlug("dsm");

			////cout  << "Numchildren: " << dagSetMembers.numChildren() << endl;
			////cout  << "NumElements: " << dagSetMembers.numElements() << endl;
			for (int j = 0; j < dagSetMembers.numElements(); j++)
			{
				MPlugArray meshPlugs;
				dagSetMembers[j].connectedTo(meshPlugs, true, false);
				MFnDependencyNode mesh(meshPlugs[0].node());

				batch.SetMatSwitched((std::string)getParentDagNodeName(meshPlugs[0].node()).asChar(), (std::string) mat.name().asChar());
			}
		}
	}
}

void SetupMaterials(MObject &node)
{
	MFnMesh mesh(node);

	//nr of parents
	int numParents = mesh.parentCount();
	/*//cout  << "Number of Parents: " << numParents << endl;*/
	//cout  << "Entered SetupMaterials" << endl;

	//Loop through
	for (int i = 0; i < numParents; i++)
	{
		MFnDependencyNode node(mesh.parent(i));

		MObjectArray shaders;

		MIntArray indiciesPerFace;

		mesh.getConnectedShaders(i, shaders, indiciesPerFace);

		if (shaders.length() == 0)
		{
			//cout  << "MESH " << mesh.name().asChar() << " HAS NO SHADER" << endl;
		}
		else
		{
			MFnDependencyNode func(shaders[0]);

			MPlug surfShader = func.findPlug("surfaceShader");

			MPlugArray mats;

			//Get the actual materials
			surfShader.connectedTo(mats, true, false);

			if (mats.length())
			{
				MFnDependencyNode funcMat(mats[0].node());
				//cout  <<"Material Name: " << funcMat.name().asChar() << endl;
				if (mats[0].node().hasFn(MFn::kLambert))
				{
					MPlug plug;
					plug = funcMat.findPlug("color", true, &status);

					MColor color;

					//cout  << plug.name() << endl;
					//cout  << "NumChildren: "<< plug.numChildren() << endl;

					for(int j = 0; j< plug.numChildren(); j++)
					{
						//cout  << plug.child(j).name().asChar() << endl;
						plug.child(j, &status).getValue(color[j]);
					}

					//Get plugs connected to color
					MPlugArray plugs;
					plug.connectedTo(plugs, true, false);
					MString textureName = "";
					for (int j = 0; j < plugs.length(); j++)
					{
						if (plugs[j].node().apiType() == MFn::kFileTexture)
						{
							MFnDependencyNode dep(plugs[j].node());
							MPlug ftn = dep.findPlug("ftn");
							//cout  << "plugName: " << ftn.name().asChar() << endl;
							ftn.getValue(textureName);
						}
					}

					if (textureName == "")
					{
						//cout  << "R: " << color.r << endl;
						//cout  << "G: " << color.g << endl;
						//cout  << "B: " << color.b << endl;
					/*	//cout  << "A: " << color.a << endl;*/
						float rgb[3];
						rgb[0] = color.r;
						rgb[1] = color.g;
						rgb[2] = color.b;
						/*batch.SetMaterial((std::string)funcMat.name().asChar(), rgb,3);*/
					}
					else
					{
						//cout  << "Texture: " << textureName << endl;
						/*batch.SetMaterial((std::string)funcMat.name().asChar(), (std::string)textureName.asChar());*/
					}

		/*			plug.getValue(color.r);
					//cout  <<"R: " << color.r << endl;*/

					/*//cout  << rgb.length() << endl;
					for (int j = 0; j < rgb.length(); j++)
					{
						//cout  << rgb[j].name().asChar() << endl;
					}*/

					/*//cout  << plug.*/
				}
				else
				{
					//"special" material type like Stingray PBS or something else not deferred from lambert.
					// Doesn't have the standard "color" plug and should be handled differently here.
				}
			}
			else
			{
				//cout  << "MESH " << mesh.name().asChar() << "HAS NO MATS" << endl;
			}
		}
	}
}

//For use only on MObjects with a connected MFn::kMesh.
void SendMesh(MObject Mnode)
{
	if (Mnode.hasFn(MFn::kMesh))
	{
		MFnDependencyNode meshNode(Mnode);
		MFnMesh inputMesh(Mnode);

		MFloatPointArray vertexArr;
		inputMesh.getPoints(vertexArr);

		const char* temp = meshNode.name().asChar();
		string meshName(temp);

		//Note, the name is technically the transform and not the mesh.
		MFnDagNode dagSearch(Mnode);
		MObject handle = dagSearch.parent(0);

		//Replace old vertex array and print the new info out.
		mapOfVertexArrays[meshName] = vertexArr.length();
		
		//Getting the number of verts
		MeshHeader meshHead;

		meshHead.nrOfVerts = inputMesh.numFaceVertices(&status);

		//Enter the name.
		for (int i = 0; i < meshNode.name().length(); i++)
		{
			meshHead.meshName[i] = getParentDagNodeName(Mnode).asChar()[i];
		}

		//In certain cases such as when a mesh is extruded or undo-ed the transform must be sent as well.
		SendTransform(handle);

		//Getting the Index count
		MIntArray triCounts;
		MIntArray triVerts;

		inputMesh.getTriangleOffsets(triCounts, triVerts);

		meshHead.indexCount = triVerts.length();

		//Put indicies into an int-array
		int * triIndicies = new int[meshHead.indexCount];
		triVerts.get(triIndicies);

		//for (int i = 0; i < meshHead.indexCount; i++)
		//{
		//	//cout  << triIndicies[i] << " ";
		//}
		////cout  << endl;

		int numFaceVertices = meshHead.nrOfVerts;

		float ** posArr = new float*[numFaceVertices];
		for (int i = 0; i < numFaceVertices; i++)
		{
			posArr[i] = new float[4];
		}

		float * normArr = new float[numFaceVertices * 3];
		float * UVArr = new float[numFaceVertices * 2];
		int nCount = 0;
		int uvCount = 0;

		MVector normals;
		int i = 0;
		MItMeshFaceVertex iterator(Mnode, &status);
		while (!iterator.isDone())
		{
			iterator.position(MSpace::kWorld, &status).get(posArr[i]);
			iterator.getNormal(normals, MSpace::kObject);
			float2 temp;
			iterator.getUV(temp);

			double get[3];
			normals.get(get);
			for (int j = 0; j < 3; j++)
			{
				/*normArr[i*3+j];*/
				normArr[nCount] = get[j];
				/*//cout  << normArr[nCount] << " ";*/
				nCount++;
			}
			// Note: These are just initial UVs. There should technically be a callback for when the UVs change.
			for (int j = 0; j < 2; j++)
			{
				UVArr[uvCount] = temp[j];
				/*	//cout  << UVArr[uvCount] << " ";*/
				uvCount++;
			}

			i++;
			iterator.next();
		}

		//3+3+2 = 8 values per vert.
		float * verts = new float[numFaceVertices * 8];
		int count = 0;
		int count2 = 0;
		int count3 = 0;

		//cout  << numFaceVertices << endl;

		for (int i = 0; i < numFaceVertices; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				verts[count] = posArr[i][j];
				count++;
			}

			for (int j = 0; j < 3; j++)
			{
				verts[count] = normArr[count2];
				count2++;
				count++;
			}

			for (int j = 0; j < 2; j++)
			{
				verts[count] = UVArr[count3];
				count3++;
				count++;
			}
		}

		//Sending the collected information
		batch.SetMesh(meshHead, triIndicies, verts);

		for (int i = 0; i < numFaceVertices; i++)
		{
			delete[] posArr[i];
		}
		delete[] posArr;
		delete[] UVArr;
		delete[] normArr;
		delete[] verts;
	}
	else
	{
	//cout  << "**********************************************************************************" << endl;
	//cout  << "*********************** CRASH SAFETY NET WARNING!! *******************************" << endl;
	//cout  << " ATTEMPTED TO CALL FUNCTION 'SetMesh' ON A MObject WITHOUT 'MFn::kMesh'." << endl;
	//cout  << "************************* FUNCTION CALL IGNORED **********************************" << endl;
	//cout  << "**********************************************************************************" << endl;
	}
}

//Variable for Determining if outmesh call is a vertchange
bool isVertChange = false;
std::vector<int> changedVerts;

void nodeMeshAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	cout  << plug.name() << ": " << plug.partialName() << endl;

	// For individually or soft select moved verticies. 
	if (msg & MNodeMessage::kAttributeSet)
	{
		/*MObject transformNode = plug.node();*/

		if (plug.isElement())
		{
			/*if (changedVerts.size() == changedVerts.capacity())
			{
				//cout  << "Add size" << endl;
				changedVerts.reserve(changedVerts.capacity() * 3);
			}
			
			changedVerts.emplace_back(plug.logicalIndex());
			if (isVertChange == false)
			{
				isVertChange = true;
			}*/
			/*//cout  << "Entered plug change" << endl;*/
			//Note, the name is technically the transform and not the mesh.

			//////cout  << plug.partialName() << " Was changed! The vertex values are now: " << endl;

			//// As we already have the vertID I feel like the face verticies should be accesable through the control point.
			//// However as MPoint has no such functionality this was the best I could figure out.
			////MItMeshFaceVertex iterator(plug.node(), &status);

			MFnMesh mesh(plug.node());
			int faceVertexId;
			MItMeshVertex vertIt(plug.node());

			//Note that we don't actually care about the previous index and thus assign it to faceVertexId temporarily.
			vertIt.setIndex(plug.logicalIndex(), faceVertexId);

			MIntArray conFaces;
			vertIt.getConnectedFaces(conFaces);

			//No need to take the position of each face vert individually as they have identical position.
			MPoint point;
			mesh.getPoint(plug.logicalIndex(), point);
			float pos[4];
			point.get(pos);
		/*	//cout  << conFaces.length() << endl;*/
			//Still need to send the position to the value on each face vert.
			for (int i = 0; i < conFaces.length(); i++)
			{
				mesh.getFaceVertexBlindDataIndex(conFaces[i], plug.logicalIndex(), faceVertexId);
				batch.SetVertPos((std::string)getParentDagNodeName(plug.node()).asChar(), faceVertexId, pos);
			}
		}
	}
	// For actual changes to topology
	else if (msg & MNodeMessage::kAttributeEval && plug.partialName() == "o")
	{
		MFnDependencyNode meshNode(plug.node());
		MFnMesh inputMesh(plug.node());

		MObject Mnode = plug.node();
		cout  << "entered Topology change" << endl;

		MFloatPointArray vertexArr;

		inputMesh.getPoints(vertexArr);

		const char* temp = meshNode.name().asChar();

		string meshName(temp);

		int arrLen = mapOfVertexArrays.find(meshName)->second;

		// If the lengths are different between the vertex arrays the topology has changed in a way
		// That won't be caught by the plug.isElement() check.
		// Since we only check the length updating the map in the other check is currently irrelevant.
		if (arrLen != vertexArr.length())
		{
			SendMesh(Mnode);
			SendMaterialName(Mnode);
		}
		else if (isExtruding == true)
		{
			MSelectionList selected;
			MGlobal::getActiveSelectionList(selected);
			////cout  << "------IS EXTRUDING-------" << endl;
			////cout  << "Selected length: " << selected.length() << endl;

			MItSelectionList selList(selected);

			MObject obj;
			MDagPath path;
			while (!selList.isDone())
			{
				selList.getDagPath(path, obj);

				////cout  << obj.apiTypeStr() << endl;
				////cout  << path.fullPathName() << endl;
				if (obj.hasFn(MFn::kMeshPolygonComponent))
				{
					MFnMesh mesh(path);

					MItMeshPolygon itPoly(path, obj);
					while (!itPoly.isDone())
					{

						int faceVertexID;
						MIntArray faceIds;
						itPoly.getConnectedFaces(faceIds);

						faceIds.append(itPoly.index());

						//For this and each connected face
						for (int i = 0; i < faceIds.length(); i++)
						{
							//For each face, send all of its verts so that the face normals stay accurate.
							MIntArray curVerts;
							mesh.getPolygonVertices(faceIds[i], curVerts);

							//Technically the pos of the connected ones don't change so one improvement would be to skip that.
							for (int j = 0; j < curVerts.length(); j++)
							{
								/*//cout  << "Vert Index: " << Verts[i] << endl;*/

								mesh.getFaceVertexBlindDataIndex(faceIds[i], curVerts[j], faceVertexID);

								//Position is the same for global and facevertices.
								MPoint point;
								mesh.getPoint(curVerts[j], point);
								float pos[4];
								point.get(pos);

								MVector normal;
								/*cout << "FaceId: " << faceIds[i] << endl;
								cout << "Global vertId: " << globalVertIds[j] << endl;*/

								mesh.getFaceVertexNormal(faceIds[i], curVerts[j], normal);

								double normalArr[3] = { 0 };
								status = normal.get(normalArr);

								float2 uv;
								mesh.getUVAtPoint(point, uv);

								float vertData[8];
								for (int k = 0; k < 3; k++)
								{
									vertData[k] = pos[k];
								}

								for (int k = 0; k < 3; k++)
								{
									vertData[k+3] = normalArr[k];
								}

								for (int k = 0; k < 2; k++)
								{
									vertData[k+6] = uv[k];
								}

								batch.SetVert((std::string)getParentDagNodeName(plug.node()).asChar(), faceVertexID, vertData);

							}
						}
						itPoly.next();
					}
				}
				selList.next();
			}
		}
	}
	else if ((msg & MNodeMessage::kConnectionMade) && (plug.partialName() == "iog"))
	{
		//New connection was made in the Inst Obj Groups. Most likely it's a material change.
		SendMaterialName(plug.node());
	}
	else if (msg & MNodeMessage::kConnectionMade)
	{
		cout<<"CONNECTION MADE: " << plug.name() << ": " << plug.partialName() << endl;
	}
}

void matChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	/* //cout  << plug.name() << ": " << plug.partialName() << endl;*/
	 ////cout  << "message:" << endl;
	 ////cout  << msg << endl;
	 ////cout  << plug.node().apiTypeStr() << endl;

	 if (msg & MNodeMessage::kAttributeSet)
	 {
		 //One of the attributes such as color or transparency has been set.
		 if (plug.partialName() == "c")
		 {
			 SendMaterial(plug.node());
			 SendMaterialNameMatInput(plug.node());
			 /*MFnDependencyNode mat(plug.node());*/
			 /*SendMaterialName()*/
			 //cout  << "color changed" << endl;
			 //cout  << "____" << endl;
		 }
	 }
	 else if (msg & MNodeMessage::kConnectionMade)
	 {
		 if (plug.partialName() == "c")
		 {
			 //Something was plugged into the color plug, most likely a texture.
			 SendMaterial(plug.node());
			 SendMaterialNameMatInput(plug.node());
			 //cout  << "connection MADE" << endl;
			 //cout  << "____" << endl;
		 }
	 }
	 else if (msg & MNodeMessage::kConnectionBroken)
	 {
		 if (plug.partialName() == "c")
		 {
			 //Something was unplugged from the color plug, most likely a texture.
			 SendMaterial(plug.node());
			 SendMaterialNameMatInput(plug.node());
			 //cout  << "connection Broken" << endl;
			 //cout  << "____" << endl;
		 }
	 }
	/* //cout  << "____" << endl;*/
	/*//cout  << "Material Info" << endl;*/
}

void textureChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	/*//cout  << plug.name() << ": " << plug.partialName() << endl;*/
	if (msg & MNodeMessage::kAttributeSet && plug.partialName() == "ftn")
	{
		// Called every time the texture for a file is changed.
		// Thus we can also send it to the viewer in real time.
		MFnDependencyNode fileNode(plug.node());
		MString texName;
		MPlugArray matPlugs;

		fileNode.findPlug("oc").connectedTo(matPlugs, false, true);

		/*MPlug test = fileNode.findPlug("oc");*/

		MFnDependencyNode matNode(matPlugs[0].node());
		plug.getValue(texName);
		//cout  << "Texture changed for Material "<< matNode.name().asChar() <<" to " << texName << endl;
		SendMaterial(matPlugs[0].node());
		SendMaterialNameMatInput(matPlugs[0].node());
	}

}

// Used to help make some callback assignments more general
// Such as kShading engine which appears for every material whereas the type of the node before varies with material.
MObject lastAddedNode;

/*
 * how Maya calls this method when a node is added.
 * new POLY mesh: kPolyXXX, kTransform, kMesh
 * new MATERIAL : kBlinn, kShadingEngine, kMaterialInfo
 * new LIGHT    : kTransform, [kPointLight, kDirLight, kAmbientLight]
 * new JOINT    : kJoint
 */
void nodeAdded(MObject &node, void * clientData)
{
	/*MFn::Type (node.apiType());*/
	/*//cout  << node.apiType() << endl;*/
	//... implement this and other callbacks

	// since all nodes are dependency nodes this should print all nodes.
	MString name;
	MFnDependencyNode dependNode(node);
	name = dependNode.name();

	// Not entirely certain why but identical names are printed several times sometimes.
	// Might be important but for now I'd rather just see when they change.
	if (lastName != name)
	{
		cout  << "Added node: " + name << endl;
		cout  << node.apiTypeStr() << endl;
	}

	if (node.hasFn(MFn::kMesh))
	{

		// Creating a map between the mesh and its first vertex array in order to prevent multiple printing.
		MFnMesh inputMesh(node);
		//MObjectArray shaders;
		//MObjectArray comps;

		//MIntArray indiciesPerFace;

		//MFnDagNode path(node);
		//MFnMesh get(path.dagPath(&status).node());

		//inputMesh.getConnectedSetsAndMembers(path.dagPath(&status).instanceNumber(), shaders, comps, true);
		//
		////get.getConnectedShaders(0, shaders, indiciesPerFace);

		////cout  << shaders.length() << endl;

		MFloatPointArray vertexArr;
		inputMesh.getPoints(vertexArr);
		const char* temp = name.asChar();
		string meshName(temp);
		//cout  << "MeshName: "<< meshName << endl;
		mapOfVertexArrays.insert(std::make_pair(meshName, vertexArr.length()));

		/*callbackIdArray.append(MPolyMessage::addPolyTopologyChangedCallback(node, topologyChanged, NULL, &status));*/
		MCallbackId tempId = MNodeMessage::addAttributeChangedCallback(node, nodeMeshAttributeChanged, NULL, &status);
		callbackIdArray.append(tempId);

		deletedCallbackArray.insert(std::make_pair(meshName, tempId));
	}
	else if (node.hasFn(MFn::kTransform))
	{
		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(node, nodeTransformChanged, NULL, &status));
	}
	else if (node.hasFn(MFn::kShadingEngine))
	{
		SendMaterial(lastAddedNode);
		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(lastAddedNode, matChanged, NULL, &status));
	}
	else if (node.hasFn(MFn::kFileTexture))
	{
		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(node, textureChanged, NULL, &status));
	}
	lastName = name;
}

void nodeRemoved(MObject &node, void * clientData)
{
	MString name;
	MFnDependencyNode dependNode(node);
	name = dependNode.name();

	if (node.hasFn(MFn::kTransform))
	{
		batch.RemoveObject(name.asChar());
	}

	//If this is not erased, recreating the object doesn't proc the right processes.
	//UPDATE: It appears using control Z to bring an item back crashes if we erase it here

	// The basic issue is that when a mesh is brought back through control Z it does not proc the attachment of a callback
	// until after the geometry has re-generated.

	//MMessage::removeCallback(deletedCallbackArray[name.asChar()]);
	//deletedCallbackArray.erase(name.asChar());
	//mapOfVertexArrays.erase(name.asChar());

	// As the check for adding a new mesh is based on the length of the mesh array this fixes undo issues.
	// However it is a temporary fix and has several issues.
	// (Such as adding every deleted node to the map with 0 without real reason)
	// The "Correct" way of doing this would technically be to remove the callback for deleted meshes
	// as well as erase it from the map. However when an undo call is made after a mesh is deleted
	// It re-generates the geometry before the callback has time to be re-attached.
	// As such this band-aid solution is used currently.
	mapOfVertexArrays[name.asChar()] = 0;
	//cout  << "Removed node: " + name << endl;
}

void timerCallback(float elapsedTime, float lastTime, void *clientData)
{
	/*//cout  << "Time Elapsed: " + to_string(elapsedTime) << endl;*/
	//if (batch.GetMasterHeader()->camChanged)
	//{
	//	//cout  << "CAM WAS CHANGED" << endl;
	//}

	if ((batch.GetMasterHeader()->meshCount != 0) || (batch.GetMasterHeader()->transformCount != 0) 
		|| batch.GetMasterHeader()->removedCount != 0 || batch.GetMasterHeader()->camSwitched 
		|| batch.GetMasterHeader()->matCount !=0 || batch.matSwitchedMap.size() != 0 
		|| batch.GetMasterHeader()->zoomCount != 0 || batch.GetMasterHeader()->camCount !=0 
		|| batch.GetMasterHeader()->numMeshChanged != 0 || batch.GetMasterHeader()->numRenamed != 0)
	{
		batch.GetMasterHeader()->msgNr++;
		/*//cout  << "MsgNr: " << batch.GetMasterHeader()->msgNr << endl;*/
		producer.send(batch.GetMasterHeader(), sizeof(MasterHeader));
		/*cout<< "Mesh Count: " << batch.GetMasterHeader()->meshCount << endl;
		//cout  << "Transform Count " << batch.GetMasterHeader()->transformCount << endl;
		//cout  << "Cam Count " << batch.GetMasterHeader()->camCount << endl;
		//cout  << "Mat Count " << batch.GetMasterHeader()->matCount << endl;
		//cout  << "MatSwitched Count " << batch.GetMasterHeader()->matSwitchedCount << endl;
		//cout  << "Cam switched " << batch.GetMasterHeader()->camSwitched << endl;*/

		for (const auto& nameIt : batch.renamingMap)
		{
			int nLen = nameIt.first.length();
			//Size of old name.
			producer.send(&nLen, sizeof(int));
			//Send old name.
			producer.send(nameIt.first.c_str(), nameIt.first.length());

			nLen = nameIt.second.length();
			//Size of new name.
			producer.send(&nLen, sizeof(int));
			//Send new name.
			producer.send(nameIt.second.c_str(), nameIt.second.length());
		}

		for (const auto& it1 : batch.camMap)
		{
			producer.send(it1.first.c_str(), it1.first.length());

			for (int i = 0; i < 6; i++)
			{
				//cout  << it1.second[i] << endl;
			}
			producer.send(it1.second, 6 * sizeof(float));
		}

		if (batch.GetMasterHeader()->camSwitched)
		{
			//cout  << "Sending Switched Camera: " << batch.getSwitchedName()->c_str() << endl;
			producer.send(batch.getSwitchedName()->c_str(), batch.getSwitchedName()->length());
		}

		for (int i = 0; i < batch.removeNames.size(); i++)
		{
			//cout  << "SENDING " << batch.removeNames[i].c_str() << endl;
			producer.send(batch.removeNames[i].c_str(), batch.removeNames[i].length());
		}

		for (const auto& mat : batch.matMap)
		{
			MatHeader head;
			//cout  << "Material Name: " << mat.first << endl;
			//this allows program to crash if name is too long. That's silly.
			//for (int i = 0; i < mat.first.length(); i++)
			//{
			//	head.matName[i] = mat.first.at(i);
			//}
			if (mat.second.name != "")
			{
				//cout  << "Sending mat: " << mat.first << endl;
				//cout  << "With texture: " << mat.second.name << endl;
				head.isTextured = true;
				head.lenMatName = mat.first.length();
				head.lenTextureName = mat.second.name.length();
				producer.send(&head, sizeof(head));

				cout << "Texture Material Name: " << mat.first << endl;
				producer.send(mat.first.c_str(), mat.first.length());
				producer.send(mat.second.name.c_str(), mat.second.name.length());
				//for (int i = 0; i < mat.second.name.length(); i++)
				//{
				//	head.textureName[i] = mat.second.name.at(i);
				//}
				////cout  << "Texture Name: " << head.textureName << endl;
				//Allows me to use just one message but at what cost?
				/*producer.send(&head, sizeof(MatHeader));*/
			}
			else
			{
				head.isTextured = false;
				head.lenMatName = mat.first.length();
				head.lenTextureName = mat.second.name.length();
				producer.send(&head, sizeof(head));

				cout << "Material Name: " << mat.first << endl;
				producer.send(mat.first.c_str(), mat.first.length());
				producer.send(mat.second.colors, mat.second.numFloats*sizeof(float));

				//cout  << "Sending" << endl;
				for (int i = 0; i < mat.second.numFloats; i++)
				{
					//cout  << "c" << i << ": " << mat.second.colors[i] << endl;
				}
			}
			
		}

		for (const auto& nr : batch.meshMap)
		{
			//MeshHeader could just be saved in batch later
			MeshHeader meshHead;
			for (int i = 0; i < nr.first.length(); i++)
			{
				meshHead.meshName[i] = nr.first.at(i);
			}
			meshHead.indexCount = nr.second.GetIndexCount();
			meshHead.nrOfVerts = nr.second.GetNrOfVerts();
			string s(meshHead.meshName);
			//cout  << "MeshName: " << s << endl;
			//cout  << "IndexCount: " << meshHead.indexCount << endl;
			//cout  << "nrOfVerts: " << meshHead.nrOfVerts << endl;

			producer.send(&meshHead, sizeof(MeshHeader));
			producer.send(nr.second.GetIndicies(), meshHead.indexCount * sizeof(int));
			producer.send(nr.second.GetVerts(), meshHead.nrOfVerts * 8 * sizeof(float));
		}

		/*cout<<"SWITCHED COUNT: " << batch.GetMasterHeader()->matSwitchedCount << endl;*/

		for (const auto& nr : batch.matSwitchedMap)
		{
			//cout  << "Mesh: " << nr.first << endl;
			//cout  << "Switched material to: " << nr.second << endl;
			MatSwitchedHeader lengths;

			lengths.lenMeshName = nr.first.length();
			lengths.lenMatName = nr.second.length();

			producer.send(&lengths, sizeof(lengths));
			producer.send(nr.first.c_str(), nr.first.length());
			producer.send(nr.second.c_str(), nr.second.length());
		}
		int nrOfTransformsSent = 0;
		for (const auto& it2 : batch.transformMap)
		{
			////cout  << "---------------------------------" << endl;
			////cout  << "SENDING TRANSFORM FOR: " << endl;
			////cout  << it2.first << endl;

			////cout  << "Translation: " << it2.second[0] << "," << it2.second[1] << "," << it2.second[2] << endl;
			////cout  << "scale: " << it2.second[3] << "," << it2.second[4] << "," << it2.second[5] << endl;
			////cout  << "Rotation: " << it2.second[6] << "," << it2.second[7] << "," << it2.second[8] << endl;
			////cout  << "---------------------------------" << endl;
			/*int nr = 1;
			producer.send(&nr, sizeof(int));*/
			producer.send(it2.first.c_str(), it2.first.length());
			producer.send(it2.second, 10*sizeof(double));
			nrOfTransformsSent++;
		}

		for (const auto& vertMeshIt : batch.vertMap)
		{
			/*//cout  << "SENDING CHANGED VERTS FOR MESH: " << vertMeshIt.first << endl;*/
			int len = vertMeshIt.first.length();
			//Send name and size of name for mesh.
			producer.send(&len, sizeof(int));
			producer.send(vertMeshIt.first.c_str(), len);

			int nrOfVerts = vertMeshIt.second.size();
			producer.send(&nrOfVerts, sizeof(int));

			for (const auto& vertIt : vertMeshIt.second)
			{
				/*//cout  << "Point ID: " << vertIt.first << endl;*/
				//Send the vertex ID
				producer.send(&vertIt.first, sizeof(int));

				int nrFloats;
				
				//Must explicitly make it float to avoid rounding errors.
				float val = 0.123456;

				if ((vertIt.second[3] == val) && (vertIt.second[4] == val) && (vertIt.second[5] == val))
				{
			/*		cout << "Send pos" << endl;*/
					nrFloats = 3;
					producer.send(&nrFloats, sizeof(int));
					producer.send(vertIt.second, sizeof(float) * 3);
				}
				else
				{
					//cout << "Send all" << endl;
					nrFloats = 8;
					producer.send(&nrFloats, sizeof(int));
					producer.send(vertIt.second, sizeof(float) * 8);
				}
				//for (int i = 0; i < 3; i++)
				//{
				//	//cout  << i << ": " << vertIt.second[i] << endl;
				//}
			}
		}

		for (const auto& zoomIt : batch.orthoZoomMap)
		{
			int len = zoomIt.first.length();
			//Send length of the name:
			producer.send(&len, sizeof(int));
			//Send the name:
			producer.send(zoomIt.first.c_str(), zoomIt.first.length());
			//Send the values:
			producer.send(zoomIt.second, sizeof(float)*2);
		}

		/*//cout  << "Sent nr: " << nrOfTransformsSent << endl;
		//cout  << endl;*/
		batch.Reset();
	}
}

void nameChanged(MObject &node, const MString &prevName, void *clientData)
{
	if (prevName != "" && prevName != "manipulator#")
	{
		
		MString name;
		MFnDependencyNode dependNode(node);
		name = dependNode.name();

		if (prevName != name)
		{
			cout  << "Node name changed!" << endl;
			cout  << "Node: " + prevName << endl;
			cout  << "Is now Node: " + name << endl;

			if (node.hasFn(MFn::kTransform))
			{
				cout << "enter batch" << endl;
				batch.SetRename((std::string)prevName.asChar(), (std::string)name.asChar());
				cout << "done batch" << endl;
			}
			//else
			//{
			//	batch.SetRename((std::string)prevName.asChar(), (std::string)name.asChar());
			//}
		}
	}
}

void viewChangedCB(const MString& str, MObject &node, void* clientData)
{
	std::string tmp(getParentDagNodeName(node).asChar());
	batch.SwitchedCamera(tmp);
}

double camMoveStartPos[3] = { 0.0f };
double rotStartPos[3] = { 0.0f };

MTransformationMatrix moveStartMatrix;

void cameraAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	/*//cout  << plug.name() << ":" << plug.partialName() << endl;*/
	if(msg & MNodeMessage::kAttributeSet)
	{
		/*//cout  << "Cam Transform Changed!" << endl;*/
		MObject transformNode = plug.node();

		if (transformNode.hasFn(MFn::kTransform))
		{
			MFnDagNode dagNode(plug.node());
			//How do filter away rotation translate, while keeping panning translate
			/*//cout  << plug.name() << ":" << plug.partialName() << endl;*/
			if ((plug.partialName() == "rpt") || (plug.partialName() == "t")
				|| (plug.partialName() == "tx") || (plug.partialName() == "ty") || (plug.partialName() == "tz"))
			{
				/*//cout  << plug.name() << ":" << plug.partialName() << endl;*/
				MDagPath tNodeDag;
				dagNode.getPath(tNodeDag);
				MMatrix worldMat = tNodeDag.inclusiveMatrix();

				const MTransformationMatrix aMat(worldMat);
				MVector trans = aMat.getTranslation(MSpace::kWorld, &status);
				double tmp[3];
				trans.get(tmp);

				for (int i = 0; i < 3; i++)
				{
					camMoveStartPos[i] = roundf(camMoveStartPos[i] * 1000.0f) / 1000.0f;
					tmp[i] = roundf(tmp[i] * 1000.0f) / 1000.0f;

					/*//cout  << "Quat: " << quat[i] << " Start: " << quatStartPos[i] << endl;*/
				}

				////cout  << "Checking pos" << endl;
				//for (int i = 0; i < 3; i++)
				//{
				//	//cout  << i << ": " << camMoveStartPos[i] << endl;
				//}
				////cout  << "Temp:" << endl;
				//for (int i = 0; i < 3; i++)
				//{
				//	//cout  << i << ": " << tmp[i] << endl;
				//}
		/*		//cout  << "Current Mat:" << endl;
				//cout  << aMat.asMatrix() << endl;
				//cout  << "Moved Mat:" << endl;
				//cout  << moveStartMatrix.asMatrix() << endl;*/
				////cout  << "Compare" << endl;
				//Rounding off for inconsequential minor things.
				//for (int i = 0; i < 3; i++)
				//{
				//	quat[i] = roundf(quat[i] * 1000.0f) / 1000.0f;
				//	rotStartPos[i] = roundf(rotStartPos[i] * 1000.0f) / 1000.0f;

				//	/*//cout  << "Quat: " << quat[i] << " Start: " << quatStartPos[i] << endl;*/
				//}

				//for (int i = 0; i < 4; i++)
				//{
				//	if (quatStartPos[i] == quat[i])
				//	{

				//	}
				//}
				//quatStartPos

				double rot[3];
				MTransformationMatrix::RotationOrder wat;
				aMat.getRotation(rot, wat);

				//Maya has an issue with this callback where it sometimes uses the original position of the camera rather than the location it's at now.
				//This filters that.
				if (((tmp[0] != camMoveStartPos[0]) || (tmp[1] != camMoveStartPos[1]) || (tmp[2] != camMoveStartPos[2]))
					&& ((rot[0] == rotStartPos[0]) && (rot[1] == rotStartPos[1]) && (rot[2] == rotStartPos[2])))
				{
					//If the plug is translation and rotational values haven't changed we enter here.
					//This way we enter during panning but avoid the translation plug while rotating.
					/*//cout  << "ENTERED" << endl;*/
					SendTransform(transformNode);
					producer.send(batch.GetMasterHeader(), sizeof(MasterHeader));
					for (const auto& it2 : batch.transformMap)
					{
						/*//cout  << it2.first << endl;*/
						producer.send(it2.first.c_str(), it2.first.length());
						producer.send(it2.second, 10 * sizeof(double));
					}
					batch.Reset();
				}
				else if((plug.partialName() == "rpt") && ((tmp[0] != camMoveStartPos[0]) || (tmp[1] != camMoveStartPos[1]) || (tmp[2] != camMoveStartPos[2])))
				{
					// If the camera has rotated we want the final value which is given by the "rpt" plug.
					// I may have to add the translation check too for the "bug"
					/*//cout  << "ENTERED ROTATION" << endl;*/
					SendTransform(transformNode);
					producer.send(batch.GetMasterHeader(), sizeof(MasterHeader));
					for (const auto& it2 : batch.transformMap)
					{
						/*//cout  << it2.first << endl;*/
						producer.send(it2.first.c_str(), it2.first.length());
						producer.send(it2.second, 10 * sizeof(double));
					}
					batch.Reset();
				}
				else
				{
					/*//cout  << "NO ENTRY" << endl;*/
				}
			}
		}
	}
	/* //cout  << plug.name() << ": " << plug.partialName() << endl;*/
}

void SendCam(MObject node)
{
	MFnCamera cam(node);
	//make degrees
	float arr[6];

	arr[0] = cam.isOrtho();
	if (cam.isOrtho())
	{
		arr[1] = cam.orthoWidth();
		// [1][1] in an orthographic projection matrix = 2/top - bottom. 
		// In mayas case this result is also negated.
		// As such we can extract the height of the projection matrix using simple equation.
		// 2/height = [1][1] -> 2/[1][1] = height.
		arr[2] = 2/cam.projectionMatrix()[1][1];
		/*//cout  << "HEIGHT HERE: " <<cam.projectionMatrix()[2][2] << endl;*/
	}
	else
	{
		arr[1] = cam.horizontalFieldOfView() *(180 / 3.14159265359);
		arr[2] = cam.verticalFieldOfView()*(180 / 3.14159265359);
	}
	arr[3] = cam.aspectRatio();
	arr[4] = cam.nearClippingPlane();
	arr[5] = cam.farClippingPlane();

	batch.SetCamera(arr, getParentDagNodeName(node).asChar());
	/*//cout  <<
		cam.aspectRatio() << ", " <<
		cam.farClippingPlane() << ", " <<
		cam.nearClippingPlane() << ", " <<
		cam.verticalFieldOfView() << ", " <<
		cam.horizontalFieldOfView() << ", " <<
		cam.isOrtho() << endl;*/
}

void camMoveStart(MObject &node, void *clientData)
{
	MFnDagNode dagSearch(node);
	MObject handle = dagSearch.parent(0);
	MFnDagNode parent(handle);
	//cout  << "____________________________________________________________" << endl;
	//cout  << "CAM MOVE START" << endl;
	//cout  << "For " << parent.name() << endl;
	////cout  << "____________________________________________________________" << endl;

	MDagPath tNodeDag;
	parent.getPath(tNodeDag);
	MMatrix worldMat = tNodeDag.inclusiveMatrix();

	const MTransformationMatrix aMat(worldMat);

	/*moveStartMatrix = aMat;*/
	MVector trans = aMat.getTranslation(MSpace::kWorld, &status);
	trans.get(camMoveStartPos);

	MTransformationMatrix::RotationOrder wat;
	aMat.getRotation(rotStartPos, wat);
	//cout  << wat << endl;
	//cout  << "____________________________________________________________" << endl;
	/*aMat.getRotationQuaternion(quatStartPos[0], quatStartPos[1], quatStartPos[2], quatStartPos[3]);*/
	//for (int i = 0; i < 3; i++)
	//{
	//	//cout  << i << ": " << camMoveStartPos[i] << ", ";
	//}
	////cout  << endl;

}

void zoomCB(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	//cout  << plug.name() << ":" << plug.partialName() << endl;
	if (plug.partialName() == "ow")
	{
		/*SendCam(plug.node());*/

		MFnCamera cam(plug.node());
		float size[2];
		size[0] = cam.orthoWidth();
		// [1][1] in an orthographic projection matrix = 2/top - bottom. 
		// In mayas case this result is also negated.
		// As such we can extract the height of the projection matrix using simple equation.
		// 2/height = [1][1] -> 2/[1][1] = height.
		size[1] = 2 / cam.projectionMatrix()[1][1];

		batch.SetCamOrthoZoom((string)getParentDagNodeName(plug.node()).asChar(), size);
		//cout  << "The heck: " << cam.orthoWidth() << endl;
		//cout  << "Horizontal: " << cam.horizontalFieldOfView() *(180 / 3.14159265359) << endl;
		//cout  << "Vertical: " << cam.verticalFieldOfView()*(180 / 3.14159265359) << endl;
		//cout  << "Aspect ratio: " << cam.aspectRatio() << endl;
	}
}

//DON'T FORGET Sending initialize info to the viewer should also happen here
void addCallbacksToExistingNodes()
{
	//Iterator for all dependency nodes that are meshes.
	MItDependencyNodes iterator(MFn::kMesh);
	//Iterator for all dependency nodes that are transforms.
	MItDependencyNodes iterator2(MFn::kTransform);
	MItDependencyNodes camIterator(MFn::kCamera);
	MItDependencyNodes matIterator(MFn::kLambert); //Most maya materials are based off of lambert.
	MItDependencyNodes textureIterator(MFn::kFileTexture);
	/*MItDependencyNodes iterator(MFn::kCameraView)*/

	//From the look of things I'll need to use "MCallbackId MUiMessage:: addCameraChangedCallback". But in order to do that I need the panel names. 
	//It's either that or something with the M3dView class. Or possibly using the kCameraview to get the imagePlane?
	// Worst case: https://forums.cgsociety.org/t/get-modelpanel-name-from-m3dview-c-api/1598993
	//https://help.autodesk.com/view/MAYAUL/2019/ENU/?guid=Maya_SDK_MERGED_cpp_ref_class_m3d_view_html
	//3Dview has a getCamera function. We simply MUST use it.
	//Potential, DIRTY solution. Add a second timer callback and check the active viewport every few miliseconds. I would rather have a callback though.

	while (!iterator.isDone())
	{
		// Getting already existing meshes vertecies.
		MFnMesh inputMesh(iterator.thisNode());
		MFloatPointArray vertexArr;
		inputMesh.getPoints(vertexArr);
		// Getting the name of the meshNode.
		MString name;
		MFnDependencyNode dependNode(iterator.thisNode());
		name = dependNode.name();
		const char* temp = name.asChar();
		string meshName(temp);
		// Pairing the map of the meshName and amount of vertices.
		mapOfVertexArrays.insert(std::make_pair(meshName, vertexArr.length()));

		MObject Mnode = iterator.thisNode();
		//Batching already existing meshes for the viewer.
		SendMesh(Mnode);
		SendMaterialName(Mnode);

		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(iterator.thisNode(), nodeMeshAttributeChanged, NULL, &status));

		iterator.next();
	}

	while (!iterator2.isDone())
	{
		MFnDagNode dagSearch(iterator2.thisNode());
		MObject handle = dagSearch.child(0);

		//Camera is handled seperatly since timer callback apparently isn't fired during camera movement.
		if (handle.hasFn(MFn::kCamera) == false)
		{
			SendTransform(iterator2.thisNode());
			callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(iterator2.thisNode(), nodeTransformChanged, NULL, &status));
		}

		iterator2.next();
	}

	// I DESPISE the fact that I had to use a MEL-command from the API.
	// Is there truly no way to access modelPanels directly from the API??
	// I couldn't find any...
	MStringArray modelPanels;
	if (MGlobal::executeCommand("getPanel -type \"modelPanel\"", modelPanels))
	{
		int nrOfPanels = modelPanels.length();
		for (int i = 0; i < nrOfPanels; i++)
		{
			/*//cout  << modelPanels[i] << endl;*/
			callbackIdArray.append(MUiMessage::addCameraChangedCallback(modelPanels[i], viewChangedCB, NULL, &status));
			/*callbackIdArray.append(MUiMessage::add3dViewPreRenderMsgCallback(modelPanels[i], attempt, NULL, &status));*/
			M3dView view;
			M3dView::getM3dViewFromModelPanel(modelPanels[i], view);

			//callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(modelPanels[i],))
			/*MObject a(view);*/
			/*view.getCamera()*/
			/*MFnDependencyNode window(view);*/
			//view.object
			///*MObject a = (MObject)view;*/
			/*callbackIdArray.append(MUiMessage::add3dViewPostRenderMsgCallback(modelPanels[i], gah, NULL, &status));*/
		}
	}

	while (!camIterator.isDone())
	{
		MFnDagNode dagSearch(camIterator.thisNode());
		MObject handle = dagSearch.parent(0);

		/*//cout  <<"Camera Name: " << dagSearch.name() << endl;*/
		////cout  << "HERE" << endl;
		////cout  << parent.name().asChar() << endl;
		////cout  << dagSearch.name().asChar() << endl;

		SendCam(camIterator.thisNode());

		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(handle, cameraAttributeChanged, NULL, &status));
		callbackIdArray.append(MCameraMessage::addBeginManipulationCallback(camIterator.thisNode(), camMoveStart, NULL, &status));
		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(camIterator.thisNode(), zoomCB, NULL, &status));
		
		//Trick the transform callback into triggering so all cameras start at correct positions in viewer.
		SendTransform(handle);

		camIterator.next();
	}

	//cout  << "nrOf3DViews: " << M3dView::numberOf3dViews() << endl;
	M3dView test = M3dView::active3dView();
	//for (int i = 0; i < M3dView::numberOf3dViews(); i++)
	//{
	//	M3dView current;
	//	M3dView::get3dView(i, current);

	//	callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(, cameraAttributeChanged, NULL, &status));
	//}

	MDagPath cam;
	test.getCamera(cam);
	MFnDagNode node(cam);
	//cout  << "Cam Name: " << node.name().asChar() << endl;
	MObject handle = node.parent(0);
	MFnDagNode parent(handle);

	//cout  << "PARENT NAME: " << parent.name() << endl;
	batch.SwitchedCamera((std::string)parent.name().asChar());

	while (!matIterator.isDone())
	{	
		//We want to send all the materials initially.
		//Then when a material is switched here we want to switch in the viewer
		//If a new material is created, we want to send that to the viewer also.
		/*SetupMaterials(matIterator.thisNode());*/
		SendMaterial(matIterator.thisNode());
		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(matIterator.thisNode(), matChanged, NULL, &status));
		matIterator.next();
	}

	while (!textureIterator.isDone())
	{
		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(textureIterator.thisNode(), textureChanged, NULL, &status));
		textureIterator.next();
	}
}

void selectionChangedCB(void* x)
{
	MSelectionList selected;
	MGlobal::getActiveSelectionList(selected);

	MObject obj;

	//Extruding generally ends when the user selects something different.
	//It is far from an optimal solution and too general but will most likely work.
	if (isExtruding)
	{
		isExtruding = false;
	}

	for (int i = 0; i < selected.length(); i++)
	{
		selected.getDependNode(i, obj);

		if (obj.apiType() == MFn::kPolyExtrudeFacet)
		{
			isExtruding = true;
		}
	}


	/*//cout  << "SELECTION CHANGED!" << endl;*/
}

//Duplication of mesh doesn't trigger attribute changed properly.
//This is an elaborate scheme to trigger it.
void duplicateCB(void*clientData)
{
	MSelectionList selected;
	MGlobal::getActiveSelectionList(selected);

	MObject obj;

	for (int i = 0; i < selected.length(); i++)
	{
		selected.getDependNode(i, obj);

		if (obj.hasFn(MFn::kTransform))
		{
			MFnTransform trans(obj);

			for (int j = 0; j < trans.childCount(); j++)
			{
				MObject child = trans.child(j);

				if (child.hasFn(MFn::kMesh))
				{
					MFnMesh mesh(child);
					//This triggers the attribute changed callback.
					MPoint point;
					mesh.getPoint(0, point);
					mesh.setPoint(0, point);
				}
			}
		}
	}

}

/*
 * Plugin entry point
 * For remote control of maya
 * open command port: commandPort -nr -name ":1234"
 * close command port: commandPort -cl -name ":1234"
 * send command: see loadPlugin.py and unloadPlugin.py
 */
EXPORT MStatus initializePlugin(MObject obj) {

	MStatus res = MS::kSuccess;

	MFnPlugin myPlugin(obj, "level editor", "1.0", "Any", &res);

	if (MFAIL(res)) {
		CHECK_MSTATUS(res);
		return res;
	}  
	
	// redirect //cout  to cerr, so that when we do //cout  goes to cerr
	// in the maya output window (not the scripting output!)
	std::cout.set_rdbuf(MStreamUtils::stdOutStream().rdbuf());
	std::cerr.set_rdbuf(MStreamUtils::stdErrorStream().rdbuf());
	cout  << "Plugin loaded ===========================" << endl;
	
	// register callbacks here for
	callbackIdArray.append(MDGMessage::addNodeAddedCallback(nodeAdded, "dependNode", NULL, &status));
	callbackIdArray.append(MDGMessage::addNodeRemovedCallback(nodeRemoved, "dependNode", NULL, &status));
	callbackIdArray.append(MTimerMessage::addTimerCallback(0.02f, timerCallback, NULL, &status));
	callbackIdArray.append(MNodeMessage::addNameChangedCallback(MObject::kNullObj, nameChanged, NULL, &status));
	callbackIdArray.append(MModelMessage::addAfterDuplicateCallback(duplicateCB, NULL, &status));
	
	callbackIdArray.append(MModelMessage::addCallback(MModelMessage::kActiveListModified, selectionChangedCB, NULL, &status));

	addCallbacksToExistingNodes();

	changedVerts.reserve(3000);
	// a handy timer, courtesy of Maya
	gTimer.clear();
	gTimer.beginTimer();

	return res;
}
	

EXPORT MStatus uninitializePlugin(MObject obj) {
	MFnPlugin plugin(obj);

	cout  << "Plugin unloaded =========================" << endl;

	MMessage::removeCallbacks(callbackIdArray);

	return MS::kSuccess;
}