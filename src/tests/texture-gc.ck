GPlane plane --> GG.scene();
-5 => plane.posZ;

fun void ApplyTexture(string path) {
    FileTexture tex;
    tex.path(me.dir() + path);
    plane.mat().diffuseMap(tex);
}


while (true) { 
    ApplyTexture("textures/chuck-logo.png");
    GG.nextFrame() => now;
    GG.nextFrame() => now;
    ApplyTexture("textures/awesomeface.png");
    GG.nextFrame() => now;
    GG.nextFrame() => now;
}