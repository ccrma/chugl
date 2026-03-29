//-----------------------------------------------------------------------------
// name: geo-and-mat.ck
// desc: UI-driven example for geometry + material
// NOTE: this is a useful example for exploring what you can do with the 
//       built-in geometries and materials
// requires: ChuGL + chuck-1.5.5.5 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//-----------------------------------------------------------------------------
// use orbit camera as main camera
GG.scene().camera( new GOrbitCamera );
// the mesh to render
GMesh mesh --> GG.scene();

// geometries
[
    null,
    new PlaneGeometry,
    new SuzanneGeometry,
    new SphereGeometry,
    new CubeGeometry,
    new CircleGeometry,
    new TorusGeometry,
    new CylinderGeometry,
    new KnotGeometry,
    new PolygonGeometry,
    new PolyhedronGeometry,
] @=> Geometry geometries[];

// create ChuGL UI integer for communication with picker widget
UI_Int geometry_index;
[ "None" ] @=> string builtin_geometries[];
for (1 => int i; i < geometries.size(); i++) {
    builtin_geometries << Type.of(geometries[i]).name();
}

UVMaterial uv_material;
NormalMaterial normal_material;
FlatMaterial flat_material;
PhongMaterial phong_material;
PBRMaterial pbr_material;

// materials
[
    null,
    uv_material,
    normal_material,
    flat_material,
    phong_material,
    pbr_material,
] @=> Material materials[];

// create ChuGL UI integer for communication with a picker widget
UI_Int material_index;
[ "None" ] @=> string builtin_materials[];
for (1 => int i; i < materials.size(); i++) {
    builtin_materials << Type.of(materials[i]).name();
}

// material params
UI_Int material_topology_index(3); // default to triangle list
[
    "PointList",
    "LineList",
    "LineStrip",
    "TriangleList",
    "TriangleStrip",
] @=> string material_topologies[];
UI_Bool wireframe(false);

// uv test image
[
    0x474e5089, 0x0a1a0a0d, 0x0d000000, 0x52444849, 0x20000000, 0x20000000, 0x00000608, 0x7a7a7300, 0x0a0000f4, 0x414449a9, 0x64017854, 0x5c8c6b57, 0xe67e1965, 0xcce7339c, 0x57b6e76d, 0xdb02dbba, 0x942da882, 0x40205110, 0x2c5020a9, 0xbc4a9a35, 0x04010a24, 0x289a2189, 0x44c47f88, 0x1fe098ff, 0xa4d11a8d, 0x51a4109a, 0xa18a8424, 0x95b86d88, 0xdbbb50b6, 0xed974b76, 0x3b33a7de, 0xb9d9f73b, 0x5f3cf8cf, 0xedb8988b, 0xfbe739c9, 0xef7df3be, 0xf3cfbcfb, 0x4f58efbe, 0xdea3f93d, 0xb0791ff7, 0xcfaecf77, 0xcbfcf4f7, 0xfbd93d9f, 0xda7fcfed, 0x0f3bfc7b, 0xadf7bef5, 0x7df77afb, 0xff19b765, 0xfdd68d70, 0xf60fdbe0, 0xdbd0c736, 0xd1ef471a, 0xbffaeeaf, 0xee6fb1eb, 0xc76d7dee, 0xaf9bbd2d, 0xc6fbd2d9, 0x5f7b5bce, 0x79837edc, 0xbaedb5de, 0xafdbae9e, 0x7f7cc6bf, 0x567a1f79, 0xe9c15ca9, 0x6d118763, 0x7a9e9197, 0xbe0e470f, 0x5fb7c982, 0x09e9f2c3, 0xebe6f3bc, 0x30e7a6a8, 0xfc75f875, 0xf01bd0fb, 0x1d73796a, 0x7c75fb7d, 0xd11bfaea, 0x62b901eb, 0xdec75319, 0xef23b1c5, 0x87c3dde0, 0x38ea70b0, 0x137a1e8e, 0xa38fc793, 0x7960c85d, 0x4e670a7a, 0x9f23429f, 0x1a6e6a41, 0x449d4f27, 0x0d88b325, 0xcba0361b, 0x7cd5adef, 0x0f0f1701, 0xdd00dd60, 0xcf2101e9, 0x5455a6e5, 0x0e8c756a, 0x756ad1c6, 0x30562d90, 0x2c449436, 0xe8d73412, 0x56e70260, 0x0034c80b, 0xd6a3f5cf, 0x6d57236a, 0xd82cb1c0, 0xb5f90f96, 0x3f41a206, 0x1e83ae82, 0x05f50dff, 0x6a16dae1, 0xc3fa0ecd, 0x0b3ed801, 0x06dabeb2, 0x5c57cdac, 0x47e7a08e, 0xc4c55cae, 0xf9d202cc, 0x268d6a12, 0x618f8cb6, 0x9ce51128, 0xc75dde6b, 0x0240e8c1, 0x38792b4b, 0xd8d36cdc, 0xb8343a30, 0x002ee4d1, 0xd6b907c6, 0xf9c398ef, 0x6bbb468a, 0x368dab9e, 0x3ae175d7, 0xc227d616, 0xd51b1cf0, 0x384b4b66, 0xcf96034e, 0x5a9b31a5, 0x33e7cf81, 0xcb9eccde, 0x91b7efc2, 0x76719429, 0xa2c66b39, 0x579620e1, 0xd5ad280b, 0xd08f1031, 0xf5aa22fb, 0x3160d9a6, 0xc7cb02bd, 0x3c8c68fd, 0x58e0f26f, 0xf39945d0, 0x44ccf83e, 0xc39752b6, 0x7483b83c, 0x2c9446a9, 0xcbab4458, 0x39616b59, 0xc0284885, 0x68343d05, 0x032325d9, 0xae74f606, 0x6c346488, 0x1b6c7736, 0xca18c923, 0xc3c4356b, 0x04318c61, 0x04262205, 0x133eb112, 0xf4159d4b, 0x2faef79f, 0x37b042e0, 0x3b341dda, 0x16a0c85d, 0x1b2215ef, 0xdd3a6865, 0xc1f08d2e, 0xd0d67da1, 0x7e023f40, 0x3700b697, 0x9b02bd6f, 0xa6c51d2f, 0xfb5c61d1, 0x07c371f1, 0x8470ae41, 0xa5721e03, 0x68a379ea, 0xcf24c2bd, 0x1210a1a3, 0x7a190f61, 0xa3682e26, 0x98a0f93c, 0x3930f20b, 0xfbd01a42, 0x601cf416, 0xd43a5c28, 0xb12271a2, 0x3e36db65, 0x58a34a58, 0x2bf899ad, 0xc76eed04, 0xc84cbdc4, 0x06e3fa47, 0x60ae54fa, 0x648e21c2, 0xc1b1fcc9, 0x9404c924, 0x00e8356b, 0xa37284da, 0xe4f90885, 0xb29110f1, 0xb9e9d24a, 0x347cb0ce, 0x6975d0ae, 0x2e8d0750, 0x643a7560, 0x215c6126, 0x3cbc32f9, 0x6aeb2c5a, 0x0f504cdc, 0x27d04e72, 0x1c2622fb, 0x7f51735a, 0x8f8d5202, 0xa2c40eee, 0xd544fc10, 0x1d761ec0, 0x910b6df8, 0xae410b80, 0x9068e1e7, 0x171f9fde, 0xe683525a, 0xa1c2084c, 0x58962190, 0x15f5a41d, 0x317a684a, 0xbd31146d, 0xd134c194, 0xf4abf411, 0x31a8e15a, 0x8552b98e, 0xeffc19e3, 0x8873d85f, 0xae626588, 0x6df12ef1, 0x63bf73d2, 0x02a65c73, 0x528d72e9, 0x891a6b6c, 0x001e4790, 0xc81e8256, 0x69bab12b, 0xc9265f34, 0xf9ee0692, 0x6191aacc, 0xc3f1e838, 0x41262e16, 0xc2295845, 0xa9da97a6, 0x54da1949, 0xd42abc90, 0x8a52089a, 0x0442b43a, 0xf0ae1e3d, 0x342deb94, 0x41627599, 0x10760697, 0x7a49f80d, 0x01a3dda0, 0x18301a71, 0x5d5883ef, 0x38d9f261, 0xdbc1043a, 0x2fc622a0, 0xf019d41a, 0x4be6e9b8, 0xb88520c7, 0x91d2f4dc, 0x6ac87441, 0x93288b9e, 0x96364d8c, 0xe788bb28, 0x49b6d090, 0x8f61fd3a, 0x1f04f367, 0x9ce44320, 0x8f1631d4, 0xaa4a0940, 0x4d6739b0, 0x628fbd4e, 0x0c024485, 0x85b6fdfb, 0x6f9e6ca5, 0xba0ffd1f, 0x627031ae, 0x6ce49166, 0x65286c99, 0x90e25c58, 0x8ae1bcc4, 0xf5213891, 0x825b2661, 0x9f72eeb4, 0x31a881f1, 0x24dae026, 0x8665a3a7, 0x5a86611f, 0xc0a08339, 0xc51b6db2, 0xa9861aea, 0x79c615f3, 0x27fa9579, 0x8d7704e1, 0x3f7e0828, 0xdbe62d59, 0x193e94dc, 0xe8dd98b5, 0x4552b141, 0x86d3a19d, 0x1660810a, 0x8740e212, 0x8a195427, 0x4a544a15, 0xb35f9028, 0x3ee825ab, 0x44980910, 0xf4f73d66, 0xdd71b0af, 0x06188630, 0x5881f7e5, 0x705d68c4, 0x2fc1760b, 0x63fb872f, 0x493af606, 0x6504cfd8, 0x6f35a00c, 0x480ffabf, 0xca2032ff, 0x30e60ad8, 0x4d510c3d, 0xc2b1121d, 0xeb10ec54, 0x6922b91a, 0xea6596da, 0x09700c86, 0xf66ca5a1, 0xf197befc, 0xc05e678b, 0x604def1b, 0xe1afcbef, 0xcd7b3da9, 0x17def4f5, 0xfdbedcf1, 0x2fb3f5f8, 0xf71799e0, 0x6f03a5e3, 0x8e875fe1, 0xf697d9e2, 0x075af99b, 0x6c92d368, 0x4c73096a, 0x976627e1, 0x2cf4e471, 0xe0ce4c0e, 0xcd6753cd, 0x148599f3, 0x61cf17a6, 0x1d66953e, 0x667941c9, 0xabc3932e, 0x83263313, 0xbf6d375a, 0x5f369b0d, 0xddb7db8d, 0xbbae9b88, 0xbcaf9b16, 0x79df7712, 0xdaef7607, 0xd96f5b89, 0xde779d86, 0xbb9f1d8e, 0x5eb75b0d, 0xdd37ed85, 0x6db71d80, 0xaeab96c7, 0x8c0fcc42, 0xb7ab8104, 0x7ecf85dd, 0x697c5ef9, 0x7ed83df7, 0xe4785dc7, 0xbee2efd1, 0xadc01ffb, 0xebe2763b, 0xc78fdef7, 0x1e04f87f, 0xfc09fc7b, 0xe09fc9f8, 0xc21f8791, 0x80bebbdd, 0x1fb81f87, 0x5bf01749, 0x0beb1580, 0xa1cb5021, 0xeac59a9a, 0x4aec1ce2, 0x04fc3596, 0x409c1f32, 0x662cf979, 0x38b126a7, 0x6528127d, 0x419f9d91, 0x3a0cee6e, 0xfb441585, 0x06e01007, 0x708646eb, 0xcc11d8f2, 0xbe127fbf, 0x26662aca, 0x7b3b228e, 0x1662dd1a, 0x421c8fc7, 0x4a04e66e, 0x9d5853f3, 0x4855809e, 0x42d46361, 0xa7c45da2, 0xf99d733f, 0x8afc92de, 0x8492afd4, 0xd494b6df, 0x162ed5a8, 0x6b1455f3, 0x2899280d, 0x53c63da6, 0x0d315465, 0x64e3d593, 0x501afcf8, 0x04189528, 0xf08d1920, 0xa2f533ee, 0x77a9e246, 0x1ebe32d8, 0xd639664a, 0xe79a4e10, 0x2dc92633, 0xbda59462, 0x3559f8c2, 0xd2e557ea, 0xdc421aa5, 0xf0b24ab8, 0xa7c4782c, 0x894204c4, 0xa4bd3543, 0x3a2e834d, 0xd5abb9cc, 0xc8e5543a, 0x2b32a42d, 0xb550b108, 0x5697d299, 0xd1138872, 0x3cf7cb28, 0x69552ce3, 0xac82f97c, 0x9615a2a5, 0xa817cd79, 0x2197569e, 0x90ab5628, 0x18a16474, 0x4a66e31f, 0xaad90e12, 0xa24a85ee, 0x42242d01, 0x3af0c42e, 0x898058ab, 0x8cbbc5a9, 0x1b65c4fb, 0x287ad599, 0xb3a998b1, 0x0becd552, 0x41743006, 0xf44f1a55, 0x0ebfa185, 0xa5d49793, 0x7e597579, 0x9804068b, 0x986de464, 0x3e8b4c59, 0x844c94bc, 0xd8c7de86, 0x9e83c4c9, 0xaa422a49, 0x3215c3c9, 0x0e38e521, 0x6d430fe1, 0x063248da, 0x0ca41b41, 0x63424413, 0x946b31aa, 0x536381ee, 0x6d054f0c, 0x9579ab7e, 0xb7371efc, 0x4a3a4c12, 0x5de1ca1a, 0x0b3d2bba, 0x5041f166, 0x658d4bcb, 0x7b044858, 0xee850d94, 0x1101d721, 0xb3e9e9c4, 0x7cb59a50, 0x05b39616, 0x6085ba53, 0x724ca19d, 0x599122ef, 0xd2371b62, 0x52a1371f, 0x86c71c58, 0x25b192ee, 0x66300c15, 0xe26dbf01, 0x82c6e41b, 0x187c9b59, 0x63401902, 0x01a3d821, 0x6162eb17, 0xf684dc78, 0x817d8862, 0xd7b7486a, 0x94fb9445, 0xcbc935d9, 0x4d090bc1, 0x70c37e65, 0xd659063f, 0x43f5e445, 0x37c9f849, 0x07c73d25, 0x8bb35358, 0x20d79748, 0x4d8875c8, 0xbab30597, 0xd52b6519, 0xc65e316f, 0xf0db1df8, 0xb5ce1941, 0x5f472d2d, 0x7afc3ce4, 0x247eb809, 0xb7356f19, 0xf0976793, 0x8619a9ce, 0x24788227, 0x7cf87bc4, 0xaa1ee1ad, 0x3cb8ea35, 0x3e48069b, 0xe543a66d, 0xa9d8de6f, 0x38ad58b4, 0x26d752c3, 0x95de6de3, 0x8eecf3cf, 0x5aefac5a, 0x6eead923, 0x990c80fb, 0x211421df, 0x1b196d8f, 0xaa2b329a, 0xbe749a9a, 0x750cfa68, 0x94d675d6, 0xd820791a, 0xd0a5cd4f, 0x842f1b60, 0xe1a20a8d, 0x21624630, 0x09bf3d84, 0x21e82241, 0x6d492b40, 0x444db303, 0x16e7cc2a, 0x6a52690d, 0xf2151a1d, 0x45ad663b, 0xbf592e03, 0xc1681c8e, 0x190d1f78, 0x5f94a835, 0x35023636, 0xed6cc3b0, 0x1a88a887, 0xa3d82c65, 0xccab806c, 0x09930a2f, 0xb3078041, 0xbb5d1b70, 0xd2c2c5b6, 0xd3849a41, 0x99ae2a95, 0x1ab2eaf2, 0x6099245a, 0xdf3f1ad6, 0x96891775, 0x894c65ce, 0xa954e756, 0x0fe41037, 0x8ab7351b, 0xbabf8e79, 0x45bcf46a, 0x5d5be9f9, 0xa19050d1, 0xe8e68218, 0x84872c13, 0xbc153950, 0xb62a7a65, 0x844b1cbb, 0xfea7ad3c, 0xe6cfca3d, 0x1c0e8d0f, 0x6bc582cb, 0x08570c8d, 0x932044fc, 0xc9ee99cd, 0xf3531c22, 0x2df58829, 0x6366d522, 0xb528152b, 0xf5ee5916, 0x65ce922e, 0x0931c8b2, 0x1d369ba9, 0x87900c91, 0x3aa47e4d, 0xfff017dd, 0x938be0f7, 0x7910c811, 0xe416631c, 0x08e9a619, 0x4d81085d, 0xb1f93e03, 0x1559a64d, 0x28f143ac, 0x9bc17ac4, 0xbbf51920, 0x392ae842, 0x6b3a4149, 0xae471924, 0x1d1f4558, 0xbbd82134, 0xecb22f77, 0x166cf288, 0x78478864, 0xa0639ec3, 0x25d03d57, 0x5a243a8b, 0xe62e5093, 0xaea8862b, 0xee9e996d, 0xa77e7b2f, 0x5b94fe69, 0x98940d8f, 0x7ff28f92, 0x68665f26, 0x025abc9a, 0xbc264101, 0x94f76c4d, 0xd69864ac, 0x602b4508, 0x0674a903, 0x0be7acf3, 0xcf6626bc, 0x960fbde1, 0xe2ca9b30, 0x059a42e4, 0xacc38d24, 0xcccf87f1, 0x92ccf8e3, 0x26493759, 0xc0b33d1f, 0xc1e20090, 0xc41c0f3f, 0xe1f7e79e, 0x9e19ec4f, 0x7f107fdf, 0xf616f83f, 0x7e0af8bc, 0xa017e7f3, 0x75ffdff1, 0xb7f8fe1f, 0x0ffde67d, 0xa18f567c, 0xf0a46c35, 0x0000001f, 0x92e2ffff, 0x0000cb7f, 0x44490600, 0x00035441, 0x15ceaec3, 0xc744d839, 0x00000000, 0x444e4549, 0x826042ae,
] @=> int uv_checker_png_data[];
Texture.load(uv_checker_png_data) @=> Texture uv_tex;
phong_material.colorMap() @=> Texture default_color_map;

// create ChuGL UI params to map to material and geometry params
UI_Bool normal_material_worldspace(normal_material.worldspaceNormals());

// Phong material params
UI_Bool phong_colormap(false);
UI_Float3 phong_specular(phong_material.specular());
UI_Float3 phong_diffuse(phong_material.color());
UI_Float phong_shine(phong_material.shine());
UI_Float3 phong_emission(phong_material.emission());
UI_Float phong_normal_factor(phong_material.normalFactor());
UI_Float phong_ao_factor(phong_material.aoFactor());
UI_Float2 phong_uv_offset(phong_material.uvOffset());
UI_Float2 phong_uv_scale(phong_material.uvScale());

// PBR material params
UI_Float3 pbr_albedo(pbr_material.color());
UI_Float pbr_metallic(pbr_material.metallic());
UI_Float pbr_roughness(pbr_material.roughness());
UI_Float pbr_ao_factor(pbr_material.aoFactor());
UI_Float pbr_normal_factor(pbr_material.normalFactor());

// plane geometry params
geometries[1] $ PlaneGeometry @=> PlaneGeometry@ plane_geo;
UI_Float plane_width(plane_geo.width());
UI_Float plane_height(plane_geo.height());
UI_Int plane_width_segments(plane_geo.widthSegments());
UI_Int plane_height_segments(plane_geo.heightSegments());
fun void buildPlane() {
    plane_geo.build(
        plane_width.val(),
        plane_height.val(),
        plane_width_segments.val(),
        plane_height_segments.val()
    );
}

// suzanne geometry has no params

// sphere geometry params
geometries[3] $ SphereGeometry @=> SphereGeometry@ sphere_geo;
UI_Float sphere_radius(sphere_geo.radius());
UI_Int sphere_width(sphere_geo.widthSegments());
UI_Int sphere_height(sphere_geo.heightSegments());
UI_Float sphere_phi_start(sphere_geo.phiStart());
UI_Float sphere_phi_length(sphere_geo.phiLength());
UI_Float sphere_theta_start(sphere_geo.thetaStart());
UI_Float sphere_theta_length(sphere_geo.thetaLength());
fun void buildSphere() {
    sphere_geo.build(
        sphere_radius.val(),
        sphere_width.val(),
        sphere_height.val(),
        sphere_phi_start.val(),
        sphere_phi_length.val(),
        sphere_theta_start.val(),
        sphere_theta_length.val()
    );
}

// box geometry params
geometries[4] $ CubeGeometry @=> CubeGeometry@ cube_geo;
UI_Float box_width(cube_geo.width());
UI_Float box_height(cube_geo.height());
UI_Float box_depth(cube_geo.depth());
UI_Int box_width_segments(cube_geo.widthSegments());
UI_Int box_height_segments(cube_geo.heightSegments());
UI_Int box_depth_segments(cube_geo.depthSegments());
fun void buildBox() {
    cube_geo.build(
        box_width.val(),
        box_height.val(),
        box_depth.val(),
        box_width_segments.val(),
        box_height_segments.val(),
        box_depth_segments.val()
    );
}

// circle geometry params
geometries[5] $ CircleGeometry @=> CircleGeometry@ circle_geo;
UI_Float circle_radius(circle_geo.radius());
UI_Int circle_segments(circle_geo.segments());
UI_Float circle_theta_start(circle_geo.thetaStart());
UI_Float circle_theta_length(circle_geo.thetaLength());
fun void buildCircle() {
    circle_geo.build(
        circle_radius.val(),
        circle_segments.val(),
        circle_theta_start.val(),
        circle_theta_length.val()
    );
}

// torus geometry params
geometries[6] $ TorusGeometry @=> TorusGeometry@ torus_geo;
UI_Float torus_radius(torus_geo.radius());
UI_Float torus_tube_radius(torus_geo.tubeRadius());
UI_Int torus_radial_segments(torus_geo.radialSegments());
UI_Int torus_tubular_segments(torus_geo.tubularSegments());
UI_Float torus_arc_length(torus_geo.arcLength());
fun void buildTorus() {
    torus_geo.build(
        torus_radius.val(),
        torus_tube_radius.val(),
        torus_radial_segments.val(),
        torus_tubular_segments.val(),
        torus_arc_length.val()
    );
}

// cylinder geometry params
geometries[7] $ CylinderGeometry @=> CylinderGeometry@ cylinder_geo;
UI_Float cylinder_radius_top(cylinder_geo.radiusTop());
UI_Float cylinder_radius_bottom(cylinder_geo.radiusBottom());
UI_Float cylinder_height(cylinder_geo.height());
UI_Int cylinder_radial_segments(cylinder_geo.radialSegments());
UI_Int cylinder_height_segments(cylinder_geo.heightSegments());
UI_Bool cylinder_open_ended(cylinder_geo.openEnded());
UI_Float cylinder_theta_start(cylinder_geo.thetaStart());
UI_Float cylinder_theta_length(cylinder_geo.thetaLength());
fun void buildCylinder() {
    cylinder_geo.build(
        cylinder_radius_top.val(),
        cylinder_radius_bottom.val(),
        cylinder_height.val(),
        cylinder_radial_segments.val(),
        cylinder_height_segments.val(),
        cylinder_open_ended.val(),
        cylinder_theta_start.val(),
        cylinder_theta_length.val()
    );
}

// knot geometry params
geometries[8] $ KnotGeometry @=> KnotGeometry@ knot_geo;
UI_Float knot_radius(knot_geo.radius());
UI_Float knot_tube(knot_geo.tube());
UI_Int knot_tubular_segments(knot_geo.tubularSegments());
UI_Int knot_radial_segments(knot_geo.radialSegments());
UI_Int knot_p(knot_geo.p());
UI_Int knot_q(knot_geo.q());
fun void buildKnot() {
    knot_geo.build(
        knot_radius.val(),
        knot_tube.val(),
        knot_tubular_segments.val(),
        knot_radial_segments.val(),
        knot_p.val(),
        knot_q.val()
    );
}

// polyhedron geometry params 
geometries[10] $ PolyhedronGeometry @=> PolyhedronGeometry polyhedron_geo;
UI_Int polyhedron_type_index(0);
[
    "Tetrahedron",
    "Cube",
    "Octahedron",
    "Dodecahedron",
    "Icosahedron",
] @=> string polyhedron_types[];


// ChuGL UI bool for toggling rotation
UI_Bool rotate;

// ChuGL UI update function
// this can be either called repeatedly from any render loop 
// OR can be modified to be its own render loop that can be sporked
fun void updateUI()
{
    // make UI window transparent
    UI.setNextWindowBgAlpha(0.00);
    
    // create a ChuGL UI pane
    if (UI.begin("Geometry and Material Example"))
    {
        // add scene graph viewer
        UI.scenegraph( GG.scene() );
        
        // add rotation checkbox
        UI.checkbox("rotate", rotate);
        
        // draw list of geometries
        if (UI.listBox("builtin geometries", geometry_index, builtin_geometries)) {
            mesh.geometry(geometries[geometry_index.val()]);
        }
        
        // draw list of materials
        if (UI.listBox("builtin materials", material_index, builtin_materials)) {
            mesh.material(materials[material_index.val()]);
            
            // update material params
            if (mesh.material() != null) {
                material_topology_index.val() => mesh.material().topology;
            }
        }
        
        // add separator line
        UI.separatorText("Base Material Params");
        
        // shared by all materials
        if (mesh.material() != null) {            
            // material topology
            if (UI.listBox("topology", material_topology_index, material_topologies)) {
                mesh.material().topology(material_topology_index.val());
            }

            // add/draw checkbox for wireframe
            UI.checkbox("wireframe", wireframe);
            // set wireframe
            mesh.material().wireframe(wireframe.val());
        }
        
        // normal material param
        if (mesh.material() == normal_material) {
            UI.separatorText("Normal Material Params");
            if (UI.checkbox("worldspace normals", normal_material_worldspace)) {
                normal_material.worldspaceNormals(normal_material_worldspace.val());
            }
        }
        
        // Phong material params
        if (mesh.material() == phong_material) {
            UI.separatorText("Phong Material Params");
            if (UI.colorEdit("specular", phong_specular, 0)) phong_specular.val() => phong_material.specular;
            if (UI.colorEdit("diffuse", phong_diffuse, 0)) phong_diffuse.val() => phong_material.color;
            if (UI.slider("shine", phong_shine, 0, 10)) phong_shine.val() => phong_material.shine;
            if (UI.colorEdit("emission", phong_emission, 0)) phong_emission.val() => phong_material.emission;
            if (UI.slider("normal factor", phong_normal_factor, 0, 1)) phong_normal_factor.val() => phong_material.normalFactor;
            if (UI.slider("ao factor", phong_ao_factor, 0, 1)) phong_ao_factor.val() => phong_material.aoFactor;

            if (UI.checkbox("color map", phong_colormap)) {
                if (phong_colormap.val()) uv_tex => phong_material.colorMap;
                else default_color_map => phong_material.colorMap;
            } 

            if (UI.slider("uv offset", phong_uv_offset, 0, 10)) phong_uv_offset.val() => phong_material.uvOffset;
            if (UI.slider("uv scale", phong_uv_scale, 0, 10)) phong_uv_scale.val() => phong_material.uvScale;
        }
        
        // PBR material params
        if (mesh.material() == pbr_material)
        {
            UI.separatorText("PBR Material Params");
            if (UI.colorEdit("albedo", pbr_albedo, 0)) pbr_albedo.val() => pbr_material.color;
            if (UI.slider("metallic", pbr_metallic, 0, 1)) pbr_metallic.val() => pbr_material.metallic;
            if (UI.slider("roughness", pbr_roughness, 0, 1)) pbr_roughness.val() => pbr_material.roughness;
            if (UI.slider("ao factor", pbr_ao_factor, 0, 1)) pbr_ao_factor.val() => pbr_material.aoFactor;
            if (UI.slider("normal factor", pbr_normal_factor, 0, 1)) pbr_normal_factor.val() => pbr_material.normalFactor;
        }
        
        // draw separator line in UI
        UI.separatorText("Geometry Params");
        
        // plane geometry params
        if (mesh.geometry() == plane_geo) {
            if (UI.slider("width", plane_width, 0.1, 10)) buildPlane();
            if (UI.slider("height", plane_height, 0.1, 10)) buildPlane();
            if (UI.slider("width segments", plane_width_segments, 1, 64)) buildPlane();
            if (UI.slider("height segments", plane_height_segments, 1, 64)) buildPlane();
        }
        
        // sphere geometry params
        if (mesh.geometry() == sphere_geo) {
            if (UI.slider("radius", sphere_radius, 0.1, 10)) buildSphere();
            if (UI.slider("width segments", sphere_width, 3, 64)) buildSphere();
            if (UI.slider("height segments", sphere_height, 2, 64)) buildSphere();
            if (UI.slider("phi start", sphere_phi_start, 0, 2 * Math.PI)) buildSphere();
            if (UI.slider("phi length", sphere_phi_length, 0, 2 * Math.PI)) buildSphere();
            if (UI.slider("theta start", sphere_theta_start, 0, Math.PI)) buildSphere();
            if (UI.slider("theta length", sphere_theta_length, 0, Math.PI)) buildSphere();
        }
        
        // box geometry params
        if (mesh.geometry() == cube_geo) {
            if (UI.slider("width", box_width, 0.1, 10)) buildBox();
            if (UI.slider("height", box_height, 0.1, 10)) buildBox();
            if (UI.slider("depth", box_depth, 0.1, 10)) buildBox();
            if (UI.slider("width segments", box_width_segments, 1, 64)) buildBox();
            if (UI.slider("height segments", box_height_segments, 1, 64)) buildBox();
            if (UI.slider("depth segments", box_depth_segments, 1, 64)) buildBox();
        }
        
        // circle geometry params
        if (mesh.geometry() == circle_geo) {
            if (UI.slider("radius", circle_radius, 0.1, 10)) buildCircle();
            if (UI.slider("segments", circle_segments, 1, 64)) buildCircle();
            if (UI.slider("theta start", circle_theta_start, 0, 2 * Math.PI)) buildCircle();
            if (UI.slider("theta length", circle_theta_length, 0, 2 * Math.PI)) buildCircle();
        }
        
        // torus geometry params
        if (mesh.geometry() == torus_geo) {
            if (UI.slider("radius", torus_radius, 0.1, 10)) buildTorus();
            if (UI.slider("tube radius", torus_tube_radius, 0.1, 10)) buildTorus();
            if (UI.slider("radial segments", torus_radial_segments, 3, 64)) buildTorus();
            if (UI.slider("tubular segments", torus_tubular_segments, 3, 64)) buildTorus();
            if (UI.slider("arc length", torus_arc_length, 0, 2 * Math.PI)) buildTorus();
        }
        
        // cylinder geometry params
        if (mesh.geometry() == cylinder_geo) {
            if (UI.slider("radius top", cylinder_radius_top, 0.1, 10)) buildCylinder();
            if (UI.slider("radius bottom", cylinder_radius_bottom, 0.1, 10)) buildCylinder();
            if (UI.slider("height", cylinder_height, 0.1, 10)) buildCylinder();
            if (UI.slider("radial segments", cylinder_radial_segments, 3, 64)) buildCylinder();
            if (UI.slider("height segments", cylinder_height_segments, 1, 64)) buildCylinder();
            if (UI.checkbox("open ended", cylinder_open_ended)) buildCylinder();
            if (UI.slider("theta start", cylinder_theta_start, 0, 2 * Math.PI)) buildCylinder();
            if (UI.slider("theta length", cylinder_theta_length, 0, 2 * Math.PI)) buildCylinder();
        }
        
        // knot geometry params
        if (mesh.geometry() == knot_geo) {
            if (UI.slider("radius", knot_radius, 0.1, 10)) buildKnot();
            if (UI.slider("tube", knot_tube, 0.1, 10)) buildKnot();
            if (UI.slider("tubular segments", knot_tubular_segments, 3, 64)) buildKnot();
            if (UI.slider("radial segments", knot_radial_segments, 3, 64)) buildKnot();
            if (UI.slider("p", knot_p, 1, 20)) buildKnot();
            if (UI.slider("q", knot_q, 1, 20)) buildKnot();
        }

        
        // polyhedron geometry params
        if (mesh.geometry() == polyhedron_geo) {
            // draw list of materials
            if (UI.listBox("polyhedron types", polyhedron_type_index, polyhedron_types)) {
                polyhedron_geo.build(polyhedron_type_index.val());
            }
        }
    }
    
    // this marks the end the UI window/pane
    UI.end();
}

// main render loop
while (true)
{
    // synchronize with graphics
    GG.nextFrame() => now;
    
    // update / draw ChuGL UI
    updateUI();

    // check UI and rotate if toggled
    if (rotate.val()) {
        GG.dt() * .3 => mesh.rotateY;
    }
}

/* IDEAS TO EXPLORE FROM HERE
Add builtin textures to apply to materials
Add builtin skybox + IBL lighting
*/
