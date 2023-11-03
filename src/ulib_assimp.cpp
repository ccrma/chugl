//-----------------------------------------------------------------------------
// name: ulib_assimp.cpp
// desc: asset import module
//
// author: Andrew Zhu Aday
//         Ge Wang (https://ccrma.stanford.edu/~ge/
// date: Fall 2023
//-----------------------------------------------------------------------------
#include "ulib_assimp.h"
#include "ulib_cgl.h"
#include "scenegraph/Command.h"
#include "renderer/scenegraph/SceneGraphObject.h"


//-----------------------------------------------------------------------------
// assimp implementation
//-----------------------------------------------------------------------------
CK_DLL_SFUN(chugl_assimp_load); // query function
CK_DLL_CTOR(assloader_obj_ctor);
CK_DLL_DTOR(assloader_obj_dtor);

// load
CK_DLL_SFUN(assloader_load);
// load with options
CK_DLL_SFUN(assloader_load);


//-----------------------------------------------------------------------------
// assimp implementation
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_assimp(Chuck_DL_Query *QUERY)
{
    // begin class
    QUERY->begin_class( QUERY, "AssLoader", "Object" );
    // add class documentation
    QUERY->doc_class(QUERY, "Utility for asset loading; based on the AssImp library.");

    /*
    // add ctor
    QUERY->add_ctor(QUERY, assloader_obj_ctor);
    // add dtor
    QUERY->add_dtor(QUERY, assloader_obj_dtor);

    // load an asset and insert result by name in map
    QUERY->add_sfun( QUERY, assloader_cache_insert, "int", "cache" );
    QUERY->add_arg( QUERY, "string", "path" );
    QUERY->add_arg( QUERY, "string", "identifier" );
    QUERY->doc_func( QUERY, "Load an asset and insert the resulting GMesh graph into AssLoader's global table by name." );

    // lookup an asset and insert result by name in map
    QUERY->add_sfun(QUERY, assloader_cache_insert, "int", "cache");
    QUERY->add_arg(QUERY, "string", "path");
    QUERY->add_arg(QUERY, "string", "identifier");
    QUERY->doc_func(QUERY, "Load an asset and insert the resulting GMesh graph into AssLoader's global table by name.");

    // load
    QUERY->add_sfun(QUERY, assloader_load, "GMesh", "load");
    QUERY->add_arg(QUERY, "string", "path");
    QUERY->doc_func(QUERY, "Load an asset and return a GMesh object; if the asset is hierarchical, "
                           "the return value is root node in a scene graph of GMesh objects that can "
                           "be traversed using the standard parent/children relationship of GGens. "
                           "The respective names of GMesh scene graph nodes can be queried using "
                           "GGen's .name() method." );
    */

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
