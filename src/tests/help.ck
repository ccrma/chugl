/*
Playground for testing chuck features and .help()

Nothing to do with CGL
*/

@(1., 2., 3., 4.) => vec4 vec;
@(1., 2., 3., 4.) => vec4 other_vec;

@(1.0, 0., 0.) => vec3 vecA;
@(1.00, 0., 0.) => vec3 vecB;

<<< vec.x >>>;
<<< vec.y >>>;
<<< vec.z >>>;
<<< vec.w >>>;

Phasor drive => blackhole;

if (vecA == vecB) {
    <<< "vec approx equal" >>>;
} else {
    <<< "vec not equal" >>>;

}

int arr[0];
1 => arr["hello"];
arr.help();
/*
void clear();
    Clear the contents of the array.
int erase( string key );
    (map only) Erase all elements with the specified key.
int find( string key );
    (map only) Get number of elements with the specified key.
void getKeys( string[] keys );
    Return all keys found in associative array in keys
void popBack();
    Remove the last item of the array.
void popOut( int position );
    Removes the item with position from the array
void reset();
    Reset array size to 0, set capacity to (at least) 8.
void reverse();
    Reverses the array in-place
void shuffle();
    Shuffle the contents of the array.
int size( int newSize );
    Set the size of the array. If the new size is less than the current size, el
ements will be deleted from the end; if the new size is larger than the current
size, 0 or null elements will be added to the end.
void zero();
    Zero out the contents of the array; size is unchanged.
*/
string keys[0];
arr.getKeys(keys);
<<< "keys size pre erase", keys.size() >>>;
<<< "arr find pre erase", arr.isInMap("hello") >>>;

arr.erase("hello");
arr.getKeys(keys);
<<< "keys size post erase", keys.size() >>>;
<<< "arr find pos erase", arr.isInMap("hello") >>>;
<<< @(1.0, 1.0, 2.0) >>>;


adc => Gain g => dac;



while (true) {
    // <<< drive.last() >>>;
    1::samp => now;
}