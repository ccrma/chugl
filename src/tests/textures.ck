<<< "Testing Texture features" >>>;

<<< "Initializing Texture" >>>;
FileTexture tex;

<<< "Printing static vars" >>>;
<<< "Texture.WRAP_REPEAT = ", Texture.WRAP_REPEAT >>>; 
<<< "Texture.WRAP_MIRRORED = ", Texture.WRAP_MIRRORED >>>; 
<<< "Texture.WRAP_CLAMP = ", Texture.WRAP_CLAMP >>>; 
<<< "Texture.FILTER_NEAREST = ", Texture.FILTER_NEAREST >>>; 
<<< "Texture.FILTER_LINEAR = ", Texture.FILTER_LINEAR >>>; 

<<< "=========Testing Texture setters and getters=========" >>>;
tex.wrap(Texture.WRAP_MIRRORED, Texture.WRAP_MIRRORED);
tex.filter(Texture.FILTER_NEAREST, Texture.FILTER_LINEAR);

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
GScene scene;
GMesh mesh;
BoxGeometry boxGeo;
mesh.set(boxGeo, phong);
mesh --> scene;

/*

DataTexture dt;
dt.data(width, height, [1,2,3...])
// inherit sampler properties (filter, wrap, mipmaps etc)

*/


0 => int frameCounter;
now => time lastTime;
NextFrameEvent UpdateEvent;

while (true) {
    GG.Render();
    UpdateEvent => now;

    // compute timing
    frameCounter++;
    now - lastTime => dur deltaTime;
    now => lastTime;
    deltaTime/second => float dt;

    // rotate light
}


