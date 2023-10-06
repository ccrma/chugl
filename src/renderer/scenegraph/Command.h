#pragma once

#include "Material.h"
#include "Geometry.h"
#include "Group.h"
#include "Scene.h"
#include "Mesh.h"
#include "Scene.h"
#include "Light.h"
#include "CGL_Texture.h"

struct Chuck_Object;


class SceneGraphCommand
{
public:
    // constructor called by chuck audio thread.
    // DO NOT store pointer references to audio-thread scenegraph

	virtual ~SceneGraphCommand() {}

    // execute called by renderer thread.
    // must NOT use any pointer references to audio-thread scenegraph
    // @param scene: the render-thread's scene graph
	virtual void execute(Scene* scene) = 0;
};

//==================== Window Manager Commands =======================//
class SetMouseModeCommand : public SceneGraphCommand
{
public:
    SetMouseModeCommand(int mode) : m_Mode(mode) {};
    virtual void execute(Scene* scene) override {
        // so hacky...but we do this to decouple from the renderer
        Scene::updateMouseMode = true;
        Scene::mouseMode = m_Mode;
    }
private:
    int m_Mode;
};

class SetWindowModeCommand : public SceneGraphCommand
{
public:
    SetWindowModeCommand(int mode, int width = 0, int height = 0) 
        : m_Mode(mode), m_Width(width), m_Height(height) {};
    virtual void execute(Scene* scene) override {
        Scene::updateWindowMode = true;
        Scene::windowMode = m_Mode;

        if (m_Width > 0 && m_Height > 0) {
            Scene::windowedWidth = m_Width;
            Scene::windowedHeight = m_Height;
        }
    }
private:
    int m_Mode;
    int m_Width, m_Height;
};

//==================== Creation Commands =====a==================//
// TODO: all this creation command logic can be moved into the classes themselves
// add a virtual Clone() = 0 to base class SceneGraphNode

// TODO: should use observer pattern here, create hooks for all
// creation commands that the renderer can listen to in order to
// setup GPU-side data 

class CreateMaterialCommand : public SceneGraphCommand
{
public:
    CreateMaterialCommand(Material* mat) : newMat(mat->Clone()) {
        assert(mat->GetMaterialType() != MaterialType::Base);  // must be a concrete material
    };
    virtual void execute(Scene* scene) override {
        std::cout << "copied material with id: " + std::to_string(newMat->GetID()) 
                  << std::endl;

        scene->RegisterNode(newMat);
    }
private:
    Material* newMat;
};

// create geometry
class CreateGeometryCommand : public SceneGraphCommand
{
public:
    CreateGeometryCommand(Geometry* g) : geo(g->Clone()) {};
    virtual void execute(Scene* scene) override {
        // std::cout << "copied geometry with id: " + std::to_string(geo->GetID())
        //     << std::endl;
        scene->RegisterNode(geo);
    }

private:
    Geometry* geo;
};

class CreateTextureCommand : public SceneGraphCommand
{
public:
    CreateTextureCommand(CGL_Texture* tex) : texture(nullptr) {
        texture = tex->Clone();  // for when chuck eventually has constructors
        std::cout << "created texture with id: " + std::to_string(tex->GetID()) << std::endl;
    }

    virtual void execute(Scene* scene) override {
        scene->RegisterNode(texture);
        // TODO register to texture resource list

        // create the texture on the gpu
        /*
        TODO: 
        - should be registering texture in rendererState,
        because a texture could belong to multiple scenes

        - BUT, command right now is decoupled from the renderer, if we pass
        RendererState as a param to Creation Commands, that couples
            - is that ok?
            - way to remain decoupled: allow passing in a callback function to the Command class somehow...
        
        Also need to the memory for passing data buffers from client code
        */
    }
private:
    CGL_Texture* texture;
};


class CreateLightCommand : public SceneGraphCommand
{
public:
    CreateLightCommand(
        Light* l, Scene* audioThreadScene, Chuck_Object* ckobj
        // TODO add data offset here too?
    ) : light(l->Clone()) {
        audioThreadScene->RegisterLight(l);
        l->m_ChuckObject = ckobj;

        light->SetID(l->GetID());
    };
    virtual void execute(Scene* renderThreadScene) override {
        std::cout << "copied light with id: " + std::to_string(light->GetID())
            << std::endl;

        renderThreadScene->RegisterLight(light);
    }
private:
    Light* light;  // DON"T DELETE passed to renderer scenegraph
};

// create Group
class CreateGroupCommand : public SceneGraphCommand
{
public:
    CreateGroupCommand(Group* group) : group(group) {};
    virtual void execute(Scene* scene) override {  // TODO: just add virtual clone() interface to scenegraph node
        Group* newGroup = new Group();
        newGroup->SetID(group->GetID());  // copy ID
        std::cout << "copied group with id: " + std::to_string(newGroup->GetID())
            << std::endl;

        scene->RegisterNode(newGroup);
    }
private:
    Group* group;
};

// create base GGen (semantically same as empty group)
class CreateSceneGraphObjectCommand : public SceneGraphCommand
{
public:
    CreateSceneGraphObjectCommand(SceneGraphObject* obj) : m_ID(obj->GetID()) {}
    virtual void execute(Scene* scene) override {
        SceneGraphObject* obj = new SceneGraphObject;
        obj->SetID(m_ID);  // copy ID
        scene->RegisterNode(obj);
    }
private:
    size_t m_ID;
};

// create Mesh
class CreateMeshCommand : public SceneGraphCommand
{
public:
    CreateMeshCommand(Mesh* mesh) : mesh(mesh) {
        // TODO: clone this instead. follow light model. 
        std::cerr << "creating mesh with id: " << mesh->GetID() << std::endl;

    };
    virtual void execute(Scene* scene) override {
        // Get the cloned material and geometry
        Material* clonedMat = nullptr;
        Geometry* clonedGeo = nullptr;

        if (mesh->GetMaterial())
            clonedMat = dynamic_cast<Material*>(scene->GetNode(mesh->GetMaterial()->GetID()));
        if (mesh->GetGeometry())
            clonedGeo = dynamic_cast<Geometry*>(scene->GetNode(mesh->GetGeometry()->GetID()));

        Mesh* newMesh = new Mesh(clonedGeo, clonedMat);
        newMesh->SetID(mesh->GetID());  // copy ID
        std::cout << "copied mesh with id: " + std::to_string(newMesh->GetID())
            << std::endl;

        scene->RegisterNode(newMesh);
    }

private:
    Mesh* mesh;
};

// create Camera
class CreateCameraCommand : public SceneGraphCommand
{
public:
    CreateCameraCommand(
        Camera* camera, Scene* audioThreadScene, Chuck_Object* ckobj,
        t_CKUINT data_offset
    );

    virtual void execute(Scene* renderThreadScene) override {
        // dud for now until muliple cameras are supported
        // renderThreadScene->RegisterCamera(m_Camera);
    }
private:
    Camera* m_Camera;
};

// Create Scene
class CreateSceneCommand : public SceneGraphCommand
{
public:
    CreateSceneCommand(Scene* scene) : m_Scene(scene) {};

    // TODO: this is weird, we have to pass in a scene pointer to create a scene...
    // basically the only point is to copy the ID and register itself in its node map
    virtual void execute(Scene* scene) override {
        scene->SetID(m_Scene->GetID());  // copy ID
        std::cout << "copied scene with id: " + std::to_string(m_Scene->GetID())
            << std::endl;

        scene->RegisterNode(scene);  // register itself so it shows up in node lookups
    }
private:
    Scene* m_Scene;
};

//==================== SceneGraph Relationship Commands =======================//

// add child
class RelationshipCommand : public SceneGraphCommand
{
public:
    enum Relation : unsigned int {
        AddChild = 0,
        RemoveChild
    };

    RelationshipCommand(SceneGraphObject* parent, SceneGraphObject* child, Relation r) :
        m_ParentID(parent->GetID()), m_ChildID(child->GetID()), rel(r) {
            CreateRelation(parent, child, r);
    };

    virtual void execute(Scene* scene) override {
        SceneGraphObject* parent = dynamic_cast<SceneGraphObject*>(scene->GetNode(m_ParentID));
        SceneGraphObject* child = dynamic_cast<SceneGraphObject*>(scene->GetNode(m_ChildID));

        if (!parent || !child) __debugbreak();

        assert(parent && child);

        CreateRelation(parent, child, rel);
    }
private:
    Relation rel;
    size_t m_ParentID, m_ChildID;

    void CreateRelation(SceneGraphObject* parent, SceneGraphObject* child, Relation r) {
        switch (r) {
        case AddChild:
            parent->AddChild(child);
            break;
        case RemoveChild:
            parent->RemoveChild(child);
            break;
        }
    }
};


//==================== Parameter Modification Commands =======================//

// set transform
class TransformCommand : public SceneGraphCommand
{
public:
    TransformCommand(SceneGraphObject* obj) :
        m_ID(obj->GetID()), m_Position(obj->GetPosition()), m_Rotation(obj->GetRotation()), m_Scale(obj->GetScale())
    {}
    virtual void execute(Scene* scene) override {
        SceneGraphObject* obj = dynamic_cast<SceneGraphObject*>(scene->GetNode(m_ID));
        assert(obj);
        obj->SetPosition(m_Position);
        obj->SetRotation(m_Rotation);
        obj->SetScale(m_Scale);
    }

private:
    size_t m_ID; // which scenegraph object to modify

    // transform  (making public for now for easier debug)
    glm::vec3 m_Position;
    glm::quat m_Rotation;
    glm::vec3 m_Scale;
};

// Set Mesh params (geometry and material)
class SetMeshCommand : public SceneGraphCommand
{
public:
    SetMeshCommand(Mesh* mesh) :
        m_MeshID(mesh->GetID()), 
        m_MatID(mesh->GetMaterial() ? mesh->GetMaterial()->GetID() : 0), 
        m_GeoID(mesh->GetGeometry()->GetID())
    {}

    virtual void execute(Scene* scene) override {
        Mesh* mesh = dynamic_cast<Mesh*>(scene->GetNode(m_MeshID));
        Material* mat = dynamic_cast<Material*>(scene->GetNode(m_MatID));
        Geometry* geo = dynamic_cast<Geometry*>(scene->GetNode(m_GeoID));

        assert(mesh && geo);  // don't need to assert mat because we have default material

        mesh->SetGeometry(geo);
        mesh->SetMaterial(mat);
    }


private:  
    // store IDs, not pointers, because we want to point to the renderer's copy
    // not the original.
    // Also not safe to read from the original during command queue flush
    // because it may be being written to by chuck side
    // TODO: need to do this for all the other commands .... :(
    size_t m_MeshID, m_MatID, m_GeoID;
};


/*
This command is an example pattern for how to update arbitrary
data in any SceneGraphNode

a material needs to define
1. GenUpdate()  -- allocates the data for the copy
2. ApplyUpdate() -- reads in the data and applies the changes
3. FreeUpdate() -- frees the allocated memory

This is necessary to prevent accessing the original material while executing
the command, as we are outside the "critical region" and no-longer have gauranteed
thread-safe read/write access
*/
class UpdateMaterialCommand : public SceneGraphCommand
{
public:
    UpdateMaterialCommand(Material* mat) 
        : m_MatData(mat->GenUpdate()), m_MatID(mat->GetID()) {
        assert(mat->GetMaterialType() != MaterialType::Base);
    };
    ~UpdateMaterialCommand() { m_Mat->FreeUpdate(m_MatData); }
    virtual void execute(Scene* scene) override {
        m_Mat = dynamic_cast<Material*>(scene->GetNode(m_MatID));
        assert(m_Mat);  
        m_Mat->ApplyUpdate(m_MatData);
    }
private:
    void* m_MatData;
    size_t m_MatID;
    Material* m_Mat;
};

// only updates a single uniform, rather than bulk
class UpdateMaterialUniformCommand : public SceneGraphCommand
{
public:
    UpdateMaterialUniformCommand(Material* mat, MaterialUniform uniform) 
        : m_MatID(mat->GetID()), m_Uniform(uniform) {};

    virtual void execute(Scene* scene) override {
        Material* m_Mat = dynamic_cast<Material*>(scene->GetNode(m_MatID));
        assert(m_Mat);  
        m_Mat->SetUniform(m_Uniform);
    }
private:
    size_t m_MatID;
    MaterialUniform m_Uniform;
};

// update single material option
class UpdateMaterialOptionCommand: public SceneGraphCommand
{
public:
    UpdateMaterialOptionCommand(Material* mat, MaterialOption option) 
        : m_MatID(mat->GetID()), m_Option(option)
    {};
    virtual void execute(Scene* scene) override {
        Material* mat = dynamic_cast<Material*>(scene->GetNode(m_MatID));
        assert(mat);  

        mat->SetOption(m_Option);
    }
private:
    size_t m_MatID;
    MaterialOption m_Option;
};

class UpdateMaterialShadersCommand : public SceneGraphCommand
{
public:
    UpdateMaterialShadersCommand(ShaderMaterial* mat) 
        : m_MatID(mat->GetID()),
        m_VertexShaderPath(mat->m_VertShaderPath),
        m_FragmentShaderPath(mat->m_FragShaderPath)
    {};

    virtual void execute(Scene* scene) override {
        ShaderMaterial* mat = dynamic_cast<ShaderMaterial*>(scene->GetNode(m_MatID));
        assert(mat);  

        mat->m_VertShaderPath = m_VertexShaderPath;
        mat->m_FragShaderPath = m_FragmentShaderPath;
    }

private:
    size_t m_MatID;
    std::string m_VertexShaderPath, m_FragmentShaderPath;
};

class UpdateGeometryCommand : public SceneGraphCommand
{
public:
    UpdateGeometryCommand(Geometry* geo)
        : m_Geo(geo), m_GeoData(geo->GenUpdate()), m_GeoID(geo->GetID()) {};
    ~UpdateGeometryCommand() {
        if (m_GeoData) {
            m_Geo->FreeUpdate(m_GeoData);
        }
    }
    virtual void execute(Scene* scene) override {
        Geometry* geo = dynamic_cast<Geometry*>(scene->GetNode(m_GeoID));
        assert(geo);
        geo->ApplyUpdate(m_GeoData);

        std::cout << "updated geometry with id: " + std::to_string(geo->GetID())
            << std::endl;
    }
private:
    Geometry* m_Geo;
    void* m_GeoData;
    size_t m_GeoID;
};

class UpdateGeometryAttributeCommand : public SceneGraphCommand
{
public:
    UpdateGeometryAttributeCommand(
        Geometry* geo,
        std::string attribName,
        unsigned int location,
        unsigned int numComponents,
        std::vector<double>& attribData,
        bool normalize = false
    ) : m_GeoID(geo->GetID()), m_Attrib() {
        // copy data into m_attrib
        m_Attrib.name = attribName;
        m_Attrib.location = location;
        m_Attrib.numComponents = numComponents;
        m_Attrib.normalize = normalize;
        m_Attrib.data.reserve(attribData.size());
        for (auto& val : attribData) {
            m_Attrib.data.emplace_back(static_cast<float>(val));
        }
    };

    virtual void execute(Scene* scene) override {
        Geometry* geo = dynamic_cast<Geometry*>(scene->GetNode(m_GeoID));
        assert(geo);
        geo->AddAttribute(m_Attrib);
    }

private:
    size_t m_GeoID;
    CGL_GeoAttribute m_Attrib;
};

class UpdateGeometryIndicesCommand : public SceneGraphCommand
{
public:
    UpdateGeometryIndicesCommand(
        Geometry* geo,
        std::vector<t_CKUINT>& indices
    ) : m_GeoID(geo->GetID()) {
        m_Indices.reserve(indices.size());
        for (auto& val : indices) {
            m_Indices.emplace_back(val);
        }
    };

    virtual void execute(Scene* scene) override {
        Geometry* geo = dynamic_cast<Geometry*>(scene->GetNode(m_GeoID));
        assert(geo);
        geo->SetIndices(m_Indices);
    }

private:
    size_t m_GeoID;
    std::vector<unsigned int> m_Indices;
};

class UpdateTextureSamplerCommand : public SceneGraphCommand
{
public:
    UpdateTextureSamplerCommand(CGL_Texture * tex) : texID(tex->GetID()) {
        samplerParams = tex->m_SamplerParams;
    };

    virtual void execute(Scene* scene) override {
        CGL_Texture* tex = dynamic_cast<CGL_Texture*>(scene->GetNode(texID));
        assert(tex);

        // copy sampler params
        tex->m_SamplerParams = samplerParams;
        tex->SetNewSampler();  // set flag to let renderer know it needs to update texture sampling params on GPU
    }

private:
    size_t texID;
    CGL_TextureSamplerParams samplerParams;
};


class UpdateTexturePathCommand : public SceneGraphCommand
{
public:
    UpdateTexturePathCommand(CGL_Texture * tex) : texID(tex->GetID()) {
        filePath = tex->m_FilePath;
    };

    virtual void execute(Scene* scene) override {
        CGL_Texture* tex = dynamic_cast<CGL_Texture*>(scene->GetNode(texID));
        assert(tex);

        // if no change, do nothing
        if (tex->m_FilePath == filePath) return;
        std::cout << " changing path to " << filePath << std::endl;

        tex->m_FilePath = filePath;
        tex->SetNewFilePath();
        tex->SetNewSampler();  // need to reset sampler after regerating texture
    }

private:
    size_t texID;
    std::string filePath;
};

class UpdateTextureDataCommand : public SceneGraphCommand
{
public:
    UpdateTextureDataCommand(size_t id, std::vector<double>& ck_array, int w, int h) 
    : texID(id), width(w), height(h) {
        // copy tex params
        dataBuffer.reserve(ck_array.size());
        for (auto& val : ck_array) {
            dataBuffer.emplace_back(static_cast<unsigned char>(val));
        }
    };

    virtual void execute(Scene* scene) override {
        CGL_Texture* tex = dynamic_cast<CGL_Texture*>(scene->GetNode(texID));
        assert(tex);

        // first check if dimensions changed and we need to regen
        if (tex->m_Width != width || tex->m_Height != height) {
            tex->m_Width = width;
            tex->m_Height = height;
            tex->SetNewDimensions();
            tex->SetNewSampler();  // need to reset sampler after regerating texture
        }

        // move vector into texture
        tex->m_DataBuffer = std::move(dataBuffer);
        tex->SetNewRawData();
    }

private:
    size_t texID;
    std::vector<unsigned char> dataBuffer;
    int width, height;
};

class UpdateCameraCommand : public SceneGraphCommand
{
public:
    UpdateCameraCommand(Camera* cam) : 
        m_CamID(cam->GetID()), params(cam->params)
    {};
    virtual void execute(Scene* scene) override {
        Camera* cam = dynamic_cast<Camera*>(scene->GetNode(m_CamID));
        assert(cam);
        cam->params = params;
    }
private:
    size_t m_CamID;
    CameraParams params;
};

// TODO refactor scene to have uniform cache map like base Material class
class UpdateSceneFogCommand : public SceneGraphCommand
{
public:
    UpdateSceneFogCommand(Scene* scene) : fog(scene->m_FogUniforms)
    {};

    virtual void execute(Scene* scene) override {
        scene->m_FogUniforms = fog;
    }
private:
    FogUniforms fog;
};

class UpdateSceneBackgroundColorCommand : public SceneGraphCommand
{
public:
    UpdateSceneBackgroundColorCommand(Scene* scene) : color(scene->GetBackgroundColor())
    {};

    virtual void execute(Scene* scene) override {
        scene->SetBackgroundColor(color.x, color.y, color.z);
    }
private:
    glm::vec3 color;
};