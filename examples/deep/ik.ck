//-----------------------------------------------------------------------------
// name: ik.ck
// desc: inverse kinematics
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
// adapted from: http://www.andreasaristidou.com/FABRIK.html
//   date: Fall 2024
//-----------------------------------------------------------------------------

// geometry
SphereGeometry sphere_geo;
// material
FlatMaterial flat_material;

// number of lengths
40 => int NUM_LENGTHS;
float lengths[NUM_LENGTHS];
for (int i; i < NUM_LENGTHS; i++) {
    4.0 / NUM_LENGTHS => lengths[i];
}
// N points
vec3 points[lengths.size() + 1];

// joints
GMesh joints[0];
for (int i; i < points.size(); i++) {
    GMesh joint(sphere_geo, flat_material) --> GG.scene();
    joints << joint;
    .1 => joint.sca;
}

// camera projection
GG.scene().camera().orthographic();

// FABRIK == forwards-and-backwards-reaching-inverse-kinematics
fun void fabrik(vec3 start_target, vec3 end_target, vec3 points[], float lengths[])
{
    0.01 => float TOLERANCE;
    10 => int MAX_ITERATIONS;
    0 => float total_length;
    for (0 => int i; i < lengths.size(); i++) {
        lengths[i] +=> total_length;
    }
    // case: end target out of reach
    end_target - start_target => vec3 dir;
    if (total_length < dir.magnitude()) {
        dir.normalize();
        // set all points to point towards end_target
        for (1 => int i; i < points.size(); i++) {
            dir * lengths[i - 1] + points[i - 1] => points[i];
        }
        return;
    }

    points.size() => int N;
    
    // iterate until convergence or max reached
    repeat (MAX_ITERATIONS) {
        if (Math.euclidean(points[points.size() - 1], end_target) < TOLERANCE) {
            return;
        }

        // forward
        end_target => points[N-1]; // put last point at target
        for (N - 2 => int i; i >= 0; i--) {
            points[i] - points[i+1] => vec3 dir;
            dir.normalize();
            points[i + 1] + dir * lengths[i] => points[i];
        }

        // backwards
        start_target => points[0];
        for (1 => int i; i < points.size() - 1; i++) {
            points[i] - points[i-1] => vec3 dir; 
            dir.normalize();
            lengths[i-1] * dir + points[i-1] => points[i];
        }
    }
}

// game loop
while (true)
{
    // synchronize
    GG.nextFrame() => now;
    // get mouse position
    GG.scene().camera().screenCoordToWorldPos(GWindow.mousePos(), 5) => vec3 end_target;
    // position on plane
    0 => end_target.z;
    // run IK solver
    fabrik(@(0, 0, 0), end_target, points, lengths);
    // update point locations
    for (int i; i < points.size(); i++) {
        points[i] => joints[i].pos;
    }
}
