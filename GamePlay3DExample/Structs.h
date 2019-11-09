#pragma once

struct MsgType
{
	enum type
	{
		mesh = 0,
		transform = 1,
		nameChange = 2,
	};
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