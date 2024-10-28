Texture.load(
    me.dir() + "../assets/bridge/posx.jpg", // right
    me.dir() + "../assets/bridge/negx.jpg", // left
    me.dir() + "../assets/bridge/posy.jpg", // top
    me.dir() + "../assets/bridge/negy.jpg", // bottom
    me.dir() + "../assets/bridge/posz.jpg", // back
    me.dir() + "../assets/bridge/negz.jpg"  // front
) @=> Texture cubemap;

GG.scene().envMap(cubemap);
GG.scene().backgroundColor(Color.WHITE);

GOrbitCamera camera --> GG.scene();
// GFlyCamera camera --> GG.scene();
GG.scene().camera(camera);

GCube cube --> GG.scene();

UI_Float3 background_color(GG.scene().backgroundColor());

while (true) {
    GG.nextFrame() => now;

    if (UI.begin("Skybox")) {
        if (UI.colorEdit("Background Color", background_color, 0)) {
            GG.scene().backgroundColor(background_color.val());
        }
    }
    UI.end();
}



// test API

// need cubemap for skybox shader
// need cubemap for envmapping in phong/pbr materials

// cubemap @=> GG.scene().skybox();  // set skybox and envmapping for phong/pbr materials


// GG.scene().skybox;

// SkyboxMaterial 
