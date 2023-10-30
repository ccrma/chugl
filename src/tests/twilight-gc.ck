fun void breaks() {
    GCube c --> GCube d;
    <<< "c id:", c.id(), "d id:", d.id() >>>;
}

while (true) {
    spork ~ breaks();

    GG.nextFrame() => now;
}