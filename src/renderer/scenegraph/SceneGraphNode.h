#pragma once
#include <iostream>
#include <string>
/*
* Base class for all possible scenegraph entities -- objects, meshes, geometries, and materials (eventually add textures?)
* Calling "Node" instead of "entity" in case we ever switch to ECS system, where Entity means something different
*/

struct Chuck_Object;


struct EnumClassHash
{
	template <typename T>
	std::size_t operator()(T t) const
	{
		return static_cast<std::size_t>(t);
	}
};

class SceneGraphNode
{
public:

// data
	size_t m_ID;

// methods
	SceneGraphNode() : m_ID(SceneGraphNode::idCounter++), m_ChuckObject(nullptr) {
		// std::cout << "created node with id: " + std::to_string(m_ID) << std::endl;
	}
	virtual ~SceneGraphNode() {}  // virtual destructor to enable deleting polymorphically

	inline size_t GetID() { return m_ID; }
	inline void SetID(size_t id) { m_ID = id; }
	void NewID() { m_ID = SceneGraphNode::idCounter++; }

// static
	static size_t idCounter;

	Chuck_Object* m_ChuckObject;
};
