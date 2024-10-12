/*
ChuGL Integration Test Runner
*/
FileIO dir;
dir.open(me.dir() + "./unit");

dir.dirList() @=> string tests[];

for (auto test : tests) {
    if (test == "all.ck") continue; // ignore test runner
    if (test == "tester.ck") continue; // ignore self
    if (test == "T.ck") continue; // ignore test harness
    // run test
    T.println("Running test: " + test + " --------------------");
    Machine.add(me.dir() + "./unit/" + test);
}