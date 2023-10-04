#pragma once
#include "SceneGraphNode.h"
#include <vector>
#include <string>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"


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
	virtual ~SceneGraphObject() {}

	
	// Useful Constants (move to separate constants file?)
	static const glm::vec3 UP, FORWARD, RIGHT, LEFT, DOWN, BACKWARD;
	static const glm::quat IDENTITY_QUAT;


	// Transform methods ======================================
	inline glm::vec3 GetPosition() const { return m_Position; }
	inline glm::quat GetRotation() const { return m_Rotation; }
	inline glm::vec3 GetEulerRotationRadians() const { return glm::eulerAngles(m_Rotation); }  // returns in radians
	inline glm::vec3 GetScale() const { return m_Scale; }
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
	glm::vec3 GetWorldPosition();
	glm::vec3 GetRight(); 
	glm::vec3 GetForward();
	glm::vec3 GetUp();  // cross forward and right
	glm::mat4 GetModelMatrix();
	glm::mat4 GetInvModelMatrix();
	glm::mat4 GetNormalMatrix();  // returns transpose(inverse(model matrix)). used for updating normals under non-uniform scaling
	inline void Translate(const glm::vec3& t) { m_Position += t; }
	// inline void Rotate(const glm::quat& q) { m_Rotation = glm::normalize(q * m_Rotation); };
	inline void Rotate(const glm::quat& q) { m_Rotation = q * m_Rotation; };
	inline void Scale(const glm::vec3& s) { m_Scale *= s; };
	void ApplyTransform(const glm::mat4& mat);  // updates model matrix and decomposes into pos, rot, scale
	void AssignTransform(const glm::mat4& mat);  // replaces existing transform


	// SceneGraph relationships ========================================
	void AddChild(SceneGraphObject* child);
	void RemoveChild(SceneGraphObject* child);
	bool HasChild(SceneGraphObject* child);
	inline SceneGraphObject* GetParent() { return m_Parent;  }
	const std::string& GetName() const { return m_Name; }
	void SetName(const std::string& name) { m_Name = name; }
	const std::vector<SceneGraphObject*>& GetChildren() { return m_Children; }
	bool BelongsToSceneObject(SceneGraphObject* sgo);


	// type methods ========================================
	virtual bool IsLight() { return false; }
	virtual bool IsCamera() { return false; }
	virtual bool IsMesh() { return false; }
	virtual bool IsScene() { return false; }

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
