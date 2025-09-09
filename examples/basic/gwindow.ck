//--------------------------------------------------------------------
// name: gwindow.ck
// desc: window management demo
//       (the screen is blank; see below for interactions)
// requires: ChuGL + chuck-1.5.5.5 or higher
//
// press '1' to go fullscreen.
// press '2' to go windowed.
// press '3' to maximize (windowed fullscreen).
// press '4' to iconify / restore
// press '5' to toggle window opacity
// press '6' to toggle mouse mode (normal - hidden - disabled)
//
// listens for keyboard input on space key
// listens for drag and drop and returns a list of filepaths
// 
// author: Andrew Zhu Aday
//   date: June 2024
//--------------------------------------------------------------------

// uncomment to disable <esc> and close button; allows you to manually
// handle close events and perform any necessary cleanup, e.g. saving game state
// GWindow.closeable( false ); 

// uncomment to set size limits
// GWindow.sizeLimits(100, 100, 1920, 1080, @(16, 9));

// initial window dimensions
GWindow.windowed(1200, 675);
// center window on screen
GWindow.center();
// give your window a title!
"Hello GWindow" => string title;
GWindow.title(title);

//--------------------------------------------------------------------
// window configurations
// NOTE: must be called BEFORE first GG.nextFrame(), which creates the
//       ChuGL window
//--------------------------------------------------------------------
// window transparency (default: false)
true => GWindow.transparent;
// whether window is permanently on top of everything (default: false)
false => GWindow.floating;
// whether window has top bar containing close/minimize/maximize (default: true)
true => GWindow.decorated;
// can the window be resized by the user (default: true)
true => GWindow.resizable;

// set mouse mode
GWindow.mouseMode( GWindow.MOUSE_NORMAL );
// GWindow.mouseMode( GWindow.MOUSE_DISABLED );
// GWindow.mouseMode( GWindow.MOUSE_HIDDEN );

// add a text object to scene
GText text --> GG.scene();
// text size
text.size(.2);
// set text
text.text("drag and drop files here!");

// mouse input listener
fun void mouseListener()
{
    // render loop
    while (true)
    {
        // synchronize
        GG.nextFrame() => now;
        // test for mouse buttons
        // also prints mouse cursor position in window when left mouse button is held
        if (GWindow.mouseLeft())
        { <<< "left mouse button held", "cursor pos in windows:", GWindow.mousePos(), "delta", GWindow.mouseDeltaPos() >>>; }
        if (GWindow.mouseRight()) <<< "right mouse button held" >>>;
        if (GWindow.mouseLeftDown()) <<< "left mouse button pressed" >>>;
        if (GWindow.mouseRightDown()) <<< "right mouse button pressed" >>>;
        if (GWindow.mouseLeftUp()) <<< "left mouse button released" >>>;
        if (GWindow.mouseRightUp()) <<< "right mouse button released" >>>;
        
        // scroll (e.g., two-finger scroll or mouse wheel)
        GWindow.scroll() => vec2 scroll;
        if( scroll.magnitude() > 0 ) <<< "scroll delta:", scroll >>>;
    }
}
// spork the mouse listener
spork ~ mouseListener();

// keyboard listener
fun void kbListener()
{
    // render loop
    while (true)
    {
        // synchronize
        GG.nextFrame() => now;
        // how to text for a particular key (e.g., space bar)
        if (GWindow.key(GWindow.KEY_SPACE)) <<< "space key held" >>>;
        if (GWindow.keyDown(GWindow.KEY_SPACE)) <<< "space key pressed" >>>;
        if (GWindow.keyUp(GWindow.KEY_SPACE)) <<< "space key released" >>>;
    }
}
// spork keyboard listener
spork ~ kbListener();

// listener for window closing
fun void closeCallback() {
    while (true) {
        // listening for event closing event
        GWindow.closeEvent() => now;
        // print
        <<< "user tried to close window" >>>;
        // NOTE: add any cleanup / state save here
        // closes the window
        GWindow.close();
    }
}
// spork the window closing
spork ~ closeCallback();

// listener for window resizing
fun void resizeCallback() {
    while (true) {
        GWindow.resizeEvent() => now;
        <<< "user resized window | window size: ", GWindow.windowSize(), " | framebuffer size: ", GWindow.framebufferSize() >>>;
    }
} spork ~ resizeCallback(); // spork it

// listener for content resizing
// NOTE: this typically doesn't happen except when, say, dragging a window to a different monitor with a display resolution
fun void contentScaleCallback() {
    while (true) {
        GWindow.contentScaleEvent() => now;
        <<< "content scale changed | content scale: ", GWindow.contentScale() >>>;
    }
} spork ~ contentScaleCallback(); // spork it

// an array for getting dragged-and-dropped filepaths
GWindow.files() @=> string files[];
// render loop
while (true)
{
    // synchronize 
    GG.nextFrame() => now;

    // update window title
    GWindow.title(title + " | Frame: " + GG.fc());

    // detect when files are dropped onto this window
    if (GWindow.files() != files) {
        GWindow.files() @=> files;
        string newly_dropped_files;
        // concatenate into a single string for display
        for (auto filepath : files) {
            (filepath + "\n") +=> newly_dropped_files;
        }
        // set the text to be displayed
        text.text(newly_dropped_files);
    }

    // '1' to enter fullscreen
    if (UI.isKeyPressed(UI_Key.Num1)) GWindow.fullscreen();
    // '2' to entered windowed mode
    if (UI.isKeyPressed(UI_Key.Num2)) {
        GWindow.windowed(800, 600);
        GWindow.center();
    }
    // '3' to maxmize window
    if (UI.isKeyPressed(UI_Key.Num3)) {
        // disable size limits
        GWindow.sizeLimits(0, 0, 0, 0, @(0, 0));
        GWindow.maximize();
    }
    // '4' to minimize window (AND come back after 60 frames)
    if (UI.isKeyPressed(UI_Key.Num4)) {
        // minimize
        GWindow.iconify();
        // wait a bit
        repeat (60) {
            GG.nextFrame() => now;
        }
        // restore
        GWindow.restore();
    }
    // '5' to toggle transparency
    if (UI.isKeyPressed(UI_Key.Num5)) {
        if (GWindow.opacity() > 0.5) {
            GWindow.opacity(0.4);
        } else {
            GWindow.opacity(1.0);
        }
    }
}
