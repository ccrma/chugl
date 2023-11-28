#pragma once

#include "Material.h"
#include "Geometry.h"
#include "Scene.h"
#include "Mesh.h"
#include "Scene.h"
#include "Light.h"
#include "CGL_Texture.h"
#include "Camera.h"
#include "chugl_postprocess.h"


// Forward Declarations =========================================
struct Chuck_Object;


//==================== SceneGraph Commands =======================//

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
    SetMouseModeCommand(Scene* audioThreadScene, int mode) : m_Mode(mode) {
        audioThreadScene->m_MouseMode = m_Mode;
    };
    virtual void execute(Scene* renderThreadScene) override {
        renderThreadScene->m_UpdateMouseMode = true;
        renderThreadScene->m_MouseMode = m_Mode;
    }
private:
    int m_Mode;
};

class SetWindowModeCommand : public SceneGraphCommand
{
public:
    SetWindowModeCommand(Scene* audioThreadScene, int mode, int width = 0, int height = 0) 
        : m_Mode(mode), m_Width(width), m_Height(height) {
        audioThreadScene->m_WindowMode = m_Mode;

        // TODO make fullscreen a separate command
        if (width > 0 && height > 0) {
            audioThreadScene->m_WindowedWidth = m_Width;
            audioThreadScene->m_WindowedHeight = m_Height;
        }
    };

    virtual void execute(Scene* renderThreadScene) override {
        renderThreadScene->m_UpdateWindowMode = true;
        renderThreadScene->m_WindowMode = m_Mode;

        if (m_Width > 0 && m_Height > 0) {
            renderThreadScene->m_WindowedWidth = m_Width;
            renderThreadScene->m_WindowedHeight = m_Height;
        }
    }
private:
    int m_Mode;
    int m_Width, m_Height;
};


class CloseWindowCommand : public SceneGraphCommand
{
public:
    CloseWindowCommand(Scene* audioThreadScene) {
        audioThreadScene->m_WindowShouldClose = true;
    };
    virtual void execute(Scene* scene) override {
        scene->m_WindowShouldClose = true;
    }
};

class SetWindowTitleCommand : public SceneGraphCommand
{
public:
    SetWindowTitleCommand(Scene* audioThreadScene, std::string title) : m_Title(title) {
        audioThreadScene->m_UpdateWindowTitle = true;
        audioThreadScene->m_WindowTitle = m_Title;
    };
    virtual void execute(Scene* renderThreadScene) override {
        renderThreadScene->m_UpdateWindowTitle = true;
        renderThreadScene->m_WindowTitle = m_Title;
    }
private:
    std::string m_Title;
};

//==================== Creation Commands =====a==================//
// TODO: should use observer pattern here, create hooks for all
// creation commands that the renderer can listen to in order to
// setup GPU-side data 

class CreateSceneGraphNodeCommand : public SceneGraphCommand
{
public:
    CreateSceneGraphNodeCommand(
        SceneGraphNode* node,
        Scene* audioThreadScene,
        Chuck_Object* ckobj,
        t_CKUINT data_offset,
        CK_DL_API API
    );
    virtual void execute(Scene* scene) override;
private:
    SceneGraphNode* m_Clone;
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

class DisconnectCommand : public SceneGraphCommand
{
public: 
    DisconnectCommand(SceneGraphObject* obj) : m_ID(obj->GetID()) {
        obj->Disconnect();
    };
    virtual void execute(Scene* scene) override {
        SceneGraphObject* obj = dynamic_cast<SceneGraphObject*>(scene->GetNode(m_ID));
        // obj will be NULL in the case that a GGen is GC'd on the audio-thread before
        // GG.nextFrame() is ever called
        if (obj) obj->Disconnect();
    }
private:
    size_t m_ID;
};


//==================== Parameter Modification Commands =======================//
class UpdateNameCommand : public SceneGraphCommand
{
public:
    UpdateNameCommand(SceneGraphNode* node, const std::string& name) :
        m_ID(node->GetID()), m_Name(name)
    {
        assert(name.size() > 0);
        node->SetName(m_Name);
    }

    virtual void execute(Scene* scene) override {
        SceneGraphNode* node = scene->GetNode(m_ID);
        assert(node);
        node->SetName(m_Name);
    }

private:
    size_t m_ID;
    std::string m_Name;
};

class UpdatePositionCommand : public SceneGraphCommand
{
public:
    UpdatePositionCommand(SceneGraphObject* obj) :
        m_ID(obj->GetID()), m_Position(obj->GetPosition())
    {}
    virtual void execute(Scene* scene) override {
        SceneGraphObject* obj = dynamic_cast<SceneGraphObject*>(scene->GetNode(m_ID));
        assert(obj);
        obj->SetPosition(m_Position);
    }

private:
    size_t m_ID;
    glm::vec3 m_Position;
};

class UpdateRotationCommand : public SceneGraphCommand
{
public:
    UpdateRotationCommand(SceneGraphObject* obj) :
        m_ID(obj->GetID()), m_Rotation(obj->GetRotation())
    {}
    virtual void execute(Scene* scene) override {
        SceneGraphObject* obj = dynamic_cast<SceneGraphObject*>(scene->GetNode(m_ID));
        assert(obj);
        obj->SetRotation(m_Rotation);
    }
private:
    size_t m_ID;
    glm::quat m_Rotation;
};

class UpdateScaleCommand : public SceneGraphCommand
{
public:
    UpdateScaleCommand(SceneGraphObject* obj) :
        m_ID(obj->GetID()), m_Scale(obj->GetScale())
    {}
    virtual void execute(Scene* scene) override {
        SceneGraphObject* obj = dynamic_cast<SceneGraphObject*>(scene->GetNode(m_ID));
        assert(obj);
        obj->SetScale(m_Scale);
    }
private:
    size_t m_ID;
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
        : m_MatData(mat->GenUpdate()), m_MatID(mat->GetID()),
        m_Mat(nullptr)
    {
        assert(mat->GetMaterialType() != MaterialType::Base);
    };
    ~UpdateMaterialCommand() { if (m_Mat) m_Mat->FreeUpdate(m_MatData); }
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
        m_VertexShader(mat->GetVertShader()),
        m_FragmentShader(mat->GetFragShader()),
        m_VertIsPath(mat->GetVertIsPath()),
        m_FragIsPath(mat->GetFragIsPath())
    {

    };

    virtual void execute(Scene* scene) override {
        ShaderMaterial* mat = dynamic_cast<ShaderMaterial*>(scene->GetNode(m_MatID));
        assert(mat);  

        mat->SetVertShader(m_VertexShader, m_VertIsPath);
        mat->SetFragShader(m_FragmentShader, m_FragIsPath);
    }

private:
    size_t m_MatID;
    std::string m_VertexShader, m_FragmentShader;
    bool m_VertIsPath, m_FragIsPath;
};

class UpdateGeometryCommand : public SceneGraphCommand
{
public:
    UpdateGeometryCommand(Geometry* geo)
        : m_Geo(nullptr), m_GeoData(geo->GenUpdate()), m_GeoID(geo->GetID()) {};
    ~UpdateGeometryCommand() {
        if (m_GeoData && m_Geo) {
            m_Geo->FreeUpdate(m_GeoData);
        }
    }
    virtual void execute(Scene* scene) override {
        m_Geo = dynamic_cast<Geometry*>(scene->GetNode(m_GeoID));
        assert(m_Geo);
        m_Geo->ApplyUpdate(m_GeoData);
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
        std::vector<t_CKFLOAT>& attribData,
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
    UpdateTextureSamplerCommand(CGL_Texture * tex) : m_TexID(tex->GetID()) {
        m_SamplerParams = tex->GetSamplerParams();
    };

    virtual void execute(Scene* scene) override {
        CGL_Texture* tex = dynamic_cast<CGL_Texture*>(scene->GetNode(m_TexID));
        assert(tex);

        // copy sampler params
        tex->SetSamplerParams(m_SamplerParams);
        tex->SetNewSampler();  // set flag to let renderer know it needs to update texture sampling params on GPU
    }

private:
    size_t m_TexID;
    CGL_TextureSamplerParams m_SamplerParams;
};

class UpdateTextureColorSpaceCommand : public SceneGraphCommand
{
public:
    UpdateTextureColorSpaceCommand(CGL_Texture * tex, CGL_TextureColorSpace colorspace) 
    : m_TexID(tex->GetID()), m_ColorSpace(colorspace) {
        tex->SetColorSpace(colorspace);
    };

    virtual void execute(Scene* scene) override {
        CGL_Texture* tex = dynamic_cast<CGL_Texture*>(scene->GetNode(m_TexID));
        assert(tex);

        // copy sampler params
        tex->SetColorSpace(m_ColorSpace);
        tex->SetNewColorSpace();  // set flag to let renderer know it needs to update texture sampling params on GPU
        tex->SetNewSampler();  // changing color space requires regenerating texture, need to reset sampler
    }
private:
    size_t m_TexID;
    CGL_TextureColorSpace m_ColorSpace;
};

class UpdateTexturePathCommand : public SceneGraphCommand
{
public:
    UpdateTexturePathCommand(FileTexture2D* tex, const std::string& path) 
        : m_TexID(tex->GetID()), m_Path(path) 
    {
            tex->SetFilePath(path);
    };

    virtual void execute(Scene* scene) override {
        FileTexture2D* tex = dynamic_cast<FileTexture2D*>(scene->GetNode(m_TexID));
        assert(tex);

        tex->SetFilePath(m_Path);
        tex->SetNewFilePath();
        tex->SetNewSampler();  // need to reset sampler after regerating texture
    }

private:
    size_t m_TexID;
    std::string m_Path;
};

class UpdateCubeMapPathsCommand : public SceneGraphCommand
{
public:
    UpdateCubeMapPathsCommand(
        CGL_CubeMap * tex, const std::vector<std::string>& paths
    ) : texID(tex->GetID()), m_Paths(paths)
    {   
        tex->SetFilePaths(paths);
    }

    virtual void execute(Scene* renderThreadScene) override {
        CGL_CubeMap* tex = dynamic_cast<CGL_CubeMap*>(renderThreadScene->GetNode(texID));
        assert(tex);

        tex->SetFilePaths(m_Paths);
        tex->SetNewFilePath();
        tex->SetNewSampler();  // need to reset sampler after regerating texture
    }

private:
    size_t texID;
    std::vector<std::string> m_Paths;
};

class UpdateTextureDataCommand : public SceneGraphCommand
{
public:
    UpdateTextureDataCommand(DataTexture2D* tex, std::vector<t_CKFLOAT>& ck_array, int w, int h) 
    : m_TexID(tex->GetID()), width(w), height(h) {
        // copy tex params
        m_DataBuffer.reserve(ck_array.size());
        for (auto& val : ck_array) {
            m_DataBuffer.emplace_back(static_cast<unsigned char>(val));
        }

        // update dimensions
        // we are NOT setting raw data here to minimize blocking on audio thread
        // the array should already be accessible anyways from the chuck script
        tex->SetRawData(m_DataBuffer, width, height);
    };

    UpdateTextureDataCommand(size_t id, unsigned char* data, int w, int h, int numChannels)
    : m_TexID(id), width(w), height(h) {
        // copy tex params
        m_DataBuffer.reserve(w * h * numChannels);
        for (int i = 0; i < w * h * numChannels; i++) {
            m_DataBuffer.emplace_back(data[i]);
        }
    }

    virtual void execute(Scene* scene) override {
        DataTexture2D* tex = dynamic_cast<DataTexture2D*>(scene->GetNode(m_TexID));
        assert(tex);

        // move vector into texture
        if (tex->SetRawData(m_DataBuffer, width, height, false, true)) {
            // dimensions have changed and we need to regen
            tex->SetNewDimensions();
            tex->SetNewSampler();  // need to reset sampler after regerating texture

        }
        tex->SetNewRawData();
    }

private:
    size_t m_TexID;
    std::vector<unsigned char> m_DataBuffer;
    int width, height;
};

class UpdateCameraCommand : public SceneGraphCommand
{
public:
    UpdateCameraCommand(Camera* cam);
    virtual void execute(Scene* scene) override;
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

class UpdateSceneSkyboxCommand : public SceneGraphCommand
{
public:
    UpdateSceneSkyboxCommand(
        Scene* audioThreadScene, 
        CGL_CubeMap* cubeMap
        // const SkyboxPaths& paths
    ) : m_CubeMapID(cubeMap->GetID()) {
        // audioThreadScene->UpdateSkybox(m_Paths);
        audioThreadScene->SetSkybox(cubeMap);
        audioThreadScene->SetSkyboxEnabled(true);
    };

    virtual void execute(Scene* renderThreadScene) override {
        CGL_CubeMap* cubeMap = dynamic_cast<CGL_CubeMap*>(renderThreadScene->GetNode(m_CubeMapID));

        assert(cubeMap);

        renderThreadScene->SetSkybox(cubeMap);
        renderThreadScene->SetSkyboxEnabled(true);
        // renderThreadScene->UpdateSkybox(m_Paths);
    }

private:
    // SkyboxPaths m_Paths;
    size_t m_CubeMapID;
};

class UpdateSceneSkyboxEnabledCommand : public SceneGraphCommand
{
public:
    UpdateSceneSkyboxEnabledCommand(Scene* audioThreadScene, bool enabled) : m_Enabled(enabled) {
        audioThreadScene->SetSkyboxEnabled(m_Enabled);
    };

    virtual void execute(Scene* renderThreadScene) override {
        renderThreadScene->SetSkyboxEnabled(m_Enabled);
    }
private: 
    bool m_Enabled;
};


// TODO: just copying the entire light for now, eventually want to just update individual
// params, like how material and geo does it
class UpdateLightCommand : public SceneGraphCommand
{
public:
    UpdateLightCommand(Light* light) : 
        params(light->m_Params), m_LightID(light->GetID())
    {};
    
    virtual void execute(Scene* scene) override {
        Light* light = dynamic_cast<Light*>(scene->GetNode(m_LightID));
        assert(light);
        light->m_Params = params;
    }

private:
    size_t m_LightID;
    LightParams params;
};

//========================= Scenegraph Node Deletion Commands =========================//

class DestroySceneGraphNodeCommand : public SceneGraphCommand
{
public:
    DestroySceneGraphNodeCommand(
        Chuck_Object* ckobj,
        t_CKUINT data_offset,
        CK_DL_API API,
        Scene* audioThreadScene
    );

    virtual void execute(Scene* renderThreadScene) override;

private:
    size_t m_ID;
};

//========================= Post Processing Commands =========================//

// Sets the root post processing effect of the scene
// only meant to be called once on initialization, to propogate root to render thread
// NOT exposed through chuck API
class SetSceneRootEffectCommand : public SceneGraphCommand
{
public:
    SetSceneRootEffectCommand(Scene* audioThreadScene, PP::Effect* effect) 
        : m_EffectID(effect->GetID()) {
        audioThreadScene->SetRootEffect(m_EffectID);
    };

    virtual void execute(Scene* renderThreadScene) override {
        PP::Effect* effect = dynamic_cast<PP::Effect*>(renderThreadScene->GetNode(m_EffectID));
        assert(effect);
        
        renderThreadScene->SetRootEffect(m_EffectID);
    }
private:
    size_t m_EffectID;
};

class AddEffectCommand : public SceneGraphCommand
{
public:
    AddEffectCommand(PP::Effect* LHS, PP::Effect* RHS) 
        : m_LHS_ID(LHS->GetID()), m_RHS_ID(RHS->GetID()) {
        LHS->Add(RHS);
    };

    virtual void execute(Scene* renderThreadScene) override {
        PP::Effect* lhs = dynamic_cast<PP::Effect*>(renderThreadScene->GetNode(m_LHS_ID));
        PP::Effect* rhs = dynamic_cast<PP::Effect*>(renderThreadScene->GetNode(m_RHS_ID));
        assert(lhs);
        assert(rhs);

        lhs->Add(rhs);
    }
private:
    size_t m_LHS_ID, m_RHS_ID;
};

class RemoveEffectCommand : public SceneGraphCommand
{
public:
    RemoveEffectCommand(PP::Effect* effect) 
        : m_ID(effect->GetID()) {
        effect->RemoveNext();
    };

    virtual void execute(Scene* renderThreadScene) override {
        PP::Effect* effect = dynamic_cast<PP::Effect*>(renderThreadScene->GetNode(m_ID));
        assert(effect);

        effect->RemoveNext();
    }
private:
    size_t m_ID;
};

class BypassEffectCommand : public SceneGraphCommand
{
public:
    BypassEffectCommand(PP::Effect* effect, bool bypass) 
        : m_ID(effect->GetID()), m_Bypass(bypass) {
        effect->SetBypass(bypass);
    };

    virtual void execute(Scene* renderThreadScene) override {
        PP::Effect* effect = dynamic_cast<PP::Effect*>(renderThreadScene->GetNode(m_ID));
        assert(effect);

        effect->SetBypass(m_Bypass);
    }
private:
    size_t m_ID;
    bool m_Bypass;
};


class UpdateEffectUniformCommand: public SceneGraphCommand
{
public:
    UpdateEffectUniformCommand(PP::Effect* effect, const MaterialUniform& uniform) 
        : m_ID(effect->GetID()), m_Uniform(uniform)
    {
        effect->SetUniform(m_Uniform);
    }

    virtual void execute(Scene* renderThreadScene) override {
        PP::Effect* effect = dynamic_cast<PP::Effect*>(renderThreadScene->GetNode(m_ID));
        assert(effect);

        effect->SetUniform(m_Uniform);
    }

private:
    size_t m_ID;
    MaterialUniform m_Uniform;
};