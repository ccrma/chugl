// controls
InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

GSphere sphere --> GG.scene();
GCube cube --> GG.scene();
cube.posX(2);
// disable default dir light
// GG.scene().light().intensity(0);

fun void fpser(dur d) {
    while (d => now) <<< GG.fps() >>>;
}
spork ~ fpser (1::second);

while (true) { 
    GG.nextFrame() => now; 
}