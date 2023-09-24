#pragma once
#include <iostream>
#include <string>
/*
* Base class for all possible scenegraph entities -- objects, meshes, geometries, and materials (eventually add textures?)
* Calling "Node" instead of "entity" in case we ever switch to ECS system, where Entity means something different
*/

class SceneGraphNode
{
public:

// data
	size_t m_ID;

// methods
	SceneGraphNode() : m_ID(SceneGraphNode::idCounter++) {
		// std::cout << "created node with id: " + std::to_string(m_ID) << std::endl;
	}
	virtual ~SceneGraphNode() {}  // virtual destructor to enable deleting polymorphically

	inline size_t GetID() { return m_ID; }
	inline void SetID(size_t id) { m_ID = id; }

// static
	static size_t idCounter;
};
