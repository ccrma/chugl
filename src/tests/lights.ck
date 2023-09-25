0 => int frameCounter;
now => time lastTime;

// controls
InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

CglUpdate UpdateEvent;

NormMat normMat;
BoxGeo boxGeo;
SphereGeo sphereGeo;

CglScene scene;
CglMesh mesh;
CglMesh fileMesh;
CglMesh lightbulb;
CglMesh shaderMesh; ShaderMat shaderMat; 
shaderMat.shaderPaths(
    "renderer/shaders/BasicLightingVert.glsl",
    "renderer/shaders/mangoFrag.glsl"
);
// shaderMat.wireframe(1);
CglGroup group;

// Lighting
PointLight light;
DirLight dirLight;
FileTexture tex;
DataTexture dataTex;
dataTex.data([1.0, 0.0, 1.0, 1.0], 1, 1);
dataTex.filter(CglTexture.FILTER_NEAREST, CglTexture.FILTER_NEAREST);
tex.path("./tests/textures/awesomeface.png");
PhongMat phongMat;
phongMat.diffuseMap(dataTex);

PhongMat filePhongMat;
filePhongMat.diffuseMap(tex);

mesh.set( boxGeo, phongMat );
fileMesh.set(boxGeo, filePhongMat);
shaderMesh.set( boxGeo, shaderMat );
lightbulb.set( sphereGeo, normMat );

scene.AddChild( mesh );
scene.AddChild(fileMesh);
scene.AddChild(group);
scene.AddChild(dirLight);
scene.AddChild(shaderMesh);

fileMesh.SetPosition(@(0, 1.5, 0));
shaderMesh.SetPosition(@(0, -1.5, 0));
lightbulb.SetPosition( @(2, 0, 0) );
lightbulb.SetScale( @(0.1, 0.1, 0.1) );
group.AddChild( lightbulb );
lightbulb.AddChild( light );

fun void AlternateTextures() {
    while (true) {
        // its faster to just create two textures and switch between them
        // this is just to test texture reloading
        1::second => now;
        tex.path("./tests/textures/chuck-logo.png");
        1::second => now;
        tex.path("./tests/textures/awesomeface.png");

        1::second => now;
        dataTex.data([255.0, 0.0, 255.0, 255.0], 1, 1);
        1::second => now;
        dataTex.data(
            [
                255.0, 0.0, 255.0, 255.0,
                255.0, 255.0, 0.0, 255.0,
                0.0, 255.0, 255.0, 255.0,
                255.0, 132.0, 20.0, 255.0,
            ], 
            2, 2
        );
    }
} spork ~ AlternateTextures();



while (true) {
    // CGL.Render();


    // compute timing
    frameCounter++;
    now - lastTime => dur deltaTime;
    now => lastTime;
    deltaTime/second => float dt;

    // rotate light
    group.RotateY( .85 * dt );
    dirLight.RotateX( .75 * dt);

    // 
    shaderMat.uniformFloat("u_Time", now/second);
    // <<< "u_Time: " + now/second >>>;

    // UpdateEvent => now;
    CGL.nextFrame() => now;
}


