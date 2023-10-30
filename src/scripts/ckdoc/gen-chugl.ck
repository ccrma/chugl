//---------------------------------------------------------------------
// name: gen-chugl.ck
// desc: generate HTML documentation for ChuGL
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
//   date: Fall 2023
//---------------------------------------------------------------------

// instantiate a CKDoc object
CKDoc doc; // documentation orchestra
// set the examples root
"../examples/" => doc.examplesRoot;

// add group
doc.addGroup(
    // class names
    [
        "GG", 
        "WindowResizeEvent",
        "Color",
    ],
    // group name
    "ChuGL Basic Classes",
    // file name
    "chugl-basic", 
    // group description
    "Basic classes for ChuGL: strongly-timed audiovisual programming in ChucK."
);

// add group
doc.addGroup(
    // class names
    [
        "GGen", 
        "GScene", 
        "GPoints", 
        "GLines", 
        "GMesh", 
        "GCircle", 
        "GPlane", 
        "GCube", 
        "GSphere", 
        "GTorus",
        "GCylinder",
        "GLight", 
        "GPointLight", 
        "GDirLight", 
        "GCamera",
    ],
    // group name
    "Graphics Generators",
    // file name
    "chugl-ggens", 
    // group description
    "Graphics generators (GGens) that can be composed together in a scene graph."
);

// add group
doc.addGroup(
    // class names
    [
        "Geometry", 
        "BoxGeometry", 
        "SphereGeometry", 
        "CircleGeometry", 
        "LatheGeometry", 
        "PlaneGeometry", 
        "TorusGeometry", 
        "CylinderGeometry",
        "CustomGeometry",
    ],
    // group name
    "ChuGL Geometries",
    // file name
    "chugl-geo", 
    // group description
    "ChuGL geometries contain vertex data such as positions, normals, and UV coordinates."
);

// add group
doc.addGroup(
    // class names
    [ 
        "Material", 
        "NormalsMaterial", 
        "FlatMaterial", 
        "PhongMaterial", 
        "PointMaterial", 
        "LineMaterial", 
        "MangoUVMaterial", 
        "ShaderMaterial",
    ],
    // group name
    "ChuGL Materials",
    // file name
    "chugl-mat", 
    // group description
    "ChuGL materials describe appearance of geometries, including color and shading properties."
);

// add group
doc.addGroup(
    // class names
    [ "Texture", "FileTexture", "DataTexture" ],
    // group name
    "ChuGL Textures",
    // file name
    "chugl-tex", 
    // group description
    "Textures can be loaded from file, created dynamically from data; they are passed into materials and mapped onto 2D and 3D surfaces."
);

// add GUI group
doc.addGroup(
    // class names
    [
        "UI_Element",
        "UI_Window",
        "UI_Button",
        "UI_SliderFloat",
        "UI_SliderInt",
        "UI_Checkbox",
        "UI_Color3",
        "UI_Dropdown",
    ],
    // group name
    "ChuGL GUI",
    // file name
    "chugl-gui", 
    // group description
    "API methods for creating widgets such as buttons, sliders, dropdowns, etc.
    Use widgets to control parameter values during runtime. Great for experimentation
    and fine-tuning values. Also usable for building application user-interfaces."
);

// generate
doc.outputToDir( ".", "ChuGL [alpha] API Reference (v0.1.2)" );
