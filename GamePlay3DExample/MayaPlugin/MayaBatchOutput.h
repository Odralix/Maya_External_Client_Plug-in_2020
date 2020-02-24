#pragma once
#include "../Structs.h"
#include "Mesh.h"
#include "TempStruct.h"
#include <unordered_map>
#include <vector>

class MayaBatchOutput
{
private:
	MasterHeader m_MasterHead;
	std::string m_SwitchedCamName;
public:
	MayaBatchOutput();
	~MayaBatchOutput();

	void SetMesh(MeshHeader head, int* indexArr, float* verts);
	void SetTransform(TransHeader head, double transform[10]);
	void SetCamChanged(bool change);
	void SetCamera(float attr[6], std::string Name);
	void SetMaterial(std::string &matName,float* vals, int len);
	void SetMaterial(std::string &matName, std::string &textureName);
	void SetMatSwitched(std::string & meshName, std::string matName);
	void SetCamOrthoZoom(std::string & camName, float zoom[2]);
	void SetVertPos(std::string & meshName, unsigned int vertID, float vertVals[4]);
	void SetVert(std::string& meshName, unsigned int vertID, float vertVals[8]);
	/*void SetVerts(std::string & meshName, std::vector<float[3]> &changedVerts);*/

	void RemoveObject(std::string name);
	void SwitchedCamera(std::string& name);
	std::string* getSwitchedName();

	MasterHeader* GetMasterHeader();
	std::unordered_map<std::string, Mesh> meshMap;
	std::unordered_map <std::string, double*> transformMap;
	std::vector<std::string> removeNames;
	std::unordered_map <std::string, float*> camMap;
	std::unordered_map <std::string,materialTemp> matMap;
	std::unordered_map <std::string, std::string> matSwitchedMap;
	std::unordered_map <std::string, float[2]> orthoZoomMap;
	std::unordered_map<std::string, std::unordered_map<unsigned int, float[8]>> vertMap;
	//std::unordered_map<std::string, std::vector<float[3]>> movedVertMap;
	//std::unordered_map<std::string, float[4]> vertMap;

	void Reset();
};
