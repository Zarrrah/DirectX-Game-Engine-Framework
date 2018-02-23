#pragma once
#include "SceneNode.h"
#include "Mesh.h"
class Cube : public SceneNode, public Mesh
{
public:
	Cube();
	~Cube() {};

	virtual bool Initialise();
	virtual void Render();
	inline virtual void Shutdown() {};

	void BuildGeometryBuffers();
	void BuildShaders();
	void BuildVertexLayout();
	void BuildConstantBuffer();
	void BuildTexture();
};

