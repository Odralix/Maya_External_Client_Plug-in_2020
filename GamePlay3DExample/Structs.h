#pragma once


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

struct NewName
{
	char oldName[42];
	char newName[42];
};