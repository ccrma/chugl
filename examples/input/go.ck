//-----------------------------------------------------------------------------
// name: go.ck
// desc: top-level program to run for kb-test
//
// NOTE: this is an example of running a ChuGL project,
//       from a single file and with dependencies
//-----------------------------------------------------------------------------

// chugl modules you'd want to use
Machine.add( "KB.ck" );
Machine.add( "Mouse.ck" );
Machine.add( "FlyCam.ck" );

// add your program here
Machine.add( "kb-test.ck" );
