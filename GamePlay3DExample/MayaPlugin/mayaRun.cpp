// Original Example expanded upon by Ossian Edström
#include "maya_includes.h"
#include <maya/MTimer.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>
#include <string>
#include <map>
#include "ComLib_reference.h"
#include "../Structs.h"

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
			//Replace old vertex array and print the new info out.
			mapOfVertexArrays[meshName] = vertexArr.length();
			cout << meshNode.name() << " Topology Changed! " << "New vertex locations: " << endl;
		/*	VertexArray verts;*/

		//	float ** a = new float*[vertexArr.length()];

		//	for (int i = 0; i < vertexArr.length(); i++)
		//	{
		//		a[i] = new float[4];
		//	}

		//	/*float arr*[4] = new float*[4];*/

		////https://stackoverflow.com/questions/1810083/c-pointers-pointing-to-an-array-of-fixed-size

		//	vertexArr.get(a);

		//	producer.send(&verts, sizeof(verts));

			//Getting the number of verts
			MeshHeader meshHead;
			meshHead.nrOfVerts = inputMesh.numFaceVertices(&status);

			//Getting the Index count
			MIntArray triCounts;
			MIntArray triVerts;

			// I HAAAAVE TO GET INDICES A DIFFERENT WAY. THIS IS INNACURATE FOR OTHER PROGRAM
			inputMesh.getTriangleOffsets(triCounts, triVerts);

			//int fID = 0;
			//for (int i = 0; i < triVerts.length(); i++)
			//{
			//	if (i != 0)
			//	{
			//		if (i % 6 == 0)
			//		{
			//			fID++;
			//		}
			//		//FaceID*3 + FaceID - FaceVertexID = vertID


			//	}
			//}

			MIntArray nrOfTri;
			MIntArray triVertsID;

			inputMesh.getTriangles(nrOfTri, triVertsID);

			cout << nrOfTri << endl;

			MIntArray faceVertIndicies;

			MIntArray TheFinalVerts;


			cout << triVertsID << endl;

			int trisCount = 0;
			for (int i = 0; i < triVertsID.length(); i++)
			{
	
				if (trisCount < 3 )
				{
					faceVertIndicies.append(triVertsID[i]);
					trisCount++;
				}
				else
				{
					//Skip 2 duplicates.
					i += 2;
					//Append the 4th vert before moving on to the next set of triangles.
					faceVertIndicies.append(triVertsID[i]);
					trisCount = 0;
				}
			}

			//trisCount = 0;
			//int trisID = 0;

			////I will ignore N-gon case. Assume all Tris or Quads
			//int nrOfPolygons = nrOfTri.length();
			//int polyID = 0;

			//for (int i = 0; i < triVertsID.length(); i++)
			//{
			//	//If starting on new triangle
			//	if (trisCount == 2)
			//	{
			//		trisCount = 0;
			//		trisID++;
			//	}

			//	int polyCheck = 0;

			//	for (int j = 0; j < nrOfPolygons; j++)
			//	{
			//		polyCheck += nrOfTri[j];

			//		if (polyCheck >= trisID)
			//		{
			//			polyID = j;
			//			j = nrOfPolygons;
			//		}
			//	}

			///*	int vertList[3] = { 0 };
			//	inputMesh.getPolygonTriangleVertices(polyID, trisID, vertList);*/
			//	MIntArray polyVertList;

			//	inputMesh.getPolygonVertices(polyID, polyVertList);

			//	//Note formula only works for a mesh made of only quads.
			//	TheFinalVerts.append(polyVertList[0]);
			//	TheFinalVerts.append(polyVertList[1]);
			//	TheFinalVerts.append(polyVertList[2]);

			//	trisCount++;
			//}

			//cout << "______________________" << endl;
			//cout << TheFinalVerts << endl;


			cout << faceVertIndicies << endl;

			float * vPosArr = new float[faceVertIndicies.length() * 3];
			int posCount = 0;

			for (int i = 0; i < faceVertIndicies.length(); i++)
			{
				MPoint pos;
				inputMesh.getPoint(faceVertIndicies[i], pos);
				float arr[4] = { 0.0f };
				pos.get(arr);

				vPosArr[posCount] = arr[0];
				posCount++;
				vPosArr[posCount] = arr[1];
				posCount++;
				vPosArr[posCount] = arr[2];
				posCount++;
			}

			int n = 0;
			for (int i = 0; i < faceVertIndicies.length() * 3; i++)
			{

				cout << vPosArr[i] << " ";
				n++;
				if (n == 3)
				{
					cout << endl;
					n = 0;
				}
			}
			//_______________________________________________________________________________
			/*MIntArray vCount;
			MIntArray vList;
			inputMesh.getVertices(vCount, vList);

			cout << vCount << endl;
			cout << vList << endl;

			float * vPosArr = new float[vList.length() * 3];
			int posCount = 0;

			for (int i = 0; i < vList.length(); i++)
			{
				MPoint pos;
				inputMesh.getPoint(vList[i], pos);
				float arr [4] = { 0.0f };
				pos.get(arr);

				vPosArr[posCount] = arr[0];
				posCount++;
				vPosArr[posCount] = arr[1];
				posCount++;
				vPosArr[posCount] = arr[2];
				posCount++;
			}

			int n = 0;
			for (int i = 0; i < vList.length()*3; i++)
			{

				cout << vPosArr[i] << " ";
				n++;
				if (n == 3)
				{
					cout << endl;
					n = 0;
				}
			}*/

			//MFloatPointArray points;
			//inputMesh.getPoints(points, MSpace::kWorld);
			//

			//MFloatPoint * pointArr = new MFloatPoint[]


			meshHead.indexCount = triVerts.length();

			int * triIndicies = new int[meshHead.indexCount];
			triVerts.get(triIndicies);

			for (int i = 0; i < meshHead.indexCount; i++)
			{
				cout << triIndicies[i] << " ";
			}
			cout << endl;

	/*		MIntArray vtxCount;
			MIntArray vtxArr;

			inputMesh.getVertices(vtxCount, vtxArr);

			cout << vtxArr << endl;*/

			//MIntArray collectTris;

			//MItMeshPolygon iterator2(Mnode, &status);
			//while (!iterator2.isDone())
			//{
			//	MPointArray triPoints;
			//	MIntArray triList;
			//	int count = 0;
			//	iterator2.numTriangles(count);
			//	iterator2.getTriangles(triPoints, triList);

			//	for (int i = 0; i < count; i++)
			//	{
			//		collectTris.append(triList[i]);
			//	}
			//	//for (int i = 0; i < iterator2.polygonVertexCount(); i++)
			//	//{
			//	//	
			//	//	cout << iterator2.polygonVertexCount() << endl;
			//	//	cout << iterator2. << endl;
			//	//}
			//	iterator2.next();
			//}

			//for (int i = 0; i < collectTris.length(); i++)
			//{
			//	cout << collectTris[i] << " ";
			//}
			//cout << endl;

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

			int * newTriIndicies = new int[meshHead.indexCount];
			int iCount = 0;

			int triCount = 0;

			MVector normals;
			int i = 0;
			MItMeshFaceVertex iterator(Mnode, &status);
			while (!iterator.isDone())
			{
				iterator.position(MSpace::kWorld, &status).get(posArr[i]);
				iterator.getNormal(normals, MSpace::kObject);
				float2 temp;
				iterator.getUV(temp);

				//for (int j = 0; j < meshHead.indexCount; j++)
				//{
				//	if (triVerts[j] == i)
				//	{
				//		newTriIndicies[iCount] = triVerts[j];
				//		triVerts[j] = -1; //So it never matches again.
				//		iCount++;
				//	}
				//}

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
				/*cout << endl;*/
				/*cout << "]" << endl;*/

				/*		iterator.getNormal();
						iterator.getBinormal();
						iterator.getTangent();
						iterator.getUV()*/

				//cout << "[";
				//for (int j = 0; j < 4; j++)
				//{
				//	cout << posArr[i][j];
				//	cout << " ";
				//}
				//cout << "]" << endl;

				i++;
				triCount++;
				iterator.next();
			}

			//for (int i = 0; i < meshHead.indexCount; i++)
			//{
			//	cout << newTriIndicies[i] << " ";
			//}
			//cout << endl;

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
				cout << endl;

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

			/*cout << "Single Array: " << endl;
			cout << "nr of verts: " << endl;
			cout << meshHead.nrOfVerts << endl;
			int c = 0;
			for (int i = 0; i < numFaceVertices*3 *2; i++)
			{
				cout << verts[i] << " ";
				c++;
				if (c == 6)
				{
					cout << endl;
					c = 0;
				}
			}
			cout << endl;*/

			//for (int i = 0; i < meshHead.indexCount; i++)
			//{
			//	cout << triIndicies[i] << " ";
			//}
			//cout << endl;

			producer.send(&meshHead, sizeof(MeshHeader));
			producer.send(triIndicies, meshHead.indexCount * sizeof(int));
			producer.send(verts, meshHead.nrOfVerts * 8 * sizeof(float));

			for (int i = 0; i < numFaceVertices; i++)
			{
				delete[] posArr[i];
			}
			delete[] posArr;
			delete[] newTriIndicies;

			delete[] UVArr;
		/*	producer.send(posArr, meshHead.nrOfVerts * 4 * sizeof(float));*/

			/*producer.send(points)*/
			//for (int i = 0; i < inputMesh.numVertices; i++)
			//{
			//	if (i = 0)
			//	{
			//		producer.send(&inputMesh.numVertices, sizeof(int));
			//	}
			//	producer.send(&points[i], sizeof(float));
			//}

			delete[] verts;

			/*cout << vertexArr << endl;*/
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
	callbackIdArray.append(MTimerMessage::addTimerCallback(5, timerCallback, NULL, &status));
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