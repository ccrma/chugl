#pragma once

#include "Material.h"
#include "Geometry.h"
#include "Group.h"
#include "Scene.h"
#include "Mesh.h"
#include "Scene.h"
#include "Light.h"
#include "CGL_Texture.h"



class SceneGraphCommand
{
public:
	virtual ~SceneGraphCommand() {}
	virtual void execute(Scene* scene) = 0;
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
    CreateMaterialCommand(Material* mat) : mat(mat) {
        assert(mat->GetMaterialType() != MaterialType::Base);  // must be a concrete material
    };
    virtual void execute(Scene* scene) override {
        Material* newMat = mat->Clone();
        std::cout << "copied material with id: " + std::to_string(newMat->GetID()) 
                  << std::endl;

        scene->RegisterNode(newMat);
        // TODO: also register materials
    }
private:
    Material* mat;
};

// create geometry
class CreateGeometryCommand : public SceneGraphCommand
{
public:
    CreateGeometryCommand(Geometry* geo) : geo(geo) {};
    virtual void execute(Scene* scene) override {
        Geometry* newGeo = geo->Clone();

        // assign the gpu buffer data
        // JK we do this in renderer instead
        // newGeo->BuildGeometry();

        newGeo->SetID(geo->GetID());  // copy ID
        std::cout << "copied geometry with id: " + std::to_string(newGeo->GetID())
            << std::endl;

        scene->RegisterNode(newGeo);
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
    CreateLightCommand(Light* l) : light(nullptr) {
        light = l->Clone();
        light->SetID(l->GetID());
    };
    virtual void execute(Scene* scene) override {
        std::cout << "copied light with id: " + std::to_string(light->GetID())
            << std::endl;

        scene->RegisterNode(light);
        scene->RegisterLight(light);
    }
private:
    Light* light;
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

// create Mesh
class CreateMeshCommand : public SceneGraphCommand
{
public:
    CreateMeshCommand(Mesh* mesh) : mesh(mesh) {

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
    CreateCameraCommand(Camera* camera) : m_Camera(camera) {};
    virtual void execute(Scene* scene) override {
        Camera* newCamera = m_Camera->Clone();
        newCamera->SetID(m_Camera->GetID());  // copy ID
        std::cout << "copied camera with id: " + std::to_string(newCamera->GetID())
            << std::endl;

        scene->RegisterNode(newCamera);
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
class AddChildCommand : public SceneGraphCommand
{
public:
    AddChildCommand(SceneGraphObject* parent, SceneGraphObject* child) :
        m_ParentID(parent->GetID()), m_ChildID(child->GetID()) {};
    virtual void execute(Scene* scene) override {
        SceneGraphObject* parent = dynamic_cast<SceneGraphObject*>(scene->GetNode(m_ParentID));
        SceneGraphObject* child = dynamic_cast<SceneGraphObject*>(scene->GetNode(m_ChildID));

        assert(parent && child);
        parent->AddChild(child);
    }
private:
    size_t m_ParentID, m_ChildID;
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
TODO: let this command be the example pattern for how to update any SceneGraphNode

a material needs to define
1. GenUpdate()  -- allocates the data for the copy
2. ApplyUpdate() -- reads in the data and applies the changes

This is necessary to prevent accessing the original material while executing
the command, as we are outside the "critical region" and no-longer have gauranteed
thread-safe read/write access
*/
class UpdateMaterialUniformsCommand : public SceneGraphCommand
{
public:
    UpdateMaterialUniformsCommand(Material* mat) 
        : m_MatData(mat->GenUpdate()), m_MatID(mat->GetID()) {
        assert(mat->GetMaterialType() != MaterialType::Base);
    };
    ~UpdateMaterialUniformsCommand() {
        if (m_MatData) delete m_MatData;
    }
    virtual void execute(Scene* scene) override {
        Material* m_Mat = dynamic_cast<Material*>(scene->GetNode(m_MatID));
        assert(m_Mat);  
        m_Mat->ApplyUpdate(m_MatData);
    }
private:
    std::vector<MaterialUniform>* m_MatData;
    size_t m_MatID;
};

// this probably should go under UpdateMaterialCommand but don't have
// time right now to figure out the inheritence stuff
class UpdateWireframeCommand : public SceneGraphCommand
{
public:
    UpdateWireframeCommand(Material* mat) 
        : m_MatID(mat->GetID()),
        m_Wireframe(mat->GetWireFrame()),
        m_WireframeLineWidth(mat->GetWireFrameWidth())
    {};
    virtual void execute(Scene* scene) override {
        Material* mat = dynamic_cast<Material*>(scene->GetNode(m_MatID));
        assert(mat);  

        mat->SetWireFrame(m_Wireframe);
        mat->SetWireFrameWidth(m_WireframeLineWidth);

        std::cout << "updated material wireframe with id: " + std::to_string(mat->GetID()) 
                  << std::endl;
    }
private:
    size_t m_MatID;
    bool m_Wireframe;
    float m_WireframeLineWidth;
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
        tex->m_NewSampler = true;  // set flag to let renderer know it needs to update texture sampling params on GPU
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

        tex->m_FilePath= filePath;
        tex->m_NewFilePath = true;
    }


private:
    size_t texID;
    std::string filePath;
};


