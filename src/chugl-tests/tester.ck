/*
ChuGL Integration Test Runner

`chuck T.ck tester.ck`
*/

FileIO dir;
dir.open(me.dir());

dir.dirList() @=> string tests[];

for (auto test : tests) {
    if (test == "tester.ck") continue; // ignore self
    if (test == "T.ck") continue; // ignore test harness
    // run test
    T.println("Running test: " + test + " --------------------");
    Machine.add(me.dir() + "/" + test);
}
