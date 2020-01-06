#include "Mesh.h"

Mesh::Mesh()
{
	triIndicies = nullptr;
	verts = nullptr;
	nrOfVerts = -1;
	indexCount = -1;
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
	newVerts = new float[size];
	for (int i = 0; i < size; i++)
	{
		verts[i] = newVerts[i];
	}
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

int * Mesh::GetIndicies()
{
	return triIndicies;
}

float * Mesh::GetVerts()
{
	return verts;
}

//double * Mesh::GetTransform()
//{
//	return transform;
//}

int Mesh::GetNrOfVerts()
{
	return nrOfVerts;
}

int Mesh::GetIndexCount()
{
	return indexCount;
}
