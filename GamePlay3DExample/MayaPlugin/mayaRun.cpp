// Original Example expanded upon by Ossian Edström
#include "maya_includes.h"
#include <maya/MTimer.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>
//#include <string>
#include <map>
#include "ComLib_reference.h"
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

void nodeTransformChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	if (msg & MNodeMessage::kAttributeSet)
	{
		MObject transformNode = plug.node();

		if (transformNode.hasFn(MFn::kTransform))
		{
			MFnDependencyNode nameFetch(plug.node());

			cout << nameFetch.name() << " Transform changed!" << endl;

			//Local transform
			MFnTransform getTransform(transformNode);

			MMatrix tMat = getTransform.transformation().asMatrix();

			//Global Transform:
			MFnDagNode path(transformNode);
			MDagPath tNodeDag;

			path.getPath(tNodeDag);

			MMatrix worldMat = tNodeDag.inclusiveMatrix();

			cout << "Local Transform: " << endl;
			cout << tMat << endl;

			cout << " Global Transform: " << endl;
			cout << worldMat << endl;

		/*	float transform[4][4] = { 0 };

			worldMat.get(transform);*/

			MFnTransform parser;
			const MTransformationMatrix aMat(worldMat);
		/*	parser.set(aMat);

			cout << worldMat << endl;
			cout << parser.transformation(&status).asMatrix() << endl;
			cout << aMat.asMatrix() << endl;

			MVector translation = parser.getTranslation(MSpace::kWorld);
			double transDouble[3];
			translation.get(transDouble);

			double scaleDouble[3];
			parser.getScale(scaleDouble);

			double quatDouble[4];
			parser.getRotationQuaternion(quatDouble[0], quatDouble[1], quatDouble[2], quatDouble[3], MSpace::kWorld);
			*/

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
				transform[i] = quatDouble[i-6];
			}

			int len = 0;
			const char* name = nameFetch.name().asChar(len);
			
			for (int i = 0; i < 10; i++)
			{
				cout << transform[i] << endl;
			}
			
			/*int nr = 1;
			producer.send(&nr, sizeof(int));
			producer.send(name, len);
			producer.send(transform, 10*sizeof(double));*/
			TransHeader head;
			for (int i = 0; i < len; i++)
			{
				head.name[i] = name[i];
			}
			batch.SetTransform(head, transform);
		}
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
			cout << plug.partialName() << " Was changed! The vertex values are now: " << endl;
			MFnMesh inputMesh(plug.node());

			int index = -1;

			//Extract values.
			float x = 0;
			float y = 0;
			float z = 0;

			plug.child(0).getValue(x);
			plug.child(1).getValue(y);
			plug.child(2).getValue(z);

			/*producer.send(&z, sizeof(float));*/

			cout << "X: " << x << " Y: " << y << " Z: " << z << endl;
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

			for (int i = 0; i < meshHead.indexCount; i++)
			{
				cout << triIndicies[i] << " ";
			}
			cout << endl;

			int numFaceVertices = meshHead.nrOfVerts;

			float ** posArr = new float*[numFaceVertices];
			for (int i = 0; i < numFaceVertices; i++)
			{
				posArr[i] = new float[4];
			}

			float * normArr = new float[numFaceVertices*3];
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
			// Sending the collected information.

			/*MsgType type = meshType;*/
			//int nr = 0;
			//producer.send(&nr, sizeof(int));
			//producer.send(&meshHead, sizeof(MeshHeader));
			//producer.send(triIndicies, meshHead.indexCount * sizeof(int));
			//producer.send(verts, meshHead.nrOfVerts * 8 * sizeof(float));
			batch.SetMesh(meshHead, triIndicies, verts);//And here

			for (int i = 0; i < numFaceVertices; i++)
			{
				delete[] posArr[i];
			}
			delete[] posArr;
			delete[] UVArr;
			delete[] normArr;
			delete[] verts;
		}
	}
}


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
	}

	if (node.hasFn(MFn::kMesh))
	{
		// Saving the mesh in queue for later.
		newMeshes.push(node);

		// Creating a map between the mesh and its first vertex array in order to prevent multiple printing.
		MFnMesh inputMesh(node);
		MFloatPointArray vertexArr;
		inputMesh.getPoints(vertexArr);
		const char* temp = name.asChar();

		string meshName(temp);

		mapOfVertexArrays.insert(std::make_pair(meshName, vertexArr.length()));

		/*callbackIdArray.append(MPolyMessage::addPolyTopologyChangedCallback(node, topologyChanged, NULL, &status));*/
		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback (node, nodeMeshAttributeChanged, NULL, &status));
	}
	else if (node.hasFn(MFn::kTransform))
	{
		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(node, nodeTransformChanged, NULL, &status));
	}

	lastName = name;
}

void nodeRemoved(MObject &node, void * clientData)
{
	MString name;
	MFnDependencyNode dependNode(node);
	name = dependNode.name();

	cout << "Removed node: " + name << endl;
}

void timerCallback(float elapsedTime, float lastTime, void *clientData)
{
	/*cout << "Time Elapsed: " + to_string(elapsedTime) << endl;*/
	if (batch.GetMasterHeader()->camChanged)
	{
		cout << "CAM WAS CHANGED" << endl;
	}

	if ((batch.GetMasterHeader()->meshCount != 0) || (batch.GetMasterHeader()->transformCount != 0))
	{
		producer.send(batch.GetMasterHeader(), sizeof(MasterHeader));
		//cout<< "Mesh Count: " << batch.GetMasterHeader()->meshCount << endl;
		//cout << "Transform Count " << batch.GetMasterHeader()->transformCount << endl;

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

			/*for (int i = 0; i < meshHead.indexCount; i++)
			{
				cout << "index Nr:" << endl;
				cout << nr.second.GetIndicies()[i] << endl;
			}
			for (int i = 0; i < meshHead.nrOfVerts * 8; i += 8)
			{
				cout << "Vert nr:" << i / 8 << endl;
				cout << "Pos: " << nr.second.GetVerts()[i] << "," << nr.second.GetVerts()[i + 1] << "," << nr.second.GetVerts()[i + 2] << endl;
				cout << "Norm: " << nr.second.GetVerts()[i + 3] << "," << nr.second.GetVerts()[i + 4] << "," << nr.second.GetVerts()[i + 5] << endl;
				cout << "UV: " << nr.second.GetVerts()[i + 6] << "," << nr.second.GetVerts()[i + 7] << "," << nr.second.GetVerts()[i + 8] << endl;
			}*/
			/*MsgType type = meshType;*/
			//int nr = 0;
			//producer.send(&nr, sizeof(int));
			producer.send(&meshHead, sizeof(MeshHeader));
			producer.send(nr.second.GetIndicies(), meshHead.indexCount * sizeof(int));
			producer.send(nr.second.GetVerts(), meshHead.nrOfVerts * 8 * sizeof(float));
		}

		for (const auto& it2 : batch.transformMap)
		{
			cout << it2.first << endl;

	/*		cout << "Translation: " << it2.second[0] << "," << it2.second[1] << "," << it2.second[2] << endl;
			cout << "scale: " << it2.second[3] << "," << it2.second[4] << "," << it2.second[5] << endl;
			cout << "Rotation: " << it2.second[6] << "," << it2.second[7] << "," << it2.second[8] << endl;*/
			/*int nr = 1;
			producer.send(&nr, sizeof(int));*/
			producer.send(it2.first.c_str(), it2.first.length());
			producer.send(it2.second, 10*sizeof(double));
		}
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

//DON'T FORGET Sending initialize info to the viewer should also happen here
void addCallbacksToExistingNodes()
{

	MItDependencyNodes iterator(MFn::kMesh);
	MItDependencyNodes iterator2(MFn::kTransform);

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

		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(iterator.thisNode(), nodeMeshAttributeChanged, NULL, &status));

		iterator.next();
	}

	while (!iterator2.isDone())
	{
		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(iterator2.thisNode(), nodeTransformChanged, NULL, &status));
		iterator2.next();
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