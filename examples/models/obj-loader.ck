//-----------------------------------------------------------------------------
// name: obj.ck
// desc: OBJ model loader
// requires: ChuGL + chuck-1.5.1.5 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2023
//-----------------------------------------------------------------------------

// Example usage ========================================
GG.scene().backgroundColor(Color.BLACK);

GMesh mesh;
NormalsMaterial mat;

// load the obj file and assign it to the mesh geometry
mesh.set( 
    ObjLoader.load(me.dir() + "assets/suzanne.obj"),   // set obj geometry
    mat                                                       // set material
);

mesh --> GG.scene();
mesh.posZ(-5);

while (true) { 
    GG.dt() => mesh.rotateY;
    GG.nextFrame() => now; 
}

// class implementation ==================================
public class ObjLoader
{
    class Vertex 
    {
        vec3 pos;
        vec3 norm;
        vec2 tex;
    }

    // loads a .obj file at given path into a custom geometry
    fun static CustomGeometry @ load(string path)
    {
        // buffers for storing data from obj file
        vec3 positions[0];
        vec3 normals[0];
        vec2 texcoords[0];
        // buffer to generate vertex data from obj face indices 
        Vertex vertices[0];
        // instantiate a file IO object
        FileIO fio;
        // a string tokenizer
        StringTokenizer tokenizer;
        // a line of text
        string line;
        // a word token
        string word;

        // open the file
        fio.open( path, FileIO.READ );

        // ensure it's ok
        if( !fio.good() )
        {
            cherr <= "can't open file: " <= path <= " for reading..."
                <= IO.newline();
            me.exit();
        }

        // loop until end
        while( fio.more() )
        {
            // read current line
            fio.readLine() => line;
            // for each line, tokenize
            tokenizer.set( line );
            // loop over tokens
            while( tokenizer.more() )
            {
                // get the next word
                tokenizer.next() => word;

                // position data
                if (word == "v") {  
                    // get next three tokens as floats
                    positions << @(
                        Std.atof(tokenizer.next()),
                        Std.atof(tokenizer.next()),
                        Std.atof(tokenizer.next())
                    );
                // normal data
                } else if (word == "vn") {
                    normals << @(
                        Std.atof(tokenizer.next()),
                        Std.atof(tokenizer.next()),
                        Std.atof(tokenizer.next())
                    );
                // texture coordinate data
                } else if (word == "vt") {
                    texcoords << @(
                        Std.atof(tokenizer.next()),
                        Std.atof(tokenizer.next())
                    );
                // face data
                } else if (word == "f") {
                    // reconstruct vertices from face indices
                    repeat (3) {
                        Vertex v;
                        // zero out vertex attributes
                        @(0,0,0) => v.pos;
                        @(0,0,0) => v.norm;
                        @(0,0) => v.tex;
                        // get the next token
                        tokenizer.next() => string face;
                        face.replace( "//", "/0/");  // in case no texcoord is specified
                        face.replace( "/", " ");     // replace slashes with spaces
                        // tokenize it
                        StringTokenizer faceTokenizer;
                        faceTokenizer.set( face );

                        // NOTE: obj files are 1-indexed, so we subtract 1
                        // get the position index
                        Std.atoi(faceTokenizer.next()) - 1 => int posIndex;
                        // get the texture coordinate index
                        Std.atoi(faceTokenizer.next()) - 1 => int texIndex;
                        // get the normal index
                        Std.atoi(faceTokenizer.next()) - 1 => int normIndex;

                        if (posIndex >= 0) {
                            positions[posIndex] => v.pos;
                        }
                        if (texIndex >= 0) {
                            texcoords[texIndex] => v.tex;
                        }
                        if (normIndex >= 0) {
                            normals[normIndex] => v.norm;
                        }

                        vertices << v;
                    }
                }
            }
        }

        // At this point vertices contains all the vertex data
        // now we need to deconstruct it back buffers for each attribute
        vec3 geoPositions[vertices.size()];
        vec3 geoNormals[vertices.size()];
        vec2 geoTexcoords[vertices.size()];
        for (auto v : vertices) {
            geoPositions << v.pos;
            geoNormals << v.norm;
            geoTexcoords << v.tex;
        }

        // assign to custom geometry
        CustomGeometry customGeo;
        customGeo.positions(geoPositions);
        customGeo.normals(geoNormals);
        customGeo.uvs(geoTexcoords);

        return customGeo;
    }
}
