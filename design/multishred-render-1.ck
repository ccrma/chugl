// register this shred with ChuGL; OPTIONAL since render() will
// implicitly register this shred with ChuGL
// ChuGL.register( me );
// syntactic suger @graphic
// ChuGL.add() == ChuGL.context(0).add()
// is ChuGL an event? is ChuGL the default context event??

// OPTIONAL explicitly show default context/window; this front-loads
// the graphics and windowing initialization -> potentially smoother
// runtime?
// ChuGL.show();

// FUTURE WORK could have separate contexts for different windows
// by default, ChuGL operates on the default context
ChuGL.context(1) @=> GGEvent e;

// do stuff to the scene graph
fun void rearrangeSceneGraph()
{
    // generally speaking, this is meant to be instantaneous
    // i.e., don't advance time in here
}

// a chugl render loop
while( true )
{
    // change anything (but don't advance time in here)
    rearrangeSceneGraph();

    // "this shred is ready to render the current frame"
    // also, register this shred with ChuGL, if not already
    // registered; also implicitly activates the main thread
    // hook to show deafult context/window
    ChuGL.render() => now;

    // internally, chugl checks to see if all registered shreds
    // are ready to render; if all registered shreds are ready,
    // cascade GGen.update() through scenegraph on audio thread;
    // signal the graphics thread to actuall render the frame;
    // at the earliest, the graphics thread queue_broadcasts the
    // chuck event

    // TODO this is done using a non-existent function called
    // "start_waiting()" on the audio thread from VM;

    // TODO a callback to know when a shred is removed from VM

    // TODO graphics "watchdog" could detect possible deadlock
    // e.g., if a registered shred fails to render()=>now
}
