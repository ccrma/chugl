// GGen API rework...
// 2023.10.19 (Andrew & Ge)
// make API self-consistent across pos, rot, sca

// sets
float posX( float )
float posX()

float posY( float )
float posY()

float posZ( float )
float posZ()

vec3 posWorld( float )
vec3 posWorld()

vec3 pos( vec3 )
vec3 pos()

GGen translate( vec3 )
GGen translateX( float )
GGen translateY( float )
GGen translateZ( float )

// sets
float rotX( float )
float rotX()

float rotY( float )
float rotY()

float rotZ( float )
float rotZ();

vec3 rot( vec3 )
vec3 rot()

GGen rotateX( float )
GGen rotateY( float )
GGen rotateZ( float )

GGen rotateOnLocalAxis( vec3, float )
GGen rotateOnWorldAxis( vec3, float )
GGen lookAt( vec3 pos )
vec3 lookAtDir(); // same as forward()

// sets
float scaX( float )
float scaX( )

float scaY( float )
float scaX( )

float scaZ( float )
float scaZ( )

vec3 sca( vec3 )
vec3 sca( float )
vec3 sca()

vec3 scaWorld( vec3 )
vec3 scaWorld()

void update( float dt )
This method is automatically invoked once per frame for all GGens connected
to the scene graph. Override this method in custom GGen classes to implement
your own update logic.