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
	//unordered_map MaterialMap;
};

#endif
