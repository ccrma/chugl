
/*
azaday 10/20/2026
ChuGL Dev Branch version: 0.3.1

desc: Another GC test for GGens / Geometries / GMeshes

Alex Han reported a bug in ChuGL 0.2.9 where creating GGens and detaching them each frame leaked 
memory and degraded fps over time
*/


GGen holder --> GG.scene();

GG.nextFrame() => now;

// TODO also check #entries in hashmap locator 
GG.stats() @=> ChuGL_Stats init_stats;

fun void compareStats() {
    GG.stats() @=> ChuGL_Stats stats;

    <<< "component deltas:", 
    "| ggen -", stats.ggen_count - init_stats.ggen_count,
    "| geo -", stats.geometry_count - init_stats.geometry_count,
    "| mat -", stats.material_count - init_stats.material_count,
    "| mesh -", stats.mesh_count - init_stats.mesh_count,
    "| glight -", stats.glight_count - init_stats.glight_count
    >>>;
}

// test grucking + ungrucking individual ggens
repeat (60) {
    GG.nextFrame() => now;

    holder.rotateY(GG.dt());

    GLines l --> holder; 
    GGen g --> holder;
    GCube cube --> holder;
    l.positions([@(0,0), @(1,1)]); 

    holder.detachChildren();
}

GG.nextFrame() => now;
GG.nextFrame() => now;
compareStats();

// test grucking + ungrucking ggen subtrees
repeat (60) {
    GG.nextFrame() => now;

    // single chain
    GGen A --> GGen B --> holder;
    GCube cube --> B;
    GPolyhedron poly --> cube;
    GGen C --> cube;
    GLight light --> C;
    GModel model --> light;
    GText text --> model;
    text.text("testing text");
    text.font("chugl:proggy-tiny");


    holder.detachSubgraph();
}
GG.nextFrame() => now;
GG.nextFrame() => now;
compareStats();

