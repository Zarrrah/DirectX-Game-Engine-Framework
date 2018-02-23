#pragma once
#include "SceneNode.h"
#include <list>

typedef shared_ptr<SceneNode> SceneNodePointer;

class SceneGraph : public SceneNode
{
public:
	SceneGraph() : SceneNode(L"Root") {};
	SceneGraph(wstring name) : SceneNode(name) {};
	~SceneGraph(void) {};

	inline virtual bool Initialise(void) { return true; };
	inline virtual void Render(void) { for (SceneNodePointer child : _children) child->Render(); };
	inline virtual void Shutdown(void) { for (SceneNodePointer child : _children) child->Shutdown(); };
private:
	list<SceneNodePointer> _children = list<SceneNodePointer>();
};
typedef shared_ptr<SceneGraph> SceneGraphPointer;