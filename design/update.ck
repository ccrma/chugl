public class Helicopter extends GGen {

	Mesh helicopterBody -> output;  // similar to chugraph
	Mesh helicopterBlade -> output;

	0.0 => float bladRotationSpd;
	
	fun void update(dur dt) {
		// rotate blade
		helicopterBlade.RotateOnLocalAxis(UP, bladeRotationSpd*0.01);
	}

	fun void render() {
	}

}

Helicopter heli -> GGroot;

tween(heli.bladeRotation, 360.0, 1.0::second);

while (10::ms => now) {
	gt.x => heli.bladRotationSpd;
}