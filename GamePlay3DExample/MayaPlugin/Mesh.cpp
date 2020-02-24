#include "Mesh.h"
#include <unordered_map>

Mesh::Mesh()
{
	triIndicies = nullptr;
	verts = nullptr;
	nrOfVerts = -1;
	indexCount = -1;
	matName = nullptr;
	//for (int i = 0; i < 10; i++)
	//{
	//	transform[i] = -1;
	//}
	//for (int i = 0; i < 42; i++)
	//{
	//	meshName[i] = '\0';
	//}
}

Mesh::~Mesh()
{
	delete[] triIndicies;
	delete[] verts;
	delete[] matName;
}

void Mesh::SetTriIndicies(int* indicies, int size)
{
	delete[] triIndicies;
	triIndicies = new int[size];
	for (int i = 0; i < size; i++)
	{
		triIndicies[i] = indicies[i];
	}
}

void Mesh::SetVerts(float* newVerts, int size)
{
	delete[] verts;
	//newVerts = new float[size]; <-- Possibly the dumbest and most basic mistake I've ever made.
	verts = new float[size];
	for (int i = 0; i < size; i++)
	{
		verts[i] = newVerts[i];
	}
}


void Mesh::SetMatName(const char * name, int len)
{
	if (matName != nullptr)
	{
		delete[] matName;
	}
	matName = new char[len];

	for (int i = 0; i < len; i++)
	{
		matName[i] = name[i];
	}
	matNameLen = len;
}

//void Mesh::SetTransform(double * newTransform)
//{
//	for (int i = 0; i < 10; i++)
//	{
//		transform[i] = newTransform[i];
//	}
//}

void Mesh::SetNrOfVerts(int newNr)
{
	nrOfVerts = newNr;
}

void Mesh::SetICount(int newCount)
{
	indexCount = newCount;
}

int * Mesh::GetIndicies() const
{
	return triIndicies;
}

float * Mesh::GetVerts() const
{
	return verts;
}

char * Mesh::GetMatName() const
{
	return matName;
}

int Mesh::GetMatLen() const
{
	return matNameLen;
}

//double * Mesh::GetTransform()
//{
//	return transform;
//}

int Mesh::GetNrOfVerts() const
{
	return nrOfVerts;
}

int Mesh::GetIndexCount() const
{
	return indexCount;
}
