<<< "starting" >>>;

0 => int frameCounter;
now => time lastTime;

CglUpdate UpdateEvent;
CglFrame FrameEvent;

NormMat normMat;
BoxGeo boxGeo;

CglScene scene;
CglMesh mesh;

mesh.set( boxGeo, normMat );
scene.AddChild( mesh );


while (true) {
    CGL.Render();
    UpdateEvent => now;

    // compute timing
    frameCounter++;
    now - lastTime => dur deltaTime;
    now => lastTime;
    <<< "frame: ", frameCounter, " delta: ", deltaTime/second, "FPS: ", second / deltaTime>>>;
}


