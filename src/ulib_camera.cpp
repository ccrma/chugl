#include "ulib_camera.h"
#include "ulib_cgl.h"
#include "scenegraph/Command.h"
#include "renderer/scenegraph/Camera.h"

//-----------------------------------------------------------------------------
// Object -> BaseCamera
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_cam_ctor);
CK_DLL_DTOR(cgl_cam_dtor);

CK_DLL_MFUN(cgl_cam_set_mode_persp); // switch to perspective mode
CK_DLL_MFUN(cgl_cam_set_mode_ortho); // switch to perspective mode
CK_DLL_MFUN(cgl_cam_get_mode);		 // switch to perspective mode

CK_DLL_MFUN(cgl_cam_set_clip);
CK_DLL_MFUN(cgl_cam_get_clip_near);
CK_DLL_MFUN(cgl_cam_get_clip_far);

// perspective camera params
// (no aspect, that's set automatically by renderer window resize callback)
CK_DLL_MFUN(cgl_cam_set_pers_fov); // set in degrees
CK_DLL_MFUN(cgl_cam_get_pers_fov); //

// ortho camera params
CK_DLL_MFUN(cgl_cam_set_ortho_size); // size of view volume (preserves screen aspect ratio)
CK_DLL_MFUN(cgl_cam_get_ortho_size); //

// mouse cast from camera
CK_DLL_MFUN(chugl_cam_screen_coord_to_world_ray);
CK_DLL_MFUN(chugl_cam_world_pos_to_screen_coord);

//-----------------------------------------------------------------------------
// init_chugl_cam()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_camera(Chuck_DL_Query *QUERY)
{
	// EM_log(CK_LOG_INFO, "ChuGL Camera");
	// CGL camera
	QUERY->begin_class(QUERY, "GCamera", "GGen");
	QUERY->doc_class(QUERY, "Camera class. Static--all instances point to the same underlying ChuGL main camera");
    QUERY->add_ex(QUERY, "basic/mousecast.ck");

	QUERY->add_ctor(QUERY, cgl_cam_ctor);

	// static vars
	// perspective mode
	QUERY->add_svar(QUERY, "int", "PERSPECTIVE", true, (void *)&Camera::MODE_PERSPECTIVE);
	QUERY->add_svar(QUERY, "int", "ORTHO", true, (void *)&Camera::MODE_ORTHO);

	QUERY->add_mfun(QUERY, cgl_cam_set_mode_persp, "void", "perspective");
	QUERY->doc_func(QUERY, "Set camera to perspective mode");

	QUERY->add_mfun(QUERY, cgl_cam_set_mode_ortho, "void", "orthographic");
	QUERY->doc_func(QUERY, "Set camera to orthographic mode");

	QUERY->add_mfun(QUERY, cgl_cam_get_mode, "int", "mode");
	QUERY->doc_func(QUERY, "Get camera mode. Can be GCamera.MODE_PERSP or GCamera.MODE_ORTHO");


	// clipping planes
	QUERY->add_mfun(QUERY, cgl_cam_set_clip, "void", "clip");
	QUERY->add_arg(QUERY, "float", "near");
	QUERY->add_arg(QUERY, "float", "far");
	QUERY->doc_func(QUERY, "Set camera clipping planes");

	QUERY->add_mfun(QUERY, cgl_cam_get_clip_near, "float", "clipNear");
	QUERY->doc_func(QUERY, "get near clipping plane");


	QUERY->add_mfun(QUERY, cgl_cam_get_clip_far, "float", "clipFar");
	QUERY->doc_func(QUERY, "get far clipping plane");

	// fov (in degrees)
	QUERY->add_mfun(QUERY, cgl_cam_set_pers_fov, "float", "fov");
	QUERY->add_arg(QUERY, "float", "f");
	QUERY->doc_func(QUERY, "(perspective mode) set the field of view in degrees");

	QUERY->add_mfun(QUERY, cgl_cam_get_pers_fov, "float", "fov");
	QUERY->doc_func(QUERY, "(perspective mode) get the field of view in degrees");


	// ortho view size
	QUERY->add_mfun(QUERY, cgl_cam_set_ortho_size, "float", "viewSize");
	QUERY->add_arg(QUERY, "float", "s");
	QUERY->doc_func(QUERY, "(orthographic mode) set the height of the view in pixels. Width is automatically calculated based on aspect ratio");

	QUERY->add_mfun(QUERY, cgl_cam_get_ortho_size, "float", "viewSize");
	QUERY->doc_func(QUERY, "(orthographic mode) get the height of the view in pixels");

	// raycast from mousepos
	QUERY->add_mfun(QUERY, chugl_cam_screen_coord_to_world_ray, "vec3", "screenCoordToWorldRay");
	QUERY->add_arg(QUERY, "float", "screenX");
	QUERY->add_arg(QUERY, "float", "screenY");
	QUERY->doc_func(QUERY, 
		"Get a ray in world space representing the normalized directional vector from camera world position to screen coordinate"
		"screenX and screenY are screen coordinates, which you can get from GG.mouseX() and GG.mouseY() or you can pass coordinates directly"
		"useful if you want to do mouse picking or raycasting"
	);

	QUERY->add_mfun(QUERY, chugl_cam_world_pos_to_screen_coord, "vec3", "worldPosToScreenCoord");
	QUERY->add_arg(QUERY, "vec3", "worldPos");
	QUERY->doc_func(QUERY, 
		"Get a screen coordinate from a world position by casting a ray from worldPos back to the camera and finding the intersection with the near clipping plane"
		"worldPos is a vec3 representing a world position"
		"returns a vec3. X and Y are screen coordinates, Z is the depth-value of the worldPos"
		"Remember, screen coordinates have origin at the top-left corner of the window"
	);

	QUERY->end_class(QUERY);

	return true;
}

// CGL Camera ===============================================================
CK_DLL_CTOR(cgl_cam_ctor)
{
	// for now just return the main camera
	// very important: have to access main scene through CGL::GetMainScene() because
	// that's where the default camera construction happens
	Scene *scene = (Scene *)CGL::GetSGO(CGL::GetMainScene(SHRED, API, VM));
	OBJ_MEMBER_INT(SELF, CGL::GetGGenDataOffset()) = (t_CKINT)(scene->GetMainCamera());
}

CK_DLL_DTOR(cgl_cam_dtor)
{
	// TODO: no destructors for static vars (we don't want one camera handle falling out of scope to delete the only camera)

	// Camera* mainCam = (Camera*) CGL::GetSGO(SELF);
	// don't call delete! because this is a static var
	OBJ_MEMBER_INT(SELF, CGL::GetGGenDataOffset()) = 0; // zero out the memory
}

CK_DLL_MFUN(cgl_cam_set_mode_persp)
{
	Scene *scene = (Scene *)CGL::GetSGO(CGL::GetMainScene(SHRED, API, VM));
	// Camera *cam = (Camera *) CGL::GetSGO(SELF);
	Camera * cam = scene->GetMainCamera();
	cam->SetPerspective();

	CGL::PushCommand(new UpdateCameraCommand(cam));
}

CK_DLL_MFUN(cgl_cam_set_mode_ortho)
{
	Camera *cam = (Camera *) CGL::GetSGO(SELF);
	cam->SetOrtho();

	CGL::PushCommand(new UpdateCameraCommand(cam));
}

CK_DLL_MFUN(cgl_cam_get_mode)
{
	Camera *cam = (Camera *) CGL::GetSGO(SELF);
	RETURN->v_int = cam->GetMode();
}

CK_DLL_MFUN(cgl_cam_set_clip)
{
	Camera *cam = (Camera *) CGL::GetSGO(SELF);
	t_CKFLOAT n = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT f = GET_NEXT_FLOAT(ARGS);
	cam->SetClipPlanes(n, f);

	CGL::PushCommand(new UpdateCameraCommand(cam));
}

CK_DLL_MFUN(cgl_cam_get_clip_near)
{
	Camera *cam = (Camera *) CGL::GetSGO(SELF);
	RETURN->v_float = cam->GetClipNear();
}

CK_DLL_MFUN(cgl_cam_get_clip_far)
{
	Camera *cam = (Camera *) CGL::GetSGO(SELF);
	RETURN->v_float = cam->GetClipFar();
}

CK_DLL_MFUN(cgl_cam_set_pers_fov)
{
	Camera *cam = (Camera *) CGL::GetSGO(SELF);
	t_CKFLOAT f = GET_NEXT_FLOAT(ARGS);
	cam->SetFOV(f);

	RETURN->v_float = f;

	CGL::PushCommand(new UpdateCameraCommand(cam));
}

CK_DLL_MFUN(cgl_cam_get_pers_fov)
{
	Camera *cam = (Camera *) CGL::GetSGO(SELF);
	RETURN->v_float = cam->GetFOV();
}

CK_DLL_MFUN(cgl_cam_set_ortho_size)
{
	Camera *cam = (Camera *) CGL::GetSGO(SELF);
	t_CKFLOAT s = GET_NEXT_FLOAT(ARGS);
	cam->SetSize(s);

	RETURN->v_float = s;

	CGL::PushCommand(new UpdateCameraCommand(cam));
}

CK_DLL_MFUN(cgl_cam_get_ortho_size)
{
	Camera *cam = (Camera *) CGL::GetSGO(SELF);
	RETURN->v_float = cam->GetSize();
}

CK_DLL_MFUN(chugl_cam_screen_coord_to_world_ray)
{
	Camera *cam = (Camera *) CGL::GetSGO(SELF);
	t_CKFLOAT screenX = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT screenY = GET_NEXT_FLOAT(ARGS);

	// first convert to normalized device coordinates in range [-1, 1]
	auto windowSize = CGL::GetWindowSize();
	float x = ( (2.0f * screenX) / windowSize.first ) - 1.0f;
	float y = 1.0f - ( (2.0f * screenY) / windowSize.second );
	float z = 1.0f;
	glm::vec4 ray_clip = glm::vec4(x, y, -1.0, 1.0);
	// convert to eye space
	glm::mat4 proj = cam->GetProjectionMatrix();
	glm::vec4 ray_eye = glm::inverse(proj) * ray_clip;
	// convert to world space
	ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);
	glm::mat4 view = cam->GetViewMatrix();
	glm::vec3 ray_wor = glm::inverse(view) * ray_eye;
	// normalize
	ray_wor = glm::normalize(ray_wor);
	RETURN->v_vec3 = {ray_wor.x, ray_wor.y, ray_wor.z};
}

CK_DLL_MFUN(chugl_cam_world_pos_to_screen_coord)
{
	Camera *cam = (Camera *) CGL::GetSGO(SELF);
	t_CKVEC3 worldPos = GET_NEXT_VEC3(ARGS);

	// first convert to clip space
	glm::mat4 view = cam->GetViewMatrix();
	glm::mat4 proj = cam->GetProjectionMatrix();
	glm::vec4 clipPos = proj * view * glm::vec4(worldPos.x, worldPos.y, worldPos.z, 1.0f);

	// convert to normalized device coordinates in range [-1, 1]
	auto windowSize = CGL::GetWindowSize();
	float x = (clipPos.x / clipPos.w + 1.0f) / 2.0f * windowSize.first;
	// need to invert y because screen coordinates are top-left origin
	float y = (1.0f - clipPos.y / clipPos.w) / 2.0f * windowSize.second;
	// z is depth value
	float z = clipPos.z / clipPos.w;
	RETURN->v_vec3 = {x, y, z};
}
