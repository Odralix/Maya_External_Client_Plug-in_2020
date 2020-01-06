#pragma once

#define NAME_LEN 42

struct MasterHeader
{
	int meshCount;
	int transformCount;
	//int matCount;
	bool camChanged;
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
	char meshName[NAME_LEN];
};

struct TransHeader
{
	char name[NAME_LEN];
	//double transform[10];
};

struct NewName
{
	char oldName[NAME_LEN];
	char newName[NAME_LEN];
};