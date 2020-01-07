#include "MayaBatchOutput.h"

MayaBatchOutput::MayaBatchOutput()
{
	m_MasterHead.camChanged = false;
	m_MasterHead.meshCount = 0;
	m_MasterHead.transformCount = 0;
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
			m_MasterHead.camChanged = true;
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
	m_MasterHead.camChanged = change;
}

void  MayaBatchOutput::RemoveObject(std::string name)
{
	removeNames.push_back(name);
	m_MasterHead.removedCount++;
}

MasterHeader* MayaBatchOutput::GetMasterHeader()
{
	return &m_MasterHead;
}

void MayaBatchOutput::Reset()
{
	m_MasterHead.camChanged = false;
	m_MasterHead.meshCount = 0;
	m_MasterHead.transformCount = 0;
	m_MasterHead.removedCount = 0;
	removeNames.clear();
	for (const auto& nr : transformMap)
	{
		delete[] nr.second;
	}
	transformMap.clear();
	meshMap.clear();
}
