#include "ulib_ggen.h"
#include "ulib_cgl.h"
#include "scenegraph/Command.h"
#include "renderer/scenegraph/SceneGraphObject.h"

//-----------------------------------------------------------------------------
// GGen API Declarations
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_obj_ctor);
CK_DLL_DTOR(cgl_obj_dtor);

// internal
CK_DLL_MFUN(cgl_obj_get_id);
CK_DLL_MFUN(cgl_obj_update);
CK_DLL_MFUN(cgl_obj_get_name);
CK_DLL_MFUN(cgl_obj_set_name);

// transform API
CK_DLL_MFUN(cgl_obj_get_right);
CK_DLL_MFUN(cgl_obj_get_forward);
CK_DLL_MFUN(cgl_obj_get_up);

	// Position 
	CK_DLL_MFUN(cgl_obj_get_pos_x);
	CK_DLL_MFUN(cgl_obj_set_pos_x);

	CK_DLL_MFUN(cgl_obj_get_pos_y);
	CK_DLL_MFUN(cgl_obj_set_pos_y);

	CK_DLL_MFUN(cgl_obj_get_pos_z);
	CK_DLL_MFUN(cgl_obj_set_pos_z);

	CK_DLL_MFUN(cgl_obj_set_pos);
	CK_DLL_MFUN(cgl_obj_get_pos);

	CK_DLL_MFUN(cgl_obj_get_pos_world);
	CK_DLL_MFUN(cgl_obj_set_pos_world);

	CK_DLL_MFUN(cgl_obj_translate);
	CK_DLL_MFUN(cgl_obj_translate_x);
	CK_DLL_MFUN(cgl_obj_translate_y);
	CK_DLL_MFUN(cgl_obj_translate_z);

	// Rotation
	CK_DLL_MFUN(cgl_obj_get_rot_x);
	CK_DLL_MFUN(cgl_obj_set_rot_x);

	CK_DLL_MFUN(cgl_obj_get_rot_y);
	CK_DLL_MFUN(cgl_obj_set_rot_y);

	CK_DLL_MFUN(cgl_obj_get_rot_z);
	CK_DLL_MFUN(cgl_obj_set_rot_z);

	CK_DLL_MFUN(cgl_obj_set_rot);
	CK_DLL_MFUN(cgl_obj_get_rot);

	CK_DLL_MFUN(cgl_obj_rotate);
	CK_DLL_MFUN(cgl_obj_rotate_x);
	CK_DLL_MFUN(cgl_obj_rotate_y);
	CK_DLL_MFUN(cgl_obj_rotate_z);

	CK_DLL_MFUN(cgl_obj_rot_on_local_axis);
	CK_DLL_MFUN(cgl_obj_rot_on_world_axis);

	CK_DLL_MFUN(cgl_obj_lookat_vec3);
	CK_DLL_MFUN(cgl_obj_lookat_dir);
	// CK_DLL_MFUN(cgl_obj_rotate_by);  // no rotate by design because converting from euler angles to quat is ambiguous

	// Scale
	CK_DLL_MFUN(cgl_obj_get_scale_x);
	CK_DLL_MFUN(cgl_obj_set_scale_x);

	CK_DLL_MFUN(cgl_obj_get_scale_y);
	CK_DLL_MFUN(cgl_obj_set_scale_y);

	CK_DLL_MFUN(cgl_obj_get_scale_z);
	CK_DLL_MFUN(cgl_obj_set_scale_z);

	CK_DLL_MFUN(cgl_obj_set_scale);
	CK_DLL_MFUN(cgl_obj_set_scale_uniform);
	CK_DLL_MFUN(cgl_obj_get_scale);
	
	CK_DLL_MFUN(cgl_obj_get_scale_world);
	CK_DLL_MFUN(cgl_obj_set_scale_world);

// transformation matrix API
CK_DLL_MFUN(cgl_obj_local_pos_to_world_pos);

// parent-child scenegraph API
// CK_DLL_MFUN(cgl_obj_disconnect);
CK_DLL_MFUN(cgl_obj_get_parent);
CK_DLL_MFUN(cgl_obj_get_child_default);
CK_DLL_MFUN(cgl_obj_get_child);
CK_DLL_MFUN(cgl_obj_get_num_children);
CK_DLL_GFUN(ggen_op_gruck);	  // add child
CK_DLL_GFUN(ggen_op_ungruck); // remove child


//-----------------------------------------------------------------------------
// GGen API Definitions 
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_obj(Chuck_DL_Query *QUERY)
{
	QUERY->begin_class(QUERY, "GGen", "Object");
	QUERY->doc_class(QUERY, "Base class for all Gens. Can be extended to create your own, or initialized as an empty group container");
	QUERY->add_ex(QUERY, "basic/orbits.ck");
	QUERY->add_ex(QUERY, "basic/circles.ck");

	QUERY->add_ctor(QUERY, cgl_obj_ctor);
	QUERY->add_dtor(QUERY, cgl_obj_dtor);
	CGL::SetGGenDataOffset(QUERY->add_mvar(QUERY, "int", "@ggen_data", false));

	QUERY->add_mfun(QUERY, cgl_obj_get_id, "int", "id");
	QUERY->doc_func(QUERY, "Internal debug. Get the unique ChuGL ID of this GGen");

	QUERY->add_mfun(QUERY, cgl_obj_get_name, "string", "name");
	QUERY->doc_func(QUERY, "Get the custom name of this GGen");

	QUERY->add_mfun(QUERY, cgl_obj_set_name, "string", "name");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->doc_func(QUERY, "Set the custom name of this GGen");

	QUERY->add_mfun(QUERY, cgl_obj_update, "void", "update");
	QUERY->add_arg(QUERY, "float", "dt");
	QUERY->doc_func(
		QUERY, 
		"This method is automatically invoked once per frame for all GGens connected to the scene graph."
		"Override this method in custom GGen classes to implement your own update logic."
	);

	QUERY->add_mfun(QUERY, cgl_obj_get_right, "vec3", "right");
	QUERY->doc_func(QUERY, "Get the right vector of this GGen in world space");

	QUERY->add_mfun(QUERY, cgl_obj_get_forward, "vec3", "forward");
	QUERY->doc_func(QUERY, "Get the forward vector of this GGen in world space");

	QUERY->add_mfun(QUERY, cgl_obj_get_up, "vec3", "up");
	QUERY->doc_func(QUERY, "Get the up vector of this GGen in world space");

	// Position ===============================================================

	// float posX()
	QUERY->add_mfun(QUERY, cgl_obj_get_pos_x, "float", "posX");
	QUERY->doc_func(QUERY, "Get X position of this GGen in local space");

	// float posX(float)
	QUERY->add_mfun(QUERY, cgl_obj_set_pos_x, "float", "posX");
	QUERY->add_arg(QUERY, "float", "pos");
	QUERY->doc_func(QUERY, "Set X position of this GGen in local space");

	// float posY()
	QUERY->add_mfun(QUERY, cgl_obj_get_pos_y, "float", "posY");
	QUERY->doc_func(QUERY, "Get Y position of this GGen in local space");

	// float posY(float)
	QUERY->add_mfun(QUERY, cgl_obj_set_pos_y, "float", "posY");
	QUERY->add_arg(QUERY, "float", "pos");
	QUERY->doc_func(QUERY, "Set Y position of this GGen in local space");

	// float posZ()
	QUERY->add_mfun(QUERY, cgl_obj_get_pos_z, "float", "posZ");
	QUERY->doc_func(QUERY, "Get Z position of this GGen in local space");

	// float posZ(float)
	QUERY->add_mfun(QUERY, cgl_obj_set_pos_z, "float", "posZ");
	QUERY->add_arg(QUERY, "float", "pos");
	QUERY->doc_func(QUERY, "Set Z position of this GGen in local space");

	// vec3 pos()
	QUERY->add_mfun(QUERY, cgl_obj_get_pos, "vec3", "pos");
	QUERY->doc_func(QUERY, "Get object position in local space");

	// vec3 pos( vec3 )
	QUERY->add_mfun(QUERY, cgl_obj_set_pos, "vec3", "pos");
	QUERY->add_arg(QUERY, "vec3", "pos");
	QUERY->doc_func(QUERY, "Set object position in local space");

	// vec3 posWorld()
	QUERY->add_mfun(QUERY, cgl_obj_get_pos_world, "vec3", "posWorld");
	QUERY->doc_func(QUERY, "Get object position in world space");

	// vec3 posWorld( float )
	QUERY->add_mfun(QUERY, cgl_obj_set_pos_world, "vec3", "posWorld");
	QUERY->add_arg(QUERY, "vec3", "pos");
	QUERY->doc_func(QUERY, "Set object position in world space");

	// GGen translate( vec3 )
	QUERY->add_mfun(QUERY, cgl_obj_translate, "GGen", "translate");
	QUERY->add_arg(QUERY, "vec3", "translation");
	QUERY->doc_func(QUERY, "Translate this GGen by the given vector");

	// GGen translateX( float )
	QUERY->add_mfun(QUERY, cgl_obj_translate_x, "GGen", "translateX");
	QUERY->add_arg(QUERY, "float", "amt");
	QUERY->doc_func(QUERY, "Translate this GGen by given amount on the X axis in local space");

	// GGen translateY( float )
	QUERY->add_mfun(QUERY, cgl_obj_translate_y, "GGen", "translateY");
	QUERY->add_arg(QUERY, "float", "amt");
	QUERY->doc_func(QUERY, "Translate this GGen by given amount on the Y axis in local space");

	// GGen translateZ( float )
	QUERY->add_mfun(QUERY, cgl_obj_translate_z, "GGen", "translateZ");
	QUERY->add_arg(QUERY, "float", "amt");
	QUERY->doc_func(QUERY, "Translate this GGen by given amount on the Z axis in local space");

	// Rotation ===============================================================

	// float rotX()
	QUERY->add_mfun(QUERY, cgl_obj_get_rot_x, "float", "rotX");
	QUERY->doc_func(QUERY, "Get the rotation of this GGen on the X axis in local space");

	// float rotX( float )
	QUERY->add_mfun(QUERY, cgl_obj_set_rot_x, "float", "rotX");
	QUERY->add_arg(QUERY, "float", "radians");
	QUERY->doc_func(QUERY, "Set the rotation of this GGen on the X axis in local space to the given radians");

	// float rotY()
	QUERY->add_mfun(QUERY, cgl_obj_get_rot_y, "float", "rotY");
	QUERY->doc_func(QUERY, "Get the rotation of this GGen on the Y axis in local space");

	// float rotY( float )
	QUERY->add_mfun(QUERY, cgl_obj_set_rot_y, "float", "rotY");
	QUERY->add_arg(QUERY, "float", "radians");
	QUERY->doc_func(QUERY, "Set the rotation of this GGen on the Y axis in local space to the given radians");

	// float rotZ()
	QUERY->add_mfun(QUERY, cgl_obj_get_rot_z, "float", "rotZ");
	QUERY->doc_func(QUERY, "Get the rotation of this GGen on the Z axis in local space");

	// float rotZ( float )
	QUERY->add_mfun(QUERY, cgl_obj_set_rot_z, "float", "rotZ");
	QUERY->add_arg(QUERY, "float", "radians");
	QUERY->doc_func(QUERY, "Set the rotation of this GGen on the Z axis in local space to the given radians");

	// vec3 rot()
	QUERY->add_mfun(QUERY, cgl_obj_get_rot, "vec3", "rot");
	QUERY->doc_func(QUERY, "Get object rotation in local space as euler angles in radians");

	// vec3 rot( vec3 )
	QUERY->add_mfun(QUERY, cgl_obj_set_rot, "vec3", "rot");
	QUERY->add_arg(QUERY, "vec3", "eulers");
	QUERY->doc_func(QUERY, "Set rotation of this GGen in local space as euler angles");

	// GGen rotate( vec3 )
	QUERY->add_mfun(QUERY, cgl_obj_rotate, "GGen", "rotate");
	QUERY->add_arg(QUERY, "vec3", "eulers");
	QUERY->doc_func(QUERY, "Rotate this GGen by the given euler angles in local space");

	// GGen rotateX( float )
	QUERY->add_mfun(QUERY, cgl_obj_rotate_x, "GGen", "rotateX");
	QUERY->add_arg(QUERY, "float", "radians");
	QUERY->doc_func(QUERY, "Rotate this GGen by the given radians on the X axis in local space");

	// GGen rotateY( float )
	QUERY->add_mfun(QUERY, cgl_obj_rotate_y, "GGen", "rotateY");
	QUERY->add_arg(QUERY, "float", "radians");
	QUERY->doc_func(QUERY, "Rotate this GGen by the given radians on the Y axis in local space");

	// GGen rotateZ( float )
	QUERY->add_mfun(QUERY, cgl_obj_rotate_z, "GGen", "rotateZ");
	QUERY->add_arg(QUERY, "float", "radians");
	QUERY->doc_func(QUERY, "Rotate this GGen by the given radians on the Z axis in local space");

	// GGen rotateOnLocalAxis( vec3, float )
	QUERY->add_mfun(QUERY, cgl_obj_rot_on_local_axis, "GGen", "rotateOnLocalAxis");
	QUERY->add_arg(QUERY, "vec3", "axis");
	QUERY->add_arg(QUERY, "float", "radians");
	QUERY->doc_func(QUERY, "Rotate this GGen by the given radians on the given axis in local space");

	// GGen rotateOnWorldAxis( vec3, float )
	QUERY->add_mfun(QUERY, cgl_obj_rot_on_world_axis, "GGen", "rotateOnWorldAxis");
	QUERY->add_arg(QUERY, "vec3", "axis");
	QUERY->add_arg(QUERY, "float", "radians");
	QUERY->doc_func(QUERY, "Rotate this GGen by the given radians on the given axis in world space");

	// GGen lookAt( vec3 )
	QUERY->add_mfun(QUERY, cgl_obj_lookat_vec3, "GGen", "lookAt");
	QUERY->add_arg(QUERY, "vec3", "pos");
	QUERY->doc_func(QUERY, "Look at the given position in world space");

	// vec3 lookAtDir()
	QUERY->add_mfun(QUERY, cgl_obj_get_forward, "vec3", "lookAtDir");
	QUERY->doc_func(QUERY, "Get the direction this GGen is looking, i.e. the forward vector of this GGen in world space");

	// Scale ===============================================================

	// float scaX()
	QUERY->add_mfun(QUERY, cgl_obj_get_scale_x, "float", "scaX");
	QUERY->doc_func(QUERY, "Get X scale of this GGen in local space");

	// float scaX( float )
	QUERY->add_mfun(QUERY, cgl_obj_set_scale_x, "float", "scaX");
	QUERY->add_arg(QUERY, "float", "scale");
	QUERY->doc_func(QUERY, "Set X scale of this GGen in local space");

	// float scaY()
	QUERY->add_mfun(QUERY, cgl_obj_get_scale_y, "float", "scaY");
	QUERY->doc_func(QUERY, "Get Y scale of this GGen in local space");

	// float scaY( float )
	QUERY->add_mfun(QUERY, cgl_obj_set_scale_y, "float", "scaY");
	QUERY->add_arg(QUERY, "float", "scale");
	QUERY->doc_func(QUERY, "Set Y scale of this GGen in local space");

	// float scaZ()
	QUERY->add_mfun(QUERY, cgl_obj_get_scale_z, "float", "scaZ");
	QUERY->doc_func(QUERY, "Get Z scale of this GGen in local space");

	// float scaZ( float )
	QUERY->add_mfun(QUERY, cgl_obj_set_scale_z, "float", "scaZ");
	QUERY->add_arg(QUERY, "float", "scale");
	QUERY->doc_func(QUERY, "Set Z scale of this GGen in local space");

	// vec3 sca()
	QUERY->add_mfun(QUERY, cgl_obj_get_scale, "vec3", "sca");
	QUERY->doc_func(QUERY, "Get object scale in local space");

	// vec3 sca( vec3 )
	QUERY->add_mfun(QUERY, cgl_obj_set_scale, "vec3", "sca");
	QUERY->add_arg(QUERY, "vec3", "scale");
	QUERY->doc_func(QUERY, "Set object scale in local space");

	// vec3 sca( float )
	QUERY->add_mfun(QUERY, cgl_obj_set_scale_uniform, "vec3", "sca");
	QUERY->add_arg(QUERY, "float", "scale");
	QUERY->doc_func(QUERY, "Set object scale in local space uniformly across all axes");

	// vec3 scaWorld()
	QUERY->add_mfun(QUERY, cgl_obj_get_scale_world, "vec3", "scaWorld");
	QUERY->doc_func(QUERY, "Get object scale in world space");

	// vec3 scaWorld( vec3 )
	QUERY->add_mfun(QUERY, cgl_obj_set_scale_world, "vec3", "scaWorld");
	QUERY->add_arg(QUERY, "vec3", "scale");
	QUERY->doc_func(QUERY, "Set object scale in world space");

	// Matrix transform API ===============================================================
	QUERY->add_mfun(QUERY, cgl_obj_local_pos_to_world_pos, "vec3", "posLocalToWorld");
	QUERY->add_arg(QUERY, "vec3", "localPos");
	QUERY->doc_func(QUERY, "Transform a position in local space to world space");

	// scenegraph relationship methods =======================================
	QUERY->add_mfun(QUERY, cgl_obj_get_parent, "GGen", "parent");
    QUERY->doc_func(QUERY, "Get the parent of this GGen");

    QUERY->add_mfun(QUERY, cgl_obj_get_child, "GGen", "child");
    QUERY->add_arg(QUERY, "int", "n");
    QUERY->doc_func(QUERY, "Get the n'th child of this GGen");

    QUERY->add_mfun(QUERY, cgl_obj_get_child_default, "GGen", "child");
    QUERY->doc_func(QUERY, "Get the 0th child of this GGen");

    QUERY->add_mfun(QUERY, cgl_obj_get_num_children, "int", "numChildren");
    QUERY->doc_func(QUERY, "Get the number of children for this GGen");


	// overload GGen --> GGen
	QUERY->add_op_overload_binary(QUERY, ggen_op_gruck, "GGen", "-->",
								  "GGen", "lhs", "GGen", "rhs");

	// overload GGen --< GGen
	QUERY->add_op_overload_binary(QUERY, ggen_op_ungruck, "GGen", "--<",
								  "GGen", "lhs", "GGen", "rhs");

	QUERY->end_class(QUERY);

	return true;
}

// CGLObject DLL ==============================================
CK_DLL_CTOR(cgl_obj_ctor)
{
	Chuck_DL_Api::Type type = API->type->lookup(VM, "GGen");  // TODO cache this
	auto thisType = API->object->get_type(SELF);
	if (
		API->type->is_equal(type, thisType)  // this type is a GGen
		||
		thisType->originHint == te_originUserDefined  // this type is defined .ck file
		|| 
		thisType->originHint == te_originImport       // .ck file included in search path
	)
	{
		SceneGraphObject *cglObj = new SceneGraphObject;
		CGL::PushCommand(new CreateSceneGraphNodeCommand(cglObj, &CGL::mainScene, SELF, CGL::GetGGenDataOffset()));
	}
}

CK_DLL_DTOR(cgl_obj_dtor)
{
	// unregister from Shred2GGen map 
	// (so we don't geta null ptr reference when the host SHRED exits and tries to detach all GGens)
	CGL::UnregisterGGenFromShred(SHRED, SELF);

	// push command to destroy this object on render thread as well
	CGL::PushCommand(new DestroySceneGraphNodeCommand(SELF, CGL::GetGGenDataOffset(), &CGL::mainScene));
}

CK_DLL_MFUN(cgl_obj_get_id)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	RETURN->v_int = cglObj->GetID();
}

CK_DLL_MFUN(cgl_obj_get_name)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	RETURN->v_string = (Chuck_String *)API->object->create_string(
		VM, cglObj->GetName().c_str(), false
	);
}

CK_DLL_MFUN(cgl_obj_set_name)
{
	SceneGraphObject *obj = CGL::GetSGO(SELF);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	CGL::PushCommand(new UpdateNameCommand(obj, name->str()));
	RETURN->v_string = name;
}

CK_DLL_MFUN(cgl_obj_update) {}

CK_DLL_MFUN(cgl_obj_get_right)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	const auto &right = cglObj->GetRight();
	RETURN->v_vec3 = {right.x, right.y, right.z};
}
CK_DLL_MFUN(cgl_obj_get_forward)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	const auto &vec = cglObj->GetForward();
	RETURN->v_vec3 = {vec.x, vec.y, vec.z};
}

CK_DLL_MFUN(cgl_obj_get_up)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	const auto &vec = cglObj->GetUp();
	RETURN->v_vec3 = {vec.x, vec.y, vec.z};
}

// Position Impl ===============================================================

CK_DLL_MFUN(cgl_obj_get_pos_x)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	RETURN->v_float = cglObj->GetPosition().x;
}

CK_DLL_MFUN(cgl_obj_set_pos_x)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKFLOAT posX = GET_NEXT_FLOAT(ARGS);
	glm::vec3 pos = cglObj->GetPosition();
	pos.x = posX;
	cglObj->SetPosition(pos);
	RETURN->v_float = posX;
	CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_pos_y)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	RETURN->v_float = cglObj->GetPosition().y;
}

CK_DLL_MFUN(cgl_obj_set_pos_y)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKFLOAT posY = GET_NEXT_FLOAT(ARGS);
	glm::vec3 pos = cglObj->GetPosition();
	pos.y = posY;
	cglObj->SetPosition(pos);
	RETURN->v_float = posY;
	CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_pos_z)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	RETURN->v_float = cglObj->GetPosition().z;
}

CK_DLL_MFUN(cgl_obj_set_pos_z)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKFLOAT posZ = GET_NEXT_FLOAT(ARGS);
	glm::vec3 pos = cglObj->GetPosition();
	pos.z = posZ;
	cglObj->SetPosition(pos);
	RETURN->v_float = posZ;
	CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_pos)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	const auto &vec = cglObj->GetPosition();
	RETURN->v_vec3 = {vec.x, vec.y, vec.z};
}

CK_DLL_MFUN(cgl_obj_set_pos)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->SetPosition(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_vec3 = vec;
	CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_pos_world)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	const auto &vec = cglObj->GetWorldPosition();
	RETURN->v_vec3 = {vec.x, vec.y, vec.z};
}

CK_DLL_MFUN(cgl_obj_set_pos_world)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->SetWorldPosition(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_object = SELF;
	CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_translate)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKVEC3 trans = GET_NEXT_VEC3(ARGS);
	cglObj->Translate(glm::vec3(trans.x, trans.y, trans.z));

	// add to command queue
	CGL::PushCommand(new UpdatePositionCommand(cglObj));

	RETURN->v_object = SELF;
}

CK_DLL_MFUN(cgl_obj_translate_x)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKFLOAT amt = GET_NEXT_FLOAT(ARGS);
	cglObj->Translate({amt, 0, 0});

	// add to command queue
	CGL::PushCommand(new UpdatePositionCommand(cglObj));

	RETURN->v_object = SELF;
}

CK_DLL_MFUN(cgl_obj_translate_y)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKFLOAT amt = GET_NEXT_FLOAT(ARGS);
	cglObj->Translate({0, amt, 0});

	// add to command queue
	CGL::PushCommand(new UpdatePositionCommand(cglObj));

	RETURN->v_object = SELF;
}

CK_DLL_MFUN(cgl_obj_translate_z)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKFLOAT amt = GET_NEXT_FLOAT(ARGS);
	cglObj->Translate({0, 0, amt});

	// add to command queue
	CGL::PushCommand(new UpdatePositionCommand(cglObj));

	RETURN->v_object = SELF;
}

// Rotation Impl ===============================================================


CK_DLL_MFUN(cgl_obj_get_rot_x)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	RETURN->v_float = cglObj->GetRotation().x;
}

CK_DLL_MFUN(cgl_obj_set_rot_x)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKFLOAT rad = GET_NEXT_FLOAT(ARGS);
	auto eulers = cglObj->GetEulerRotationRadians();
	eulers.x = rad;
	cglObj->SetRotation(eulers);
	RETURN->v_float = rad;
	CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_rot_y)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	RETURN->v_float = cglObj->GetRotation().y;
}

CK_DLL_MFUN(cgl_obj_set_rot_y)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKFLOAT rad = GET_NEXT_FLOAT(ARGS);
	auto eulers = cglObj->GetEulerRotationRadians();
	// https://gamedev.stackexchange.com/questions/200292/applying-incremental-rotation-with-quaternions-flickering-or-hesitating
	// For continuous rotation, wrap rad to be in range [-PI/2, PI/2]
	// i.e. after exceeding PI/2, rad = rad - PI
	rad = glm::mod(rad + glm::half_pi<double>(), glm::pi<double>()) - glm::half_pi<double>();

	eulers.y = rad;
	cglObj->SetRotation(eulers);
	RETURN->v_float = rad;
	CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_rot_z)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	RETURN->v_float = cglObj->GetRotation().z;
}

CK_DLL_MFUN(cgl_obj_set_rot_z)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKFLOAT rad = GET_NEXT_FLOAT(ARGS);
	auto eulers = cglObj->GetEulerRotationRadians();
	eulers.z = rad;
	cglObj->SetRotation(eulers);
	RETURN->v_float = rad;
	CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_rot)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	const auto &vec = cglObj->GetEulerRotationRadians();
	RETURN->v_vec3 = {vec.x, vec.y, vec.z};
}

CK_DLL_MFUN(cgl_obj_set_rot)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->SetRotation(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_vec3 = vec;
	CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_rotate)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->Rotate(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_object = SELF;
	CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_rotate_x)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKFLOAT rad = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateX(rad);
	RETURN->v_object = SELF;
	CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_rotate_y)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKFLOAT rad = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateY(rad);
	RETURN->v_object = SELF;
	CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_rotate_z)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKFLOAT rad = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateZ(rad);
	RETURN->v_object = SELF;
	CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_rot_on_local_axis)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	t_CKFLOAT deg = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateOnLocalAxis(glm::vec3(vec.x, vec.y, vec.z), deg);

	CGL::PushCommand(new UpdateRotationCommand(cglObj));

	RETURN->v_object = SELF;
}

CK_DLL_MFUN(cgl_obj_rot_on_world_axis)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	t_CKFLOAT deg = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateOnWorldAxis(glm::vec3(vec.x, vec.y, vec.z), deg);

	CGL::PushCommand(new UpdateRotationCommand(cglObj));

	RETURN->v_object = SELF;
}

CK_DLL_MFUN(cgl_obj_lookat_vec3)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->LookAt(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_object = SELF;
	CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

// Scale impl ===============================================================

CK_DLL_MFUN(cgl_obj_get_scale_x)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	RETURN->v_float = cglObj->GetScale().x;
}

CK_DLL_MFUN(cgl_obj_set_scale_x)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKFLOAT scaleX = GET_NEXT_FLOAT(ARGS);
	glm::vec3 scale = cglObj->GetScale();
	scale.x = scaleX;
	cglObj->SetScale(scale);
	RETURN->v_float = scaleX;
	CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_scale_y)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	RETURN->v_float = cglObj->GetScale().y;
}

CK_DLL_MFUN(cgl_obj_set_scale_y)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKFLOAT scaleY = GET_NEXT_FLOAT(ARGS);
	glm::vec3 scale = cglObj->GetScale();
	scale.y = scaleY;
	cglObj->SetScale(scale);
	RETURN->v_float = scaleY;
	CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_scale_z)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	RETURN->v_float = cglObj->GetScale().z;
}

CK_DLL_MFUN(cgl_obj_set_scale_z)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKFLOAT scaleZ = GET_NEXT_FLOAT(ARGS);
	glm::vec3 scale = cglObj->GetScale();
	scale.z = scaleZ;
	cglObj->SetScale(scale);
	RETURN->v_float = scaleZ;
	CGL::PushCommand(new UpdateScaleCommand(cglObj));
}


CK_DLL_MFUN(cgl_obj_get_scale)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	const auto &vec = cglObj->GetScale();
	RETURN->v_vec3 = {vec.x, vec.y, vec.z};
}

CK_DLL_MFUN(cgl_obj_set_scale)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->SetScale(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_vec3 = vec;
	CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_set_scale_uniform)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKFLOAT s = GET_NEXT_FLOAT(ARGS);
	cglObj->SetScale({s, s, s});
	RETURN->v_vec3 = {s, s, s};
	CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_scale_world)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	const auto &vec = cglObj->GetWorldScale();
	RETURN->v_vec3 = {vec.x, vec.y, vec.z};
}

CK_DLL_MFUN(cgl_obj_set_scale_world)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->SetWorldScale(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_vec3 = vec;
	CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

// Transformation API ===============================================================

CK_DLL_MFUN(cgl_obj_local_pos_to_world_pos)
{
	SceneGraphObject *cglObj = CGL::GetSGO(SELF);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	glm::vec3 worldPos = cglObj->GetWorldMatrix() * glm::vec4(vec.x, vec.y, vec.z, 1.0f);
	RETURN->v_vec3 = {worldPos.x, worldPos.y, worldPos.z};
}

// Scenegraph Relationship Impl ===============================================================
CK_DLL_GFUN(ggen_op_gruck)
{
	// get the arguments
	Chuck_Object *lhs = GET_NEXT_OBJECT(ARGS);
	Chuck_Object *rhs = GET_NEXT_OBJECT(ARGS);

	if (!lhs || !rhs) {
		std::string errMsg = std::string("in gruck operator: ") + (lhs?"LHS":"[null]") + " --> " + (rhs?"RHS":"[null]");
		// nullptr exception
		API->vm->throw_exception(
			"NullPointerException",
			errMsg.c_str(),
			SHRED
		);
		return;
	}

	// get internal representation
	SceneGraphObject *LHS = CGL::GetSGO(lhs);
	SceneGraphObject *RHS = CGL::GetSGO(rhs);

	// command
	CGL::PushCommand(new RelationshipCommand(RHS, LHS, RelationshipCommand::Relation::AddChild));

	// return RHS
	RETURN->v_object = rhs;
}

CK_DLL_GFUN(ggen_op_ungruck)
{
	// get the arguments
	Chuck_Object *lhs = GET_NEXT_OBJECT(ARGS);
	Chuck_Object *rhs = GET_NEXT_OBJECT(ARGS);
	// get internal representation
	SceneGraphObject *LHS = CGL::GetSGO(lhs);
	SceneGraphObject *RHS = CGL::GetSGO(rhs);

	// command
	CGL::PushCommand(new RelationshipCommand(RHS, LHS, RelationshipCommand::Relation::RemoveChild));

	// return RHS
	RETURN->v_object = rhs;
}

CK_DLL_MFUN(cgl_obj_get_parent)
{
    SceneGraphObject *cglObj = CGL::GetSGO(SELF);
    auto* parent = cglObj->GetParent();
    // TODO: shouldn't have to refcount here, right?
    RETURN->v_object = parent ? parent->m_ChuckObject : nullptr;
}

CK_DLL_MFUN(cgl_obj_get_child_default)
{
    SceneGraphObject *cglObj = CGL::GetSGO(SELF);
    auto& children = cglObj->GetChildren();
    RETURN->v_object = children.empty() ? nullptr : children[0]->m_ChuckObject;
}

CK_DLL_MFUN(cgl_obj_get_child)
{
    SceneGraphObject *cglObj = CGL::GetSGO(SELF);
    auto& children = cglObj->GetChildren();
    int n = GET_NEXT_INT(ARGS);
    if (n < 0 || n >= children.size())
    {
		API->vm->em_log(
            1,
			"Warning: GGen::child() index out of bounds!\n"
		);
        RETURN->v_object = nullptr;
    }
    else
    {
        RETURN->v_object = children[n]->m_ChuckObject;
    }
}

CK_DLL_MFUN(cgl_obj_get_num_children)
{
    SceneGraphObject *cglObj = CGL::GetSGO(SELF);
    RETURN->v_int = cglObj->GetChildren().size();
}