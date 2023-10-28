#include "ulib_assimp.h"
#include "ulib_cgl.h"
#include "scenegraph/Command.h"
#include "renderer/scenegraph/SceneGraphObject.h"


//-----------------------------------------------------------------------------
// assimp implementation
//-----------------------------------------------------------------------------
CK_DLL_SFUN(chugl_assimp_load);


//-----------------------------------------------------------------------------
// assimp implementation
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_assimp(Chuck_DL_Query *QUERY)
{
    return TRUE;
}


// load
CK_DLL_SFUN(chugl_assimp_load)
{
    Assimp::Importer importer;
    // const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
    
    // create GMesh to mirror scene
    // link up child / parent
    // for each GMesh, create Geometry (and default Material / e.g., PhongMaterial)
    // load the position, color, normal, uv into geometry
    // see Geometry.cpp / addVertex()
    // see GPlane, GCube etc. for proper refcounting in creating GMesh + Geometry
    // if assets include textures, also create Texture, bind to Material
}
