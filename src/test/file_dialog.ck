SinOsc a => dac;

    GG.openFileDialogAsync() @=> OpenFileEvent e;
    e => now;
    <<< "opened files: ">>>;
    for (auto file : e.files)
        <<< file >>>;


while (1) GG.nextFrame() => now;