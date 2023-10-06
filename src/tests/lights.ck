0 => int frameCounter;
now => time lastTime;

GG.camera() @=> GCamera cam;

// controls
InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

NormMat normMat;
BoxGeometry boxGeo;
SphereGeometry  SphereGeometry ;

GScene scene;
GMesh mesh;
GMesh fileMesh;
GMesh lightbulb;
GMesh shaderMesh; ShaderMat shaderMat; 
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
dataTex.filter(Texture.FILTER_NEAREST, Texture.FILTER_NEAREST);
tex.path("./tests/textures/awesomeface.png");
PhongMat phongMat;
phongMat.diffuseMap(dataTex);

PhongMat filePhongMat;
filePhongMat.diffuseMap(tex);

mesh.set( boxGeo, phongMat );
fileMesh.set(boxGeo, filePhongMat);
shaderMesh.set( boxGeo, shaderMat );
lightbulb.set( SphereGeometry , normMat );

mesh --> scene;
fileMesh --> scene;
group --> scene;
// dirLight --> scene;
shaderMesh --> scene;

fileMesh.position(@(0, 1.5, 0));
shaderMesh.position(@(0, -1.5, 0));
lightbulb.position( @(2, 0, 0) );
lightbulb.scale( @(0.1, 0.1, 0.1) );
light --> lightbulb --> group;

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
    // GG.Render();


    // compute timing
    frameCounter++;
    now - lastTime => dur deltaTime;
    now => lastTime;
    deltaTime/second => float dt;

    // rotate light
    group.rotY( .85 * dt );
    dirLight.rotX( .75 * dt);

    // <<< "light pos", light.worldPos() >>>;
    // <<< "lightbulb pos", lightbulb.worldPos() >>>;

    // 
    shaderMat.uniformFloat("u_Time", now/second);
    // <<< "u_Time: " + now/second >>>;

    GG.nextFrame() => now;
}


