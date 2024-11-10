/*----------------------------------------------------------------------------
 ChuGL: Unified Audiovisual Programming in ChucK

 Copyright (c) 2023 Andrew Zhu Aday and Ge Wang. All rights reserved.
   http://chuck.stanford.edu/chugl/
   http://chuck.cs.princeton.edu/chugl/

 MIT License

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
-----------------------------------------------------------------------------*/
#pragma once

// tangents follow glTF 2.0 spec
// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes
// see table on vertex attributes
// TODO add tangent (vec4)
// XYZW vertex tangents where the XYZ portion is normalized, and the W component
// is a sign value (-1 or +1) indicating handedness of the tangent basis
// unity code for calculating tangents
// https://discussions.unity.com/t/calculating-tangents-for-the-mesh/321060

// https://github.com/mmikk/MikkTSpace
// tangent space calculation
struct Vertex {
    f32 x, y, z;    // position
    f32 nx, ny, nz; // normal
    f32 u, v;       // uv

    // TODO: static assert offsets match the strides
    // use these offsets to calculate strides when creating vertex buffers

    static void pos(Vertex* v, char c, f32 val);
    static void norm(Vertex* v, char c, f32 val);
};

struct PlaneParams {
    f32 width = 1.0f, height = 1.0f;
    u32 widthSegments = 1, heightSegments = 1;
};

struct SphereParams {
    f32 radius   = 0.5;
    u32 widthSeg = 32, heightSeg = 16;
    f32 phiStart = 0.0, phiLength = 2 * PI; // how much along circumference
    f32 thetaStart = 0.0, thetaLength = PI; // how much along central diameter
};

struct BoxParams {
    f32 width = 1.0f, height = 1.0f, depth = 1.0f;
    u32 widthSeg = 1, heightSeg = 1, depthSeg = 1;
};

struct CircleParams {
    float radius      = .5f;
    int segments      = 32;
    float thetaStart  = 0.0f;
    float thetaLength = PI * 2.0f;
};

struct TorusParams {
    float radius        = 1.0f;
    float tubeRadius    = 0.4f;
    int radialSegments  = 12;
    int tubularSegments = 48;
    float arcLength     = PI * 2.0f;
};

struct CylinderParams {
    float radiusTop             = .2f;
    float radiusBottom          = .2f;
    float height                = 1.0f;
    unsigned int radialSegments = 32;
    unsigned int heightSegments = 1;
    bool openEnded              = false;
    float thetaStart            = 0.0f;
    float thetaLength           = PI * 2.0f;
};

struct KnotParams {
    float radius        = 1.0f;
    float tube          = 0.4f;
    int tubularSegments = 64;
    int radialSegments  = 8;
    int p               = 2;
    int q               = 3;
};

// Arena builders
struct Arena;

struct GeometryArenaBuilder {
    Arena* pos_arena;
    Arena* norm_arena;
    Arena* uv_arena;
    Arena* indices_arena;
};

// void Geometry_computeTangents(GeometryArenaBuilder* builder);
void Geometry_buildPlane(GeometryArenaBuilder* builder, PlaneParams* params);
void Geometry_buildSphere(GeometryArenaBuilder* builder, SphereParams* params);
void Geometry_buildSuzanne(GeometryArenaBuilder* builder);
void Geometry_buildBox(GeometryArenaBuilder* gab, BoxParams* params);
void Geometry_buildCircle(GeometryArenaBuilder* gab, CircleParams* params);
void Geometry_buildTorus(GeometryArenaBuilder* gab, TorusParams* params);
void Geometry_buildCylinder(GeometryArenaBuilder* gab, CylinderParams* params);
void Geometry_buildKnot(GeometryArenaBuilder* gab, KnotParams* params);

#if 0
// add hasColor / hasTangent / has... flags
#define CHUGL_FLOATS_PER_VERTEX (3 + 3 + 2)
struct Vertices {
    u32 vertexCount;
    u32 indicesCount;
    f32* vertexData; // alloc. owned
    u32* indices;    // alloc. owned

    // vertex data stored in single contiguous array
    // [positions | normals | texcoords | tangents]

    static f32* positions(Vertices* v);
    static f32* normals(Vertices* v);
    static f32* texcoords(Vertices* v);
    static f32* tangents(Vertices* v);

    static void buildTangents(Vertices* v);

    // returns offsets to contiguous attribute arrays in bytes
    static size_t positionOffset(Vertices* v);
    static size_t normalOffset(Vertices* v);
    static size_t texcoordOffset(Vertices* v);
    static size_t tangentOffset(Vertices* v);

    static void init(Vertices* v, u32 vertexCount, u32 indicesCount);
    static void setVertex(Vertices* vertices, Vertex v, u32 index);
    static void setIndices(Vertices* vertices, u32 a, u32 b, u32 c, u32 index);
    static void free(Vertices* v);
    static void print(Vertices* v);

    // copy from existing arrays
    static void copy(Vertices* v, Vertex* vertices, u32 vertexCount, u32* indices,
                     u32 indicesCount);

    // shapes
    static void createPlane(Vertices* vertices, PlaneParams* params);
    static void createSphere(Vertices* vertices, SphereParams* params);
};
#endif
