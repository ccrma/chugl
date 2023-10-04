fun void chugl_shred() {
    repeat(5) {
        GG.nextFrame() => now;
    }
    <<< "chugl_shred() done" >>>;
}

fun void shred_printer() {
    while (1::second => now) {
        <<< "num registerd", GG.numRegisteredShreds() >>>;
        <<< "num registerd and waiting", GG.numRegisteredWaitingShreds() >>>;
    }
}

spork ~ shred_printer();

while (10::ms => now) {
    spork ~ chugl_shred();
}