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

#include <stb/stb_image.h>

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
	QUERY->add_ex(QUERY, "models/backpa.ck");

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


//-----------------------------------------------------------------------------
// internal asset loader data structure
//-----------------------------------------------------------------------------
struct AssLoader
{
    Chuck_VM * vm;
    Chuck_VM_Shred * shred;
    CK_DL_API api;
    const aiScene * scene;
    string directory;

    // mirrors the aiScene's meshes table
    vector<CustomGeometry*> geometries;
    // material cache
    vector<Material*> materials;
    // texture cache
    unordered_map<string, CGL_Texture*> textures;

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
    // create material
    Material * CreateMaterial( aiMaterial * assMat );
    // create texture
    CGL_Texture * CreateTexture( aiMaterial * assMat, aiTextureType type );
    // process a node
    SceneGraphObject* ProcessNode(SceneGraphObject* parent, aiNode* assNode);
};


//-----------------------------------------------------------------------------
// load
//-----------------------------------------------------------------------------
// create GMesh to mirror scene
// link up child / parent
// for each GMesh, create Geometry (and default Material / e.g., PhongMaterial)
// load the position, color, normal, uv into geometry
// see Geometry.cpp / addVertex()
// see GPlane, GCube etc. for proper refcounting in creating GMesh + Geometry
// if assets include textures, also create Texture, bind to Material
//-----------------------------------------------------------------------------
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


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
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




//-----------------------------------------------------------------------------
// create material
//-----------------------------------------------------------------------------
Material* AssLoader::CreateMaterial( aiMaterial * assMat )
{
    PhongMaterial* mat = new PhongMaterial;
    CGL::CreateChuckObjFromMat(api, vm, mat, shred, false);

    // currently only support specular and diffuse textures
    // in the FUTURE: may support normal mapping and ambient oclusion
    CGL_Texture* diffuseMap = CreateTexture(assMat, aiTextureType_DIFFUSE);
    CGL_Texture* specularMap = CreateTexture(assMat, aiTextureType_SPECULAR);

    if(diffuseMap) {
        // TODO: the diffuseMap should be set in constructor of command
        mat->SetDiffuseMap(diffuseMap);
        CGL::PushCommand(new UpdateMaterialUniformCommand(
            mat, *mat->GetUniform(Material::DIFFUSE_MAP_UNAME)));
    }

    if(specularMap) {
        // TODO: the diffuseMap should be set in constructor of command
        mat->SetSpecularMap(specularMap);
        CGL::PushCommand(new UpdateMaterialUniformCommand(
            mat, *mat->GetUniform(Material::SPECULAR_MAP_UNAME)));
    }

    // Apply material properties
    // Ref: https://assimp-docs.readthedocs.io/en/latest/usage/use_the_lib.html#material-system

    // name
    aiString name;
    if (assMat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
        CGL::PushCommand(new UpdateNameCommand(mat, name.C_Str()));
    }

    // ambient color
    // TODO: should be scaled by ambient light
    // currently phong shader does not support ambient color
    // and instead multiples ambient light color by diffuse color
    // aiColor3D ambientColor;
    // if (assMat->Get(AI_MATKEY_COLOR_AMBIENT, ambientColor) == AI_SUCCESS) {
    //     mat->SetColor(ambientColor.r, ambientColor.g, ambientColor.b);
    //     CGL::PushCommand(new UpdateMaterialUniformCommand(
    //         mat, *mat->GetUniform(Material::COLOR_UNAME)));
    // }

    // diffuse color

    // specular color
    aiColor3D specularColor;
    if (assMat->Get(AI_MATKEY_COLOR_SPECULAR, specularColor) == AI_SUCCESS) {
        // TODO change specularColor shader param to vec3
        mat->SetSpecularColor(specularColor.r, specularColor.g, specularColor.b);
        CGL::PushCommand(new UpdateMaterialUniformCommand(
            mat, *mat->GetUniform(Material::SPECULAR_COLOR_UNAME)));
    }

    // shininess exponent
    // TODO: something seems wrong with phong specular calculation
    // at higher values of shine, specular highlight flashes way too quickly,
    // seems too sensitive to direction
    float shine = 0.0f;
    if (assMat->Get(AI_MATKEY_SHININESS, shine) == AI_SUCCESS && shine > 0.0f) {
        mat->SetLogShininess(std::log2(shine));
        CGL::PushCommand(new UpdateMaterialUniformCommand(
            mat, *mat->GetUniform(Material::SHININESS_UNAME)));
    }

    // TODO: many others to support like emissiveColor, opacity, etc.

    return mat;
}




//-----------------------------------------------------------------------------
// create texture
//-----------------------------------------------------------------------------
CGL_Texture * AssLoader::CreateTexture(aiMaterial* assMat, aiTextureType type)
{
    CGL_Texture* firstTexture = NULL;
    for(unsigned int i = 0; i < assMat->GetTextureCount(type); i++)
    {
        // https://assimp-docs.readthedocs.io/en/latest/usage/use_the_lib.html#textures
        // some textures are embedded in the model, some are referenced by path
        // E.g. .glb files embed their textures, but .obj files reference them by path
        // see above link for documentation on how to handle this

        aiString path;
        assMat->GetTexture(type, i, &path);

        // check if already in map
        if(textures.find(path.C_Str()) != textures.end()) {
            if(i == 0) firstTexture = textures[path.C_Str()];
            continue;
        }

        // create new texture
        CGL_Texture* texture;
        
        // first check if this is an embedded texture
        const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(path.C_Str());
        if (embeddedTexture) {
            // data texture
            texture = new DataTexture2D;
        } else {
            // file texture
            texture = new FileTexture2D; 
        }
        
        // create
        CGL::CreateChuckObjFromTex(api, vm, texture, shred, false);

        // set data
        if (embeddedTexture) {
            DataTexture2D* dataTexture = dynamic_cast<DataTexture2D*>(texture);
            assert(dataTexture);
            // check if embedded texture is compressed
            if (embeddedTexture->mHeight == 0) {
                // decode
                int width, height, channels;
                int desiredChannels = 4;
                unsigned char* decoded = stbi_load_from_memory(
                    (unsigned char*)embeddedTexture->pcData,
                    embeddedTexture->mWidth,
                    &width,
                    &height,
                    &channels,
                    desiredChannels 
                );
                // not storing the decoded data on audio thread side
                dataTexture->SetDimensions(width, height);
                // // propagate to render thread
                CGL::PushCommand(new UpdateTextureDataCommand(
                    texture->GetID(),
                    decoded,
                    width,
                    height,
                    desiredChannels  // fix to rgba 4 channels
                ));
                // free
                stbi_image_free(decoded);
            } else {
                dataTexture->SetDimensions(
                    embeddedTexture->mWidth,
                    embeddedTexture->mHeight
                );
                // propagate to render thread
                CGL::PushCommand(new UpdateTextureDataCommand(
                    texture->GetID(),
                    // TODO add format support to CGL_Texture and set to ARGB8888 
                    (unsigned char*)embeddedTexture->pcData,
                    embeddedTexture->mWidth,
                    embeddedTexture->mHeight,
                    4
                ));
            }
        } else {  // not embedded, must be a normal filepath
            FileTexture2D* fileTexture = dynamic_cast<FileTexture2D*>(texture);
            assert(fileTexture);
            // set (TODO this should happen in the UpdateTexturePathCommand constructor)
            cerr << "[ChuGL::AssLoader] Loading Texture at PATH: " << fileTexture->GetFilePath() << endl;
            // propagate to render thread
            CGL::PushCommand(new UpdateTexturePathCommand(fileTexture, directory + "/" + path.C_Str()));
        }

        // put in cache
        textures[path.C_Str()] = texture;

        // only supporting one texture per type for now
        if(i == 0) firstTexture = texture;
    }

    return firstTexture;
}




//-----------------------------------------------------------------------------
// create mesh
//-----------------------------------------------------------------------------
Mesh* AssLoader::CreateMesh(t_CKUINT index)
{
    // check for out of bound
    if(index >= geometries.size())
    {
        cerr << "[chugl] AssLoader.load() encountered invalid mesh index: " << index << " out of " << geometries.size() << endl;
        return NULL;
    }

    // get assimp mesh
    aiMesh* assMesh = scene->mMeshes[index];

    // check if material specified
    Material* mat = NULL;
    if(assMesh->mMaterialIndex >= 0)
    {
        unsigned int matIndex = assMesh->mMaterialIndex;
        mat = materials[matIndex] ? materials[matIndex] : CreateMaterial(scene->mMaterials[matIndex]);

        if(!mat)
        {
            cerr << "[chugl] AssLoader.load() failed to create material: " << index << " out of " << materials.size() << endl;
        }
    }
    // if mat is still NULL
    if( !mat )
    {
        // create material
        mat = new PhongMaterial;
        // create corresponding chuck object, refcount=false since MeshSet() later will refcount
        CGL::CreateChuckObjFromMat(api, vm, mat, shred, false);
    }

    // get type
    Chuck_DL_Api::Type type = api->type->lookup(vm, "GMesh");
    // verify
    assert(type != NULL);
    // create object
    Chuck_DL_Api::Object meshObj = api->object->create(shred, type, false);

    // look up the geo, creating it from corresponding aiMesh if necessary
    CustomGeometry* geo = geometries[index] ? geometries[index] : CreateGeometry(assMesh);

    // create mesh
    Mesh* mesh = new Mesh;
    // create HACK: would not need this if pre-ctors are called as part of create() above!
    CGL::PushCommand(new CreateSceneGraphNodeCommand(mesh, &CGL::mainScene, meshObj, CGL::GetGGenDataOffset()));
    // set geo and mat into mesh
    CGL::MeshSet(mesh, geo, mat);

    return mesh;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
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


//-----------------------------------------------------------------------------
// helper function to import asset
//-----------------------------------------------------------------------------
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

    // set directory
    directory = path.substr(0, path.find_last_of('/'));

    // resize
    geometries.resize(scene->mNumMeshes, NULL);
    // resize
    materials.resize(scene->mNumMaterials, NULL);

    // process starting from root node
    SceneGraphObject * cgobj = ProcessNode( NULL, scene->mRootNode );
    // return
    return cgobj ? cgobj->m_ChuckObject : NULL;
}
