/*
ChuGL Integration Test Runner
*/
FileIO dir;
dir.open(me.dir() + "./unit");

dir.dirList() @=> string tests[];

// chuck string sorting is broken (sorts by pointer, not strcmp)
// workaround: put strings in assoc array, somehow getKeys returns them in order
for (auto test : tests) {
    "" => tests[test];
}
string keys[0];
tests.getKeys(keys);

for (auto test : keys) {
    // run test
    T.println("Running test: " + test + " --------------------");
    Machine.add(me.dir() + "./unit/" + test);
}