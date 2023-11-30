// controls ==============================================================
InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

// scene ==============================================================

50 => int NUM_TEXT;
for (NUM_TEXT => int i; i >= 0; i--)
// for (0 => int i; i < NUM_TEXT; i++)
{
    GText text --> GG.scene();
    text.text("Hello World! " + i);
    text.translate(@(0, 0, -i));
    // text.sca(i);
}
// GG.scene().backgroundColor( @(115.0, 193.0, 245.0) / 255.0 );
GG.font(me.dir() + "../../examples/assets/fonts/SourceSerifPro-Regular.otf");

GText fpsText --> GG.scene();

fun void fpser() {
    while (true) {
        <<< "fps:", GG.fps() >>>;
        fpsText.text("fps: " + GG.fps());
        fpsText.posZ(1);
        GG.nextFrame() => now;
    }
} spork ~ fpser();

while (true) { 
    // .2 * GG.dt() => text.rotateZ;
    GG.nextFrame() => now; 
}
