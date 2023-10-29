#include "SceneGraphObject.h"

/*==========================Static constants=================================*/
const glm::vec3 SceneGraphObject::UP	   = glm::vec3(0.0f, 1.0f, 0.0f);
const glm::vec3 SceneGraphObject::DOWN	   = glm::vec3(0.0f, -1.0f, 0.0f);
const glm::vec3 SceneGraphObject::LEFT	   = glm::vec3(-1.0f, 0.0f, 0.0f);
const glm::vec3 SceneGraphObject::RIGHT	   = glm::vec3(1.0f, 0.0f, 0.0f);
const glm::vec3 SceneGraphObject::FORWARD  = glm::vec3(0.0f, 0.0f, -1.0f);
const glm::vec3 SceneGraphObject::BACKWARD = glm::vec3(0.0f, 0.0f, 1.0f);

const glm::quat SceneGraphObject::IDENTITY_QUAT = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

/*==========================Transform Methods=================================*/
glm::mat4 SceneGraphObject::GetModelMatrix()
{
	return glm::translate(glm::mat4(1.0f), m_Position) *
		glm::toMat4(m_Rotation) *
		glm::scale(glm::mat4(1.0f), m_Scale);
}

glm::mat4 SceneGraphObject::GetInvModelMatrix()
{
	return glm::inverse(GetModelMatrix());
}

glm::mat4 SceneGraphObject::GetNormalMatrix()
{
	return glm::transpose(GetInvModelMatrix());
}

// computes world matrix by walking down scene graph to current object
// TODO: can optimize by caching Model and World matrices
glm::mat4 SceneGraphObject::GetWorldMatrix()
{
	if (m_Parent == nullptr) {
		return GetModelMatrix();
	}

	// premature optimization: do it in place instead of recursing
	// just in case someone has a SceneGraph deep enough to blow the stack...
	auto* parentPtr = m_Parent;
	glm::mat4 worldMat = GetModelMatrix();
	while (parentPtr != nullptr) {
		worldMat = parentPtr->GetModelMatrix() * worldMat;
		parentPtr = parentPtr->GetParent();  // walk up the graph
	}
	return worldMat;
}

// walks scene graph, gets world quaternion rotation
glm::quat SceneGraphObject::GetWorldRotation()
{
	// TODO: is this bugged? does scale affect rotation??
	if (m_Parent == nullptr)
		return m_Rotation;

	auto* parentPtr = m_Parent;
	glm::quat worldRot = m_Rotation;
	while (parentPtr != nullptr) {
		worldRot = parentPtr->GetRotation() * worldRot;
		parentPtr = parentPtr->GetParent();  // walk up the graph
	}
	return worldRot;
}

glm::vec3 SceneGraphObject::GetWorldPosition() const
{
	if (!m_Parent) return m_Position;
	// multiply by parent's world matrix to get world position
	// DON'T multiply by this world matrix, because it double counts our own transform
	return m_Parent->GetWorldMatrix() * glm::vec4(m_Position, 1.0);
}

glm::vec3 SceneGraphObject::GetWorldScale()
{
	auto scale = m_Scale;
    auto* obj = m_Parent;
	while( obj )
	{
		scale *= obj->m_Scale;
		obj = obj->m_Parent;
	}
	return scale;
}

glm::vec3 SceneGraphObject::SetWorldPosition(const glm::vec3 &pos)
{
	if (!m_Parent) {
		m_Position = pos;
		return m_Position;
	}
	// inverse matrix maps from world space --> local space
	m_Position = glm::inverse(m_Parent->GetWorldMatrix()) * glm::vec4(pos, 1.0);
	return m_Position;
}

glm::vec3 SceneGraphObject::SetWorldScale(const glm::vec3 &scale)
{
	if (!m_Parent) {
		m_Scale = scale;
		return m_Scale;
	}
	m_Scale = scale / m_Parent->GetWorldScale();
    return m_Scale;
}

// get the forward direction in world space
glm::vec3 SceneGraphObject::GetForward()
{
	return glm::normalize(glm::rotate(GetWorldRotation(), FORWARD));
}


// get the right direction in world space
// again, should probably be caching here...
glm::vec3 SceneGraphObject::GetRight()
{
	return glm::normalize(glm::rotate(GetWorldRotation(), RIGHT));
}

// get the up direction in world space
glm::vec3 SceneGraphObject::GetUp()
{
	return glm::normalize(glm::rotate(GetWorldRotation(), UP));
}

void SceneGraphObject::RotateOnLocalAxis(const glm::vec3& axis, float deg)
{
	// just flip the order of multiplication to go from local <--> world. so elegant...
	m_Rotation = m_Rotation * glm::angleAxis(deg, glm::normalize(axis));
}

void SceneGraphObject::RotateOnWorldAxis(const glm::vec3& axis, float deg)
{
	m_Rotation = glm::angleAxis(deg, glm::normalize(axis)) * m_Rotation;
} // set back to false

void SceneGraphObject::RotateX(float deg)
{
	RotateOnLocalAxis(RIGHT, deg);
}

void SceneGraphObject::RotateY(float deg)
{
	RotateOnLocalAxis(UP, deg);
}

void SceneGraphObject::RotateZ(float deg)
{
	RotateOnLocalAxis(FORWARD, deg);
}

// rotates object to point towards position, updates 
void SceneGraphObject::LookAt(const glm::vec3& pos)
{
	auto abs_rotation = glm::conjugate(glm::toQuat(glm::lookAt(GetWorldPosition(), pos, UP)));
	auto local_rotation = abs_rotation;

	// convert into relative local rotation based on parent transforms
	if (m_Parent != nullptr) {
		local_rotation = glm::inverse(m_Parent->GetWorldRotation()) * abs_rotation;
	}
	m_Rotation = local_rotation;
	return;
}

// applies the input transform to the existing transform
void SceneGraphObject::ApplyTransform(const glm::mat4& mat)
{
	glm::vec3 tra, sca, skew;
	glm::quat rot;
	glm::vec4 persp;
	glm::decompose(mat, sca, rot, tra, skew, persp);
	
	Translate(tra);
	Rotate(rot);
	Scale(sca);
}

// replaces old transform with input transform
void SceneGraphObject::AssignTransform(const glm::mat4& mat)
{
	glm::vec3 tra, sca, skew;
	glm::quat rot;
	glm::vec4 persp;
	glm::decompose(mat, sca, rot, tra, skew, persp);

	m_Position = tra;
	m_Rotation = rot;
	m_Scale = sca;
}

/*==========================Relationship Methods=================================*/

void SceneGraphObject::AddChild(SceneGraphObject* child)
{
	// Object cannot be added as child of itself
	if (child == this) {
		std::cerr << "GGen cannot be added as child of itself" << std::endl;
		return;
	}

	if (child == nullptr) {
		std::cerr << "cannot add nullptr as child of GGen " << std::endl;
		return;
	}

	if (child->IsScene()) {  // necessary to prevent cycles
		std::cerr << "cannot add make GScene a child of another GGen" << std::endl;
		return;
	}

	// we are already the parent, do nothing
	if (child->GetParent() == this) {
		return;
	}

	// remove child from old parent
	if (child->GetParent() != nullptr) {
		child->GetParent()->RemoveChild(child);
	}

	// assign to new parent
	child->m_Parent = this;
    // reference count
	CHUGL_ADD_REF(this);

	// add to list of children
	m_Children.push_back(child);
    // add ref to kid
	CHUGL_ADD_REF(child);
}

void SceneGraphObject::RemoveChild( SceneGraphObject * child )
{
    // look for
    auto it = std::find(m_Children.begin(), m_Children.end(), child);
	if (it != m_Children.end())
    {
        // ensure
        assert( child->m_Parent == this );

        // set parent to null (must do this first to prevent double-free, bc releasing the 
		// child may trigger a disconnect its the destructor)
        child->m_Parent = NULL;

        // remove from children list
        m_Children.erase(it);

        // release ref count on child's chuck object; one less reference to it from us (parent)
		CHUGL_RELEASE(child);

        // release ref count on our (parent's) chuck object; one less reference to it from child
		CHUGL_RELEASE(this);
    }
}

bool SceneGraphObject::HasChild(SceneGraphObject* child)
{
	if (std::find(m_Children.begin(), m_Children.end(), child) != m_Children.end())
		return true;

	return false;
}

// rewturns true if sgo is a parent, grandparent, etc of this object
bool SceneGraphObject::BelongsToSceneObject(SceneGraphObject *sgo)
{
	SceneGraphObject* parent = this;
	while (parent != nullptr) {
		if (parent == sgo) return true;
		parent = parent->GetParent();
	}
    return false;
}

// disconnect from both parent and children
void SceneGraphObject::Disconnect( bool sendChildrenToGrandparent )
{
    std::vector<SceneGraphObject *> toRemove;

    // for each kid
    for( auto * kid : m_Children )
    {
        // re-parent
        if( sendChildrenToGrandparent && m_Parent )
        {
            // thanks
            m_Parent->AddChild( kid );
        }
        else
        {
            // bye
            toRemove.push_back( kid );
        }
    }

    // remove
    for( auto * kid : toRemove ) this->RemoveChild( kid );

    // if we have a parent
    if( m_Parent )
    {
        // remove child from parent
        m_Parent->RemoveChild( this );
    }
}

