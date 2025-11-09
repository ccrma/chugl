// @import "../lib/whisper/Whisper.chug"
// @import "../lib/g2d/ChuGL.chug"

public class VoiceCommand {
    null @=> Whisper@ w;
    // disable logging by default
    Whisper.log(Whisper.LOG_LEVEL_ERROR);


    string transcription_raw;
    string transcription_parsed; // the current parsed / sanitized transcription
    StringTokenizer transcription_tokenizer;
    float last_transcribe_time_ms;

    int command_list[0]; // @TODO prob don't need this variable
    string command_words[0][0];

    int command_counts_cumulative[0]; // track transcribed tokens so we don't over-cast
    int command_counts_per_frame[0]; // used to count every frame

    fun void _transcriberShred() {
        500::ms => dur step;
        step => now;
        while (1) {
            now + step => time later;
            w.transcribe() => now;
            w.text() => transcription_raw;

            { // parse and cleanup
                transcription_raw.trim().lower() => transcription_parsed;
                // search for and remove punctuation
                transcription_parsed.replace(",", "");
                transcription_parsed.replace("!", "");
                transcription_parsed.replace(".", "");
                transcription_parsed.replace("?", "");
                transcription_tokenizer.set(transcription_parsed);
            }

            if (w.wasContextReset()) command_counts_cumulative.zero();

            (now - (later - step)) / ms => last_transcribe_time_ms;

            if (now < later) later - now => now; // wait for next step
        }
    }

// =========== public API ====================
    fun VoiceCommand(string model_path) {
        adc => Whisper w(model_path) => blackhole;
        w @=> this.w;
        spork ~ this._transcriberShred();
    }

    fun void add(int command_type, string words[]) {
        if (command_list.size() != command_type) {
            <<< "VoiceCommand error: adding command out of order. Expecting index", command_list.size(), "got", command_type >>>; 
            return;
        }
        this.command_list << command_type;
        this.command_words << words;
        this.command_counts_cumulative << 0;
        this.command_counts_per_frame << 0;
    }

    fun void ui() {
        if (!UI.collapsingHeader("Whisper", 0)) return;

        UI.textColored(@(1, 1, 0, 1), "Transcription: ");
        UI.sameLine();
        UI.textWrapped(transcription_raw);

        UI.textColored(@(1, 1, 0, 1), "Transcription Time: ");
        UI.sameLine();
        UI.text(last_transcribe_time_ms + "(ms)");

        UI.textColored(@(0, 1, 0, 1), "Parsed: ");
        UI.sameLine();
        UI.text(transcription_parsed);

        UI.textColored(@(0, 1, 0, 1), "Tokens: ");
        while( transcription_tokenizer.more() )
        {
            UI.text("\t" + transcription_tokenizer.next());
        }
        transcription_tokenizer.reset();
    }

    // return array of activated commands (call once per frame)
    fun int[] activated() {
        int ret[0];
        // count all active command tokens this frame
        // @optimize use hashmap for string comparison, invert loop order so we only need to walk the
        // command list once
        for (int i; i < transcription_tokenizer.size(); i++) {
            for (int command_idx; command_idx < command_words.size(); command_idx++) {
                for (auto word : command_words[command_idx]) {
                    if (transcription_tokenizer.get(i) == word) {
                        command_counts_per_frame[command_idx]++;
                    }
                }
            }
        }
        transcription_tokenizer.reset();

        // cast!
        for (int i; i < command_words.size(); i++) {
            if (command_counts_per_frame[i] > command_counts_cumulative[i]) {
                ret << i;
                command_counts_per_frame[i] => command_counts_cumulative[i];
            }
        }
        command_counts_per_frame.zero();
        return ret;
    }
}


// unit test
if (0) { 
    VoiceCommand vc(me.dir() + "../lib/whisper/models/ggml-base.en.bin");
    vc.add(0, ["apple", "banana"]);
    vc.add(1, ["cheese", "dog", "enchilada"]);

    for (auto words : vc.command_words) {
        <<< words.size() >>>;
        for (auto w : words) {
            <<< w >>>;
        }
    }
    while (1) {
        GG.nextFrame() => now;
        vc.ui();
        vc.activated() @=> int activated[];
        for (int c : activated) <<< "activating command", c >>>;
    }
}
