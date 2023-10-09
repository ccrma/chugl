#pragma once
#include <iostream>
#include <string>

struct Chuck_Object;
struct Chuck_DL_Api;

/*
* Base class for all possible scenegraph entities -- objects, meshes, geometries, and materials (eventually add textures?)
* Calling "Node" instead of "entity" in case we ever switch to ECS system, where Entity means something different
*/


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

    // reference to chuck object container
    Chuck_Object * m_ChuckObject;

    // access the chugin runtime API
    static void SetCKAPI( const Chuck_DL_Api* api ) { s_CKAPI = api; }
    // access the chugin runtime API
    static const Chuck_DL_Api* CKAPI() { return s_CKAPI; }

protected:
    // reference to chugins runtime API
    static const Chuck_DL_Api* s_CKAPI;
};
