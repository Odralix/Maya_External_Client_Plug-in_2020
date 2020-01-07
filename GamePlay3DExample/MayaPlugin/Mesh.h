#pragma once

class Mesh
{
private:
	int nrOfVerts;
	int indexCount;
	//char meshName[42];

	int* triIndicies;
	float* verts;
	//double transform[10];
public:
	Mesh();
	~Mesh();

	void SetTriIndicies(int* indicies, int size);
	void SetVerts(float* newVerts, int size);
	void SetTransform(double* newTransform);

	void SetNrOfVerts(int newNr);
	void SetICount(int newCount);

	int* GetIndicies() const;
	float* GetVerts() const;
	double* GetTransform();

	int GetNrOfVerts() const;
	int GetIndexCount() const;
};