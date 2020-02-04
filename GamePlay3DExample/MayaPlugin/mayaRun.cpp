// Original Example expanded upon by Ossian Edström
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

using namespace std;
MCallbackIdArray callbackIdArray;
MObject m_node;
MStatus status = MS::kSuccess;
bool initBool = false;

enum NODE_TYPE { TRANSFORM, MESH };
MTimer gTimer;

ComLib producer((std::string)"stuff", 100 * 1024 * 1024, ComLib::PRODUCER);
MString lastName = "0";

// keep track of created meshes to maintain them
queue<MObject> newMeshes;
std::map<std::string, int> mapOfVertexArrays;
MayaBatchOutput batch;

void SendTransform(MObject transformNode)
{
	MFnDependencyNode nameFetch(transformNode);

	//cout << nameFetch.name() << " Transform changed!" << endl;

	//Local transform
	//MFnTransform getTransform(transformNode);

	//MMatrix tMat = getTransform.transformation().asMatrix();

	//Global Transform:
	MFnDagNode path(transformNode);
	MDagPath tNodeDag;

	path.getPath(tNodeDag);

	MMatrix worldMat = tNodeDag.inclusiveMatrix();

	MFnTransform parser;
	const MTransformationMatrix aMat(worldMat);

	MVector trans = aMat.getTranslation(MSpace::kWorld, &status);
	double transDouble[3];
	trans.get(transDouble);

	double scaleDouble[3];
	aMat.getScale(scaleDouble, MSpace::kWorld);

	double quatDouble[4];
	aMat.getRotationQuaternion(quatDouble[0], quatDouble[1], quatDouble[2], quatDouble[3]);

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

	int len = 0;
	const char* name = nameFetch.name().asChar(len);

	TransHeader head;
	for (int i = 0; i < len; i++)
	{
		head.name[i] = name[i];
	}

	batch.SetTransform(head, transform);
}

void nodeTransformChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	MFnDependencyNode nameFetch(plug.node());

	/*cout << "TRANSFORM CALLBACK FOR " << nameFetch.name() << endl;*/

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

	cout << "Name: " << funcMat.name() << endl;
	colPlug = funcMat.findPlug("color", true, &status);
	ambColPlug = funcMat.findPlug("ambc", true, &status);
	cout << colPlug.name() << endl;
	cout << "" << endl;
	cout << ambColPlug.name() << endl;
	cout << endl;

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
				cout << "hit " << j << endl;
				MFnDependencyNode dep(plugs[j].node());
				MPlug ftn = dep.findPlug("ftn");
				cout << "plugName: " << ftn.name().asChar() << endl;
				ftn.getValue(textureName);
				cout << textureName << endl;
				if (textureName != "")
				{
					batch.SetMaterial((std::string)funcMat.name().asChar(), (std::string)textureName.asChar());
				}
				else
				{
					MColor color;
					for (int j = 0; j < colPlug.numChildren(); j++)
					{
						/*cout << colPlug.child(j).name().asChar() << endl;*/
						colPlug.child(j, &status).getValue(color[j]);
					}

					cout << "R: " << color.r << endl;
					cout << "G: " << color.g << endl;
					cout << "B: " << color.b << endl;

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
			/*cout << colPlug.child(j).name().asChar() << endl;*/
			colPlug.child(j, &status).getValue(color[j]);
		}

		cout << "R: " << color.r << endl;
		cout << "G: " << color.g << endl;
		cout << "B: " << color.b << endl;
		
		float rgb[3];
		rgb[0] = color.r;
		rgb[1] = color.g;
		rgb[2] = color.b;

		batch.SetMaterial((std::string)funcMat.name().asChar(), rgb, 3);
		/*cout << "A: " << color.a << endl;*/
	}

	//MColor color;
	//for (int j = 0; j < ambColPlug.numChildren(); j++)
	//{
	//	cout << ambColPlug.child(j).name().asChar() << endl;
	//	ambColPlug.child(j, &status).getValue(color[j]);
	//}

	//cout << "ambcR: " << color.r << endl;
	//cout << "ambcG: " << color.g << endl;
	//cout << "ambcB: " << color.b << endl;
}

// Must be sent with node as a mesh that is already in the MayaBatchOutput.
// Thus should be used ASAP after SendMesh function
void SendMaterialName(MObject &node)
{
	MFnMesh mesh(node);

	//nr of parents
	int numParents = mesh.parentCount();
	/*cout << "Number of Parents: " << numParents << endl;*/
	cout << "Entered SendMaterialName" << endl;

	//Loop through
	for (int i = 0; i < numParents; i++)
	{
		/*MFnDependencyNode pNode(mesh.parent(i));*/

		MObjectArray shaders;

		MIntArray indiciesPerFace;

		mesh.getConnectedShaders(i, shaders, indiciesPerFace);

		if (shaders.length() == 0)
		{
			cout << "MESH " << mesh.name().asChar() << " HAS NO SHADER" << endl;
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
				cout << "Material Name: " << funcMat.name().asChar() << endl;
				//Note, the name is technically the transform and not the mesh.
				MFnDagNode dagSearch(node);
				MObject handle = dagSearch.parent(0);
				MFnDagNode parent(handle);

				batch.SetMatSwitched((std::string)parent.name().asChar(), (std::string) funcMat.name().asChar());
				//batch.meshMap[(std::string)parent.name().asChar()].SetMatName(funcMat.name().asChar(), funcMat.name().length());
			}
		}
	}
}

void SendMaterialNameMatInput(MObject &node)
{
	MFnDependencyNode mat(node);
	cout << "ENTERED MATERIAL NAME INPUT" << endl;
	cout << mat.name() << endl;

	MPlugArray arr;
	mat.findPlug("oc").connectedTo(arr, false, true);

	if (arr.length() != 0)
	{
		cout << arr.length() << endl;
		cout << "Entered" << endl;
		for (int i = 0; i < arr.length(); i++)
		{
			MFnDependencyNode shadingGroup(arr[i].node());

			MPlug dagSetMembers;
			dagSetMembers = shadingGroup.findPlug("dsm");

			//cout << "Numchildren: " << dagSetMembers.numChildren() << endl;
			//cout << "NumElements: " << dagSetMembers.numElements() << endl;
			for (int j = 0; j < dagSetMembers.numElements(); j++)
			{
				MPlugArray meshPlugs;
				dagSetMembers[j].connectedTo(meshPlugs, true, false);
				MFnDependencyNode mesh(meshPlugs[0].node());

				MFnDagNode dagSearch(meshPlugs[0].node());
				MObject handle = dagSearch.parent(0);
				MFnDagNode parent(handle);

				batch.SetMatSwitched((std::string)parent.name().asChar(), (std::string) mat.name().asChar());
				//batch.SetMatSwitched()
				//cout << parent.name() << endl;
			}
		}

		/*shadingGroup.findPlug("dagSetMembers", &status);*/

		//if (status == MS::kSuccess)
		//{
		//	cout << "Found DagSetMembers!" << endl;
		//}

		/*cout << "NUMBER DAGSETMEMBERS: " << << endl;*/

		//MPlugArray meshPlugs;
		//dagSetMembers.connectedTo(meshPlugs, true, false);
		//for (int i = 0; i < meshPlugs.length(); i++)
		//{
		//	MFnDependencyNode mesh(meshPlugs[i].node());
		//	cout << mesh.name() << endl;
		//}
	}
}

void SetupMaterials(MObject &node)
{
	MFnMesh mesh(node);

	//nr of parents
	int numParents = mesh.parentCount();
	/*cout << "Number of Parents: " << numParents << endl;*/
	cout << "Entered SetupMaterials" << endl;

	//Loop through
	for (int i = 0; i < numParents; i++)
	{
		MFnDependencyNode node(mesh.parent(i));

		MObjectArray shaders;

		MIntArray indiciesPerFace;

		mesh.getConnectedShaders(i, shaders, indiciesPerFace);

		if (shaders.length() == 0)
		{
			cout << "MESH " << mesh.name().asChar() << " HAS NO SHADER" << endl;
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
				cout <<"Material Name: " << funcMat.name().asChar() << endl;
				if (mats[0].node().hasFn(MFn::kLambert))
				{
					/*MPlugArray rgb;
					MFnLambertShader shader(mats[0].node());*/
					/*shader.findPlug("color").connectedTo(rgb, true, false);*/
					

					MPlug plug;
					plug = funcMat.findPlug("color", true, &status);

					if (plug.isArray())
					{
						cout << "I'm ARRAY" << endl;
					}
					MColor color;

					cout << plug.name() << endl;
					cout << "NumChildren: "<< plug.numChildren() << endl;

					for(int j = 0; j< plug.numChildren(); j++)
					{
						cout << plug.child(j).name().asChar() << endl;
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
							cout << "hit "<< j << endl;
							MFnDependencyNode dep(plugs[j].node());
							MPlug ftn = dep.findPlug("ftn");
							cout << "plugName: " << ftn.name().asChar() << endl;
							ftn.getValue(textureName);
						/*	textureName = dep.name();*/
						}
					}

					if (textureName == "")
					{
						cout << "R: " << color.r << endl;
						cout << "G: " << color.g << endl;
						cout << "B: " << color.b << endl;
					/*	cout << "A: " << color.a << endl;*/
						float rgb[3];
						rgb[0] = color.r;
						rgb[1] = color.g;
						rgb[2] = color.b;
						/*batch.SetMaterial((std::string)funcMat.name().asChar(), rgb,3);*/
					}
					else
					{
						cout << "Texture: " << textureName << endl;
						/*batch.SetMaterial((std::string)funcMat.name().asChar(), (std::string)textureName.asChar());*/
					}

		/*			plug.getValue(color.r);
					cout <<"R: " << color.r << endl;*/

					/*cout << rgb.length() << endl;
					for (int j = 0; j < rgb.length(); j++)
					{
						cout << rgb[j].name().asChar() << endl;
					}*/

					/*cout << plug.*/
				}
				else
				{
					//"special" material type like Stingray PBS or something else not deferred from lambert.
					// Doesn't have the standard "color" plug and should be handled differently here.
				}
			}
			else
			{
				cout << "MESH " << mesh.name().asChar() << "HAS NO MATS" << endl;
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
		MFnDagNode parent(handle);

		//Replace old vertex array and print the new info out.
		mapOfVertexArrays[meshName] = vertexArr.length();
		cout << parent.name().asChar() << " Topology Changed! " << "New vertex locations: " << endl;

		//Getting the number of verts
		MeshHeader meshHead;

		meshHead.nrOfVerts = inputMesh.numFaceVertices(&status);

		//Enter the name.
		for (int i = 0; i < meshNode.name().length(); i++)
		{
			meshHead.meshName[i] = parent.name().asChar()[i];
		}

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
		//	cout << triIndicies[i] << " ";
		//}
		//cout << endl;

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
				/*cout << normArr[nCount] << " ";*/
				nCount++;
			}
			// Note: These are just initial UVs. There should technically be a callback for when the UVs change.
			for (int j = 0; j < 2; j++)
			{
				UVArr[uvCount] = temp[j];
				/*	cout << UVArr[uvCount] << " ";*/
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
		int count4 = 0;

		cout << numFaceVertices << endl;

		for (int i = 0; i < numFaceVertices; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				verts[count] = posArr[i][j];
				count++;
				count4++;
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
	cout << "**********************************************************************************" << endl;
	cout << "*********************** CRASH SAFETY NET WARNING!! *******************************" << endl;
	cout << " ATTEMPTED TO CALL FUNCTION 'SetMesh' ON A MObject WITHOUT 'MFn::kMesh'." << endl;
	cout << "************************* FUNCTION CALL IGNORED **********************************" << endl;
	cout << "**********************************************************************************" << endl;
	}
}

void nodeMeshAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	/* cout << plug.name() << ": " << plug.partialName() << endl;*/

	// For individually or soft select moved verticies. 
	if (msg & MNodeMessage::kAttributeSet)
	{
		MObject transformNode = plug.node();

		if (plug.isElement())
		{
			//Note, the name is technically the transform and not the mesh.
			MFnDagNode dagSearch(plug.node());
			MObject handle = dagSearch.parent(0);
			MFnDagNode parent(handle);
			cout << "In mesh " << parent.name() << endl;
			cout << plug.partialName() << " Was changed! The vertex values are now: " << endl;

			int index = -1;

			//Extract values.
			float xyz[3] = { 0 };
			//float x = 0;
			//float y = 0;
			//float z = 0;
			plug.child(0).getValue(xyz[0]);
			plug.child(1).getValue(xyz[1]);
			plug.child(2).getValue(xyz[2]);

			/*producer.send(&z, sizeof(float));*/

			cout << "X: " << xyz[0] << " Y: " << xyz[1] << " Z: " << xyz[2] << endl;
			batch.SetVert((std::string)parent.name().asChar(), plug.logicalIndex(), xyz);

		}
	}
	// For actual changes to topology
	else if (msg & MNodeMessage::kAttributeEval && plug.partialName() == "o")
	{
		MFnDependencyNode meshNode(plug.node());
		MFnMesh inputMesh(plug.node());

		MObject Mnode = plug.node();

		/*for (int i = 0; i < dagSearch.parentCount(); i++)
		{
			MObject parHandle = dagSearch.parent(i);

			MFnDagNode par(parHandle);

			cout << par.name().asChar() << endl;
		}*/
		cout << "entered Topology change" << endl;

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
		//else
		//{
		//	//The output mesh has changed without adding any new vertices.
		//	//Or the lighting mode was changed to soft... OR probably a thousand other things...
		//	//This means that some of the mesh's verticies have transformed or are otherwise being evaluated.

		//	// In order to determine the relevant actions we identify the latest input mesh node
		//	// Put we don't wish to even check anymore if it was simply a transform for the verticies. How do I filter that?
		//	// Outmesh feels way too general, perhaps a completely seperate callback for the extrude node?
		//	MFnMesh inputMesh(plug.node());

		//	MPlug inPlug = meshNode.findPlug("inMesh", &status);

		//	cout << "InMesh plug: " << inPlug.name() << endl;

		//	MPlugArray plugArr;
		//	inPlug.connectedTo(plugArr, true, false);

		//	cout << "Plug len: " << plugArr.length() << endl;

		//	MFnDependencyNode latestNode (plugArr[0].node());

		//	cout << latestNode.typeName() << endl;

		//	if (latestNode.typeName() == "polyExtrudeFace")
		//	{
		//		//Last node connected was polyExtrude.	
		//	}

		//	//MObject lookUp;
		//	//lookUp = latestNode.getAliasAttr(false);

		//	//if (lookUp.hasFn(MFn::kExtrude))
		//	//{
		//	//	cout << "------------Extrude-------------" << endl;
		//	//}

		//	/*latestNode.*/
		//}
	}
	else if ((msg & MNodeMessage::kConnectionMade) && (plug.partialName() == "iog"))
	{
		//New connection was made in the Inst Obj Groups. Most likely it's a material change.
		SendMaterialName(plug.node());

		/*SetupMaterials(plug.node());*/
		//MPlugArray connects;
		//plug.connectedTo(connects, false, true);
		//MFnDependencyNode shadeNode(connects[0].node());
		//MFnDependencyNode meshNode(plug.node());
		//cout << "Mesh '" << meshNode.name().asChar() << "' now has shader '" << shadeNode.name().asChar() << "'" << endl;
	}
	else if (msg & MNodeMessage::kConnectionMade)
	{
		cout<<"CONNECTION MADE: " << plug.name() << ": " << plug.partialName() << endl;
	}
}

void matChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	/* cout << plug.name() << ": " << plug.partialName() << endl;*/
	 //cout << "message:" << endl;
	 //cout << msg << endl;
	 //cout << plug.node().apiTypeStr() << endl;

	 if (msg & MNodeMessage::kAttributeSet)
	 {
		 //One of the attributes such as color or transparency has been set.
		 if (plug.partialName() == "c")
		 {
			 SendMaterial(plug.node());
			 SendMaterialNameMatInput(plug.node());
			 /*MFnDependencyNode mat(plug.node());*/
			 /*SendMaterialName()*/
			 cout << "color changed" << endl;
			 cout << "____" << endl;
		 }
	 }
	 else if (msg & MNodeMessage::kConnectionMade)
	 {
		 if (plug.partialName() == "c")
		 {
			 //Something was plugged into the color plug, most likely a texture.
			 SendMaterial(plug.node());
			 SendMaterialNameMatInput(plug.node());
			 cout << "connection MADE" << endl;
			 cout << "____" << endl;
		 }
	 }
	 else if (msg & MNodeMessage::kConnectionBroken)
	 {
		 if (plug.partialName() == "c")
		 {
			 //Something was unplugged from the color plug, most likely a texture.
			 SendMaterial(plug.node());
			 SendMaterialNameMatInput(plug.node());
			 cout << "connection Broken" << endl;
			 cout << "____" << endl;
		 }
	 }
	/* cout << "____" << endl;*/
	/*cout << "Material Info" << endl;*/
}

void textureChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	/*cout << plug.name() << ": " << plug.partialName() << endl;*/
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
		cout << "Texture changed for Material "<< matNode.name().asChar() <<" to " << texName << endl;
		SendMaterial(matPlugs[0].node());
		SendMaterialNameMatInput(matPlugs[0].node());
	}

}

void extrudeChange(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	cout << "EXTRUDE CHANGE ENTER" << endl;
	cout << plug.name() << ": " << plug.partialName() << endl;
	if (msg & MNodeMessage::kAttributeSet)
	{
		cout << "HAH!" << endl;
		//MObject transformNode = plug.node();

		//if (plug.isElement())
		//{
		//	cout << plug.partialName() << " Was changed! The vertex values are now: " << endl;
		//	MFnMesh inputMesh(plug.node());

		//	int index = -1;

		//	//Extract values.
		//	float x = 0;
		//	float y = 0;
		//	float z = 0;

		//	plug.child(0).getValue(x);
		//	plug.child(1).getValue(y);
		//	plug.child(2).getValue(z);

		//	/*producer.send(&z, sizeof(float));*/

		//	cout << "X: " << x << " Y: " << y << " Z: " << z << endl;
		//}
	}
	else if (msg & MNodeMessage::kAttributeEval && plug.partialName() == "out")
	{
		cout << "HOOO!" << endl;
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
	/*cout << node.apiType() << endl;*/
	//... implement this and other callbacks

	// since all nodes are dependency nodes this should print all nodes.
	MString name;
	MFnDependencyNode dependNode(node);
	name = dependNode.name();

	// Not entirely certain why but identical names are printed several times sometimes.
	// Might be important but for now I'd rather just see when they change.
	if (lastName != name)
	{
		cout << "Added node: " + name << endl;
		/*cout << node.apiTypeStr() << endl;*/
	}

	if (node.hasFn(MFn::kMesh))
	{
		// Saving the mesh in queue for later.
		newMeshes.push(node);

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

		//cout << shaders.length() << endl;

		MFloatPointArray vertexArr;
		inputMesh.getPoints(vertexArr);
		const char* temp = name.asChar();
		string meshName(temp);

		cout << meshName << endl;
		mapOfVertexArrays.insert(std::make_pair(meshName, vertexArr.length()));

		/*callbackIdArray.append(MPolyMessage::addPolyTopologyChangedCallback(node, topologyChanged, NULL, &status));*/
		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback (node, nodeMeshAttributeChanged, NULL, &status));
	}
	else if (node.hasFn(MFn::kTransform))
	{
		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(node, nodeTransformChanged, NULL, &status));
	}
	else if (node.hasFn(MFn::kShadingEngine))
	{
		SendMaterial(lastAddedNode);
		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(lastAddedNode, matChanged, NULL, &status));
	/*	MFnDependencyNode theNode(node);*/
		//MObjectArray shaders;
		//MIntArray indiciesPerFace;

		//MPlug sPlug = theNode.findPlug("surfaceShader",&status);
		//if (status == MS::kSuccess)
		//{
		//	MPlugArray mat;

		//	//Get the actual material
		//	sPlug.connectedTo(mat, true, false);
		//	cout << "nrOfPlugs: " << mat.length() << endl;
		//	//callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(mat[0].node(), matChanged2, NULL, &status));
		//}
		//else
		//{
		//	cout << "SURFACE SHADER WASN'T FOUND!!" << endl;
		//}
	}
	else if (node.hasFn(MFn::kFileTexture))
	{
		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(node, textureChanged, NULL, &status));
	}

	MFnDependencyNode latestNode(node);

	/*cout << latestNode.typeName() << endl;*/

	if (latestNode.typeName() == "polyExtrudeFace")
	{
		/*callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(node, extrudeChange, NULL, &status));*/
	}
	/*else if (node.hasFn(MFn::kExtrude))
	{
		cout << "''''''''''''''''''''''''I was extruded''''''''''''''''''''''''''''''''''''''" << endl;
	}*/

	lastAddedNode = node;
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
	mapOfVertexArrays.erase(name.asChar());
	cout << "Removed node: " + name << endl;
}

void timerCallback(float elapsedTime, float lastTime, void *clientData)
{
	/*cout << "Time Elapsed: " + to_string(elapsedTime) << endl;*/
	//if (batch.GetMasterHeader()->camChanged)
	//{
	//	cout << "CAM WAS CHANGED" << endl;
	//}

	if ((batch.GetMasterHeader()->meshCount != 0) || (batch.GetMasterHeader()->transformCount != 0) 
		|| batch.GetMasterHeader()->removedCount != 0 || batch.GetMasterHeader()->camSwitched 
		|| batch.GetMasterHeader()->matCount !=0 || batch.matSwitchedMap.size() != 0 || batch.GetMasterHeader()->zoomCount != 0
		|| batch.GetMasterHeader()->camCount !=0 || batch.GetMasterHeader()->numVertsChanged != 0)
	{
		batch.GetMasterHeader()->msgNr++;
		cout << "MsgNr: " << batch.GetMasterHeader()->msgNr << endl;
		producer.send(batch.GetMasterHeader(), sizeof(MasterHeader));
		cout<< "Mesh Count: " << batch.GetMasterHeader()->meshCount << endl;
		cout << "Transform Count " << batch.GetMasterHeader()->transformCount << endl;
		cout << "Cam Count " << batch.GetMasterHeader()->camCount << endl;
		cout << "Mat Count " << batch.GetMasterHeader()->matCount << endl;
		cout << "MatSwitched Count " << batch.GetMasterHeader()->matSwitchedCount << endl;
		cout << "Cam switched " << batch.GetMasterHeader()->camSwitched << endl;

		for (const auto& it1 : batch.camMap)
		{
			producer.send(it1.first.c_str(), it1.first.length());

			for (int i = 0; i < 6; i++)
			{
				cout << it1.second[i] << endl;
			}
			producer.send(it1.second, 6 * sizeof(float));
		}

		if (batch.GetMasterHeader()->camSwitched)
		{
			cout << "Sending Switched Camera: " << batch.getSwitchedName()->c_str() << endl;
			producer.send(batch.getSwitchedName()->c_str(), batch.getSwitchedName()->length());
		}

		for (int i = 0; i < batch.removeNames.size(); i++)
		{
			cout << "SENDING " << batch.removeNames[i].c_str() << endl;
			producer.send(batch.removeNames[i].c_str(), batch.removeNames[i].length());
		}

		for (const auto& mat : batch.matMap)
		{
			MatHeader head;
			cout << "Material Name: " << mat.first << endl;
			//this allows program to crash if name is too long. That's silly.
			//for (int i = 0; i < mat.first.length(); i++)
			//{
			//	head.matName[i] = mat.first.at(i);
			//}
			if (mat.second.name != "")
			{
				cout << "Sending mat: " << mat.first << endl;
				cout << "With texture: " << mat.second.name << endl;
				head.isTextured = true;
				head.lenMatName = mat.first.length();
				head.lenTextureName = mat.second.name.length();
				producer.send(&head, sizeof(head));

				producer.send(mat.first.c_str(), mat.first.length());
				producer.send(mat.second.name.c_str(), mat.second.name.length());
				//for (int i = 0; i < mat.second.name.length(); i++)
				//{
				//	head.textureName[i] = mat.second.name.at(i);
				//}
				//cout << "Texture Name: " << head.textureName << endl;
				//Allows me to use just one message but at what cost?
				/*producer.send(&head, sizeof(MatHeader));*/
			}
			else
			{
				head.isTextured = false;
				head.lenMatName = mat.first.length();
				head.lenTextureName = mat.second.name.length();
				producer.send(&head, sizeof(head));

				producer.send(mat.first.c_str(), mat.first.length());
				producer.send(mat.second.colors, mat.second.numFloats*sizeof(float));

				cout << "Sending" << endl;
				for (int i = 0; i < mat.second.numFloats; i++)
				{
					cout << "c" << i << ": " << mat.second.colors[i] << endl;
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
			cout << "MeshName: " << s << endl;
			cout << "IndexCount: " << meshHead.indexCount << endl;
			cout << "nrOfVerts: " << meshHead.nrOfVerts << endl;

			producer.send(&meshHead, sizeof(MeshHeader));
			producer.send(nr.second.GetIndicies(), meshHead.indexCount * sizeof(int));
			producer.send(nr.second.GetVerts(), meshHead.nrOfVerts * 8 * sizeof(float));
		}

		/*cout<<"SWITCHED COUNT: " << batch.GetMasterHeader()->matSwitchedCount << endl;*/

		for (const auto& nr : batch.matSwitchedMap)
		{
			cout << "Mesh: " << nr.first << endl;
			cout << "Switched material to: " << nr.second << endl;
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
			//cout << "---------------------------------" << endl;
			//cout << "SENDING TRANSFORM FOR: " << endl;
			//cout << it2.first << endl;

			//cout << "Translation: " << it2.second[0] << "," << it2.second[1] << "," << it2.second[2] << endl;
			//cout << "scale: " << it2.second[3] << "," << it2.second[4] << "," << it2.second[5] << endl;
			//cout << "Rotation: " << it2.second[6] << "," << it2.second[7] << "," << it2.second[8] << endl;
			//cout << "---------------------------------" << endl;
			/*int nr = 1;
			producer.send(&nr, sizeof(int));*/
			producer.send(it2.first.c_str(), it2.first.length());
			producer.send(it2.second, 10*sizeof(double));
			nrOfTransformsSent++;
		}

		for (const auto& vertMeshIt : batch.vertMap)
		{
			cout << "SENDING CHANGED VERTS FOR MESH: " << vertMeshIt.first << endl;
			int len = vertMeshIt.first.length();
			//Send name and size of name for mesh.
			producer.send(&len, sizeof(int));
			producer.send(vertMeshIt.first.c_str(), len);

			int nrOfVerts = vertMeshIt.second.size();
			producer.send(&nrOfVerts, sizeof(int));

			for (const auto& vertIt : vertMeshIt.second)
			{
				cout << "Point ID: " << vertIt.first << endl;
				//Send the vertex ID
				producer.send(&vertIt.first, sizeof(int));
				//Send the (local) transform coordinates.
				producer.send(vertIt.second, sizeof(float) * 3);
				for (int i = 0; i < 3; i++)
				{
					cout << i << ": " << vertIt.second[i] << endl;
				}
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

		cout << "Sent nr: " << nrOfTransformsSent << endl;
		cout << endl;
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
			cout << "Node name changed!" << endl;
			cout << "Node: " + prevName << endl;
			cout << "Is now Node: " + name << endl;
		}
	}
}

void viewChangedCB(const MString& str, MObject &node, void* clientData)
{
	cout << "CALLED!!" << endl;
	cout << str << endl;

	MFnDagNode dagSearch(node);
	MObject handle = dagSearch.parent(0);
	MFnDagNode parent(handle);

	cout << "Gives Node: " << parent.name().asChar() << endl;
	std::string tmp(parent.name().asChar());
	batch.SwitchedCamera(tmp);
}

double camMoveStartPos[3] = { 0.0f };
double rotStartPos[3] = { 0.0f };

MTransformationMatrix moveStartMatrix;

void cameraAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	/*cout << plug.name() << ":" << plug.partialName() << endl;*/
	if(msg & MNodeMessage::kAttributeSet)
	{
		/*cout << "Cam Transform Changed!" << endl;*/
		MObject transformNode = plug.node();

		if (transformNode.hasFn(MFn::kTransform))
		{
			MFnDagNode dagNode(plug.node());
			//How do filter away rotation translate, while keeping panning translate
			/*cout << plug.name() << ":" << plug.partialName() << endl;*/
			if ((plug.partialName() == "rpt") || (plug.partialName() == "t")
				|| (plug.partialName() == "tx") || (plug.partialName() == "ty") || (plug.partialName() == "tz"))
			{
				/*cout << plug.name() << ":" << plug.partialName() << endl;*/
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

					/*cout << "Quat: " << quat[i] << " Start: " << quatStartPos[i] << endl;*/
				}

				//cout << "Checking pos" << endl;
				//for (int i = 0; i < 3; i++)
				//{
				//	cout << i << ": " << camMoveStartPos[i] << endl;
				//}
				//cout << "Temp:" << endl;
				//for (int i = 0; i < 3; i++)
				//{
				//	cout << i << ": " << tmp[i] << endl;
				//}
		/*		cout << "Current Mat:" << endl;
				cout << aMat.asMatrix() << endl;
				cout << "Moved Mat:" << endl;
				cout << moveStartMatrix.asMatrix() << endl;*/
				//cout << "Compare" << endl;
				//Rounding off for inconsequential minor things.
				//for (int i = 0; i < 3; i++)
				//{
				//	quat[i] = roundf(quat[i] * 1000.0f) / 1000.0f;
				//	rotStartPos[i] = roundf(rotStartPos[i] * 1000.0f) / 1000.0f;

				//	/*cout << "Quat: " << quat[i] << " Start: " << quatStartPos[i] << endl;*/
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
					/*cout << "ENTERED" << endl;*/
					SendTransform(transformNode);
					producer.send(batch.GetMasterHeader(), sizeof(MasterHeader));
					for (const auto& it2 : batch.transformMap)
					{
						/*cout << it2.first << endl;*/
						producer.send(it2.first.c_str(), it2.first.length());
						producer.send(it2.second, 10 * sizeof(double));
					}
					batch.Reset();
				}
				else if((plug.partialName() == "rpt") && ((tmp[0] != camMoveStartPos[0]) || (tmp[1] != camMoveStartPos[1]) || (tmp[2] != camMoveStartPos[2])))
				{
					// If the camera has rotated we want the final value which is given by the "rpt" plug.
					// I may have to add the translation check too for the "bug"
					/*cout << "ENTERED ROTATION" << endl;*/
					SendTransform(transformNode);
					producer.send(batch.GetMasterHeader(), sizeof(MasterHeader));
					for (const auto& it2 : batch.transformMap)
					{
						/*cout << it2.first << endl;*/
						producer.send(it2.first.c_str(), it2.first.length());
						producer.send(it2.second, 10 * sizeof(double));
					}
					batch.Reset();
				}
				else
				{
					/*cout << "NO ENTRY" << endl;*/
				}
			}
		}
	}
	/* cout << plug.name() << ": " << plug.partialName() << endl;*/
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
		/*cout << "HEIGHT HERE: " <<cam.projectionMatrix()[2][2] << endl;*/
	}
	else
	{
		arr[1] = cam.horizontalFieldOfView() *(180 / 3.14159265359);
		arr[2] = cam.verticalFieldOfView()*(180 / 3.14159265359);
	}
	arr[3] = cam.aspectRatio();
	arr[4] = cam.nearClippingPlane();
	arr[5] = cam.farClippingPlane();

	MFnDagNode dagSearch(node);
	MObject handle = dagSearch.parent(0);
	MFnDagNode parent(handle);

	batch.SetCamera(arr, parent.name().asChar());
	/*cout <<
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
	cout << "____________________________________________________________" << endl;
	cout << "CAM MOVE START" << endl;
	cout << "For " << parent.name() << endl;
	//cout << "____________________________________________________________" << endl;

	MDagPath tNodeDag;
	parent.getPath(tNodeDag);
	MMatrix worldMat = tNodeDag.inclusiveMatrix();

	const MTransformationMatrix aMat(worldMat);

	/*moveStartMatrix = aMat;*/
	MVector trans = aMat.getTranslation(MSpace::kWorld, &status);
	trans.get(camMoveStartPos);

	MTransformationMatrix::RotationOrder wat;
	aMat.getRotation(rotStartPos, wat);
	cout << wat << endl;
	cout << "____________________________________________________________" << endl;
	/*aMat.getRotationQuaternion(quatStartPos[0], quatStartPos[1], quatStartPos[2], quatStartPos[3]);*/
	//for (int i = 0; i < 3; i++)
	//{
	//	cout << i << ": " << camMoveStartPos[i] << ", ";
	//}
	//cout << endl;

}

void viewPortChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{

}

void zoomies(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	cout << plug.name() << ":" << plug.partialName() << endl;
	if (plug.partialName() == "ow")
	{
		/*SendCam(plug.node());*/
		MFnDagNode dagSearch(plug.node());
		MObject handle = dagSearch.parent(0);
		MFnDagNode parent(handle);

		MFnCamera cam(plug.node());
		float size[2];
		size[0] = cam.orthoWidth();
		// [1][1] in an orthographic projection matrix = 2/top - bottom. 
		// In mayas case this result is also negated.
		// As such we can extract the height of the projection matrix using simple equation.
		// 2/height = [1][1] -> 2/[1][1] = height.
		size[1] = 2 / cam.projectionMatrix()[1][1];

		batch.SetCamOrthoZoom((string)parent.name().asChar(), size);
		cout << "The heck: " << cam.orthoWidth() << endl;
		cout << "Horizontal: " << cam.horizontalFieldOfView() *(180 / 3.14159265359) << endl;
		cout << "Vertical: " << cam.verticalFieldOfView()*(180 / 3.14159265359) << endl;
		cout << "Aspect ratio: " << cam.aspectRatio() << endl;
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
		MFnDagNode parent(handle);

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
			/*cout << modelPanels[i] << endl;*/
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
		MFnDagNode parent(handle);

		/*cout <<"Camera Name: " << dagSearch.name() << endl;*/
		//cout << "HERE" << endl;
		//cout << parent.name().asChar() << endl;
		//cout << dagSearch.name().asChar() << endl;

		SendCam(camIterator.thisNode());

		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(handle, cameraAttributeChanged, NULL, &status));
		callbackIdArray.append(MCameraMessage::addBeginManipulationCallback(camIterator.thisNode(), camMoveStart, NULL, &status));
		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(camIterator.thisNode(), zoomies, NULL, &status));
		
		//Trick the transform callback into triggering so all cameras start at correct positions in viewer.
		SendTransform(handle);

		camIterator.next();
	}

	cout << "nrOf3DViews: " << M3dView::numberOf3dViews() << endl;
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
	cout << "Cam Name: " << node.name().asChar() << endl;
	MObject handle = node.parent(0);
	MFnDagNode parent(handle);

	cout << "PARENT NAME: " << parent.name() << endl;
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
	
	// redirect cout to cerr, so that when we do cout goes to cerr
	// in the maya output window (not the scripting output!)
	std::cout.set_rdbuf(MStreamUtils::stdOutStream().rdbuf());
	std::cerr.set_rdbuf(MStreamUtils::stdErrorStream().rdbuf());
	cout << "Plugin loaded ===========================" << endl;
	
	// register callbacks here for
	callbackIdArray.append(MDGMessage::addNodeAddedCallback(nodeAdded, "dependNode", NULL, &status));
	callbackIdArray.append(MDGMessage::addNodeRemovedCallback(nodeRemoved, "dependNode", NULL, &status));
	callbackIdArray.append(MTimerMessage::addTimerCallback(0.02f, timerCallback, NULL, &status));
	callbackIdArray.append(MNodeMessage::addNameChangedCallback(MObject::kNullObj, nameChanged, NULL, &status));

	addCallbacksToExistingNodes();

	// a handy timer, courtesy of Maya
	gTimer.clear();
	gTimer.beginTimer();

	return res;
}
	

EXPORT MStatus uninitializePlugin(MObject obj) {
	MFnPlugin plugin(obj);

	cout << "Plugin unloaded =========================" << endl;

	MMessage::removeCallbacks(callbackIdArray);

	return MS::kSuccess;
}