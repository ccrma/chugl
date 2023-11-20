// -------------------------------------------------------------
// name: tween-storming.ck
// desc: code we might like to write for tweens
//       some questions raised: closure? generics? comptime?
//
// author: Andrew Aday Zhu
//         Ge Wang
// date: fall 2023 | 2023.11.17
// -------------------------------------------------------------

GSphere sphere;
GSphere sphere2;
GCube cube;
SinOsc bar => dac;

vec3 start;
vec3 end;

float s;
float e;

Tween.LINEAR => fun float curve(float);

class TweenAction
{
    fun float curve( float input )
    {
        return input;
    }
}

template<T,F,D>
class TweenSetter
{
    F func;
    
    fun void set( T type, D data )
    {
        func(type, data);
    }
}

// ?? TweenSetter<GGen,vec3>(GGen.pos);
// ?? spork ~ Tween<T,D>( , D start, D end, TweenAction curve, dur d ).go( T );

spork ~ Tween( spher.pos, start, end, curve, 2::second ).go();
spork ~ Tween( cube.pos, start, end, curve, 2::second ).go();

Tween( bar.freq, s, e, curve, 2::second, 10::ms ) utween;

// the closure way
spork ~ Tween( (vec3 p){ p => sphere.pos; }, start, end, (float in){ return in; }, 2::second ).go();
spork ~ Tween( (vec3 p){ p => sphere2.pos; }, start, end, (float in){ return in; }, 2::second ).go();
spork ~ Tween( (vec3 p){ p => cube.pos; }, start, end, (float in){ return in; }, 2::second ).go();

// the generics way
Tween tween( GGen.pos, start, end, curve, 2::second );
spork tween.go( sphere );
spork tween.go( spher2 );
spork tween.go( cube );

// directly spork
spork ~ Tween( Sphere.pos, start, end, curve, 2::second ).go( sphere );
spork ~ Tween( GGen.pos, start, end, curve, 2::second ).go( sphere2 );
spork ~ Tween( GGen.pos, start, end, curve, 2::second ).go( cube );
