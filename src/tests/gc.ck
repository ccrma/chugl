[
    "gc-ggen.ck",
    "gc-texture.ck",
    "gc-geo.ck",
    "gc-material.ck"
] @=> string gc_test_files[];

for (auto test : gc_test_files) {
    me.dir() + test => string file;
    Machine.add(me.dir() + test);
}

1::eon => now;