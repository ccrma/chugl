//-----------------------------------------------------------------------------
// name: KB.ck
// desc: keyboard input for ChuGL and general usage
//
//   Tracks which keys are held down at any given time.
//   Uses msg.key for this because:
//   - consistent across mac and windows
//   - differentiates between keys like left shift, right shift
//   - msg.which not consistent across platforms
//   - msg.ascii doesn't pick up many keys like shift and arrow keys
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
// date: May 2023
//
// note: may eventually want to add this to builtin cpp HID input manager
//-----------------------------------------------------------------------------

public class KB
{
    // =========================
    // Key mappings from https://gist.github.com/MightyPork/6da26e382a7ad91b5496ee55fdc73db2
    // =========================
    /**
    * Modifier masks - used for the first byte in the HID report.
    * NOTE: The second byte in the report is reserved, 0x00
    */
    0x00 => static int KEY_MOD_LCTRL;  
    0x01 => static int KEY_MOD_LSHIFT; 
    0x03 => static int KEY_MOD_LALT;   
    0x07 => static int KEY_MOD_LMETA;  
    0x0f => static int KEY_MOD_RCTRL;  
    0x1f => static int KEY_MOD_RSHIFT; 
    0x3f => static int KEY_MOD_RALT;   
    0x80 => static int KEY_MOD_RMETA;
    
    /**
    * Scan codes - last N slots in the HID report (usually 6).
    * 0x00 if no key pressed.
    * 
    * If more than N keys are pressed, the HID reports 
    * KEY_ERR_OVF in all slots to indicate this condition.
    */
    0x00 => static int KEY_NONE;  // No key pressed
    0x01 => static int KEY_ERR_OVF;  //  Keyboard Error Roll Over; used for all slots if too many keys are pressed ("Phantom key")
    0x04 => static int KEY_A;  // Keyboard a and A
    0x05 => static int KEY_B;  // Keyboard b and B
    0x06 => static int KEY_C;  // Keyboard c and C
    0x07 => static int KEY_D;  // Keyboard d and D
    0x08 => static int KEY_E;  // Keyboard e and E
    0x09 => static int KEY_F;  // Keyboard f and F
    0x0a => static int KEY_G;  // Keyboard g and G
    0x0b => static int KEY_H;  // Keyboard h and H
    0x0c => static int KEY_I;  // Keyboard i and I
    0x0d => static int KEY_J;  // Keyboard j and J
    0x0e => static int KEY_K;  // Keyboard k and K
    0x0f => static int KEY_L;  // Keyboard l and L
    0x10 => static int KEY_M;  // Keyboard m and M
    0x11 => static int KEY_N;  // Keyboard n and N
    0x12 => static int KEY_O;  // Keyboard o and O
    0x13 => static int KEY_P;  // Keyboard p and P
    0x14 => static int KEY_Q;  // Keyboard q and Q
    0x15 => static int KEY_R;  // Keyboard r and R
    0x16 => static int KEY_S;  // Keyboard s and S
    0x17 => static int KEY_T;  // Keyboard t and T
    0x18 => static int KEY_U;  // Keyboard u and U
    0x19 => static int KEY_V;  // Keyboard v and V
    0x1a => static int KEY_W;  // Keyboard w and W
    0x1b => static int KEY_X;  // Keyboard x and X
    0x1c => static int KEY_Y;  // Keyboard y and Y
    0x1d => static int KEY_Z;  // Keyboard z and Z
    0x1e => static int KEY_1;  // Keyboard 1 and !
    0x1f => static int KEY_2;  // Keyboard 2 and @
    0x20 => static int KEY_3;  // Keyboard 3 and #
    0x21 => static int KEY_4;  // Keyboard 4 and $
    0x22 => static int KEY_5;  // Keyboard 5 and %
    0x23 => static int KEY_6;  // Keyboard 6 and ^
    0x24 => static int KEY_7;  // Keyboard 7 and &
    0x25 => static int KEY_8;  // Keyboard 8 and *
    0x26 => static int KEY_9;  // Keyboard 9 and (
    0x27 => static int KEY_0;  // Keyboard 0 and )
    0x28 => static int KEY_ENTER;  // Keyboard Return (ENTER)
    0x29 => static int KEY_ESC;  // Keyboard ESCAPE
    0x2a => static int KEY_BACKSPACE;  // Keyboard DELETE (Backspace)
    0x2b => static int KEY_TAB;  // Keyboard Tab
    0x2c => static int KEY_SPACE;  // Keyboard Spacebar
    0x2d => static int KEY_MINUS;  // Keyboard - and _
    0x2e => static int KEY_EQUAL;  // Keyboard = and +
    0x2f => static int KEY_LEFTBRACE;  // Keyboard [ and {
    0x30 => static int KEY_RIGHTBRACE;  // Keyboard ] and }
    0x31 => static int KEY_BACKSLASH;  // Keyboard \ and |
    0x32 => static int KEY_HASHTILDE;  // Keyboard Non-US # and ~
    0x33 => static int KEY_SEMICOLON;  // Keyboard ; and :
    0x34 => static int KEY_APOSTROPHE;  // Keyboard ' and "
    0x35 => static int KEY_GRAVE;  // Keyboard ` and ~
    0x36 => static int KEY_COMMA;  // Keyboard , and <
    0x37 => static int KEY_DOT;  // Keyboard . and >
    0x38 => static int KEY_SLASH;  // Keyboard / and ?
    0x39 => static int KEY_CAPSLOCK;  // Keyboard Caps Lock
    0x3a => static int KEY_F1;  // Keyboard F1
    0x3b => static int KEY_F2;  // Keyboard F2
    0x3c => static int KEY_F3;  // Keyboard F3
    0x3d => static int KEY_F4;  // Keyboard F4
    0x3e => static int KEY_F5;  // Keyboard F5
    0x3f => static int KEY_F6;  // Keyboard F6
    0x40 => static int KEY_F7;  // Keyboard F7
    0x41 => static int KEY_F8;  // Keyboard F8
    0x42 => static int KEY_F9;  // Keyboard F9
    0x43 => static int KEY_F10;  // Keyboard F10
    0x44 => static int KEY_F11;  // Keyboard F11
    0x45 => static int KEY_F12;  // Keyboard F12
    0x46 => static int KEY_SYSRQ;  // Keyboard Print Screen
    0x47 => static int KEY_SCROLLLOCK;  // Keyboard Scroll Lock
    0x48 => static int KEY_PAUSE;  // Keyboard Pause
    0x49 => static int KEY_INSERT;  // Keyboard Insert
    0x4a => static int KEY_HOME;  // Keyboard Home
    0x4b => static int KEY_PAGEUP;  // Keyboard Page Up
    0x4c => static int KEY_DELETE;  // Keyboard Delete Forward
    0x4d => static int KEY_END;  // Keyboard End
    0x4e => static int KEY_PAGEDOWN;  // Keyboard Page Down
    0x4f => static int KEY_RIGHT;  // Keyboard Right Arrow
    0x50 => static int KEY_LEFT;  // Keyboard Left Arrow
    0x51 => static int KEY_DOWN;  // Keyboard Down Arrow
    0x52 => static int KEY_UP;  // Keyboard Up Arrow
    0x53 => static int KEY_NUMLOCK;  // Keyboard Num Lock and Clear
    0x54 => static int KEY_KPSLASH;  // Keypad /
    0x55 => static int KEY_KPASTERISK;  // Keypad *
    0x56 => static int KEY_KPMINUS;  // Keypad -
    0x57 => static int KEY_KPPLUS;  // Keypad +
    0x58 => static int KEY_KPENTER;  // Keypad ENTER
    0x59 => static int KEY_KP1;  // Keypad 1 and End
    0x5a => static int KEY_KP2;  // Keypad 2 and Down Arrow
    0x5b => static int KEY_KP3;  // Keypad 3 and PageDn
    0x5c => static int KEY_KP4;  // Keypad 4 and Left Arrow
    0x5d => static int KEY_KP5;  // Keypad 5
    0x5e => static int KEY_KP6;  // Keypad 6 and Right Arrow
    0x5f => static int KEY_KP7;  // Keypad 7 and Home
    0x60 => static int KEY_KP8;  // Keypad 8 and Up Arrow
    0x61 => static int KEY_KP9;  // Keypad 9 and Page Up
    0x62 => static int KEY_KP0;  // Keypad 0 and Insert
    0x63 => static int KEY_KPDOT;  // Keypad . and Delete
    0x64 => static int KEY_102ND;  // Keyboard Non-US \ and |
    0x65 => static int KEY_COMPOSE;  // Keyboard Application
    0x66 => static int KEY_POWER;  // Keyboard Power
    0x67 => static int KEY_KPEQUAL;  // Keypad =
    0x68 => static int KEY_F13;  // Keyboard F13
    0x69 => static int KEY_F14;  // Keyboard F14
    0x6a => static int KEY_F15;  // Keyboard F15
    0x6b => static int KEY_F16;  // Keyboard F16
    0x6c => static int KEY_F17;  // Keyboard F17
    0x6d => static int KEY_F18;  // Keyboard F18
    0x6e => static int KEY_F19;  // Keyboard F19
    0x6f => static int KEY_F20;  // Keyboard F20
    0x70 => static int KEY_F21;  // Keyboard F21
    0x71 => static int KEY_F22;  // Keyboard F22
    0x72 => static int KEY_F23;  // Keyboard F23
    0x73 => static int KEY_F24;  // Keyboard F24
    0x74 => static int KEY_OPEN;  // Keyboard Execute
    0x75 => static int KEY_HELP;  // Keyboard Help
    0x76 => static int KEY_PROPS;  // Keyboard Menu
    0x77 => static int KEY_FRONT;  // Keyboard Select
    0x78 => static int KEY_STOP;  // Keyboard Stop
    0x79 => static int KEY_AGAIN;  // Keyboard Again
    0x7a => static int KEY_UNDO;  // Keyboard Undo
    0x7b => static int KEY_CUT;  // Keyboard Cut
    0x7c => static int KEY_COPY;  // Keyboard Copy
    0x7d => static int KEY_PASTE;  // Keyboard Paste
    0x7e => static int KEY_FIND;  // Keyboard Find
    0x7f => static int KEY_MUTE;  // Keyboard Mute
    0x80 => static int KEY_VOLUMEUP;  // Keyboard Volume Up
    0x81 => static int KEY_VOLUMEDOWN;  // Keyboard Volume Down
    0x85 => static int KEY_KPCOMMA;  // Keypad Comma
    0x87 => static int KEY_RO;  // Keyboard International1
    0x88 => static int KEY_KATAKANAHIRAGANA;  // Keyboard International2
    0x89 => static int KEY_YEN;  // Keyboard International3
    0x8a => static int KEY_HENKAN;  // Keyboard International4
    0x8b => static int KEY_MUHENKAN;  // Keyboard International5
    0x8c => static int KEY_KPJPCOMMA;  // Keyboard International6
    0x90 => static int KEY_HANGEUL;  // Keyboard LANG1
    0x91 => static int KEY_HANJA;  // Keyboard LANG2
    0x92 => static int KEY_KATAKANA;  // Keyboard LANG3
    0x93 => static int KEY_HIRAGANA;  // Keyboard LANG4
    0x94 => static int KEY_ZENKAKUHANKAKU;  // Keyboard LANG5
    0xb6 => static int KEY_KPLEFTPAREN;  // Keypad (
    0xb7 => static int KEY_KPRIGHTPAREN;  // Keypad )
    0xe0 => static int KEY_LEFTCTRL;  // Keyboard Left Control
    0xe1 => static int KEY_LEFTSHIFT;  // Keyboard Left Shift
    0xe2 => static int KEY_LEFTALT;  // Keyboard Left Alt
    0xe3 => static int KEY_LEFTMETA;  // Keyboard Left GUI
    0xe4 => static int KEY_RIGHTCTRL;  // Keyboard Right Control
    0xe5 => static int KEY_RIGHTSHIFT;  // Keyboard Right Shift
    0xe6 => static int KEY_RIGHTALT;  // Keyboard Right Alt
    0xe7 => static int KEY_RIGHTMETA;  // Keyboard Right GUI
    0xe8 => static int KEY_MEDIA_PLAYPAUSE; 
    0xe9 => static int KEY_MEDIA_STOPCD; 
    0xea => static int KEY_MEDIA_PREVIOUSSONG; 
    0xeb => static int KEY_MEDIA_NEXTSONG; 
    0xec => static int KEY_MEDIA_EJECTCD; 
    0xed => static int KEY_MEDIA_VOLUMEUP; 
    0xee => static int KEY_MEDIA_VOLUMEDOWN; 
    0xef => static int KEY_MEDIA_MUTE; 
    0xf0 => static int KEY_MEDIA_WWW; 
    0xf1 => static int KEY_MEDIA_BACK; 
    0xf2 => static int KEY_MEDIA_FORWARD; 
    0xf3 => static int KEY_MEDIA_STOP; 
    0xf4 => static int KEY_MEDIA_FIND; 
    0xf5 => static int KEY_MEDIA_SCROLLUP; 
    0xf6 => static int KEY_MEDIA_SCROLLDOWN; 
    0xf7 => static int KEY_MEDIA_EDIT; 
    0xf8 => static int KEY_MEDIA_SLEEP; 
    0xf9 => static int KEY_MEDIA_COFFEE; 
    0xfa => static int KEY_MEDIA_REFRESH; 
    0xfb => static int KEY_MEDIA_CALC; 
    
    // open kb devices
    static KBManager devices[];
    // init flag
    static int isInit;
    
    // starts all keyboard devices
    fun static void initialize()
    {
        // test flag
        if( isInit ) return;
        // set flag
        true => isInit;
        // num to open
        int num;
        Hid tester;
        while( true )
        {
            // try to open keyboard
            if( !tester.openKeyboard( num, true ) ) break;
            // create a new manager
            KBManager kb;
            // start it
            spork ~ kb.start( num );
            // append it
            devices << kb;
            // increment it
            num++;
        }
    }
    
    // get number of KB devices
    fun static int numDevices()
    {
        return devices.size();
    }
    
    // is key down (on any device?)
    fun static int isKeyDown(int keycode)
    {
        // iterate over all open devices
        for( auto device : devices )
            if( device.isKeyDown(keycode) ) return true;
        // not found, the key is not down on any device
        return false;
    }
    
    fun static Event @ keyDownEvent(int keycode, int whichDevice) {
        // in bound?
        if( whichDevice < 0 || whichDevice >= devices.size() ) return null;
        // return event on the specified device
        return devices[whichDevice].keyDownEvent( keycode );
    }
    
    fun static Event @ keyDownEvent(int keycode, string nameSubstr) {
        
        // iterate over all open devices
        for( auto device : devices )
            if( device.name().find(nameSubstr) >= 0 ) return device.keyDownEvent(keycode);
        // no match for name
        return null;
    }
    
    fun static void start()
    {
        // check flag
        if( isInit ) return;
        // HACK just to initialize the pre-ctor
        KB kb;
        // start it
        KB.initialize();
        // keep all child shreds in play
        while( true ) eon => now;
    }
}

// do this once
if( KB.isInit ) me.exit();

// static instantiate (hack)
new KBManager[0] @=> KB.devices;
// start
KB.start();

// KBManager class (not public, used by KB)
class KBManager
{
    // state
    -1 => int lastKeyPressed;
    
    // events
    Event keyDownEvents[0xff];
    Event anyKeyDownEvent;
    
    // name of the device
    string deviceName;
    
    // state vector
    int keyState[0xff];  // keycodes are 1byte, max size is 2^8 = 256
    
    fun int isKeyDown(int keycode) {
        if (keycode < 0 || keycode >= keyState.size()) { return 0; }
        return keyState[keycode];
    }
    
    fun Event @ keyDownEvent(int keycode) {
        return keyDownEvents[keycode];
    }
    
    fun string name() { return deviceName; }
    
    // start listening for kbd input and tracking state
    fun void start(int device) {
        Hid hi;
        HidMsg msg;
        
        // open keyboard (get device number from command line)
        if( !hi.openKeyboard( device ) ) {
            cherr <= "keyboard " + device + " not found" <= IO.newline();
            me.exit();
        }
        <<< "keyboard '" + hi.name() + "' ready", "" >>>;
        hi.name() => this.deviceName;
        
        while( true )
        {
            hi => now;
            while( hi.recv( msg ) )
            {
                if( msg.isButtonDown() )
                {
                    msg.key => lastKeyPressed;
                    1 => keyState[msg.key];
                    keyDownEvents[msg.key].broadcast();
                    anyKeyDownEvent.broadcast();
                }
                else  // button up
                {
                    0 => keyState[msg.key];
                }
            }
        }
    }
}

// KBManager usage
// KBManager IM;
// spork ~ IM.start(0);
// while (true) {
//     if (IM.isKeyDown(KB.KEY_SPACE)) {
//         <<< "space down" >>>;
//     } else {
//         <<< "space up" >>>;
//     }

//     16::ms => now;
// }
