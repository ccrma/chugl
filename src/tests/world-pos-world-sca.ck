GG.scene().backgroundColor(@(0.2, 0.2, 0.2));
5 => GG.camera().posZ;

GSphere s1 --> GG.scene();
GSphere s2 --> GG.scene();
2 * @(1,1,1) => s1.scale;
.5 * @(1,1,1) => s2.scale;

-2 => s1.posX; 2 => s2.posX;

GSphere child;
-1 => child.posY;

fun void parenter() {
    while (true) {
        child --> s1;
        @(0,-1,0) => child.worldPosition;
        @(1,1,1) => child.worldScale;
        <<< "child of left sphere", "local scale:", child.sca(), "world scale", child.worldSca(), "local pos", child.pos(), "world pos", child.worldPos() >>>;
        1::second => now;
        child --> s2;
        @(0,-1,0) => child.worldPosition;
        @(1,1,1) => child.worldScale;
        <<< "child of right sphere", "local scale:", child.sca(), "world scale", child.worldSca(), "local pos", child.pos(), "world pos", child.worldPos() >>>;
        1::second => now;
    }
}
spork ~ parenter();


while (true) { GG.nextFrame() => now; }