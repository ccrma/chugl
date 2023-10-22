#pragma once
#include <iostream>
#include <string>


// refcount macros ======================================
// release the ckobj bound to chugl object `x`
#define CHUGL_RELEASE(x)          do { if(IsAudioThreadObject() && x){ CKAPI()->object->release(x->m_ChuckObject); } } while(0)
#define CHUGL_ADD_REF(x)          do { if(IsAudioThreadObject() && x){ CKAPI()->object->add_ref(x->m_ChuckObject); } } while(0)

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

// TODO create NodeType enum

class SceneGraphNode
{
public:
    // data
	size_t m_ID;

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
