#include "MayaViewer.h"
#include "ComLib_reference.h"
#include <string>

// Declare our game instance
MayaViewer game;

constexpr int gModelCount = 1;
static bool gKeys[256] = {};
int gDeltaX;
int gDeltaY;
bool gMousePressed;

ComLib consumer((std::string)"stuff", 200* 1024*1024, ComLib::CONSUMER);

MayaViewer::MayaViewer()
    : _scene(NULL), _wireframe(false)
{
}

void MayaViewer::initialize()
{
    // Load game scene from file
	_scene = Scene::create();

	Camera* cam = Camera::createPerspective(45.0f, getAspectRatio(), 1.0, 100.0f);
	Node* cameraNode = _scene->addNode("orig");
	cameraNode->setCamera(cam);
	_scene->setActiveCamera(cam);
	SAFE_RELEASE(cam);

	cameraNode->translate(0, 0, 20);
	cameraNode->rotateX(MATH_DEG_TO_RAD(0.f));

	Node* lightNode = _scene->addNode("light");
	Light* light = Light::createPoint(Vector3(0.5f, 0.5f, 0.5f), 20);
	lightNode->setLight(light);
	SAFE_RELEASE(light);
	lightNode->translate(Vector3(0, 1, 5));

	msgDirector();
}

void MayaViewer::finalize()
{
    SAFE_RELEASE(_scene);
}

void MayaViewer::update(float elapsedTime)
{
	Node* camnode = _scene->getActiveCamera()->getNode();
	if (gKeys[Keyboard::KEY_W])
		camnode->translateForward(0.5);
	if (gKeys[Keyboard::KEY_S])
		camnode->translateForward(-0.5);
	if (gKeys[Keyboard::KEY_A])
		camnode->translateLeft(0.5);
	if (gKeys[Keyboard::KEY_D])
		camnode->translateLeft(-0.5);

	//Handle all incoming messages.
	msgDirector();

	if (gMousePressed) {
		camnode->rotate(camnode->getRightVectorWorld(), MATH_DEG_TO_RAD(gDeltaY / 10.0));
		camnode->rotate(camnode->getUpVectorWorld(), MATH_DEG_TO_RAD(gDeltaX / 5.0));
	}
}

void MayaViewer::render(float elapsedTime)
{
    // Clear the color and depth buffers
    clear(CLEAR_COLOR_DEPTH, Vector4(0.1f,0.0f,0.0f,1.0f), 1.0f, 0);

    // Visit all the nodes in the scene for drawing
    _scene->visit(this, &MayaViewer::drawScene);
}

bool MayaViewer::drawScene(Node* node)
{
    // If the node visited contains a drawable object, draw it
    Drawable* drawable = node->getDrawable(); 
    if (drawable)
        drawable->draw(_wireframe);

    return true;
}

void MayaViewer::keyEvent(Keyboard::KeyEvent evt, int key)
{
    if (evt == Keyboard::KEY_PRESS)
    {
		gKeys[key] = true;
        switch (key)
        {
        case Keyboard::KEY_ESCAPE:
            exit();
            break;
		};
    }
	else if (evt == Keyboard::KEY_RELEASE)
	{
		gKeys[key] = false;

		switch (key)
		{
		case Keyboard::KEY_P:
			break;
		};
	}
}

bool MayaViewer::mouseEvent(Mouse::MouseEvent evt, int x, int y, int wheelDelta)
{
	static int lastX = 0;
	static int lastY = 0;
	gDeltaX = lastX - x;
	gDeltaY = lastY - y;
	lastX = x;
	lastY = y;
	gMousePressed =
		(evt == Mouse::MouseEvent::MOUSE_PRESS_LEFT_BUTTON) ? true :
		(evt == Mouse::MouseEvent::MOUSE_RELEASE_LEFT_BUTTON) ? false : gMousePressed;

	return true;
}

void MayaViewer::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex)
{
    switch (evt)
    {
    case Touch::TOUCH_PRESS:
        _wireframe = !_wireframe;
        break;
    case Touch::TOUCH_RELEASE:
        break;
    case Touch::TOUCH_MOVE:
        break;
    };
}

Mesh* MayaViewer::createCubeMesh(float size)
{
	float a = size * 0.5f;
	float vertices[] =
	{
		-a, -a,  a,    0.0,  0.0,  1.0,   0.0, 0.0,
		a, -a,  a,    0.0,  0.0,  1.0,   1.0, 0.0,
		-a,  a,  a,    0.0,  0.0,  1.0,   0.0, 1.0,
		a,  a,  a,    0.0,  0.0,  1.0,   1.0, 1.0,
		-a,  a,  a,    0.0,  1.0,  0.0,   0.0, 0.0,
		a,  a,  a,    0.0,  1.0,  0.0,   1.0, 0.0,
		-a,  a, -a,    0.0,  1.0,  0.0,   0.0, 1.0,
		a,  a, -a,    0.0,  1.0,  0.0,   1.0, 1.0,
		-a,  a, -a,    0.0,  0.0, -1.0,   0.0, 0.0,
		a,  a, -a,    0.0,  0.0, -1.0,   1.0, 0.0,
		-a, -a, -a,    0.0,  0.0, -1.0,   0.0, 1.0,
		a, -a, -a,    0.0,  0.0, -1.0,   1.0, 1.0,
		-a, -a, -a,    0.0, -1.0,  0.0,   0.0, 0.0,
		a, -a, -a,    0.0, -1.0,  0.0,   1.0, 0.0,
		-a, -a,  a,    0.0, -1.0,  0.0,   0.0, 1.0,
		a, -a,  a,    0.0, -1.0,  0.0,   1.0, 1.0,
		a, -a,  a,    1.0,  0.0,  0.0,   0.0, 0.0,
		a, -a, -a,    1.0,  0.0,  0.0,   1.0, 0.0,
		a,  a,  a,    1.0,  0.0,  0.0,   0.0, 1.0,
		a,  a, -a,    1.0,  0.0,  0.0,   1.0, 1.0,
		-a, -a, -a,   -1.0,  0.0,  0.0,   0.0, 0.0,
		-a, -a,  a,   -1.0,  0.0,  0.0,   1.0, 0.0,
		-a,  a, -a,   -1.0,  0.0,  0.0,   0.0, 1.0,
		-a,  a,  a,   -1.0,  0.0,  0.0,   1.0, 1.0
	};
	short indices[] =
	{
		0, 1, 2, 2, 1, 3, 4, 5, 6, 6, 5, 7, 8, 9, 10, 10, 9, 11, 12, 13, 14, 14, 13, 15, 16, 17, 18, 18, 17, 19, 20, 21, 22, 22, 21, 23
	};
	unsigned int vertexCount = 24;
	unsigned int indexCount = 36;
	VertexFormat::Element elements[] =
	{
		VertexFormat::Element(VertexFormat::POSITION, 3),
		VertexFormat::Element(VertexFormat::NORMAL, 3),
		VertexFormat::Element(VertexFormat::TEXCOORD0, 2)
	};
	Mesh* mesh = Mesh::createMesh(VertexFormat(elements, 3), vertexCount, false);
	if (mesh == NULL)
	{
		GP_ERROR("Failed to create mesh.");
		return NULL;
	}
	mesh->setVertexData(vertices, 0, vertexCount);
	MeshPart* meshPart = mesh->addPart(Mesh::TRIANGLES, Mesh::INDEX16, indexCount, false);
	meshPart->setIndexData(indices, 0, indexCount);
	return mesh;
}

Mesh * MayaViewer::createImportMesh(float * verts, int* indicies, int vtxNr, int indexNr)
{

	VertexFormat::Element elements[] =
	{
		VertexFormat::Element(VertexFormat::POSITION, 3),
		VertexFormat::Element(VertexFormat::NORMAL, 3),
		VertexFormat::Element(VertexFormat::TEXCOORD0, 2)
	};
	Mesh* mesh = Mesh::createMesh(VertexFormat(elements, 3), vtxNr, false);

	if (mesh == NULL)
	{
		GP_ERROR("Failed to create mesh.");
		return NULL;
	}

	for (int i = 0; i < indexNr; i++)
	{
		int anInt = indicies[i];
		std::cout << anInt << std::endl;
	}
	//here
	mesh->setVertexData(verts, 0, vtxNr);
	MeshPart* meshPart = mesh->addPart(Mesh::TRIANGLES, Mesh::INDEX32, indexNr, false);
	meshPart->setIndexData(indicies, 0, indexNr);

	return mesh;
}

Mesh * MayaViewer::setupInputMesh(MeshHeader &mHead)
{
	size_t inSize = 0;
	size_t vertSize = 0;

	int * arr = new int[mHead.indexCount];

	consumer.recv((char*)arr, inSize);

	float * verts = new float[mHead.nrOfVerts * 8];
	consumer.recv((char*)verts, vertSize);

	vertexRef[mHead.meshName] = verts;

	return createImportMesh(verts, arr, mHead.nrOfVerts, mHead.indexCount);
}

MeshHeader MayaViewer::readHeader()
{
	MeshHeader mHead;
	size_t hSize = sizeof(MeshHeader);

	consumer.recv((char*)&mHead, hSize);

	return mHead;
}
MasterHeader prevHead;
size_t prevOffset;


char prevName[42] = {'\0'};
void MayaViewer::msgDirector()
{
	MasterHeader head;
	size_t Mlen;

	while (consumer.nextSize() != 0)
	{
		consumer.recv((char*)&head, Mlen);
		prevOffset = Mlen;

		//Apply Renaming changes
		if (head.numRenamed != 0)
		{
			for (int i = 0; i < head.numRenamed; i++)
			{
				size_t nLen;

				int nameLen = -1;

				consumer.recv((char*)&nameLen, nLen);
				char*name = new char[nameLen];
				consumer.recv(name, nLen);

				std::string oldName(name,nameLen);
				delete[] name;

				consumer.recv((char*)&nameLen, nLen);
				name = new char[nameLen];
				consumer.recv(name, nLen);

				std::string newName(name, nameLen);
				delete[] name;

				if (_scene->findNode(oldName.c_str()))
				{
					Node * namedNode = _scene->findNode(oldName.c_str());
					namedNode->setId(newName.c_str());

					//If it's a mesh we must rearrange the materials too.
					if (individualMatMap.count(oldName) != 0)
					{
						Material * tmp = individualMatMap[oldName];
						individualMatMap.erase(oldName);
						individualMatMap[newName] = tmp;

						Material * tmp2 = individualColMatMap[oldName];
						individualColMatMap.erase(oldName);
						individualColMatMap[newName] = tmp2;
					}
				}

			}
		}

		//Add new Cameras
		if (head.camCount > 0)
		{
			size_t nLen;

			for (int i = 0; i < head.camCount; i++)
			{
				char name[42] = {};
				float camAttr[6];
				consumer.recv(name, nLen);
				consumer.recv((char*)camAttr, nLen);

				//Does the Cam already exist?
				if (_scene->findNode((char*)name))
				{
					Camera* cam = _scene->findNode((char*)name)->getCamera();
					//Orthographic or perspective?
					if (cam->getCameraType() == 2)
					{
						cam->setZoomX(camAttr[1]);
						cam->setZoomY(camAttr[2]);
						cam->setAspectRatio(camAttr[3]);
						cam->setNearPlane(camAttr[4]);
						cam->setFarPlane(camAttr[5]);
					}
					else
					{
						cam->setFieldOfView(camAttr[1]);
						cam->setAspectRatio(camAttr[3]);
						cam->setNearPlane(camAttr[4]);
						cam->setFarPlane(camAttr[5]);
					}
				}
				else
				{
					//Orthographic or perspective?
					if (camAttr[0] == 1)
					{
						Camera* cam = Camera::createOrthographic(camAttr[1], camAttr[2], camAttr[3], camAttr[4], camAttr[5]);
						Node* cameraNode = _scene->addNode(name);
						cameraNode->setCamera(cam);
						SAFE_RELEASE(cam);
					}
					else
					{
						Camera* cam = Camera::createPerspective(camAttr[1], camAttr[3], camAttr[4], camAttr[5]);
						Node* cameraNode = _scene->addNode(name);
						cameraNode->setCamera(cam);
						SAFE_RELEASE(cam);
						//Set transform too?
					}
				}
			}
		}

		//Switch between existing cameras.
		if (head.camSwitched)
		{
			char name[42] = {};
			consumer.recv(name, Mlen);
			if (_scene->findNode(name))
			{
				_scene->setActiveCamera(_scene->findNode(name)->getCamera());
			}
			else
			{
				std::cout << name << " CAMERA COULD NOT BE SWITCHED TO AS IT WASN'T FOUND";
			}
		}

		//Delete objects. (Should probably be last check
		if (head.removedCount > 0)
		{
			size_t nLen;

			for (int i = 0; i < head.removedCount; i++)
			{
				char name[42] = {};

				consumer.recv(name, nLen);
				//Without this if the program could crash for attempting to find non-existent nodes.
				if (_scene->findNode(name))
				{
					_scene->removeNode(_scene->findNode(name));
				}
				else
				{
					std::cout << name << " COULD NOT BE DELETED AS IT WASN'T FOUND";
				}
			}
		}

		//Add new materials
		if (head.matCount > 0)
		{
			for (int i = 0; i < head.matCount; i++)
			{
				MatHeader mHead;
				size_t matSize = sizeof(MatHeader);
				consumer.recv((char*)&mHead, matSize);

				Material *mat;

				//std::string mName = mHead.matName;
				//std::string texName = mHead.textureName;

				//Get the materialName
				char* nMatGet = new char[mHead.lenMatName];
				consumer.recv(nMatGet, matSize);
				std::string matName(nMatGet, mHead.lenMatName);

				delete[] nMatGet;

				if (mHead.isTextured == true)
				{
					//Allocate memory for the name and then read it from memory.
					char* nTexGet = new char[mHead.lenTextureName];
					consumer.recv(nTexGet, matSize);

					//Constructing the name into a string for easier handling.
					std::string texName(nTexGet, mHead.lenTextureName);

					delete[] nTexGet;

					mat = gameplay::Material::create("res/shaders/textured.vert", "res/shaders/textured.frag", "POINT_LIGHT_COUNT 1");
					Texture::Sampler* sampler;
					sampler = mat->getParameter("u_diffuseTexture")->setValue(texName.c_str(), true);
					sampler->setFilterMode(Texture::LINEAR_MIPMAP_LINEAR, Texture::LINEAR);
				}
				else
				{
					mat = gameplay::Material::create("res/shaders/colored.vert", "res/shaders/colored.frag", "POINT_LIGHT_COUNT 1");
					float rgb[3];
					consumer.recv((char*)rgb, matSize);
					mat->getParameter("u_diffuseColor")->setValue(Vector4(rgb[0], rgb[1], rgb[2], 0.0f));
					colMatMap[matName][0] = rgb[0];
					colMatMap[matName][1] = rgb[1];
					colMatMap[matName][2] = rgb[2];
				}

				mat->setParameterAutoBinding("u_worldViewProjectionMatrix", "WORLD_VIEW_PROJECTION_MATRIX");
				mat->setParameterAutoBinding("u_inverseTransposeWorldViewMatrix", "INVERSE_TRANSPOSE_WORLD_VIEW_MATRIX");
				mat->getParameter("u_ambientColor")->setValue(Vector3(0.1f, 0.1f, 0.2f));
				mat->getParameter("u_pointLightColor[0]")->setValue(_scene->findNode("light")->getLight()->getColor());
				mat->getParameter("u_pointLightPosition[0]")->bindValue(_scene->findNode("light"), &Node::getTranslationWorld);
				mat->getParameter("u_pointLightRangeInverse[0]")->bindValue(_scene->findNode("light")->getLight(), &Light::getRangeInverse);

				mat->getStateBlock()->setCullFace(true);
				mat->getStateBlock()->setDepthTest(true);
				mat->getStateBlock()->setDepthWrite(true);

				materialMap[matName] = mat;
			}
			//std::cout << mName << std::endl;
		}

		//Add Meshes
		if (head.meshCount > 0)
		{
			delete[] inMeshArr;
			inMeshArr = new MeshHeader[head.meshCount];

			for (int i = 0; i < head.meshCount; i++)
			{
				inMeshArr[i] = readHeader();
				if (_scene->findNode(inMeshArr[i].meshName))
				{
					_scene->removeNode(_scene->findNode(inMeshArr[i].meshName));/*_scene->findNode(inMeshArr[i].meshName)*/
				}
				Model *mesh = Model::create(setupInputMesh(inMeshArr[i]));

				//**************************************Adding two Seperate Materials to every single mesh***************************************
				//Must be done since Gameplay3D appears to lack the notion of Global materials. Which is...Frustrating as a TA.
				Material * mat;
				Texture::Sampler* sampler;
				char nodeName[42] = {};
				//Move into seperate material function later.
				mat = gameplay::Material::create("res/shaders/textured.vert", "res/shaders/textured.frag", "POINT_LIGHT_COUNT 1");
				mat->setParameterAutoBinding("u_worldViewProjectionMatrix", "WORLD_VIEW_PROJECTION_MATRIX");
				mat->setParameterAutoBinding("u_inverseTransposeWorldViewMatrix", "INVERSE_TRANSPOSE_WORLD_VIEW_MATRIX");
				mat->getParameter("u_ambientColor")->setValue(Vector3(0.1f, 0.1f, 0.2f));
				mat->getParameter("u_pointLightColor[0]")->setValue(_scene->findNode("light")->getLight()->getColor());
				mat->getParameter("u_pointLightPosition[0]")->bindValue(_scene->findNode("light"), &Node::getTranslationWorld);
				mat->getParameter("u_pointLightRangeInverse[0]")->bindValue(_scene->findNode("light")->getLight(), &Light::getRangeInverse);
				sampler = mat->getParameter("u_diffuseTexture")->setValue("res/png/crate.png", true);
				sampler->setFilterMode(Texture::LINEAR_MIPMAP_LINEAR, Texture::LINEAR);
				mat->getStateBlock()->setCullFace(true);
				mat->getStateBlock()->setDepthTest(true);
				mat->getStateBlock()->setDepthWrite(true);

				//Material map to keep track of all textured materials.
				individualMatMap[(std::string)inMeshArr[i].meshName] = mat;

				Material* mat2;
				mat2 = gameplay::Material::create("res/shaders/colored.vert", "res/shaders/colored.frag", "POINT_LIGHT_COUNT 1");
				mat2->getParameter("u_diffuseColor")->setValue(Vector4(1.0f, 1.0f, 1.0f, 0.0f));
				mat2->setParameterAutoBinding("u_worldViewProjectionMatrix", "WORLD_VIEW_PROJECTION_MATRIX");
				mat2->setParameterAutoBinding("u_inverseTransposeWorldViewMatrix", "INVERSE_TRANSPOSE_WORLD_VIEW_MATRIX");
				mat2->getParameter("u_ambientColor")->setValue(Vector3(0.1f, 0.1f, 0.2f));
				mat2->getParameter("u_pointLightColor[0]")->setValue(_scene->findNode("light")->getLight()->getColor());
				mat2->getParameter("u_pointLightPosition[0]")->bindValue(_scene->findNode("light"), &Node::getTranslationWorld);
				mat2->getParameter("u_pointLightRangeInverse[0]")->bindValue(_scene->findNode("light")->getLight(), &Light::getRangeInverse);

				mat2->getStateBlock()->setCullFace(true);
				mat2->getStateBlock()->setDepthTest(true);
				mat2->getStateBlock()->setDepthWrite(true);

				//Material map to keep track of all untextured materials
				individualColMatMap[(std::string)inMeshArr[i].meshName] = mat2;

				//**************************************Adding two Seperate Materials to every single mesh***************************************

				/*mesh->setMaterial(materialMap["lambert2"]);*/
				mesh->setMaterial(mat);
				sprintf(nodeName, inMeshArr[i].meshName, i);
				Node* node = _scene->addNode(nodeName);
				node->setDrawable(mesh);
				SAFE_RELEASE(mesh);
			}
		}

		//Handle all material switching
		if (head.matSwitchedCount > 0)
		{
			for (int i = 0; i < head.matSwitchedCount; i++)
			{
				MatSwitchedHeader mSHead;
				size_t temp;
				consumer.recv((char*)&mSHead, temp);

				char* tMeshName = new char[mSHead.lenMeshName];
				char* tMatName = new char[mSHead.lenMatName];

				consumer.recv(tMeshName, temp);
				consumer.recv(tMatName, temp);

				std::string meshName(tMeshName, mSHead.lenMeshName);
				std::string matName(tMatName, mSHead.lenMatName);

				delete[] tMeshName;
				delete[] tMatName;

				if (_scene->findNode(meshName.c_str()))
				{
					Model* mesh = dynamic_cast<Model*>(_scene->findNode(meshName.c_str())->getDrawable());

					if (materialMap[matName.c_str()]->getParameter("u_diffuseTexture")->getSampler() != nullptr)
					{
						//The new material has a texture
						const char* path = materialMap[matName.c_str()]->getParameter("u_diffuseTexture")->getSampler()->getTexture()->getPath();

						if (mesh->getMaterial()->getParameter("u_diffuseTexture")->getSampler() != nullptr)
						{
							//Previous mat had a texture
							mesh->getMaterial()->getParameter("u_diffuseTexture")->setValue(path, true);
						}
						else
						{
							//Previous mat was color
							individualMatMap[meshName]->getParameter("u_diffuseTexture")->setValue(path, true);
							mesh->setMaterial(individualMatMap[meshName]);
						}
					}
					else
					{
						//The new material is a color.
						if (mesh->getMaterial()->getParameter("u_diffuseTexture")->getSampler())
						{
							//Previous mat had a texture
							individualColMatMap[meshName]->getParameter("u_diffuseColor")->setValue(Vector4(colMatMap[matName][0], colMatMap[matName][1], colMatMap[matName][2], 0.0f));
							mesh->setMaterial(individualColMatMap[meshName]);
						}
						else
						{
							//Previous Mat was a color
							mesh->getMaterial()->getParameter("u_diffuseColor")->setValue(Vector4(colMatMap[matName][0], colMatMap[matName][1], colMatMap[matName][2], 0.0f));
						}
					}
					_scene->findNode(meshName.c_str())->setDrawable(mesh);

					/*SAFE_RELEASE(mesh);*/
				}
			}
		}

		//Handle all transform changes except individual verts.
		if (head.transformCount > 0)
		{
			size_t nameLength;
			double transform[10];
			size_t tLen = sizeof(transform);

			for (int i = 0; i < head.transformCount; i++)
			{
				char name[42] = "0";
				bool work = consumer.recv(name, nameLength);
				for(int i = 0; i < 42; i++)
				{
					prevName[i] = name[i];
				}
				if (prevName[0] == '0')
				{
					std::cout << "No name found";
				}
				consumer.recv((char*)transform, tLen);
				applyTransformation(name, transform);
			}
		}

		//Handles all individual vertex changes (Including extrudes after inital vertex addition)
		if (head.numMeshChanged > 0)
		{
			for (int i = 0; i < head.numMeshChanged; i++)
			{
				int meshNameLen = 0;
				size_t size = 0;
				consumer.recv((char*)&meshNameLen, size);
				char *meshName = new char[meshNameLen+1];
				for (int j = 0; j < meshNameLen + 1; j++)
				{
					meshName[j] = '\0';
				}
				consumer.recv(meshName, size);

				Model* mesh = dynamic_cast<Model*>(_scene->findNode(meshName)->getDrawable());

				/*mesh->getMesh()->getVertexBuffer()*/
				gameplay::VertexBufferHandle a = mesh->getMesh()->getVertexBuffer();

				int nrOfVerts = 0;
				consumer.recv((char*)&nrOfVerts, size);

				/*float* verts = vertexRef[meshName];*/

				for (int j = 0; j < nrOfVerts; j++)
				{
					int vertID = -1;
					consumer.recv((char*)&vertID, size);

					//Sometimes not all vertex data is changed. 
					//Only Pos for example
					int nrFloats = -1;
					consumer.recv((char*)&nrFloats, size);

					float* vertData = new float[nrFloats];
					consumer.recv((char*)vertData, size);

					//Each vertex has 8 values.
					int vertStep = vertID * 8;

					for (int k = 0; k < nrFloats; k++)
					{
						vertexRef[meshName][vertStep + k] = vertData[k];
					}

					delete[] vertData;
				}
				mesh->getMesh()->setVertexData(vertexRef[meshName], 0, mesh->getMesh()->getVertexCount());
			}
		}

		//Handle Zooming.
		if (head.zoomCount > 0)
		{
			int len;
			size_t zLen;
			consumer.recv((char*)&len, zLen);
			char* tmpName = new char[len];
			consumer.recv(tmpName, zLen);
			float zoom[2];
			consumer.recv((char*)zoom, zLen);

			std::string camName(tmpName,len);

			if (_scene->findNode(camName.c_str()))
			{
				Camera* cam = _scene->findNode(camName.c_str())->getCamera();
				cam->setZoomX(zoom[0]);
				cam->setZoomY(zoom[1]);
			}
			delete[] tmpName;
		}

		prevHead = head;
	}
}

void MayaViewer::applyTransformation(const char * name, double * transform)
{
	Node* node = _scene->findNode(name);

	if (node)
	{
		const Vector3 translation((float)transform[0], (float)transform[1], (float)transform[2]);
		const Vector3 scale((float)transform[3], (float)transform[4], (float)transform[5]);
		const Quaternion rotation((float)transform[6], (float)transform[7], (float)transform[8], (float)transform[9]);

		node->setTranslation(translation);
		node->setScale(scale);
		node->setRotation(rotation);
	}
	
}

Material * MayaViewer::createMaterial()
{
	return nullptr;
}
