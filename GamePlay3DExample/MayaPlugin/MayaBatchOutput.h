#pragma once
#include "../Structs.h"
#include "Mesh.h"
#include <unordered_map>
#include <string>
#include <vector>

class MayaBatchOutput
{
private:
	MasterHeader m_MasterHead;

public:
	MayaBatchOutput();
	~MayaBatchOutput();

	void SetMesh(MeshHeader head, int* indexArr, float* verts);
	void SetTransform(TransHeader head, double transform[10]);
	void SetCamChanged(bool change);

	void RemoveObject(std::string name);

	MasterHeader* GetMasterHeader();
	std::unordered_map<std::string, Mesh> meshMap;
	std::unordered_map <std::string, double*> transformMap;
	std::vector<std::string> removeNames;


	void Reset();
};
