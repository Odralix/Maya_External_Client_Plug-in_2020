#include "MayaBatchOutput.h"

MayaBatchOutput::MayaBatchOutput()
{
	m_MasterHead.meshCount = 0;
	m_MasterHead.transformCount = 0;
	m_MasterHead.camSwitched = false;
	m_SwitchedCamName = "";
	//Buffer size. Guessing user will rarely delete more than 10 objects at once.
}

MayaBatchOutput::~MayaBatchOutput()
{
	Reset();
}

void MayaBatchOutput::SetMesh(MeshHeader head, int * indexArr, float * verts)
{
	if (meshMap.find(head.meshName) == meshMap.end())
	{
		//Was not found.
		//Add it!
		Mesh newMesh;
		meshMap[head.meshName] = newMesh;
		m_MasterHead.meshCount++;
	}

	meshMap[head.meshName].SetICount(head.indexCount);
	meshMap[head.meshName].SetNrOfVerts(head.nrOfVerts);

	meshMap[head.meshName].SetTriIndicies(indexArr, head.indexCount);
	int temp = head.nrOfVerts * 8;
	meshMap[head.meshName].SetVerts(verts, temp);//Here
}

void MayaBatchOutput::SetTransform(TransHeader head, double transform[10])
{
	if (transformMap.find(head.name) == transformMap.end())
	{
		//If not found
		double* arr = new double[10];
		transformMap[head.name] = arr;
		m_MasterHead.transformCount++;

		std::string check(head.name);
		if (check == "persp")
		{
		
		}
	}

	//No need to delete. Only replace the values it will be deleted at reset/destruction.
	for (int i = 0; i < 10; i++)
	{
		transformMap[head.name][i] = transform[i];
	}
}

void MayaBatchOutput::SetCamChanged(bool change)
{
	//m_MasterHead.camChanged = change;
}

void MayaBatchOutput::SetCamera(float attr[6], std::string name)
{
	if (camMap.find(name) == camMap.end())
	{
		float* arr = new float[6];
		camMap[name] = arr;
		//Not found
	}

	for (int i = 0; i < 6; i++)
	{
		camMap[name][i] = attr[i];
	}
	m_MasterHead.camCount++;
}

void MayaBatchOutput::SetMaterial(std::string & matName, float * vals, int len)
{
	if (matMap[matName].colors != nullptr)
	{
		delete[] matMap[matName].colors;
	}
	else
	{
		//If a material is changed several times within one sending, 
		//we only need to count it once
		//The same is unnecesary in the texture version. 
		//Since it would be impossible to switch a texture manually within the timeframe.
		m_MasterHead.matCount++;
	}
	matMap[matName].colors = new float[len];
	matMap[matName].numFloats = len;

	for (int i = 0; i < len; i++)
	{
		matMap[matName].colors[i] = vals[i];
	}
}

void MayaBatchOutput::SetMaterial(std::string & matName, std::string& textureName)
{
	matMap[matName].name = textureName;
	m_MasterHead.matCount++;
}

void MayaBatchOutput::SetMatSwitched(std::string & meshName, std::string matName)
{
	if (matSwitchedMap.find(meshName) == matSwitchedMap.end())
	{
		//If a material is changed several times within one sending, 
		//we only need to count it once
		m_MasterHead.matSwitchedCount++;
	}
	matSwitchedMap[meshName] = matName;
}

void MayaBatchOutput::SetCamOrthoZoom(std::string & camName, float zoom[2])
{
	orthoZoomMap[camName][0] = zoom[0];
	orthoZoomMap[camName][1] = zoom[1];
	m_MasterHead.zoomCount++;
}

void MayaBatchOutput::SetVertPos(std::string & meshName, unsigned int vertID, float vertVals[4])
{
	if (vertMap.find(meshName) == vertMap.end())
	{
		//If a map doesn't exist yet it definitely hasn't been counted yet.
		m_MasterHead.numMeshChanged++;
	}
	//else if (vertMap[meshName].find(vertID) == vertMap[meshName].end())
	//{
	//	//Even if the meshName is there it may be a new vertID in which case it must be counted.
	//	//The map only contains the most recent change so we must only count each vert once.
	//	m_MasterHead.numVertsChanged++;
	//}
	for (int i = 0; i < 3; i++)
	{
		vertMap[meshName][vertID][i] = vertVals[i];
	}
	//Fill up float with a very specific value to show that it is not to be used.
	for (int i = 3; i < 8; i++)
	{
		vertMap[meshName][vertID][i] = 0.123456;
	}
}

void MayaBatchOutput::SetVert(std::string& meshName, unsigned int vertID, float vertVals[8])
{
	if (vertMap.find(meshName) == vertMap.end())
	{
		//If a map doesn't exist yet it definitely hasn't been counted yet.
		m_MasterHead.numMeshChanged++;
	}
	//else if (vertMap[meshName].find(vertID) == vertMap[meshName].end())
	//{
	//	//Even if the meshName is there it may be a new vertID in which case it must be counted.
	//	//The map only contains the most recent change so we must only count each vert once.
	//	m_MasterHead.numVertsChanged++;
	//}
	for (int i = 0; i < 8; i++)
	{
		vertMap[meshName][vertID][i] = vertVals[i];
	}
}

void MayaBatchOutput::SetRename(std::string & oldName, std::string& newName)
{
	//Only need to add the name once.
	if (renamingMap.find(oldName) == renamingMap.end())
	{
		m_MasterHead.numRenamed++;
	}

	renamingMap[oldName] = newName;
}

void  MayaBatchOutput::RemoveObject(std::string name)
{
	removeNames.push_back(name);
	m_MasterHead.removedCount++;
}

void MayaBatchOutput::SwitchedCamera(std::string& name)
{
	m_MasterHead.camSwitched = true;
	m_SwitchedCamName = name;
}

MasterHeader* MayaBatchOutput::GetMasterHeader()
{
	return &m_MasterHead;
}

std::string * MayaBatchOutput::getSwitchedName()
{
	return &m_SwitchedCamName;
}

void MayaBatchOutput::Reset()
{
	m_MasterHead.camSwitched = false;
	m_MasterHead.camCount = 0;
	m_MasterHead.meshCount = 0;
	m_MasterHead.transformCount = 0;
	m_MasterHead.removedCount = 0;
	m_MasterHead.matCount = 0;
	m_MasterHead.matSwitchedCount = 0;
	m_MasterHead.zoomCount = 0;
	m_MasterHead.numMeshChanged = 0;
	m_MasterHead.numRenamed = 0;
	removeNames.clear();
	for (const auto& nr : transformMap)
	{
		delete[] nr.second;
	}
	for (const auto& nr : camMap)
	{
		delete[] nr.second;
	}
	for (const auto& nr : matMap)
	{
		delete[] nr.second.colors;
	}

	camMap.clear();
	transformMap.clear();
	meshMap.clear();
	matMap.clear();
	matSwitchedMap.clear();
	orthoZoomMap.clear();
	vertMap.clear();
	renamingMap.clear();
}
