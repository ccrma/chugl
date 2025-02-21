//-----------------------------------------------------------------------------
// name: keyboard-organ.ck
// desc: play your computer keyboard like an organ (monophonic)
//       instrument design by USB3
//       
// keys: use number row 1,2,3...,9,0 and 'y' to play organ
//       'x' - octave up
//       'z' - octave down
//
// author: terry feng from USB3
//         https://www.youtube.com/watch?v=es6oiJEvI_M
//-----------------------------------------------------------------------------

// webchuck keyboard device
0 => int device;

// HID input and HID message
Hid hid;
HidMsg msg;

// open keyboard device
if( !hid.openKeyboard( device ) ) me.exit();
<<< "keyboard '" + hid.name() + "' ready", "" >>>;

// MIDI notes
[49,50,52,54,56,57,59,61,62,64] @=> int notes[];
10 => int length;
58 => int spiceNote;

0 => int octave;

// Organ Patch
BeeThree organ => JCRev r => Echo e => Echo e2 => dac;
r => dac;
// Patch settings
// set delays
240::ms => e.max => e.delay;
480::ms => e2.max => e2.delay;
// set gains
.6 => e.gain;
.3 => e2.gain;
.05 => r.mix;

// infinite event loop
while( true )
{
    // wait for HID event
    hid => now;

    // get HID message
    while( hid.recv( msg ) )
    {
        if( msg.isButtonDown() ) {
            // print key (debugging)
            <<< "[key]", msg.key, "(ascii)", msg.which >>>;
            
            // Play notes
            if ( msg.which >= '1' && msg.which <= '9') {
                // convert ascii representation to notes array index
                // gets MIDI value and applies octave shift
                // convert MIDI to freq
                Std.mtof( notes[msg.which-'1'] + octave ) => float freq;
                freq => organ.freq;
                1 => organ.noteOn;
            }
            if (msg.which == '0') {
                Std.mtof( notes[length-1] + octave ) => float freq;
                freq => organ.freq;
                1 => organ.noteOn;
            }
            if (msg.which == 'Y') {
                Std.mtof( spiceNote + octave ) => float freq;
                freq => organ.freq;
                1 => organ.noteOn;
            }

            // octave shift
            if (msg.which == 'X') {
                <<< "octave up", "" >>>;
                12 +=> octave;
            } 
            else if (msg.which == 'Z')  {
                <<< "octave down", "" >>>;
                12 -=> octave;
            }
        }
        else  {
            0 => organ.noteOff;
        }
    }
}