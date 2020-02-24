#pragma once

#define NAME_LEN 42

struct MasterHeader
{
	int meshCount;
	int transformCount;
	int removedCount;
	int camCount;
	int matCount;
	int matSwitchedCount;
	int zoomCount;
	int numMeshChanged;
	//int matCount;
	//Has the viewer switched camera.
	bool camSwitched = false;
	int msgNr = 0;
};

enum MsgType
{
	meshType = 0,
	transformType = 1,
	nameChangeType = 2,
};

struct MeshHeader
{
	int nrOfVerts;
	int indexCount;
	char meshName[NAME_LEN] = { '\0' };
};

struct MatHeader
{
	bool isTextured = false;
	int lenMatName;
	int lenTextureName;
	//char matName[NAME_LEN] = { '\0' };
	//char textureName[NAME_LEN] = { '\0' };

};

struct MatSwitchedHeader
{
	bool newIsTextured = false;
	int lenMeshName;
	int lenMatName;
};

struct TransHeader
{
	char name[NAME_LEN] = { '\0' };
	//double transform[10];
};

struct NewName
{
	char oldName[NAME_LEN] = { '\0' };
	char newName[NAME_LEN] = { '\0' };
};