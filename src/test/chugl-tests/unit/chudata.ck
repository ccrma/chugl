Chudata data;
data.set(0, 1337);
T.assert(data.geti(0) == 1337);
data.save();

{
    Chudata data1;
    T.assert(data1.geti(0) == 1337);

    // test destructor saves
    data1.set(1, 8888);
}

Chudata data2;
T.assert(data2.geti(1) == 8888);
