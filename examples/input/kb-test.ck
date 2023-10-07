// KB test
// need to run KB.ck before this

// keep it going
while( true )
{
    // test for the 'a' key
    if( KB.isKeyDown(KB.KEY_A) )
    {
        <<< "A key is down, now:", now >>>;
    }
    
    // every so often!
    16::ms => now;
}
