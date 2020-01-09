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

ComLib consumer((std::string)"stuff", 100 * 1024 * 1024, ComLib::CONSUMER);

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

	//MasterHeader head1;
	//size_t Mlen;
	//consumer.recv((char*)&head1, Mlen);

	//MeshHeader head = readHeader();
	//Mesh* mesh1 = setupInputMesh(head);

	////std::string name(head.meshName);

	//Model* models[gModelCount];
	//Material* mats[gModelCount];
	//Texture::Sampler* samplers[gModelCount];

	//char nodeName[42] = {};
	//for (int i = 0; i < gModelCount; i++)
	//{
	//	models[i] = Model::create(mesh1);
	//	mats[i] = models[i]->setMaterial( "res/shaders/textured.vert", "res/shaders/textured.frag", "POINT_LIGHT_COUNT 1");
	//	mats[i]->setParameterAutoBinding("u_worldViewProjectionMatrix", "WORLD_VIEW_PROJECTION_MATRIX");
	//	mats[i]->setParameterAutoBinding("u_inverseTransposeWorldViewMatrix", "INVERSE_TRANSPOSE_WORLD_VIEW_MATRIX");
	//	mats[i]->getParameter("u_ambientColor")->setValue(Vector3(0.1f, 0.1f, 0.2f));
	//	mats[i]->getParameter("u_pointLightColor[0]")->setValue(lightNode->getLight()->getColor());
	//	mats[i]->getParameter("u_pointLightPosition[0]")->bindValue(lightNode, &Node::getTranslationWorld);
	//	mats[i]->getParameter("u_pointLightRangeInverse[0]")->bindValue(lightNode->getLight(), &Light::getRangeInverse);
	//	samplers[i] = mats[i]->getParameter("u_diffuseTexture")->setValue("res/png/crate.png", true);
	//	samplers[i]->setFilterMode(Texture::LINEAR_MIPMAP_LINEAR, Texture::LINEAR);
	//	mats[i]->getStateBlock()->setCullFace(true);
	//	mats[i]->getStateBlock()->setDepthTest(true);
	//	mats[i]->getStateBlock()->setDepthWrite(true);
	//	sprintf(nodeName, head.meshName, i);
	//	Node* node = _scene->addNode(nodeName);
	//	node->setDrawable(models[i]);
	//	SAFE_RELEASE(models[i]);
	//}
}

void MayaViewer::finalize()
{
    SAFE_RELEASE(_scene);
}

void MayaViewer::update(float elapsedTime)
{
	static float totalTime = 0;
	totalTime += elapsedTime;
	float step = 360.0 / float(gModelCount);
	char name[10] = {};

	//for (int i = 0; i < gModelCount; i++)
	//{
	//	sprintf(name, "cube%d", i);
	//	Node* node = _scene->findNode(name);
	//	if (node) {
	//		node->setScale(0.3f);
	//		node->setTranslation(
	//			cosf(MATH_DEG_TO_RAD(((int)totalTime / 10) % 360 + i * step))*5.0, 
	//			sinf(MATH_DEG_TO_RAD(((int)totalTime / 10) % 360 + i * step))*5.0,
	//			0.0);
	//	}
	//	if (i%2)
	//		node->rotateX(elapsedTime / 1000.f);
	//}	

	Node* camnode = _scene->getActiveCamera()->getNode();
	//if (gKeys[Keyboard::KEY_W])
	//	camnode->translateForward(0.5);
	//if (gKeys[Keyboard::KEY_S])
	//	camnode->translateForward(-0.5);
	//if (gKeys[Keyboard::KEY_A])
	//	camnode->translateLeft(0.5);
	//if (gKeys[Keyboard::KEY_D])
	//	camnode->translateLeft(-0.5);

	//MY STUFF__________________________________________________________
	//char stuff[1024*3] = { 0 };
	/*consumer.recv(static_cast<char*>(stuff), size);*/
	msgDirector();

	//if (gMousePressed) {
	//	camnode->rotate(camnode->getRightVectorWorld(), MATH_DEG_TO_RAD(gDeltaY / 10.0));
	//	camnode->rotate(camnode->getUpVectorWorld(), MATH_DEG_TO_RAD(gDeltaX / 5.0));
	//}
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
			msgDirector();
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

	size_t a = inSize / sizeof(int);

	//for (int i = 0; i < a; i++)
	//{
	//	int anInt = arr[i];
	//	std::cout << anInt << std::endl;
	//}

	size_t b = vertSize / sizeof(float);

	std::string checker;
	int c = 0;

	for (int i = 0; i < mHead.indexCount; i++)
	{
		checker.append(std::to_string(arr[i]));
		checker.append(" ");
	}

	checker = "";

	for (int i = 0; i < b; i++)
	{
		checker.append(std::to_string(verts[i]));
		checker.append(" ");
		c++;
		if (c == 8)
		{
			checker = "";
			c = 0;
		}
	}

	return createImportMesh(verts, arr, mHead.nrOfVerts, mHead.indexCount);
}

MeshHeader MayaViewer::readHeader()
{
	MeshHeader mHead;
	size_t hSize = sizeof(MeshHeader);

	consumer.recv((char*)&mHead, hSize);

	return mHead;
}

void MayaViewer::msgDirector()
{
	MasterHeader head;
	size_t Mlen;
	consumer.recv((char*)&head, Mlen);

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

	if (head.camCount != 0)
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

	//Should be moved to last in case transform calls make it in.
	if (head.removedCount != 0)
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

	//If unnecesary since mayaRun already checks
	if (head.meshCount != 0)
	{
		delete[] inMeshArr;
		inMeshArr = new MeshHeader[head.meshCount];

		for (int i = 0; i < head.meshCount; i++)
		{
			inMeshArr[i] = readHeader();
			Model *mesh = Model::create(setupInputMesh(inMeshArr[i]));

			Material * mat;
			Texture::Sampler* sampler;
			char nodeName[42] = {};
			//Move into seperate material function later.
			mat = mesh->setMaterial("res/shaders/textured.vert", "res/shaders/textured.frag", "POINT_LIGHT_COUNT 1");
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

			sprintf(nodeName, inMeshArr[i].meshName, i);
			Node* node = _scene->addNode(nodeName);
			node->setDrawable(mesh);
			SAFE_RELEASE(mesh);
		}
	}

	if (head.transformCount != 0)
	{
		size_t nameLength;
		double transform[10];
		size_t tLen = sizeof(transform);

		for (int i = 0; i < head.transformCount; i++)
		{
			char name[42] = "0";
			consumer.recv(name, nameLength);
			consumer.recv((char*)transform, tLen);
			applyTransformation(name, transform);
		}
	}



	//int a=0;
	//size_t hSize = sizeof(int);
	//consumer.recv((char*)&a, hSize);

	//switch (a)
	//{
	//case meshType:
	//	//MeshHeader head = readHeader();
	//	//Mesh* mesh1 = setupInputMesh(head);
	//	break;
	//case transformType:
	//	//Need len ahead of time.
	//	char name[8] = "0";
	//	size_t nameLength;
	//	double transform[10];
	//	size_t tLen = sizeof(transform);
	//	consumer.recv(name, nameLength);
	//	consumer.recv((char*)transform, tLen);
	//	applyTransformation(name, transform);
	//	break;
	//	
	//}


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
