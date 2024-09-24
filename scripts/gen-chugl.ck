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

// // add group
// doc.addGroup(
//     // class names
//     [
//         "GG", 
//         "WindowResizeEvent",
//         "Color",
//         "AssLoader"
//     ],
//     // group name
//     "ChuGL Basic Classes",
//     // file name
//     "chugl-basic", 
//     // group description
//     "Basic classes for ChuGL: strongly-timed audiovisual programming in ChucK."
// );

// // add group
// doc.addGroup(
//     // class names
//     [
//         "GGen", 
//         "GScene", 
//         "GPoints", 
//         "GLines", 
//         "GMesh", 
//         "GTriangle",
//         "GCircle", 
//         "GPlane", 
//         "GCube", 
//         "GSphere", 
//         "GTorus",
//         "GCylinder",
//         "GLight", 
//         "GPointLight", 
//         "GDirLight", 
//         "GCamera",
//         "GText"
//     ],
//     // group name
//     "Graphics Generators",
//     // file name
//     "chugl-ggens", 
//     // group description
//     "Graphics generators (GGens) that can be composed together in a scene graph."
// );

// // add group
// doc.addGroup(
//     // class names
//     [
//         "Geometry", 
//         "CubeGeometry", 
//         "SphereGeometry", 
//         "TriangleGeometry",
//         "CircleGeometry", 
//         "LatheGeometry", 
//         "PlaneGeometry", 
//         "TorusGeometry", 
//         "CylinderGeometry",
//         "CustomGeometry",
//     ],
//     // group name
//     "ChuGL Geometries",
//     // file name
//     "chugl-geo", 
//     // group description
//     "ChuGL geometries contain vertex data such as positions, normals, and UV coordinates."
// );

// // add group
// doc.addGroup(
//     // class names
//     [ 
//         "Material", 
//         "NormalsMaterial", 
//         "FlatMaterial", 
//         "PhongMaterial", 
//         "PointMaterial", 
//         "LineMaterial", 
//         "MangoUVMaterial", 
//         "ShaderMaterial",
//     ],
//     // group name
//     "ChuGL Materials",
//     // file name
//     "chugl-mat", 
//     // group description
//     "ChuGL materials describe appearance of geometries, including color and shading properties."
// );

// // add group
// doc.addGroup(
//     // class names
//     [ "Texture", "FileTexture", "DataTexture", "CubeTexture" ],
//     // group name
//     "ChuGL Textures",
//     // file name
//     "chugl-tex", 
//     // group description
//     "Textures can be loaded from file, created dynamically from data; they are passed into materials and mapped onto 2D and 3D surfaces."
// );

// doc.addGroup(
//     // class names
//     [
//         "FX",
//         "PassThroughFX",
//         "OutputFX",
//         "InvertFX",
//         "MonochromeFX",
//         "BloomFX",
//         "CustomFX"
//     ],
//     // group name
//     "ChuGL Post Processing FX",
//     // file name
//     "chugl-fx", 
//     // group description
//     "Post Processing FX are applied to the final output of the scene, GG.fx()"
// );

// add GUI group
doc.addGroup(
    // class names
    [
        "UI_Bool",
        "UI_String",
        "UI_Int",
        "UI_Int2",
        "UI_Int3",
        "UI_Int4",
        "UI_Float",
        "UI_Float2",
        "UI_Float3",
        "UI_Float4",
        "UI_Viewport",
        "UI_Style",
        "UI_WindowFlags",
        "UI_ChildFlags",
        "UI_Cond",
        "UI_Color",
        "UI_StyleVar",
        "UI_ButtonFlags",
        "UI_Direction",
        "UI_ComboFlags",
        "UI_SliderFlags",
        "UI_InputTextFlags",
        "UI_ColorEditFlags",
        "UI_TreeNodeFlags",
        "UI_SelectableFlags",
        "UI_PopupFlags",
        "UI_TableFlags",
        "UI_TableRowFlags",
        "UI_TableColumnFlags",
        "UI_TableBgTarget",
        "UI_TabBarFlags",
        "UI_MouseButton",
        "UI_Key",
        "UI_MouseCursor",
        "UI_ViewportFlags",
        "UI_FocusedFlags",
        "UI_HoveredFlags",
        "UI_DockNodeFlags",
        "UI_Callback",
        "UI_SizeCallbackData",
        "UI_SizeCallback",
        "UI_ComboCallback",
        "UI_DrawList",
        "UI",
    ],
    // group name
    "ChuGL UI",
    // file name
    "chugl-ui", 
    // group description
    "API methods for creating widgets such as buttons, sliders, dropdowns, etc.
    Use widgets to control parameter values during runtime. Great for experimentation
    and fine-tuning values. Also usable for building application user-interfaces."
);

<<< doc.disableSort() >>>;
doc.disableSort(true);
<<< doc.disableSort() >>>;

// generate
doc.outputToDir( ".", "ChuGL [alpha] API Reference (v0.1.5)" );
