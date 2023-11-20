#pragma once

#include "chugl_pch.h"
#include "SceneGraphNode.h"

class SceneGraphObject : public SceneGraphNode
{
public:
	SceneGraphObject(
		const glm::vec3& position = glm::vec3(0.0f), 
		const glm::quat& rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), 
		const glm::vec3& scale = glm::vec3(1.0f)
	) :
		m_Position(position),
		m_Rotation(rotation),
		m_Scale(scale),
		m_Parent(nullptr)
	{
	}

	virtual ~SceneGraphObject() {
		// if we're in the destructor of an audio thread object, that means the sgo is *already* disconnected from the scene
		// so only disconnect if we're on the render-thread copy
		if (!IsAudioThreadObject()) Disconnect();  // disconnect handles refcounting
	}

	// clone
	virtual SceneGraphNode* Clone() override {
		SceneGraphObject* sgo = new SceneGraphObject();
		sgo->SetID(this->GetID());
		return sgo;
	}

	
	// Useful Constants (move to separate constants file?)
	static const glm::vec3 UP, FORWARD, RIGHT, LEFT, DOWN, BACKWARD;
	static const glm::quat IDENTITY_QUAT;


	// Transform methods ======================================
	inline glm::vec3 GetPosition() const { return m_Position; }
	inline glm::quat GetRotation() const { return m_Rotation; }
	inline glm::vec3 GetEulerRotationRadians() const { return glm::eulerAngles(m_Rotation); }  // returns in radians
	const glm::vec3& GetScale() const { return m_Scale; }
	inline void SetPosition(const glm::vec3& position) { m_Position = position; }
	inline void SetPosition(float x, float y, float z) { m_Position = glm::vec3(x, y, z); }
	inline void SetRotation(const glm::quat& rotation) { m_Rotation = rotation; }
	inline void SetRotation(const glm::vec3& eulers)   { m_Rotation = glm::quat(eulers); }
	inline void SetRotation(float x, float y, float z)   { m_Rotation = glm::quat(glm::vec3(x, y, z)); }
	// inline void SetRotation(const glm::vec3& rotation) { m_Rotation = rotation; }  // euler --> quat
	inline void SetScale(const glm::vec3& scale) { m_Scale = scale; }

	// rotation helpers (shamelessly copied from THREEjs Object3D api)
	void RotateOnLocalAxis(const glm::vec3& axis, float deg);  // rotates angle deg on normalized axis in model space
	void RotateOnWorldAxis(const glm::vec3& axis, float deg);  // rotates angle deg on normalized axis in world space
	void RotateX(float deg); // rotates deg around x axis in local space
	void RotateY(float deg); // rotates deg around y axis in local space
	void RotateZ(float deg); // rotates deg around z axis in local space
	void LookAt(const glm::vec3& pos);

	

	glm::mat4 GetWorldMatrix();
	glm::quat GetWorldRotation();
	glm::vec3 GetWorldPosition() const;
	glm::vec3 GetWorldScale();
	glm::vec3 SetWorldPosition(const glm::vec3& pos);
	glm::vec3 SetWorldScale(const glm::vec3& scale);


	glm::vec3 GetRight(); 
	glm::vec3 GetForward();
	glm::vec3 GetUp();  // cross forward and right
	glm::mat4 GetModelMatrix();
	glm::mat4 GetInvModelMatrix();
	glm::mat4 GetNormalMatrix();  // returns transpose(inverse(model matrix)). used for updating normals under non-uniform scaling
	inline void Translate(const glm::vec3& t) { m_Position += t; }
	// inline void Rotate(const glm::quat& q) { m_Rotation = glm::normalize(q * m_Rotation); };
	inline void Rotate(const glm::quat& q) { m_Rotation = q * m_Rotation; };
	void Rotate(const glm::vec3& eulers) { m_Rotation = glm::quat(eulers) * m_Rotation; };
	inline void Scale(const glm::vec3& s) { m_Scale *= s; };
	void ApplyTransform(const glm::mat4& mat);  // updates model matrix and decomposes into pos, rot, scale
	void AssignTransform(const glm::mat4& mat);  // replaces existing transform


	// SceneGraph relationships ========================================
	void AddChild(SceneGraphObject* child);
	void RemoveChild(SceneGraphObject* child);
	bool HasChild(SceneGraphObject* child);
	inline SceneGraphObject* GetParent() { return m_Parent;  }
	const std::vector<SceneGraphObject*>& GetChildren() { return m_Children; }
	bool BelongsToSceneObject(SceneGraphObject* sgo);

    // disconnect from both parent and children
    void Disconnect( bool sendChildrenToGrandparent = false );

	// transform  (making public for now for easier debug)
	glm::vec3 m_Position;
	glm::quat m_Rotation;
	glm::vec3 m_Scale;

private:
	// relationships
	std::string m_Name;
	SceneGraphObject* m_Parent;
	std::vector<SceneGraphObject*> m_Children;
};
