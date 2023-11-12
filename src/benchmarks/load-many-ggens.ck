1024 => int numCube;
GCube cubes[numCube];

5 => GG.camera().posZ;

for (auto s : cubes)
{
    s --> GG.scene();
}

// graphics render loop
while( true )
{
    GG.nextFrame() => now;
}