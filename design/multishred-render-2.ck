// register this shred with ChuGL; OPTIONAL since render() will
// implicitly register this shred with ChuGL
// ChuGL.register( me );

// do stuff to the scene graph
fun void rearrangeSceneGraph2()
{
    // generally speaking, this is meant to be instantaneous
    // i.e., don't advance time in here
}

// a chugl render loop
while( true )
{
    // change anything (but don't advance time in here)
    rearrangeSceneGraph2();

    // "this shred is ready to render the current frame"
    ChuGL.render() => now;
    
    // should not do this in this ChuGL shred's render loop
    // 100::ms => now;
}

// unregister me
// ChuGL.unregister( me );

// do more stuff, but ChuGL isn't waiting on my render() => now
// ...
