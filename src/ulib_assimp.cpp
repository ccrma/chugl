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
#include "renderer/scenegraph/Geometry.h"


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
    QUERY->add_sfun( QUERY, chugl_assimp_load, "GGen", "load" );
    QUERY->add_arg( QUERY, "string", "path" );
    QUERY->doc_func( QUERY, "Load an asset and return a GGen object; if the asset is hierarchical, "
        "the return value is root node in a scene graph of GGen objects that can "
        "be traversed using the standard parent/children relationship of GGens. "
        "The respective names of GMesh scene graph nodes can be queried using "
        "GGen.name() method." );

    // end query
    QUERY->end_class( QUERY );

    return TRUE;
}

// internal asset loader data structure
struct AssLoader
{
    Chuck_VM * vm;
    Chuck_VM_Shred * shred;
    CK_DL_API api;
    const aiScene * scene;

    // mirrors the aiScene's meshes table
    vector<CustomGeometry*> geometries;

public:
    // constructor
    AssLoader( Chuck_VM* VM, Chuck_VM_Shred* SHRED, CK_DL_API API )
        : vm(VM), shred(SHRED), api(API), scene(NULL) { }

public:
    // load asset
    Chuck_Object * LoadAss(const string& path, unsigned int flags);

private:
    // look up chugl mesh by index
    Mesh * CreateMesh( t_CKUINT index );
    // create custom geo from ai mesh
    CustomGeometry * CreateGeometry(aiMesh* assMesh);
    // process a node
    SceneGraphObject* ProcessNode(SceneGraphObject* parent, aiNode* assNode);
};

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
    // asset loader
    AssLoader al(VM, SHRED, API);

    // check it
    if( !str )
    {
        // error message
        cerr << "[chugl]: AssLoader.load() null file path argument" << endl;
        // error out
        goto error;
    }

    // this should be a GMesh ggen
    RETURN->v_object = al.LoadAss( str->str(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals );

    // done
    return;

error:
    // set return value (already default to null, so this is just for good measure)
    RETURN->v_object = NULL;
}


CustomGeometry * AssLoader::CreateGeometry( aiMesh * assMesh )
{
    vector<t_CKFLOAT> positions;
    vector<t_CKFLOAT> normals;
    vector<t_CKFLOAT> uvs;
    vector<t_CKUINT> indices;

    // position, normal, uv data
    for(t_CKUINT i = 0; i < assMesh->mNumVertices; i++)
    {
        positions.push_back(assMesh->mVertices[i].x);
        positions.push_back(assMesh->mVertices[i].y);
        positions.push_back(assMesh->mVertices[i].z);

        normals.push_back(assMesh->mNormals[i].x);
        normals.push_back(assMesh->mNormals[i].y);
        normals.push_back(assMesh->mNormals[i].z);

        if(assMesh->mTextureCoords[0])
        {
            uvs.push_back(assMesh->mTextureCoords[0][i].x);
            uvs.push_back(assMesh->mTextureCoords[0][i].y);
        }
        else
        {
            uvs.push_back(0);
            uvs.push_back(0);
        }
    }

    // face data
    for(t_CKUINT i = 0; i < assMesh->mNumFaces; i++)
    {
        // get current face
        aiFace& face = assMesh->mFaces[i];
        // TODO: this should not be an assert, print error and return
        assert(face.mNumIndices == 3);
        // face indices
        for(t_CKUINT j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    // create geometry
    CustomGeometry* geo = new CustomGeometry;
    // create corresponding chuck object, refcount=false since MeshSet() later will refcount
    CGL::CreateChuckObjFromGeo(api, vm, geo, shred, false);

    // propagate data to render thread
    CGL::PushCommand(new UpdateGeometryAttributeCommand(geo, "position", Geometry::POSITION_ATTRIB_IDX, 3, positions));
    CGL::PushCommand(new UpdateGeometryAttributeCommand(geo, "normals", Geometry::NORMAL_ATTRIB_IDX, 3, normals));
    CGL::PushCommand(new UpdateGeometryAttributeCommand(geo, "uv0", Geometry::UV0_ATTRIB_IDX, 2, uvs));
    CGL::PushCommand(new UpdateGeometryIndicesCommand(geo, indices));

    return geo;
}

Mesh * AssLoader::CreateMesh( t_CKUINT index )
{
    // check for out of bound
    if(index >= geometries.size())
    {
        cerr << "[chugl] AssLoader.load() encountered invalid mesh index: " << index << " out of " << geometries.size() << endl;
        return NULL;
    }

    // get type
    Chuck_DL_Api::Type type = api->type->lookup(vm, "GMesh");
    // verify
    assert(type != NULL);
    // create object
    Chuck_DL_Api::Object meshObj = api->object->create(shred, type, false);

    // create material
    Material* mat = new PhongMaterial;
    // create corresponding chuck object, refcount=false since MeshSet() later will refcount
    CGL::CreateChuckObjFromMat(api, vm, mat, shred, false);

    // look up the geo, creating it from corresponding aiMesh if necessary
    CustomGeometry* geo = geometries[index] ? geometries[index] : CreateGeometry( scene->mMeshes[index] );

    // create mesh
    Mesh* mesh = NULL;
    // create HACK: would not need this if pre-ctors are called as part of create() above!
    CGL::PushCommand(new CreateSceneGraphNodeCommand(mesh = new Mesh, &CGL::mainScene, meshObj, CGL::GetGGenDataOffset()));
    // set geo and mat into mesh
    CGL::MeshSet(mesh, geo, mat);

    return mesh;
}

SceneGraphObject* AssLoader::ProcessNode( SceneGraphObject * parent, aiNode* assNode )
{
    // get type
    Chuck_DL_Api::Type type = api->type->lookup(vm, "GGen");
    // verify
    assert(type != NULL);
    // create dummy ggen
    Chuck_DL_Api::Object ggen = api->object->create(shred, type, false);
    // create chugl internal represensation
    SceneGraphObject * cgobj = new SceneGraphObject();
    // propagate to render thread
    CGL::PushCommand(new CreateSceneGraphNodeCommand(cgobj, &CGL::mainScene, ggen, CGL::GetGGenDataOffset()));
    // set up ggen relationship
    if( parent ) CGL::PushCommand(new RelationshipCommand(parent, cgobj, RelationshipCommand::Relation::AddChild));

    // process all the node's meshes (if any)
    for(unsigned int i = 0; i < assNode->mNumMeshes; i++)
    {
        // build chugl mesh
        Mesh * mesh = CreateMesh( assNode->mMeshes[i] );
        // set up ggen relationship
        CGL::PushCommand(new RelationshipCommand(cgobj, mesh, RelationshipCommand::Relation::AddChild));
    }

    // then do the same for each of its children
    for(unsigned int i = 0; i < assNode->mNumChildren; i++)
    {
        ProcessNode( cgobj, assNode->mChildren[i] );
    }

    // return 
    return cgobj;
}

// helper function to import asset
Chuck_Object * AssLoader::LoadAss( const string & path, unsigned int flags )
{
    // assimp importer
    Assimp::Importer importer;
    // load the asset as an Assimp scene
    scene = importer.ReadFile( path, flags );
    // check the return
    if( !scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode )
    {
        // error message
        cerr << "[chugl]: AssLoader.load() error: " << importer.GetErrorString() << endl;
        // error out
        return NULL;
    }

    // reserve
    geometries.resize(scene->mNumMeshes, NULL);

    // process starting from root node
    SceneGraphObject * cgobj = ProcessNode( NULL, scene->mRootNode );
    // return
    return cgobj ? cgobj->m_ChuckObject : NULL;
}
