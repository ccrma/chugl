Chudata.load(me.dir() + "./.chudata.chud");

// defaults
for (int i; i < 255; i++) {
    T.assert(Chudata.geti(0) == 0, "default int");
    T.assert(Chudata.gets(0) == null, "default str");
    T.assert(Chudata.getf(0) == 0, "default float");
}

Chudata.set(0, 1337);
Chudata.set(1, "yo");
Chudata.set(2, 42.42);
T.assert(Chudata.geti(0) == 1337, "Chudata.geti(0) == 1337");
T.assert(Chudata.gets(1) == "yo", "Chudata.gets(1) == yo");
T.assert(Chudata.getf(2) == 42.42, "Chudata.getf(2) == 42.42");

// // delete everything
Chudata.clear();
Chudata.save();

// // reload
Chudata.load(me.dir() + "./.chudata.chud");
T.assert(Chudata.geti(0) == 0, "Chudata.geti(0) == 0");
T.assert(Chudata.gets(200) == null, "Chudata.gets(200) == null");
Chudata.set(1, 1337);
Chudata.set(2, "heya");
Chudata.set(3, 42.42);
Chudata.set(100, 100);
Chudata.save();

Chudata.clear();
Chudata.load(me.dir() + "./.chudata.chud");
T.assert(Chudata.geti(1) == 1337, "Chudata.geti(1) == 1337");
T.assert(Chudata.gets(2) == "heya", "Chudata.gets(2) == heya");
T.assert(Chudata.getf(3) == 42.42, "Chudata.getf(3) == 42.42");
T.assert(Chudata.geti(100) == 100, "Chudata.geti(100) == 100");

// check idx out of bounds
T.assert(Chudata.geti(-10) == 0, "Chudata.geti(-10) == 0");
T.assert(Chudata.geti(256) == 0, "Chudata.geti(256) == 0");
T.assert(Chudata.gets(-10) == null, "Chudata.geti(-10) == 0");
T.assert(Chudata.gets(256) == null, "Chudata.geti(256) == 0");
T.assert(Chudata.getf(-10) == 0, "Chudata.getf(-10) == 0");
T.assert(Chudata.getf(256) == 0, "Chudata.getf(256) == 0");

// test wrong type get
T.assert(Chudata.gets(100) == null, "Chudata.gets(100) == null");
T.assert(Chudata.geti(2) == 0, "Chudata.geti(2) == 0");
T.assert(Chudata.getf(1) == 0, "Chudata.getf(1) == 0");

// delete everything for clean slate
Chudata.clear();
Chudata.save();
