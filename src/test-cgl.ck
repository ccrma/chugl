<<< "starting" >>>;

0 => int frameCounter;
now => time lastTime;

CglUpdate UpdateEvent;
CglFrame FrameEvent;

NormMat normMat;
BoxGeo boxGeo;
SphereGeo sphereGeo;

CglScene scene;
CglMesh mesh;
CglMesh lightbulb;
CglGroup group;

// Lighting
PointLight light;
PhongMat phongMat;

mesh.set( boxGeo, phongMat );
lightbulb.set( sphereGeo, normMat );

scene.AddChild( mesh );
scene.AddChild(group);
lightbulb.SetPosition( @(2, 0, 0) );  
group.AddChild( lightbulb );
lightbulb.AddChild( light );



while (true) {
    CGL.Render();
    UpdateEvent => now;

    // compute timing
    frameCounter++;
    now - lastTime => dur deltaTime;
    now => lastTime;
    <<< "frame: ", frameCounter, " delta: ", deltaTime/second, "FPS: ", second / deltaTime>>>;

    deltaTime/second => float dt;

    // rotate light
    group.RotateY( 0.5 * dt );
    <<< "light position", light.GetWorldPosition() >>>;
    <<< "lightbulb position", lightbulb.GetWorldPosition() >>>;
    // <<< "lightbulb local position", lightbulb.GetPosition() >>>;
    // <<< "group position", group.GetWorldPosition() >>>;
}


