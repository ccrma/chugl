//-----------------------------------------------------------------------------
// name: granular.ck
// desc: interactive visualization of granular synthesis
//       hold left mouse button while moving the cursor
//       x-position for grain position
//       y-position for grain rate (playback speed)
//       uses Andrew's granulator + particles.ck code
// 
// author: Kunwoo Kim (kunwoo@ccrma.stanford.edu)
//         Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//   date: fall 2024
//-----------------------------------------------------------------------------
// scene setup
GG.camera().orthographic();
// fullscreen
GWindow.fullscreen();

// -------- Particle System Parameters --------- //
// particle duration
1 => float lifetime;
// particle geometry
CircleGeometry particle_geo;

// particle colors (Homer Simpson color codes)
@(84, 175, 216) / 255.0 * 0.2 => GG.scene().backgroundColor;
[@(245, 217, 66) / 255.0, 
 @(245, 217, 66) / 255.0, 
 @(245, 217, 66) / 255.0, 
 @(131, 200, 249) / 255.0, 
 @(202, 173, 112) / 255.0, 
 @(58, 75, 71) / 255.0, 
 @(1, 1, 1)] @=> vec3 particleColors[];

// -------- Granular Synthesis -------- //
// sample file
"special:dope" => string filepath;
// initialize "granulator"
Granulator gran;
gran.init(filepath);
gran.spork_interp();
spork ~ gran.granulate();
gran.mute();

// particle setup
Gain main_gain(1) => dac;
// rather than continuous random size, discrete random size is more effective
[0, 2, 4, 6, 8, 10, 12, 14, 16, 18] @=> int particleSizePool[]; 

// initialize particle system
256 => int PARTICLE_POOL_SIZE;
// array of custom particles (see class Particle below)
Particle particles[PARTICLE_POOL_SIZE];
// particle system (see class ParticleSystem below)
ParticleSystem ps;

// main render loop
fun void graphical_loop()
{
    while (true)
    {
        // sychronize audio and graphics
        GG.nextFrame() => now;

        // unmute granular synthesis once left mouse button is down
        if (GWindow.mouseLeftDown()) {
            0 => gran.MUTED;
            spork~gran.unmute(100::ms);  
        }
        // stop spawning if left mouse button is up
        else if (GWindow.mouseLeftUp())
        {
            1 => gran.MUTED;
            spork~gran.mute(300::ms);
        }
        // keep spawning particles as long as left mouse button remains down
        else if (GWindow.mouseLeft())
        {
            ps.spawnParticle(GG.camera().screenCoordToWorldPos(GWindow.mousePos(), 1.0));
        }

        // manually update the particle system with delta time
        ps.update(GG.dt());
    }
}
spork ~graphical_loop();

// track mouse
fun void mouseTracker()
{
    1.0 => float distance_from_camera;
    vec3 mousePos;
    
    while (true)
    {
        // mouseRange = windowSize
        GWindow.windowSize().x => float mouseRangeX;
        GWindow.windowSize().y => float mouseRangeY;
        
        // save mouse position
        GWindow.mousePos() => mousePos;
        15::ms => now;

        // map x position to grain position
        Math.remap(mousePos.x, 0.0, mouseRangeX, 0.0, 0.7) => float p;
        p => gran.GRAIN_POSITION;

        // map y position to grain rate (mousePos = 0 when mouse is on the top)
        mousePos.y => float r;

        // if mouse is below halfway point
        if (r > mouseRangeY/2)
            Math.remap(r, mouseRangeY/2, mouseRangeY, 0.5, 0.0) => gran.GRAIN_PLAY_RATE;
        // if mouse is above halfway point
        else
            Math.remap(r, mouseRangeY/2, 0, 0.5, 2.0) => gran.GRAIN_PLAY_RATE;
    }
}
spork ~mouseTracker();

//------------CLASSES------------//
// class for particle
class Particle
{
    // set up particle mesh
    FlatMaterial particle_mat;
    GMesh particle_mesh(particle_geo, particle_mat) --> GG.scene();
    0 => particle_mesh.sca;

    // select random size
    Math.random2(0, particleSizePool.size() - 1) => int pSize;

    // particle properties
    // - random direction
    @(0,1) => vec2 direction;
    // - spawn time
    time spawn_time;
    // - default color is white
    Color.WHITE => vec3 color;
}

// class ParticleSystem (adopted from Andrew's code)
class ParticleSystem
{
    0 => int num_active;

    // update function
    fun void update(float dt)
    {
        // update particles
        for (0 => int i; i < num_active; i++) {
            particles[i] @=> Particle p;

            // swap despawned particles to the end of the active list
            if (now - p.spawn_time >= lifetime::second) {
                0 => p.particle_mesh.sca;
                num_active--;
                particles[num_active] @=> particles[i];
                p @=> particles[num_active];
                i--;
                continue;
            }

            // update particle
            {
                // update size (based on midi)
                Math.remap(p.pSize, 0, 18, 0.3, 1) => float size_factor;
                Math.pow((now - p.spawn_time) / lifetime::second, 2) => float t;
                size_factor * (1 - t) => p.particle_mesh.sca;

                // update position
                (dt * p.direction).x => p.particle_mesh.translateX;
                (dt * p.direction).y => p.particle_mesh.translateY;
            }
        }
    }

    fun void spawnParticle(vec3 pos) {
        if (num_active < PARTICLE_POOL_SIZE) {
            particles[num_active] @=> Particle p;
            
            // set color
            particleColors[Math.random2(0, particleColors.size() - 1)] => p.particle_mat.color;

            // set random direction
            Math.random2f(0, 2 * Math.PI) => float random_angle;
            @(Math.cos(random_angle), Math.sin(random_angle)) => p.direction;

            now => p.spawn_time;
            pos => p.particle_mesh.pos;
            num_active++;
        }
    }
}

// class Granulator (adopted from Andrew's code)
class Granulator
{
  // overall volume
  1 => float MAIN_VOLUME;
  // grain duration base
  200::ms => dur GRAIN_LENGTH;
  // how much overlap when firing
  32 => float GRAIN_OVERLAP;
  // factor relating grain duration to ramp up/down time
  .5 => float GRAIN_RAMP_FACTOR;
  // playback rate
  1 => float GRAIN_PLAY_RATE;
  0 => float GRAIN_PLAY_RATE_OFF;
  1. => float GRAIN_SCALE_DEG;
  1. =>  float RATE_MOD;  // for samples not on "C"

  // grain position (0 start; 1 end)
  .3 => float GRAIN_POSITION;
  // grain position goal (for interp)
  GRAIN_POSITION => float GRAIN_POSITION_GOAL;
  // grain position randomization
  .01 => float GRAIN_POSITION_RANDOM;
  // grain jitter (0 == periodic fire rate)
  3 => float GRAIN_FIRE_RANDOM;
  /* 0 => float GRAIN_FIRE_RANDOM; */

  // max lisa voices
  30 => int LISA_MAX_VOICES;
  string sample;

  0 => int MUTED;
  float SAVED_GAIN;

  /* SndBuf @ buffy; */
  LiSa @ lisa;
  PoleZero @ blocker;
  NRev @ reverb;
  ADSR @ adsr;

  // init function
  fun void init(string filepath)
  {    
    // load file into a LiSa (use one LiSa per sound)
    filepath => this.sample;
    this.load(filepath) @=> this.lisa;

    PoleZero p @=> this.blocker;
    NRev r @=> this.reverb;
    ADSR e @=> this.adsr;
    // reverb mix
    .05 => this.reverb.mix;
    // pole location to block DC and ultra low frequencies
    .99 => this.blocker.blockZero;
    this.lisa.chan(0) => this.blocker => this.reverb => dac;
  }

  fun void spork_interp() {
    spork ~ interp( .025, 5::ms );
  }

  // interp  TODO: interp the drone pitch?
  fun void interp( float slew, dur RATE )
  {
      while( true )
      {
          // interp grain position
          (this.GRAIN_POSITION_GOAL - this.GRAIN_POSITION)*slew + this.GRAIN_POSITION => this.GRAIN_POSITION;
          // interp rate
          RATE => now;
      }
  }

  fun void granulate() {
    while( true ) {
      // fire a grain
      fireGrain();
      // amount here naturally controls amount of overlap between grains
      GRAIN_LENGTH / GRAIN_OVERLAP + Math.random2f(0,GRAIN_FIRE_RANDOM)::ms => now;
    }
  }

  fun void mute() {
    <<< "muting" >>>;
    //1 => this.MUTED;
    if (this.lisa.gain() < .01) { return; }
    this.lisa.gain() => SAVED_GAIN;
    0 => this.lisa.gain;
  }

  fun void mute(dur du) {
    <<< "muting" >>>;
    //1 => this.MUTED;
    if (this.lisa.gain() < .01) { return; }
    this.lisa.gain() => SAVED_GAIN;
    now + du => time later;
    while (now < later) {
      ((later - now) / du) => this.lisa.gain;
      1::ms => now;
    }
  }

  fun void unmute(dur du) {
    <<< "unmuting" >>>;
    //0 => this.MUTED;
    if (this.lisa.gain() > .01) { return; }
    now + du => time later;
    while (now < later) {
      SAVED_GAIN * (1 - ((later - now) / du)) => this.lisa.gain;
      1::ms => now;
    }
  }

  // fire!
  fun void fireGrain()
  {
      // grain length
      GRAIN_LENGTH => dur grainLen;
      // ramp time
      GRAIN_LENGTH * GRAIN_RAMP_FACTOR => dur rampTime;
      // play pos
      //GRAIN_POSITION + Math.random2f(0,GRAIN_POSITION_RANDOM) => float pos;
      GRAIN_POSITION => float pos;
      // a grain
      if( this.lisa != null && pos >= 0 )
          spork ~ grain(pos * this.lisa.duration(), grainLen, rampTime, rampTime,
          GRAIN_PLAY_RATE, GRAIN_PLAY_RATE_OFF, GRAIN_SCALE_DEG);
  }

  // grain sporkee
  fun void grain(dur pos, dur grainLen, dur rampUp, dur rampDown, float rate, float off, float deg )
  {
      // get a voice to use
      this.lisa.getVoice() => int voice;

      // if available
      if( voice > -1 )
      {
          // set rate
          this.lisa.rate( voice, rate );
          // set playhead
          this.lisa.playPos( voice, pos );
          // ramp up
          this.lisa.rampUp( voice, rampUp );
          // wait
          (grainLen - rampUp) => now;
          // ramp down
          this.lisa.rampDown( voice, rampDown );
          // wait
          rampDown => now;
      }
  }

// load file into a LiSa
fun LiSa load( string filename )
{
    // sound buffer
    SndBuf buffy;
    // load it
    filename => buffy.read;

    // new LiSa
    LiSa lisa;
    // set duration
    buffy.samples()::samp => lisa.duration;

    // transfer values from SndBuf to LiSa
    for( 0 => int i; i < buffy.samples(); i++ )
    {
        // args are sample value and sample index
        // (dur must be integral in samples)
        lisa.valueAt( buffy.valueAt(i  * buffy.channels()), i::samp );
    }

    // set LiSa parameters
    lisa.play( false );
    lisa.loop( false );
    lisa.maxVoices( LISA_MAX_VOICES );

    return lisa;
    }
}

// keep going!
while( true ) 1::second => now;
