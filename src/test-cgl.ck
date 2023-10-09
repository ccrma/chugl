<<< "starting" >>>;

0 => int frameCounter;
now => time lastTime;

NextFrameEvent UpdateEvent;


NormalsMaterial normMat;
BoxGeometry boxGeo;
SphereGeometry  SphereGeometry ;

GScene scene;
GMesh mesh;
GMesh lightbulb;
GGen group;

// Lighting
GPointLight light;
PhongMaterial phongMat;

mesh.set( boxGeo, phongMat );
lightbulb.set( SphereGeometry , normMat );

mesh --> scene;
light --> lightbulb --> group --> scene;
lightbulb.position( @(2, 0, 0) );  



while (true) {
    GG.Render();
    UpdateEvent => now;

    // compute timing
    frameCounter++;
    now - lastTime => dur deltaTime;
    now => lastTime;
    <<< "frame: ", frameCounter, " delta: ", deltaTime/second, "FPS: ", second / deltaTime>>>;

    deltaTime/second => float dt;

    // rotate light
    group.rotY( 0.5 * dt );
    <<< "light position", light.GetWorldPosition() >>>;
    <<< "lightbulb position", lightbulb.GetWorldPosition() >>>;
    // <<< "lightbulb local position", lightbulb.pos() >>>;
    // <<< "group position", group.GetWorldPosition() >>>;
}


