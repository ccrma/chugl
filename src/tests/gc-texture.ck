GPlane plane --> GG.scene();
-5 => plane.posZ;

fun void ApplyTexture(string path) {
    FileTexture tex;
    tex.path(me.dir() + path);
    plane.mat().diffuseMap(tex);
}

fun void ApplySkybox(
    string path1, string path2, string path3, string path4, string path5, string path6
) {
    CubeTexture localEnvMap;
    localEnvMap.paths(
        me.dir() + path1,
        me.dir() + path2,
        me.dir() + path3,
        me.dir() + path4,
        me.dir() + path5,
        me.dir() + path6
    );
    GG.scene().skybox(localEnvMap);
}


while (true) { 
    ApplyTexture("textures/chuck-logo.png");
    GG.nextFrame() => now;
    ApplySkybox(
        "./textures/skybox/water/right.jpg",
        "./textures/skybox/water/left.jpg",
        "./textures/skybox/water/top.jpg",
        "./textures/skybox/water/bottom.jpg",
        "./textures/skybox/water/front.jpg",
        "./textures/skybox/water/back.jpg"
    );
    GG.nextFrame() => now;
    ApplyTexture("textures/awesomeface.png");
    GG.nextFrame() => now;
    spork ~ ApplySkybox(
        "./textures/skybox/Yokohama3/right.jpg",
        "./textures/skybox/Yokohama3/left.jpg",
        "./textures/skybox/Yokohama3/top.jpg",
        "./textures/skybox/Yokohama3/bottom.jpg",
        "./textures/skybox/Yokohama3/front.jpg",
        "./textures/skybox/Yokohama3/back.jpg"
    );
    GG.nextFrame() => now;
}