<<< "Testing CglTexture features" >>>;

<<< "Initializing CglTexture" >>>;
CglTexture tex;

<<< "Printing static vars" >>>;
<<< "CglTexture.WRAP_REPEAT = ", CglTexture.WRAP_REPEAT >>>; 
<<< "CglTexture.WRAP_MIRRORED = ", CglTexture.WRAP_MIRRORED >>>; 
<<< "CglTexture.WRAP_CLAMP = ", CglTexture.WRAP_CLAMP >>>; 
<<< "CglTexture.FILTER_NEAREST = ", CglTexture.FILTER_NEAREST >>>; 
<<< "CglTexture.FILTER_LINEAR = ", CglTexture.FILTER_LINEAR >>>; 

<<< "=========Testing CglTexture setters and getters=========" >>>;
tex.wrap(CglTexture.WRAP_MIRRORED, CglTexture.WRAP_MIRRORED);
tex.filter(CglTexture.FILTER_NEAREST, CglTexture.FILTER_LINEAR);

<<< "tex.wrapS() = ", tex.wrapS() >>>;
<<< "tex.wrapT() = ", tex.wrapT() >>>;
<<< "tex.filterMin() = ", tex.filterMin() >>>;
<<< "tex.filterMag() = ", tex.filterMag() >>>;

tex.path("./tests/textures/chuck-logo.png");
// <<< "tex.path() = ", tex.path() >>>;

// attach to material
PhongMat phong;
phong.diffuseMap(tex);

// attach material to mesh
CglScene scene;
CglMesh mesh;
BoxGeo boxGeo;
mesh.set(boxGeo, phong);
scene.AddChild(mesh);


0 => int frameCounter;
now => time lastTime;
CglUpdate UpdateEvent;

while (true) {
    CGL.Render();
    UpdateEvent => now;

    // compute timing
    frameCounter++;
    now - lastTime => dur deltaTime;
    now => lastTime;
    deltaTime/second => float dt;

    // rotate light
}


