// Mouse test
// need to run Mouse.ck before this (or run go.ck)

Mouse mouse;
// open mouse device 0
spork ~ mouse.start(0);

// set up to listen for first three mouse buttons
for( int i : [0,1,2] )
{
    // spork shred to handle a particular button
    spork ~ waitOnButton( i );
}

// keep it going
while( true )
{
    // print
    <<< "mouse DELTAS", mouse.deltas() >>>;
    
    // print slowly so we can see other notifcations more easily
    400::ms => now;
}

// a function to be spork to listen for a particular mouse button
fun void waitOnButton( int which )
{
    // check bounds
    if( which < 0 || which > mouse.mouseDownEvents.size() )
    {
        <<< "cannot wait on button with index", which >>>;
        return;
    }
    
    // loop
    while( true )
    {
        // wait on a particular mouse button is hit
        mouse.mouseDownEvents[which] => now;
        // print
        <<< "*** mouse BUTTON HIT", which, "***" >>>;
    }
}