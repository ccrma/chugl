#pragma once

#include "chugl_pch.h"
#include "Locator.h"

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

// TODO create NodeType enum

class SceneGraphNode
{
private:
    // data
	size_t m_ID;
	std::string m_Name;
public:

    // methods
	SceneGraphNode() 
	: m_ID(SceneGraphNode::idCounter++), m_ChuckObject(nullptr), m_IsAudioThreadObject(true)
	{
		// std::cout << "created node with id: " + std::to_string(m_ID) << std::endl;
	}

	virtual ~SceneGraphNode() {
		m_ChuckObject = nullptr;  // always null out chuck object upon destruction
	}

	// ID fns
	inline size_t GetID() { return m_ID; }
	inline void SetID(size_t id) { m_ID = id; }
	void NewID() { m_ID = SceneGraphNode::idCounter++; }
	
	// name
	const std::string& GetName() const { return m_Name; }
	void SetName(const std::string& name) { m_Name = name; }

	// Creation fns
	virtual SceneGraphNode* Clone() = 0;

    // static
	static size_t idCounter;

    // reference to chuck object container
    Chuck_Object * m_ChuckObject;
	// bool isAudioThreadObject; // TODO add this check

    // access the chugin runtime API
    static void SetCKAPI( const Chuck_DL_Api* api ) { s_CKAPI = api; }
    // access the chugin runtime API
    static const Chuck_DL_Api* CKAPI() { return s_CKAPI; }

public:
	// type methods ========================================
	virtual bool IsLight() { return false; }
	virtual bool IsCamera() { return false; }
	virtual bool IsMesh() { return false; }
	virtual bool IsScene() { return false; }
	virtual bool IsMaterial() { return false; }
	virtual bool IsGeometry() { return false; }
	virtual bool IsTexture() { return false; }

protected:
    // reference to chugins runtime API
    static const Chuck_DL_Api* s_CKAPI;


private:  // sets which thread the object is owned by
	// currently used to track which SceneGraphNodes need to do chuck refcounting
	bool m_IsAudioThreadObject;  // if true, owned by chuck audio thread, else owned by main thread
public:
	inline bool IsAudioThreadObject() { return m_IsAudioThreadObject; }
	inline void SetIsAudioThreadObject(bool isAudioThreadObject) { m_IsAudioThreadObject = isAudioThreadObject; }
};
