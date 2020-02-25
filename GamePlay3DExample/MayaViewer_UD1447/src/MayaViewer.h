#ifndef MayaViewer_H_
#define MayaViewer_H_

#include "gameplay.h"
#include "../../Structs.h"
#include <unordered_map>

using namespace gameplay;

/**
 * Main game class.
 */
class MayaViewer: public Game
{
public:

    /**
     * Constructor.
     */
    MayaViewer();

    /**
     * @see Game::keyEvent
     */
	void keyEvent(Keyboard::KeyEvent evt, int key);
	
    /**
     * @see Game::touchEvent
     */
    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

	// mouse events
	bool mouseEvent(Mouse::MouseEvent evt, int x, int y, int wheelDelta);


protected:

    /**
     * @see Game::initialize
     */
    void initialize();

    /**
     * @see Game::finalize
     */
    void finalize();

    /**
     * @see Game::update
     */
    void update(float elapsedTime);

    /**
     * @see Game::render
     */
    void render(float elapsedTime);

private:

    /**
     * Draws the scene each frame.
     */
    bool drawScene(Node* node);

	Mesh* createCubeMesh(float size = 1.0f);
	Mesh * createImportMesh(float * verts, int* indicies, int vtxNr, int indexNr);
	Mesh * setupInputMesh(MeshHeader &mHead);
	MeshHeader readHeader();
	void msgDirector();
	void applyTransformation(const char * name, double * transform);
	Material* createMaterial();


    Scene* _scene;
    bool _wireframe;
	MeshHeader *inMeshArr;
	std::unordered_map<std::string, Material*> materialMap;
	std::unordered_map<std::string, float[3]> colMatMap;

	//Since gameplay3D refuses to let me apply the same material to multiple meshes
	//I'll have to create one color and one texture material for each individual mesh.
	//And then apply the values of the above maps that stores actually relevant information.
	//This is terribly inneficent and I would have prepared to simply re-use the same material*
	//The strings are the relevant node name.
	std::unordered_map<std::string, Material*> individualMatMap;
	std::unordered_map<std::string, Material*> individualColMatMap;

	// As of now I am unable to retrieve the verticies through gameplay3D's interface.
	// As such I will store the array of float values for my verts seperately when I input them for now.
	std::unordered_map<std::string, float*> vertexRef;
	//unordered_map MaterialMap;
};

#endif
