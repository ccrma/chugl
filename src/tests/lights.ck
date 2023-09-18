0 => int frameCounter;
now => time lastTime;

// controls
InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);


CglUpdate UpdateEvent;

NormMat normMat;
BoxGeo boxGeo;
SphereGeo sphereGeo;

CglScene scene;
CglMesh mesh;
CglMesh lightbulb;
CglGroup group;

// Lighting
PointLight light;
DirLight dirLight;
PhongMat phongMat;

mesh.set( boxGeo, phongMat );
lightbulb.set( sphereGeo, normMat );

scene.AddChild( mesh );
scene.AddChild(group);
scene.AddChild(dirLight);
lightbulb.SetPosition( @(2, 0, 0) ); 
lightbulb.SetScale( @(0.1, 0.1, 0.1) );
group.AddChild( lightbulb );
lightbulb.AddChild( light );



while (true) {
    CGL.Render();
    UpdateEvent => now;

    // compute timing
    frameCounter++;
    now - lastTime => dur deltaTime;
    now => lastTime;
    deltaTime/second => float dt;

    // rotate light
    group.RotateY( .85 * dt );
    dirLight.RotateX( .75 * dt);

    // camera update
    flycam.update( now, deltaTime );
}


