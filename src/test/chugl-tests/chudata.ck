Chudata.load(me.dir() + "./.chudata.chud");

// defaults
T.assert(Chudata.geti(0) == 0, "default int");
T.assert(Chudata.gets(0) == null, "default str");

Chudata.set(0, 1337);
Chudata.set(1, "yo");
T.assert(Chudata.geti(0) == 1337, "Chudata.geti(0) == 1337");
T.assert(Chudata.gets(1) == "yo", "Chudata.gets(1) == yo");

// // delete everything
Chudata.clear();
Chudata.save();

// // reload
Chudata.load(me.dir() + "./.chudata.chud");
T.assert(Chudata.geti(0) == 0, "Chudata.geti(0) == 0");
T.assert(Chudata.gets(200) == null, "Chudata.gets(200) == null");
Chudata.set(1, 1337);
Chudata.set(2, "heya");
Chudata.set(100, 100);
Chudata.save();

Chudata.clear();
Chudata.load(me.dir() + "./.chudata.chud");
T.assert(Chudata.geti(1) == 1337, "Chudata.geti(1) == 1337");
T.assert(Chudata.gets(2) == "heya", "Chudata.gets(2) == heya");
T.assert(Chudata.geti(100) == 100, "Chudata.geti(100) == 100");

// check idx out of bounds
T.assert(Chudata.geti(-10) == 0, "Chudata.geti(-10) == 0");
T.assert(Chudata.geti(256) == 0, "Chudata.geti(-10) == 0");
T.assert(Chudata.gets(-10) == null, "Chudata.geti(-10) == 0");
T.assert(Chudata.gets(256) == null, "Chudata.geti(-10) == 0");

// test wrong type get
T.assert(Chudata.gets(100) == null, "Chudata.gets(100) == null");
T.assert(Chudata.geti(2) == 0, "Chudata.geti(2) == 0");

// delete everything for clean slate
Chudata.clear();
Chudata.save();
