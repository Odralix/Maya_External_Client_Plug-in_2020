#include "MayaBatchOutput.h"

MayaBatchOutput::MayaBatchOutput()
{
	m_MasterHead.camChanged = false;
	m_MasterHead.meshCount = 0;
	m_MasterHead.transformCount = 0;
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
	meshMap[head.meshName].SetVerts(verts, head.nrOfVerts*8);
}

void MayaBatchOutput::SetTransform(TransHeader head, double transform[10])
{
	if (transformMap.find(head.name) == transformMap.end())
	{
		//If not found
		double* arr = new double[10];
		transformMap[head.name] = arr;
		m_MasterHead.transformCount++;
	}

	for (int i = 0; i < 10; i++)
	{
		transformMap[head.name][i] = transform[i];
	}
}

void MayaBatchOutput::SetCamChanged(bool change)
{
	m_MasterHead.camChanged = change;
}

void MayaBatchOutput::Reset()
{
	m_MasterHead.camChanged = false;
	m_MasterHead.meshCount = 0;
	m_MasterHead.transformCount = 0;
	for (const auto& nr : transformMap)
	{
		delete[] nr.second;
	}
	transformMap.clear();
	meshMap.clear();
}
