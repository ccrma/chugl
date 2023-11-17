[
    "gc-ggen.ck",
    "gc-texture.ck",
    "gc-geo.ck",
    "gc-material.ck",
    "gc-shred.ck"
] @=> string gc_test_files[];

for (auto test : gc_test_files) {
    me.dir() + test => string file;
    <<< "adding GC test:", file >>>;
    Machine.add(file);
}

while (true) {
    <<< "gc" >>>;
    GG.nextFrame() => now;
}
