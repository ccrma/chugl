//-----------------------------------------------------------------------------
// name: vocoder.ck
// desc: basic visualization of vocoder made by Orchisama Das (https://ccrma.stanford.edu/~orchi/220a/final_project.html)
//       takes keyboard input to play polyphonic FM sounds and mix with magnitude spectrum of the microphone.
//       Press on the keyboard (mapped on the piano key) to change pitch
//       Works best when mic and speakers are separated so you don't get feedback   
// 
// author: Kunwoo Kim (kunwoo@ccrma.stanford.edu)
//         Orchisama Das (for vocoder)
//   date: Fall 2024
//-----------------------------------------------------------------------------

// ------------ VOCODER PARAMETERS ------------- //
// connect mic input to FFT through a DC blocker
adc => PoleZero dcblock_in => FFT mic_fft => blackhole;
0.5 => adc.gain;
// connect inverse FFT through DC blocker and gain UGen to dac
PoleZero dcblock_out; 
Gain gain_out;

// set polezero position of DC blocker
0.999 => dcblock_in.blockZero;
0.999 => dcblock_out.blockZero;
0.02 => gain_out.gain;

// first note's midi number
60 => int startPitch;

// Polyphonic FM synth
13   => int maxVoices;
// change this line to whatever FM instrument you like
BeeThree synth[maxVoices];
FFT synth_fft[maxVoices];
IFFT out_ifft[maxVoices];

// ------------ GRAPHICS PIANO ROLL ------------- //
piano pianoRoll --> GG.scene();
pianoRoll.sca(@(30, 30, 1));
pianoRoll.pos(@(-22, -35, -50));

// vocoder initialization
for(0 => int i; i< maxVoices; i++){
    synth[i] => synth_fft[i] => blackhole;
    out_ifft[i] => dcblock_out => gain_out => dac;
    //to also hear synth, uncomment the following lines
    //synth[i] => dac;
    //0.1 => synth[i].gain;
}

// an array to hold the note numbers so that we can match them up with the off signal
int id[maxVoices];
int  counter;

// play synth sound
public void playSynth(int note, float vel, int pos){
    //<<<note, vel>>>;
    Std.mtof(note) => synth[pos].freq;
    vel => synth[pos].noteOn;
}

// stop synth sound
public void stopSynth(int pos){
    0 => synth[pos].noteOn;
}

// change FM depth
public void changeFMDepth(int depth){
    for(0 => int i; i< maxVoices; i++)
        depth/128.0 => synth[i].lfoDepth;
}

// change FM Modulation
public void changeFMModulation(int speed){
   for(0 => int i; i< maxVoices; i++)
      (speed/128.0) * 10 => synth[i].lfoSpeed;
}

// Map keyboard input
fun void midiSynth()
{
    while (true)
    {
        // counter is used for convenience
        0 => counter;
        // whenever A is pressed and held, play sound on corresponding pitch
        if (GWindow.key(GWindow.Key_A))
        {
            // assign pitch
            startPitch + counter => id[counter];
            // play synth
            playSynth(id[counter], 0.00000001, counter);
            // change keyboard color to skyblue
            pianoRoll.key[counter].color(Color.SKYBLUE);
        }
        else
        {
            // stop synth
            stopSynth(counter);
            // reset id
            0=>id[counter];    
            // change keyboard color to white
            pianoRoll.key[counter].color(Color.WHITE);
        }

        // COPY AND PASTE!!!
        1 => counter;
        if (GWindow.key(GWindow.Key_W))
        {
            startPitch + counter => id[counter];
            playSynth(id[counter], 0.00000001, counter);
            pianoRoll.key[counter].color(Color.SKYBLUE);
        }
        else
        {
            stopSynth(counter);
            0=>id[counter];    
            pianoRoll.key[counter].color(Color.BLACK);
        }
        
        2 => counter;
        if (GWindow.key(GWindow.Key_S))
        {
            startPitch + counter => id[counter];
            playSynth(id[counter], 0.00000001, counter);
            pianoRoll.key[counter].color(Color.SKYBLUE);
        }
        else
        {
            stopSynth(counter);
            0=>id[counter];    
            pianoRoll.key[counter].color(Color.WHITE);
        }
        
        3 => counter;
        if (GWindow.key(GWindow.Key_E))
        {
            startPitch + counter => id[counter];
            playSynth(id[counter], 0.00000001, counter);
            pianoRoll.key[counter].color(Color.SKYBLUE);
        }
        else
        {
            stopSynth(counter);
            0=>id[counter];    
            pianoRoll.key[counter].color(Color.BLACK);
        }
        
        4 => counter;
        if (GWindow.key(GWindow.Key_D))
        {
            startPitch + counter => id[counter];
            playSynth(id[counter], 0.00000001, counter);
            pianoRoll.key[counter].color(Color.SKYBLUE);
        }
        else
        {
            stopSynth(counter);
            0=>id[counter];    
            pianoRoll.key[counter].color(Color.WHITE);
        }
        
        5 => counter;
        if (GWindow.key(GWindow.Key_F))
        {
            startPitch + counter => id[counter];
            playSynth(id[counter], 0.00000001, counter);
            pianoRoll.key[counter].color(Color.SKYBLUE);
        }
        else
        {
            stopSynth(counter);
            0=>id[counter];    
            pianoRoll.key[counter].color(Color.WHITE);
        }
        
        6 => counter;
        if (GWindow.key(GWindow.Key_T))
        {
            startPitch + counter => id[counter];
            playSynth(id[counter], 0.00000001, counter);
            pianoRoll.key[counter].color(Color.SKYBLUE);
        }
        else
        {
            stopSynth(counter);
            0=>id[counter]; 
            pianoRoll.key[counter].color(Color.BLACK);   
        }
        
        7 => counter;
        if (GWindow.key(GWindow.Key_G))
        {
            startPitch + counter => id[counter];
            playSynth(id[counter], 0.00000001, counter);
            pianoRoll.key[counter].color(Color.SKYBLUE);
        }
        else
        {
            stopSynth(counter);
            0=>id[counter];    
            pianoRoll.key[counter].color(Color.WHITE);
        }
        
        8 => counter;
        if (GWindow.key(GWindow.Key_Y))
        {
            startPitch + counter => id[counter];
            playSynth(id[counter], 0.00000001, counter);
            pianoRoll.key[counter].color(Color.SKYBLUE);
        }
        else
        {
            stopSynth(counter);
            0=>id[counter];
            pianoRoll.key[counter].color(Color.BLACK);    
        }
        
        9 => counter;
        if (GWindow.key(GWindow.Key_H))
        {
            startPitch + counter => id[counter];
            playSynth(id[counter], 0.00000001, counter);
            pianoRoll.key[counter].color(Color.SKYBLUE);
        }
        else
        {
            stopSynth(counter);
            0=>id[counter];    
            pianoRoll.key[counter].color(Color.WHITE);
        }
        
        10 => counter;
        if (GWindow.key(GWindow.Key_U))
        {
            startPitch + counter => id[counter];
            playSynth(id[counter], 0.00000001, counter);
            pianoRoll.key[counter].color(Color.SKYBLUE);
        }
        else
        {
            stopSynth(counter);
            0=>id[counter];
            pianoRoll.key[counter].color(Color.BLACK);    
        }
        
        11 => counter;
        if (GWindow.key(GWindow.Key_J))
        {
            startPitch + counter => id[counter];
            playSynth(id[counter], 0.00000001, counter);
            pianoRoll.key[counter].color(Color.SKYBLUE);
        }
        else
        {
            stopSynth(counter);
            0=>id[counter];    
            pianoRoll.key[counter].color(Color.WHITE);
        }
        
        12 => counter;
        if (GWindow.key(GWindow.Key_K))
        {
            startPitch + counter => id[counter];
            playSynth(id[counter], 0.00000001, counter);
            pianoRoll.key[counter].color(Color.SKYBLUE);
        }
        else
        {
            stopSynth(counter);
            0=>id[counter];    
            pianoRoll.key[counter].color(Color.WHITE);
        }
        
        10::ms => now;
    }
}
    
fun void vocoder(){
 //the plan here is to calculate take the magnitude spectrum of each synth voice individually and change it according to the mic input
 //initialise fft size, window size, hop size
 512 => int FFT_SIZE => mic_fft.size;
 FFT_SIZE/2 => int WIN_SIZE;
 WIN_SIZE/2 => int HOP_SIZE;
 for(0 => int i; i < maxVoices; i++){
     FFT_SIZE => synth_fft[i].size => out_ifft[i].size;
     Windowing.hann(WIN_SIZE) => synth_fft[i].window => out_ifft[i].window;
 }
 
Windowing.hann(WIN_SIZE) => mic_fft.window;
complex spectrum_synth[WIN_SIZE];
complex spectrum_mic[WIN_SIZE];
polar temp_polar_synth;
polar temp_polar_mic;


while(true){

 //modulate magnitude spectrum of FMSynth with mic input
  mic_fft.upchuck();
  mic_fft.spectrum(spectrum_mic);
 
  for(0 => int i; i < maxVoices; i++){
     synth_fft[i].upchuck();
     synth_fft[i].spectrum(spectrum_synth);
     for(0 => int j; j < WIN_SIZE;j++){
         spectrum_synth[j]$polar => temp_polar_synth;
         spectrum_mic[j]$polar => temp_polar_mic;
         
         temp_polar_mic.mag => temp_polar_synth.mag;
         temp_polar_synth$complex => spectrum_synth[j];
     }
     out_ifft[i].transform(spectrum_synth);
     //the next if statement is VERY important
     //if there are no midi messages associated with a voice, chuck gives it a 
     //default frequency of 220Hz, which overpowers all other voices, and what 
     //we hear is a constant vocoder sound not changing pitch when you change   	     //notes on the MIDI keyboard.
     if(id[i] == 0)
         0 => out_ifft[i].gain;
     else
         0.2 => out_ifft[i].gain;
 }
     HOP_SIZE :: samp => now;  
 
}
}

spork ~ midiSynth();
spork ~ vocoder();

// Draw Piano Keyboard
@(1.0, 1.0, 1.0) * 0.05 => GG.scene().backgroundColor;

class piano extends GGen
{
     int doubleWhite;
     int offset;
     GPlane key[13];
     GText text[13];
     
     text[0].text("A");
     text[1].text("W");
     text[2].text("S");
     text[3].text("E");
     text[4].text("D");
     text[5].text("F");
     text[6].text("T");
     text[7].text("G");
     text[8].text("Y");
     text[9].text("H");
     text[10].text("U");
     text[11].text("J");
     text[12].text("K");
 
     [0, 2, 4, 5, 7, 9, 11, 12] @=> int whiteKey[];
     
     int counter;
     for (auto s : key)
     {
         s --> this;
         text[counter] --> this;
         s.sca(@(0.2, 1, 1));
         
         1 => int blackWhite;
         
         for (int i; i < whiteKey.size(); i++)
         {
             if (counter == whiteKey[i])
             {
                 0 => blackWhite;
             }
         }
         
         // if black key
         if (blackWhite)
         {
             0 => doubleWhite;
             s.color(Color.BLACK);
             s.pos(@((counter + offset) * 0.108, 1.42, -5));
             s.sca(@(0.2, 0.6, 1));
             text[counter].pos(s.pos() + @(0, 0.2, 0.2));
             text[counter].sca(.1);
         }
         else
         {
             if (doubleWhite)
             {
                 offset++;
             }
             1 => doubleWhite;
             s.color(Color.WHITE);
             s.pos(@((counter + offset) * 0.108, 1.2, -5.1));
             s.scaY(1.1);
             text[counter].pos(s.pos() + @(0, -0.45, 0.5));
             text[counter].sca(0.1);
         }
         
         counter++;
 }  
}

// main game loop
while(true)
{
    // synchronize graphics
    GG.nextFrame() => now;
}
