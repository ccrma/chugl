// helo.ck
// sample code showing possible composite GGen

class Helo extends GGen
{
    // sub GGen graph
    Propeller prop --> HeloBody body --> this.outlet;
    
    // param
    float rotationRate;
    
    // update (called by ChuGL)
    fun void update( dur deltaTime )
    {
        deltaTime * rotationRate => prop.rotation.y;
    }
}

// instantiate
Helo team[100];

// add to scene
for( auto x : team )
{
    x --> GGen.root;
}
