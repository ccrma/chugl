//---------------|
// shake-o-matic!
// by: Ge Wang (gewang@cs.princeton.edu)
//     Perry R. Cook (prc@cs.princeton.edu)
//------------------|

 0 => int Maraca;
 1 => int Cabasa;
 2 => int Sekere;
 3 => int Guiro;
 4 => int Water_Drops;
 5 => int Bamboo_Chimes;
 6 => int Tambourine;
 7 => int Sleigh_Bells;
 8 => int Sticks;
 9 => int Crunch;
10 => int Wrench;
11 => int Sand_Paper;
12 => int Coke_Can;
13 => int Next_Mug;
14 => int Penny_Mug;
15 => int Nickle_Mug;
16 => int Dime_Mug;
17 => int Quarter_Mug;
18 => int Franc_Mug;
19 => int Peso_Mug;
20 => int Big_Rocks;
21 => int Little_Rocks;
22 => int Tuned_Bamboo_Chimes;

[ "Maraca", "Cabasa", "Sekere", "Guiro", "Water_Drops", "Bamboo_Chimes", "Tambourine", "Sleigh_Bells", "Sticks", "Crunch", "Wrench", "Sand_Paper", "Coke_Can", "Next_Mug",
"Penny_Mug", "Nickle_Mug", "Dime_Mug", "Quarter_Mug", "Franc_Mug", "Peso_Mug", "Big_Rocks", "Little_Rocks", "Tuned_Bamboo_Chimes"] @=> string instruments[];

// our patch
Shakers shake => JCRev r => dac;
// set the gain
//.95 => r.gain;
// set the reverb mix
.025 => r.mix;

// our main loop
while( true )
{
    // frequency..
    // note: Math.randomf() returns value between 0 and 1
    if( Math.randomf() > 0.625 )
    {
        Math.random2( 0, 22 ) => shake.which;
        1 => shake.which;
        Std.mtof( Math.random2f( 0.0, 128.0 ) ) => shake.freq;
        Math.random2f( 0, 128 ) => shake.objects;
        <<< instruments[shake.which()], shake.which(),  "freq: :", shake.freq(), "| #objects: ", shake.objects() >>>;
    }

    // shake it!
    Math.random2f( 0.8, 1.3 ) => shake.noteOn;

    // note: Math.randomf() returns value between 0 and 1
    if( Math.randomf() > 0.9 )
    { 500::ms => now; }
    else if( Math.randomf() > .925 )
    { 250::ms => now; }
    else if( Math.randomf() > .05 )
    { .125::second => now; }
    else
    {
        1 => int i => int pick_dir;
        // how many times
        4 * Math.random2( 1, 5 ) => int pick;
        0.0 => float pluck;
        0.7 / pick => float inc;
        // time loop
        for( ; i < pick; i++ )
        {
            75::ms => now;
            Math.random2f(.2,.3) + i*inc => pluck;
            pluck + -.2 * pick_dir => shake.noteOn;
            // simulate pluck direction
            !pick_dir => pick_dir;
        }

        // let time pass for final shake
        75::ms => now;
    }
}