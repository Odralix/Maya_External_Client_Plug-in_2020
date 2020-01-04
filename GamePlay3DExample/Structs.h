#pragma once

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
	char meshName[42];
};

struct TransHeader
{
	char name[42];
	//double transform[10];
};

struct NewName
{
	char oldName[42];
	char newName[42];
};