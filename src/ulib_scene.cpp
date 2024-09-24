#include <chuck/chugin.h>

#include "sg_command.h"
#include "sg_component.h"

#include "ulib_helper.h"

CK_DLL_CTOR(gscene_ctor);
CK_DLL_DTOR(gscene_dtor);

CK_DLL_MFUN(gscene_set_background_color);
CK_DLL_MFUN(gscene_get_background_color);

CK_DLL_MFUN(gscene_set_main_camera);
CK_DLL_MFUN(gscene_get_main_camera);

CK_DLL_MFUN(gscene_set_ambient_light);
CK_DLL_MFUN(gscene_get_ambient_light);

CK_DLL_MFUN(gscene_get_default_light);
CK_DLL_MFUN(gscene_get_lights);

SG_Scene* ulib_scene_create(Chuck_Object* ckobj)
{
    CK_DL_API API = g_chuglAPI;

    // execute change on audio thread side
    SG_Scene* scene = SG_CreateScene(ckobj);
    // save SG_ID
    OBJ_MEMBER_UINT(ckobj, component_offset_id) = scene->id;

    CQ_PushCommand_SceneUpdate(scene);
    return scene;
}

static void ulib_gscene_query(Chuck_DL_Query* QUERY)
{
    // EM_log(CK_LOG_INFO, "ChuGL scene");
    // CGL scene
    QUERY->begin_class(QUERY, SG_CKNames[SG_COMPONENT_SCENE],
                       SG_CKNames[SG_COMPONENT_TRANSFORM]);
    QUERY->add_ctor(QUERY, gscene_ctor);

    // background color
    QUERY->add_mfun(QUERY, gscene_set_background_color, "void", "backgroundColor");
    QUERY->add_arg(QUERY, "vec3", "color");
    QUERY->doc_func(QUERY, "Set the background color of the scene");

    QUERY->add_mfun(QUERY, gscene_get_background_color, "vec3", "backgroundColor");
    QUERY->doc_func(QUERY, "Get the background color of the scene");

    // main camera
    MFUN(gscene_set_main_camera, "GCamera", "camera");
    ARG("GCamera", "camera");
    DOC_FUNC("Set the main camera of the scene");

    MFUN(gscene_get_main_camera, "GCamera", "camera");
    DOC_FUNC("Get the main camera of the scene");

    // ambient light
    MFUN(gscene_set_ambient_light, "void", "ambient");
    ARG("vec3", "color");
    DOC_FUNC(
      "Set the ambient lighting of the scene. Sets material visibility even when no "
      "light is present");

    MFUN(gscene_get_ambient_light, "vec3", "ambient");
    DOC_FUNC("Get the ambient lighting value of the scene");

    MFUN(gscene_get_default_light, SG_CKNames[SG_COMPONENT_LIGHT], "light");
    DOC_FUNC(
      "Get the first light of the scene. On the initial "
      "default scene, this returns the default directional light");

    MFUN(gscene_get_lights, "GLight[]", "lights");
    DOC_FUNC("Get array of all lights in the scene");

    // end class -----------------------------------------------------
    QUERY->end_class(QUERY);
}

CK_DLL_CTOR(gscene_ctor)
{
    ulib_scene_create(SELF);
}

CK_DLL_DTOR(gscene_dtor)
{
    // TODO
}

CK_DLL_MFUN(gscene_get_background_color)
{
    SG_Scene* scene = SG_GetScene(OBJ_MEMBER_UINT(SELF, component_offset_id));
    glm::vec4 color = scene->desc.bg_color;
    RETURN->v_vec3  = { color.r, color.g, color.b };
}

CK_DLL_MFUN(gscene_set_background_color)
{
    SG_Scene* scene = SG_GetScene(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKVEC3 color  = GET_NEXT_VEC3(ARGS);

    scene->desc.bg_color = { color.x, color.y, color.z, 1.0f };

    CQ_PushCommand_SceneUpdate(scene);
}

CK_DLL_MFUN(gscene_set_main_camera)
{
    SG_Scene* scene      = SG_GetScene(OBJ_MEMBER_UINT(SELF, component_offset_id));
    Chuck_Object* ck_cam = GET_NEXT_OBJECT(ARGS);

    SG_Camera* cam
      = ck_cam ? SG_GetCamera(OBJ_MEMBER_UINT(ck_cam, component_offset_id)) : NULL;

    if (cam && cam->id == scene->desc.main_camera_id) {
        RETURN->v_object = ck_cam;
        return;
    }

    // check if camera is connected to scene
    if (cam && !SG_Transform::isAncestor(scene, cam)) {
        CK_THROW("DisconnctedCamera",
                 "A camera must be connected (grucked) to scene before it can be set "
                 "as the main camera",
                 SHRED);
    }

    SG_Scene::setMainCamera(scene, cam);

    // update gfx thread
    CQ_PushCommand_SceneUpdate(scene);
}

CK_DLL_MFUN(gscene_get_main_camera)
{
    SG_Scene* scene = SG_GetScene(OBJ_MEMBER_UINT(SELF, component_offset_id));
    SG_Camera* cam  = SG_GetCamera(scene->desc.main_camera_id);

    RETURN->v_object = cam ? cam->ckobj : NULL;
}

CK_DLL_MFUN(gscene_set_ambient_light)
{
    SG_Scene* scene  = SG_GetScene(OBJ_MEMBER_UINT(SELF, component_offset_id));
    t_CKVEC3 ambient = GET_NEXT_VEC3(ARGS);

    scene->desc.ambient_light = { ambient.x, ambient.y, ambient.z };

    CQ_PushCommand_SceneUpdate(scene);
}

CK_DLL_MFUN(gscene_get_ambient_light)
{
    SG_Scene* scene   = SG_GetScene(OBJ_MEMBER_UINT(SELF, component_offset_id));
    glm::vec3 ambient = scene->desc.ambient_light;
    RETURN->v_vec3    = { ambient.r, ambient.g, ambient.b };
}

CK_DLL_MFUN(gscene_get_default_light)
{
    SG_Scene* scene  = SG_GetScene(OBJ_MEMBER_UINT(SELF, component_offset_id));
    SG_Light* light  = SG_Scene::getLight(scene, 0);
    RETURN->v_object = light ? light->ckobj : NULL;
}

CK_DLL_MFUN(gscene_get_lights)
{
    SG_Scene* scene = SG_GetScene(OBJ_MEMBER_UINT(SELF, component_offset_id));
    int num_lights  = ARENA_LENGTH(&scene->light_ids, SG_ID);

    Chuck_ArrayInt* light_ck_array
      = (Chuck_ArrayInt*)chugin_createCkObj("GLight[]", false, SHRED);

    for (int i = 0; i < num_lights; i++) {
        Chuck_Object* ck_light
          = SG_GetLight(*ARENA_GET_TYPE(&scene->light_ids, SG_ID, i))->ckobj;
        API->object->array_int_push_back(light_ck_array, (t_CKUINT)ck_light);
    }

    RETURN->v_object = (Chuck_Object*)light_ck_array;
}
