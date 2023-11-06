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
// load
CK_DLL_SFUN( chugl_assimp_load );
// load with options
CK_DLL_SFUN( chugl_assimp_load_options );

// function prototypes
Chuck_Object * do_assimp_load( const string & path, unsigned int flags,
    Chuck_VM * VM, Chuck_VM_Shred * SHRED, CK_DL_API API );


//-----------------------------------------------------------------------------
// assimp implementation
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_assimp(Chuck_DL_Query *QUERY)
{
    // begin class
    QUERY->begin_class( QUERY, "AssLoader", "Object" );
    // add class documentation
    QUERY->doc_class( QUERY, "Utility for asset loading; based on the AssImp library." );

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
    */

    // load
    QUERY->add_sfun( QUERY, chugl_assimp_load, "GMesh", "load" );
    QUERY->add_arg( QUERY, "string", "path" );
    QUERY->doc_func( QUERY, "Load an asset and return a GMesh object; if the asset is hierarchical, "
        "the return value is root node in a scene graph of GMesh objects that can "
        "be traversed using the standard parent/children relationship of GGens. "
        "The respective names of GMesh scene graph nodes can be queried using "
        "GGen's .name() method." );

    return TRUE;
}


// load
// create GMesh to mirror scene
// link up child / parent
// for each GMesh, create Geometry (and default Material / e.g., PhongMaterial)
// load the position, color, normal, uv into geometry
// see Geometry.cpp / addVertex()
// see GPlane, GCube etc. for proper refcounting in creating GMesh + Geometry
// if assets include textures, also create Texture, bind to Material
CK_DLL_SFUN( chugl_assimp_load )
{
    // get argument
    Chuck_String * str = (Chuck_String *)GET_NEXT_OBJECT(ARGS);
    // check it
    if( !str )
    {
        // error message
        cerr << "[chugl]: AssLoader.load() null file path argument" << endl;
        // error out
        goto error;
    }

    // this should be a GMesh ggen
    Chuck_Object * gmesh = do_assimp_load( str->str(), aiProcess_Triangulate | aiProcess_FlipUVs, VM, SHRED, API );

error:
    // set return value (already default to null, so this is just for good measure)
    RETURN->v_object;
    // done
    return;
}

// helper function to import asset
Chuck_Object * do_assimp_load( const string & path, unsigned int flags, Chuck_VM * VM, Chuck_VM_Shred * SHRED, CK_DL_API API )
{
    // assimp importer
    Assimp::Importer importer;
    // load the asset as an Assimp scene
    const aiScene * scene = importer.ReadFile( path, flags );
    // check the return
    if( !scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode )
    {
        // error message
        cerr << "[chugl]: AssLoader.load() error: " << importer.GetErrorString() << endl;
        // error out
        return NULL;
    }

    // get type
    Chuck_DL_Api::Type type = API->type->lookup( VM, "GMesh" );
    // verify
    assert( type != NULL );
    // create object
    Chuck_DL_Api::Object meshObj = API->object->create( SHRED, type, true );
    // create mesh
    Mesh * mesh = NULL;
    // create HACK: would not need this if pre-ctors are called as part of create() above!
    CGL::PushCommand( new CreateSceneGraphNodeCommand( mesh = new Mesh, &CGL::mainScene, meshObj, CGL::GetGGenDataOffset() ) );

    // create geometry
    Geometry * geo = new CustomGeometry;
    // create corresponding chuck object, refcount=false since MeshSet() later will refcount
    CGL::CreateChuckObjFromGeo( API, VM, geo, SHRED, false );
    // create material
    Material * mat = new PhongMaterial;
    // create corresponding chuck object, refcount=false since MeshSet() later will refcount
    CGL::CreateChuckObjFromMat( API, VM, mat, SHRED, false );
    // set geo and mat into mesh
    CGL::MeshSet( mesh, geo, mat );

    return NULL;
}