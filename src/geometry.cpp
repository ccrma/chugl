#include "geometry.h"

#include "suzanne_geo.cpp"

#include "core/memory.h"

#include <glm/gtc/epsilon.hpp>

#include <mikktspace/mikktspace.h>

#include <vector> // ew

// ============================================================================
// Vertex
// ============================================================================

void Vertex::pos(Vertex* v, char c, f32 val)
{
    switch (c) {
        case 'x': v->x = val; return;
        case 'y': v->y = val; return;
        case 'z': v->z = val; return;
        default: ASSERT(false); return; // error
    }
}

void Vertex::norm(Vertex* v, char c, f32 val)
{
    switch (c) {
        case 'x': v->nx = val; return;
        case 'y': v->ny = val; return;
        case 'z': v->nz = val; return;
        default: ASSERT(false); return; // error
    }
}

// ============================================================================
// Vertices
// ============================================================================

void Vertices::init(Vertices* v, u32 vertexCount, u32 indicesCount)
{

    ASSERT(indicesCount % 3 == 0); // must be a multiple of 3 triangles only
    ASSERT(v->vertexCount == 0);
    ASSERT(v->indicesCount == 0);

    v->vertexCount  = vertexCount;
    v->indicesCount = indicesCount;
    // TODO: change when adding more vertex attributes
    if (vertexCount > 0)
        v->vertexData = ALLOCATE_COUNT(f32, vertexCount * CHUGL_FLOATS_PER_VERTEX);
    if (indicesCount > 0) v->indices = ALLOCATE_COUNT(u32, indicesCount);
}

void Vertices::print(Vertices* v)
{

    f32* positions = Vertices::positions(v);
    f32* normals   = Vertices::normals(v);
    f32* texcoords = Vertices::texcoords(v);

    printf("Vertices: %d\n", v->vertexCount);
    printf("Indices: %d\n", v->indicesCount);
    // print arrays
    printf("Positions:\n");
    for (u32 i = 0; i < v->vertexCount; i++) {
        printf("\t%f %f %f\n", positions[i * 3 + 0], positions[i * 3 + 1],
               positions[i * 3 + 2]);
    }
    printf("Normals:\n");
    for (u32 i = 0; i < v->vertexCount; i++) {
        printf("\t%f %f %f\n", normals[i * 3 + 0], normals[i * 3 + 1],
               normals[i * 3 + 2]);
    }
    printf("Texcoords:\n");
    for (u32 i = 0; i < v->vertexCount; i++) {
        printf("\t%f %f\n", texcoords[i * 2 + 0], texcoords[i * 2 + 1]);
    }
    printf("Indices:\n");
    for (u32 i = 0; i < v->indicesCount; i += 3) {
        printf("\t%d %d %d\n", v->indices[i], v->indices[i + 1], v->indices[i + 2]);
    }
}

f32* Vertices::positions(Vertices* v)
{
    return v->vertexData;
}

f32* Vertices::normals(Vertices* v)
{
    return v->vertexData + v->vertexCount * 3;
}

f32* Vertices::texcoords(Vertices* v)
{
    return v->vertexData + v->vertexCount * 6;
}

f32* Vertices::tangents(Vertices* v)
{
    return v->vertexData + v->vertexCount * 8;
}

void Vertices::buildTangents(Vertices* v)
{
    SMikkTSpaceInterface mikktspaceIface = {};
    SMikkTSpaceContext mikktspaceContext = {};
    mikktspaceContext.m_pInterface       = &mikktspaceIface;
    mikktspaceContext.m_pUserData        = v;

    mikktspaceIface.m_getNumFaces = [](const SMikkTSpaceContext* pContext) {
        Vertices* v = (Vertices*)pContext->m_pUserData;
        return (int)(v->indicesCount > 0 ? v->indicesCount / 3 : v->vertexCount / 3);
    };

    mikktspaceIface.m_getNumVerticesOfFace
      = [](const SMikkTSpaceContext* pContext, const int iFace) {
            return 3; // triangles only
        };

    mikktspaceIface.m_getPosition
      = [](const SMikkTSpaceContext* pContext, f32 fvPosOut[], const int iFace,
           const int iVert) {
            Vertices* v    = (Vertices*)pContext->m_pUserData;
            u32 index      = (v->indicesCount > 0) ? v->indices[iFace * 3 + iVert] :
                                                     (iFace * 3 + iVert);
            f32* positions = Vertices::positions(v);
            fvPosOut[0]    = positions[index * 3 + 0];
            fvPosOut[1]    = positions[index * 3 + 1];
            fvPosOut[2]    = positions[index * 3 + 2];
        };

    mikktspaceIface.m_getNormal
      = [](const SMikkTSpaceContext* pContext, f32 fvNormOut[], const int iFace,
           const int iVert) {
            Vertices* v  = (Vertices*)pContext->m_pUserData;
            u32 index    = (v->indicesCount > 0) ? v->indices[iFace * 3 + iVert] :
                                                   (iFace * 3 + iVert);
            f32* normals = Vertices::normals(v);
            fvNormOut[0] = normals[index * 3 + 0];
            fvNormOut[1] = normals[index * 3 + 1];
            fvNormOut[2] = normals[index * 3 + 2];
        };

    mikktspaceIface.m_getTexCoord
      = [](const SMikkTSpaceContext* pContext, f32 fvTexcOut[], const int iFace,
           const int iVert) {
            Vertices* v    = (Vertices*)pContext->m_pUserData;
            u32 index      = (v->indicesCount > 0) ? v->indices[iFace * 3 + iVert] :
                                                     (iFace * 3 + iVert);
            f32* texcoords = Vertices::texcoords(v);
            fvTexcOut[0]   = texcoords[index * 2 + 0];
            fvTexcOut[1]   = texcoords[index * 2 + 1];
        };

    mikktspaceIface.m_setTSpaceBasic
      = [](const SMikkTSpaceContext* pContext, const f32 fvTangent[], const f32 fSign,
           const int iFace, const int iVert) {
            Vertices* v   = (Vertices*)pContext->m_pUserData;
            u32 index     = (v->indicesCount > 0) ? v->indices[iFace * 3 + iVert] :
                                                    (iFace * 3 + iVert);
            f32* tangents = Vertices::tangents(v);
            tangents[index * 4 + 0] = fvTangent[0];
            tangents[index * 4 + 1] = fvTangent[1];
            tangents[index * 4 + 2] = fvTangent[2];
            tangents[index * 4 + 3] = fSign;
        };

    genTangSpaceDefault(&mikktspaceContext);
}

size_t Vertices::positionOffset(Vertices* v)
{
    return 0;
}

size_t Vertices::normalOffset(Vertices* v)
{
    return v->vertexCount * 3 * sizeof(f32);
}

size_t Vertices::texcoordOffset(Vertices* v)
{
    return v->vertexCount * 6 * sizeof(f32);
}

size_t Vertices::tangentOffset(Vertices* v)
{
    return v->vertexCount * 8 * sizeof(f32);
}

void Vertices::setVertex(Vertices* vertices, Vertex v, u32 index)
{
    f32* positions = Vertices::positions(vertices);
    f32* normals   = Vertices::normals(vertices);
    f32* texcoords = Vertices::texcoords(vertices);

    // TODO: add tangents here?

    positions[index * 3 + 0] = v.x;
    positions[index * 3 + 1] = v.y;
    positions[index * 3 + 2] = v.z;

    normals[index * 3 + 0] = v.nx;
    normals[index * 3 + 1] = v.ny;
    normals[index * 3 + 2] = v.nz;

    texcoords[index * 2 + 0] = v.u;
    texcoords[index * 2 + 1] = v.v;
}

void Vertices::setIndices(Vertices* vertices, u32 a, u32 b, u32 c, u32 index)
{
    vertices->indices[index * 3 + 0] = a;
    vertices->indices[index * 3 + 1] = b;
    vertices->indices[index * 3 + 2] = c;
}

void Vertices::free(Vertices* v)
{
    reallocate(v->vertexData, sizeof(Vertex) * v->vertexCount, 0);
    reallocate(v->indices, sizeof(u32) * v->indicesCount, 0);
    v->vertexData   = NULL;
    v->indices      = NULL;
    v->vertexCount  = 0;
    v->indicesCount = 0;
}

void Vertices::copy(Vertices* v, Vertex* vertices, u32 vertexCount, u32* indices,
                    u32 indicesCount)
{
    Vertices::init(v, vertexCount, indicesCount);
    f32* positions = Vertices::positions(v);
    f32* normals   = Vertices::normals(v);
    f32* texcoords = Vertices::texcoords(v);

    for (u32 i = 0; i < vertexCount; i++) {
        // no memcpy here in case of compiler adding padding
        positions[i * 3 + 0] = vertices[i].x;
        positions[i * 3 + 1] = vertices[i].y;
        positions[i * 3 + 2] = vertices[i].z;

        normals[i * 3 + 0] = vertices[i].nx;
        normals[i * 3 + 1] = vertices[i].ny;
        normals[i * 3 + 2] = vertices[i].nz;

        texcoords[i * 2 + 0] = vertices[i].u;
        texcoords[i * 2 + 1] = vertices[i].v;

        // TODO tangents
    }

    memcpy(v->indices, indices, indicesCount * sizeof(u32));
}

// ============================================================================
// Plane
// ============================================================================
void Vertices::createPlane(Vertices* vertices, PlaneParams* params)
{
    ASSERT(vertices->vertexCount == 0);
    ASSERT(vertices->indicesCount == 0);

    const f32 width_half  = params->width * 0.5f;
    const f32 height_half = params->height * 0.5f;

    const u32 gridX = params->widthSegments;
    const u32 gridY = params->heightSegments;

    const u32 gridX1 = gridX + 1;
    const u32 gridY1 = gridY + 1;

    const f32 segment_width  = params->width / gridX;
    const f32 segment_height = params->height / gridY;

    Vertices::init(vertices, gridX1 * gridY1, gridX * gridY * 6);

    // f32* positions = Vertices::positions(&vertices);
    // f32* normals   = Vertices::normals(&vertices);
    // f32* texcoords = Vertices::texcoords(&vertices);

    u32 index   = 0;
    Vertex vert = {};
    for (u32 iy = 0; iy < gridY1; iy++) {
        const f32 y = iy * segment_height - height_half;
        for (u32 ix = 0; ix < gridX1; ix++) {
            const f32 x = ix * segment_width - width_half;

            vert = {
                x,  // x
                -y, // y
                0,  // z

                0, // nx
                0, // ny
                1, // nz

                (f32)ix / gridX,          // u
                1.0f - ((f32)iy / gridY), // v
            };

            Vertices::setVertex(vertices, vert, index++);
        }
    }
    ASSERT(index == vertices->vertexCount);
    index = 0;
    for (u32 iy = 0; iy < gridY; iy++) {
        for (u32 ix = 0; ix < gridX; ix++) {
            const u32 a = ix + gridX1 * iy;
            const u32 b = ix + gridX1 * (iy + 1);
            const u32 c = (ix + 1) + gridX1 * (iy + 1);
            const u32 d = (ix + 1) + gridX1 * iy;

            Vertices::setIndices(vertices, a, b, d, index++);
            Vertices::setIndices(vertices, b, c, d, index++);
        }
    }
    ASSERT(index == vertices->indicesCount / 3);
}

void Vertices::createSphere(Vertices* vertices, SphereParams* params)
{
    ASSERT(vertices->vertexCount == 0);
    ASSERT(vertices->indicesCount == 0);

    params->widthSeg  = MAX(3, params->widthSeg);
    params->heightSeg = MAX(2, params->heightSeg);

    const f32 thetaEnd = MIN(params->thetaStart + params->thetaLength, PI);

    u32 index = 0;

    std::vector<u32> grid;
    std::vector<Vertex> verts;
    std::vector<u32> indices;

    // generate vertices, normals and uvs
    for (u32 iy = 0; iy <= params->heightSeg; iy++) {

        const f32 v = (f32)iy / (f32)params->heightSeg;

        // special case for the poles
        f32 uOffset = 0;
        if (iy == 0 && glm::epsilonEqual(params->thetaStart, 0.0f, EPSILON)) {
            uOffset = 0.5f / params->widthSeg;
        } else if (iy == params->heightSeg
                   && glm::epsilonEqual(thetaEnd, PI, EPSILON)) {
            uOffset = -0.5 / params->widthSeg;
        }

        for (u32 ix = 0; ix <= params->widthSeg; ix++) {

            const f32 u = (f32)ix / (f32)params->widthSeg;

            Vertex vert;

            // vertex
            vert.x = -params->radius
                     * glm::cos(params->phiStart + u * params->phiLength)
                     * glm::sin(params->thetaStart + v * params->thetaLength);
            vert.y
              = params->radius * glm::cos(params->thetaStart + v * params->thetaLength);
            vert.z = params->radius * glm::sin(params->phiStart + u * params->phiLength)
                     * glm::sin(params->thetaStart + v * params->thetaLength);

            // normal
            glm::vec3 normal = glm::normalize(glm::vec3(vert.x, vert.y, vert.z));
            vert.nx          = normal.x;
            vert.ny          = normal.y;
            vert.nz          = normal.z;

            // uv
            vert.u = u + uOffset;
            vert.v = 1 - v;

            verts.push_back(vert);
            grid.push_back(index++);
        }
    }

    const size_t rowSize = (size_t)params->widthSeg + 1;
    for (size_t iy = 0; iy < params->heightSeg; iy++) {
        for (size_t ix = 0; ix < params->widthSeg; ix++) {

            const u32 a = grid[(iy * rowSize) + ix + 1];
            const u32 b = grid[(iy * rowSize) + ix];
            const u32 c = grid[(rowSize * (iy + 1)) + ix];
            const u32 d = grid[rowSize * (iy + 1) + (ix + 1)];

            if (iy != 0 || params->thetaStart > EPSILON) {
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(d);
            }
            if (iy != (size_t)params->heightSeg - 1 || thetaEnd < PI - EPSILON) {
                indices.push_back(b);
                indices.push_back(c);
                indices.push_back(d);
            }
        }
    }

    Vertices::copy(vertices, verts.data(), (u32)verts.size(), indices.data(),
                   (u32)indices.size());
}

// ============================================================================
// Arena builders
// ============================================================================

struct gvec3i {
    u32 x, y, z;
};

struct gvec2f {
    f32 x, y;
};

struct gvec3f {
    f32 x, y, z;

    f32* comp(char c)
    {
        switch (c) {
            case 'x': return &x;
            case 'y': return &y;
            case 'z': return &z;
            default: ASSERT(false); return NULL;
        }
    }
};

struct gvec4f {
    f32 x, y, z, w;
};

static int GAB_indicesCount(GeometryArenaBuilder* builder)
{
    return ARENA_LENGTH(builder->indices_arena, u32);
}

static int GAB_vertexCount(GeometryArenaBuilder* builder)
{
    return ARENA_LENGTH(builder->pos_arena, gvec3f);
}

static int GAB_faceCount(GeometryArenaBuilder* builder)
{
    int indices_count = GAB_indicesCount(builder);
    int ret = (indices_count > 0) ? indices_count / 3 : GAB_vertexCount(builder) / 3;
    return ret;
}

static int GAB_indexFromFace(GeometryArenaBuilder* builder, int iFace, int iVert)
{
    int indices_count  = GAB_indicesCount(builder);
    u32* indices_array = (u32*)(builder->indices_arena->base);
    return (indices_count > 0) ? indices_array[iFace * 3 + iVert] : (iFace * 3 + iVert);
}

void Geometry_computeTangents(GeometryArenaBuilder* builder)
{
    SMikkTSpaceInterface mikktspaceIface = {};
    SMikkTSpaceContext mikktspaceContext = {};
    mikktspaceContext.m_pInterface       = &mikktspaceIface;
    mikktspaceContext.m_pUserData        = builder;

    // allocate tangent memory if not already
    if (ARENA_LENGTH(builder->tangent_arena, gvec4f) == 0) {
        int vertex_count = GAB_vertexCount(builder);
        ARENA_PUSH_COUNT(builder->tangent_arena, gvec4f, GAB_vertexCount(builder));
        ASSERT(ARENA_LENGTH(builder->tangent_arena, gvec4f) == vertex_count)
    }

    mikktspaceIface.m_getNumFaces = [](const SMikkTSpaceContext* pContext) {
        return GAB_faceCount((GeometryArenaBuilder*)pContext->m_pUserData);
    };

    mikktspaceIface.m_getNumVerticesOfFace
      = [](const SMikkTSpaceContext* pContext, const int iFace) {
            return 3; // triangles only
        };

    mikktspaceIface.m_getPosition = [](const SMikkTSpaceContext* pContext,
                                       f32 fvPosOut[], const int iFace,
                                       const int iVert) {
        GeometryArenaBuilder* builder = (GeometryArenaBuilder*)pContext->m_pUserData;

        int index = GAB_indexFromFace(builder, iFace, iVert);

        f32* positions = (f32*)(builder->pos_arena->base);
        fvPosOut[0]    = positions[index * 3 + 0];
        fvPosOut[1]    = positions[index * 3 + 1];
        fvPosOut[2]    = positions[index * 3 + 2];
    };

    mikktspaceIface.m_getNormal = [](const SMikkTSpaceContext* pContext,
                                     f32 fvNormOut[], const int iFace,
                                     const int iVert) {
        GeometryArenaBuilder* builder = (GeometryArenaBuilder*)pContext->m_pUserData;

        int index = GAB_indexFromFace(builder, iFace, iVert);

        f32* normals = (f32*)(builder->norm_arena->base);
        fvNormOut[0] = normals[index * 3 + 0];
        fvNormOut[1] = normals[index * 3 + 1];
        fvNormOut[2] = normals[index * 3 + 2];
    };

    mikktspaceIface.m_getTexCoord = [](const SMikkTSpaceContext* pContext,
                                       f32 fvTexcOut[], const int iFace,
                                       const int iVert) {
        GeometryArenaBuilder* builder = (GeometryArenaBuilder*)pContext->m_pUserData;

        int index = GAB_indexFromFace(builder, iFace, iVert);

        f32* texcoords = (f32*)(builder->uv_arena->base);
        fvTexcOut[0]   = texcoords[index * 2 + 0];
        fvTexcOut[1]   = texcoords[index * 2 + 1];
    };

    mikktspaceIface.m_setTSpaceBasic = [](const SMikkTSpaceContext* pContext,
                                          const f32 fvTangent[], const f32 fSign,
                                          const int iFace, const int iVert) {
        GeometryArenaBuilder* builder = (GeometryArenaBuilder*)pContext->m_pUserData;

        int index = GAB_indexFromFace(builder, iFace, iVert);

        gvec4f* tangents = (gvec4f*)(builder->tangent_arena->base);
        // make sure index within arena bounds
        int tangent_alloc_length = ARENA_LENGTH(builder->tangent_arena, gvec4f);
        ASSERT(index < tangent_alloc_length);
        tangents[index] = { fvTangent[0], fvTangent[1], fvTangent[2], fSign };
    };

    genTangSpaceDefault(&mikktspaceContext);
}

void Geometry_buildPlane(GeometryArenaBuilder* builder, PlaneParams* params)
{
    const f32 width_half  = params->width * 0.5f;
    const f32 height_half = params->height * 0.5f;

    const u32 gridX = params->widthSegments;
    const u32 gridY = params->heightSegments;

    const u32 gridX1 = gridX + 1;
    const u32 gridY1 = gridY + 1;

    const f32 segment_width  = params->width / gridX;
    const f32 segment_height = params->height / gridY;

    const u32 vertex_count    = gridX1 * gridY1;
    const u32 index_tri_count = gridX * gridY * 2;

    // initialize arena memory
    gvec3f* pos_array  = ARENA_PUSH_COUNT(builder->pos_arena, gvec3f, vertex_count);
    gvec3f* norm_array = ARENA_PUSH_COUNT(builder->norm_arena, gvec3f, vertex_count);
    gvec2f* uv_array   = ARENA_PUSH_COUNT(builder->uv_arena, gvec2f, vertex_count);
    gvec4f* tangent_array
      = ARENA_PUSH_COUNT(builder->tangent_arena, gvec4f, vertex_count);
    gvec3i* indices_array
      = ARENA_PUSH_COUNT(builder->indices_arena, gvec3i, index_tri_count);

    u32 index = 0;
    for (u32 iy = 0; iy < gridY1; iy++) {
        const f32 y = iy * segment_height - height_half;
        for (u32 ix = 0; ix < gridX1; ix++) {
            const f32 x          = ix * segment_width - width_half;
            pos_array[index]     = { x, -y, 0 };
            norm_array[index]    = { 0, 0, 1 };
            uv_array[index]      = { (f32)ix / gridX, 1.0f - ((f32)iy / gridY) };
            tangent_array[index] = { 1, 0, 0, 1 };

            ++index;
        }
    }
    ASSERT(index == vertex_count);

    index = 0;
    for (u32 iy = 0; iy < gridY; iy++) {
        for (u32 ix = 0; ix < gridX; ix++) {
            const u32 a = ix + gridX1 * iy;
            const u32 b = ix + gridX1 * (iy + 1);
            const u32 c = (ix + 1) + gridX1 * (iy + 1);
            const u32 d = (ix + 1) + gridX1 * iy;

            indices_array[index++] = { a, b, d };
            indices_array[index++] = { b, c, d };
        }
    }
    ASSERT(index == index_tri_count);
    ASSERT(ARENA_LENGTH(builder->tangent_arena, gvec4f) == vertex_count);
}

void Geometry_buildSphere(GeometryArenaBuilder* builder, SphereParams* params)
{

    params->widthSeg  = MAX(3, params->widthSeg);
    params->heightSeg = MAX(2, params->heightSeg);

    const f32 thetaEnd = MIN(params->thetaStart + params->thetaLength, PI);

    u32 index = 0;

    std::vector<u32> grid;
    std::vector<Vertex> verts;
    std::vector<u32> indices;

    // generate vertices, normals and uvs
    for (u32 iy = 0; iy <= params->heightSeg; iy++) {

        const f32 v = (f32)iy / (f32)params->heightSeg;

        // special case for the poles
        f32 uOffset = 0;
        if (iy == 0 && glm::epsilonEqual(params->thetaStart, 0.0f, EPSILON)) {
            uOffset = 0.5f / params->widthSeg;
        } else if (iy == params->heightSeg
                   && glm::epsilonEqual(thetaEnd, PI, EPSILON)) {
            uOffset = -0.5 / params->widthSeg;
        }

        for (u32 ix = 0; ix <= params->widthSeg; ix++) {

            const f32 u = (f32)ix / (f32)params->widthSeg;

            Vertex vert;

            // vertex
            vert.x = -params->radius
                     * glm::cos(params->phiStart + u * params->phiLength)
                     * glm::sin(params->thetaStart + v * params->thetaLength);
            vert.y
              = params->radius * glm::cos(params->thetaStart + v * params->thetaLength);
            vert.z = params->radius * glm::sin(params->phiStart + u * params->phiLength)
                     * glm::sin(params->thetaStart + v * params->thetaLength);

            // normal
            glm::vec3 normal = glm::normalize(glm::vec3(vert.x, vert.y, vert.z));
            vert.nx          = normal.x;
            vert.ny          = normal.y;
            vert.nz          = normal.z;

            // uv
            vert.u = u + uOffset;
            vert.v = 1 - v;

            verts.push_back(vert);
            grid.push_back(index++);
        }
    }

    const size_t rowSize = (size_t)params->widthSeg + 1;
    for (size_t iy = 0; iy < params->heightSeg; iy++) {
        for (size_t ix = 0; ix < params->widthSeg; ix++) {

            const u32 a = grid[(iy * rowSize) + ix + 1];
            const u32 b = grid[(iy * rowSize) + ix];
            const u32 c = grid[(rowSize * (iy + 1)) + ix];
            const u32 d = grid[rowSize * (iy + 1) + (ix + 1)];

            if (iy != 0 || params->thetaStart > EPSILON) {
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(d);
            }
            if (iy != (size_t)params->heightSeg - 1 || thetaEnd < PI - EPSILON) {
                indices.push_back(b);
                indices.push_back(c);
                indices.push_back(d);
            }
        }
    }

    size_t vertex_count = verts.size();
    size_t index_count  = indices.size();
    ASSERT(index_count % 3 == 0); // must be a multiple of 3 triangles only

    gvec3f* positions  = ARENA_PUSH_COUNT(builder->pos_arena, gvec3f, vertex_count);
    gvec3f* normals    = ARENA_PUSH_COUNT(builder->norm_arena, gvec3f, vertex_count);
    gvec2f* texcoords  = ARENA_PUSH_COUNT(builder->uv_arena, gvec2f, vertex_count);
    gvec4f* tangents   = ARENA_PUSH_COUNT(builder->tangent_arena, gvec4f, vertex_count);
    u32* indices_array = ARENA_PUSH_COUNT(builder->indices_arena, u32, index_count);
    UNUSED_VAR(tangents);

    for (u32 i = 0; i < vertex_count; i++) {
        positions[i] = { verts[i].x, verts[i].y, verts[i].z };
        normals[i]   = { verts[i].nx, verts[i].ny, verts[i].nz };
        texcoords[i] = { verts[i].u, verts[i].v };
    }

    memcpy(indices_array, indices.data(), indices.size() * sizeof(*indices_array));
    ASSERT(ARENA_LENGTH(builder->indices_arena, u32) == index_count);

    // build tangents
    Geometry_computeTangents(builder);
    ASSERT(ARENA_LENGTH(builder->tangent_arena, gvec4f) == vertex_count);
}

void Geometry_buildSuzanne(GeometryArenaBuilder* builder)
{
    // copy data.
    f32* positions
      = ARENA_PUSH_COUNT(builder->pos_arena, f32, ARRAY_LENGTH(suzanne_positions));
    f32* normals
      = ARENA_PUSH_COUNT(builder->norm_arena, f32, ARRAY_LENGTH(suzanne_normals));
    f32* texcoords
      = ARENA_PUSH_COUNT(builder->uv_arena, f32, ARRAY_LENGTH(suzanne_uvs));
    f32* tangents
      = ARENA_PUSH_COUNT(builder->tangent_arena, f32, ARRAY_LENGTH(suzanne_tangents));

    memcpy(positions, suzanne_positions, sizeof(suzanne_positions));
    memcpy(normals, suzanne_normals, sizeof(suzanne_normals));
    memcpy(texcoords, suzanne_uvs, sizeof(suzanne_uvs));
    memcpy(tangents, suzanne_tangents, sizeof(suzanne_tangents));
}

// Box ============================================================================

static void Geometry_Box_buildPlane(GeometryArenaBuilder* gab, char u, char v, char w,
                                    int udir, int vdir, float width, float height,
                                    float depth, int gridX, int gridY)
{

    const float segmentWidth  = width / (float)gridX;
    const float segmentHeight = height / (float)gridY;

    const float widthHalf  = width / 2.0f;
    const float heightHalf = height / 2.0f;
    const float depthHalf  = depth / 2.0f;

    const int gridX1 = gridX + 1;
    const int gridY1 = gridY + 1;

    // save number of vertices BEFORE adding any this round
    // used to figure out indices
    const int numberOfVertices = GAB_vertexCount(gab);
    gvec3f* pos_array  = ARENA_PUSH_COUNT(gab->pos_arena, gvec3f, gridX1 * gridY1);
    gvec3f* norm_array = ARENA_PUSH_COUNT(gab->norm_arena, gvec3f, gridX1 * gridY1);
    gvec2f* uv_array   = ARENA_PUSH_COUNT(gab->uv_arena, gvec2f, gridX1 * gridY1);
    gvec3i* indices_array
      = ARENA_PUSH_COUNT(gab->indices_arena, gvec3i, gridX * gridY * 2);

    // generate vertices, normals and uvs
    int index = 0;
    for (int iy = 0; iy < gridY1; iy++) {
        const float y = iy * segmentHeight - heightHalf;
        for (int ix = 0; ix < gridX1; ix++) {
            const float x = ix * segmentWidth - widthHalf;

            gvec3f* pos   = &pos_array[index];
            *pos->comp(u) = x * udir;
            *pos->comp(v) = y * vdir;
            *pos->comp(w) = depthHalf;

            // set normals
            gvec3f* norm   = &norm_array[index];
            *norm          = { 0, 0, 0 };
            *norm->comp(w) = depth > 0 ? 1 : -1;

            // set uvs
            uv_array[index] = { (float)ix / gridX, 1.0f - (float)iy / gridY };

            ++index;
        }
    }

    // indices

    // 1. you need three indices to draw a single face
    // 2. a single segment consists of two faces
    // 3. so we need to generate six (2*3) indices per segment

    index = 0;
    for (int iy = 0; iy < gridY; iy++) {
        for (int ix = 0; ix < gridX; ix++) {

            unsigned int a = numberOfVertices + ix + gridX1 * iy;
            unsigned int b = numberOfVertices + ix + gridX1 * (iy + 1);
            unsigned int c = numberOfVertices + (ix + 1) + gridX1 * (iy + 1);
            unsigned int d = numberOfVertices + (ix + 1) + gridX1 * iy;

            // faces
            indices_array[index++] = { a, b, d };
            indices_array[index++] = { b, c, d };
        }
    }
}

void Geometry_buildBox(GeometryArenaBuilder* gab, BoxParams* params)
{
    Geometry_Box_buildPlane(gab, 'z', 'y', 'x', -1, -1, params->depth, params->height,
                            params->width, params->depthSeg, params->heightSeg); // px
    Geometry_Box_buildPlane(gab, 'z', 'y', 'x', 1, -1, params->depth, params->height,
                            -params->width, params->depthSeg, params->heightSeg); // nx
    Geometry_Box_buildPlane(gab, 'x', 'z', 'y', 1, 1, params->width, params->depth,
                            params->height, params->widthSeg, params->depthSeg); // py
    Geometry_Box_buildPlane(gab, 'x', 'z', 'y', 1, -1, params->width, params->depth,
                            -params->height, params->widthSeg, params->depthSeg); // ny
    Geometry_Box_buildPlane(gab, 'x', 'y', 'z', 1, -1, params->width, params->height,
                            params->depth, params->widthSeg, params->heightSeg); // pz
    Geometry_Box_buildPlane(gab, 'x', 'y', 'z', -1, -1, params->width, params->height,
                            -params->depth, params->widthSeg, params->heightSeg); // nz

    // build tangents
    Geometry_computeTangents(gab);
}

void Geometry_buildCircle(GeometryArenaBuilder* gab, CircleParams* params)
{
    const int num_vertices = params->segments + 2;
    const int num_indices  = params->segments;

    gvec3f* positions     = ARENA_PUSH_COUNT(gab->pos_arena, gvec3f, num_vertices);
    gvec3f* normals       = ARENA_PUSH_COUNT(gab->norm_arena, gvec3f, num_vertices);
    gvec2f* uvs           = ARENA_PUSH_COUNT(gab->uv_arena, gvec2f, num_vertices);
    gvec4f* tangents      = ARENA_PUSH_COUNT(gab->tangent_arena, gvec4f, num_vertices);
    gvec3i* indices_array = ARENA_PUSH_COUNT(gab->indices_arena, gvec3i, num_indices);

    // center vertex
    positions[0] = { 0, 0, 0 };
    normals[0]   = { 0, 0, 1.0f };
    uvs[0]       = { 0.5f, 0.5f };

    int index = 1;
    for (int s = 0, i = 3; s <= params->segments; s++, i += 3) {
        const float segment
          = params->thetaStart
            + (float)s / (float)params->segments * params->thetaLength;

        // vertex
        positions[index] = { params->radius * glm::cos(segment),
                             params->radius * glm::sin(segment), 0 };

        // normal
        normals[index] = { 0, 0, 1.0f };

        // uvs
        uvs[index] = { (positions[index].x / params->radius + 1.0f) / 2.0f,
                       (positions[index].y / params->radius + 1.0f) / 2.0f };

        // tangents
        tangents[index] = { 1, 0, 0, 1 };

        index++;
    }

    // indices
    index = 0;
    for (u32 i = 1; i <= params->segments; i++) {
        indices_array[index++] = { i, i + 1, 0 };
    }
}

void Geometry_buildTorus(GeometryArenaBuilder* gab, TorusParams* params)
{
    const int num_vertices
      = (params->radialSegments + 1) * (params->tubularSegments + 1);
    const int num_indices = params->radialSegments * params->tubularSegments * 2;

    ASSERT(sizeof(glm::vec3) == sizeof(gvec3f));
    glm::vec3* positions = ARENA_PUSH_COUNT(gab->pos_arena, glm::vec3, num_vertices);
    glm::vec3* normals   = ARENA_PUSH_COUNT(gab->norm_arena, glm::vec3, num_vertices);
    gvec2f* uvs          = ARENA_PUSH_COUNT(gab->uv_arena, gvec2f, num_vertices);
    gvec3i* indices      = ARENA_PUSH_COUNT(gab->indices_arena, gvec3i, num_indices);

    int index = 0;
    for (int j = 0; j <= params->radialSegments; j++) {
        for (int i = 0; i <= params->tubularSegments; i++) {
            const float u
              = (float)i / (float)params->tubularSegments * params->arcLength;
            const float v = (float)j / (float)params->radialSegments * PI * 2.0f;

            // vertex
            positions[index]
              = { (params->radius + params->tubeRadius * glm::cos(v)) * glm::cos(u),
                  (params->radius + params->tubeRadius * glm::cos(v)) * glm::sin(u),
                  params->tubeRadius * glm::sin(v) };

            // normal
            glm::vec3 center
              = { params->radius * glm::cos(u), params->radius * glm::sin(u), 0 };
            normals[index] = glm::normalize(positions[index] - center);

            // uv
            uvs[index] = { (float)i / (float)params->tubularSegments,
                           (float)j / (float)params->radialSegments };

            index++;
        }
    }

    // generate indices
    index = 0;
    for (u32 j = 1; j <= params->radialSegments; j++) {
        for (u32 i = 1; i <= params->tubularSegments; i++) {
            // indices
            const u32 a = (params->tubularSegments + 1) * j + i - 1;
            const u32 b = (params->tubularSegments + 1) * (j - 1) + i - 1;
            const u32 c = (params->tubularSegments + 1) * (j - 1) + i;
            const u32 d = (params->tubularSegments + 1) * j + i;

            indices[index++] = { a, b, d };
            indices[index++] = { b, c, d };
        }
    }

    // build tangents
    Geometry_computeTangents(gab);
}

// Cylinder ============================================================================
static void Geometry_Cylinder_GenerateTorso(GeometryArenaBuilder* gab,
                                            const CylinderParams& p)
{
    const float halfHeight = p.height / 2.0f;

    const int num_vertices = (p.heightSegments + 1) * (p.radialSegments + 1);
    const int num_indices  = p.heightSegments * p.radialSegments * 2;
    gvec3f* positions      = ARENA_PUSH_COUNT(gab->pos_arena, gvec3f, num_vertices);
    gvec3f* normals        = ARENA_PUSH_COUNT(gab->norm_arena, gvec3f, num_vertices);
    gvec2f* uvs            = ARENA_PUSH_COUNT(gab->uv_arena, gvec2f, num_vertices);
    gvec3i* indices        = ARENA_PUSH_COUNT(gab->indices_arena, gvec3i, num_indices);

    static Arena index_array_arena{};
    ASSERT(ARENA_LENGTH(&index_array_arena, int) == 0);
    int* indexArray = ARENA_PUSH_COUNT(&index_array_arena, int, num_vertices);
    defer(Arena::clear(&index_array_arena));

    // this will be used to calculate the normal
    const float slope = (p.radiusBottom - p.radiusTop) / p.height;

    // generate vertices, normals and uvs
    int index = 0;
    for (unsigned int y = 0; y <= p.heightSegments; y++) {

        std::vector<unsigned int> indexRow;

        const float v = (float)y / (float)p.heightSegments;

        // calculate the radius of the current row
        const float radius = v * (p.radiusBottom - p.radiusTop) + p.radiusTop;

        for (unsigned int x = 0; x <= p.radialSegments; x++) {
            const float u = (float)x / (float)p.radialSegments;

            const float theta = u * p.thetaLength + p.thetaStart;

            const float sinTheta = glm::sin(theta);
            const float cosTheta = glm::cos(theta);

            // vertex
            positions[index]
              = { radius * sinTheta, -v * p.height + halfHeight, radius * cosTheta };

            // normal
            glm::vec3 normal = glm::normalize(glm::vec3(sinTheta, slope, cosTheta));
            normals[index]   = { normal.x, normal.y, normal.z };

            // uv
            uvs[index] = { u, 1.0f - v };

            indexArray[y * (p.radialSegments + 1) + x] = index;
            index++;
        }
    }

    // generate indices
    index = 0;
    for (unsigned int x = 0; x < p.radialSegments; x++) {
        for (unsigned int y = 0; y < p.heightSegments; y++) {

            // we use the index array to access the correct indices
            const unsigned int a = indexArray[y * (p.radialSegments + 1) + x];
            const unsigned int b = indexArray[(y + 1) * (p.radialSegments + 1) + x];
            const unsigned int c = indexArray[(y + 1) * (p.radialSegments + 1) + x + 1];
            const unsigned int d = indexArray[y * (p.radialSegments + 1) + x + 1];

            // faces
            indices[index++] = { a, b, d };
            indices[index++] = { b, c, d };
        }
    }
}

static void Geometry_Cylinder_GenerateCap(GeometryArenaBuilder* gab,
                                          const CylinderParams& p, bool top)
{
    const float halfHeight = p.height / 2.0f;

    // save the index of the first center vertex
    const int centerIndexStart = GAB_vertexCount(gab);

    const int num_vertices = 2 * p.radialSegments + 1;
    const int num_indices  = p.radialSegments;

    gvec3f* positions = ARENA_PUSH_COUNT(gab->pos_arena, gvec3f, num_vertices);
    gvec3f* normals   = ARENA_PUSH_COUNT(gab->norm_arena, gvec3f, num_vertices);
    gvec2f* uvs       = ARENA_PUSH_COUNT(gab->uv_arena, gvec2f, num_vertices);
    gvec3i* indices   = ARENA_PUSH_COUNT(gab->indices_arena, gvec3i, num_indices);

    const float radius = top ? p.radiusTop : p.radiusBottom;
    const float sign   = top ? 1.0f : -1.0f;

    // first we generate the center vertex data of the cap.
    // because the geometry needs one set of uvs per face,
    // we must generate a center vertex per face/segment
    int index = 0;
    for (unsigned int x = 1; x <= p.radialSegments; x++) {
        positions[index] = { 0.0, halfHeight * sign, 0.0 };
        normals[index]   = { 0.0, sign, 0.0 };
        uvs[index]       = { 0.5, 0.5 };

        index++;
    }

    // save the index of the last center vertex
    const unsigned int centerIndexEnd = index + centerIndexStart;

    // now we generate the surrounding vertices, normals and uvs
    for (unsigned int x = 0; x <= p.radialSegments; x++) {

        const float u     = (float)x / (float)p.radialSegments;
        const float theta = u * p.thetaLength + p.thetaStart;

        const float cosTheta = glm::cos(theta);
        const float sinTheta = glm::sin(theta);

        // vertex
        positions[index] = { radius * sinTheta, halfHeight * sign, radius * cosTheta };

        // normal
        normals[index] = { 0.0, sign, 0.0 };

        // uv
        uvs[index] = { cosTheta * 0.5f + 0.5f, sinTheta * 0.5f * sign + 0.5f };

        // increase index
        index++;
    }

    // generate indices
    for (unsigned int x = 0; x < p.radialSegments; x++) {
        const unsigned int c = centerIndexStart + x;
        const unsigned int i = centerIndexEnd + x;

        if (top) {
            indices[x] = { i, i + 1, c };
        } else {
            indices[x] = { i + 1, i, c };
        }
    }
}

void Geometry_buildCylinder(GeometryArenaBuilder* gab, CylinderParams* params)
{
    // generate torso
    Geometry_Cylinder_GenerateTorso(gab, *params);

    if (!params->openEnded) {
        if (params->radiusTop > 0) Geometry_Cylinder_GenerateCap(gab, *params, true);
        if (params->radiusBottom > 0)
            Geometry_Cylinder_GenerateCap(gab, *params, false);
    }

    // build tangents
    Geometry_computeTangents(gab);
}

// Knot ============================================================================

static void Geometry_Knot_calculatePositionOnCurve(float u, int p, int q, float radius,
                                                   glm::vec3& position)
{

    const float quOverP = (f32)q / p * u;
    const float cs      = glm::cos(quOverP);

    position.x = radius * (2 + cs) * 0.5 * glm::cos(u);
    position.y = radius * (2 + cs) * glm::sin(u) * 0.5;
    position.z = radius * glm::sin(quOverP) * 0.5;
}

void Geometry_buildKnot(GeometryArenaBuilder* gab, KnotParams* params)
{
    // buffers
    const int num_vertices
      = (params->tubularSegments + 1) * (params->radialSegments + 1);
    const int num_indices = 2 * params->tubularSegments * params->radialSegments;

    glm::vec3* positions = ARENA_PUSH_COUNT(gab->pos_arena, glm::vec3, num_vertices);
    glm::vec3* normals   = ARENA_PUSH_COUNT(gab->norm_arena, glm::vec3, num_vertices);
    gvec2f* uvs          = ARENA_PUSH_COUNT(gab->uv_arena, gvec2f, num_vertices);
    gvec3i* indices      = ARENA_PUSH_COUNT(gab->indices_arena, gvec3i, num_indices);

    glm::vec3 P1 = glm::vec3(0.0f);
    glm::vec3 P2 = glm::vec3(0.0f);

    glm::vec3 B = glm::vec3(0.0f);
    glm::vec3 T = glm::vec3(0.0f);
    glm::vec3 N = glm::vec3(0.0f);

    // generate vertices, normals and uvs
    int index = 0;
    for (int i = 0; i <= params->tubularSegments; ++i) {

        // the radian "u" is used to calculate the position on the torus curve of the
        // current tubular segment

        const float u = (f32)i / params->tubularSegments * params->p * PI * 2.0f;

        // now we calculate two points. P1 is our current position on the curve, P2 is a
        // little farther ahead. these points are used to create a special "coordinate
        // space", which is necessary to calculate the correct vertex positions

        Geometry_Knot_calculatePositionOnCurve(u, params->p, params->q, params->radius,
                                               P1);
        Geometry_Knot_calculatePositionOnCurve(u + 0.01, params->p, params->q,
                                               params->radius, P2);

        // calculate orthonormal basis
        T = P2 - P1;
        N = P2 + P1;
        B = glm::normalize(glm::cross(T, N));
        N = glm::normalize(glm::cross(B, T));

        for (int j = 0; j <= params->radialSegments; ++j) {

            // now calculate the vertices. they are nothing more than an extrusion of
            // the torus curve. because we extrude a shape in the xy-plane, there is no
            // need to calculate a z-value.

            const float v  = (f32)j / params->radialSegments * PI * 2.0f;
            const float cx = -params->tube * glm::cos(v);
            const float cy = params->tube * glm::sin(v);

            // now calculate the final vertex position.
            // first we orient the extrusion with our basis vectors, then we add it to
            // the current position on the curve

            positions[index].x = P1.x + (cx * N.x + cy * B.x);
            positions[index].y = P1.y + (cx * N.y + cy * B.y);
            positions[index].z = P1.z + (cx * N.z + cy * B.z);

            // normal (P1 is always the center/origin of the extrusion, thus we can use
            // it to calculate the normal)

            normals[index] = glm::normalize(positions[index] - P1);

            // uv
            uvs[index].x = (float)i / params->tubularSegments;
            uvs[index].y = (float)j / params->radialSegments;

            index++;
        }
    }
    ASSERT(index == num_vertices);

    // generate indices

    index = 0;
    for (int j = 1; j <= params->tubularSegments; j++) {
        for (int i = 1; i <= params->radialSegments; i++) {

            // indices
            const u32 a = (params->radialSegments + 1) * (j - 1) + (i - 1);
            const u32 b = (params->radialSegments + 1) * j + (i - 1);
            const u32 c = (params->radialSegments + 1) * j + i;
            const u32 d = (params->radialSegments + 1) * (j - 1) + i;

            // faces
            indices[index++] = { a, b, d };
            indices[index++] = { b, c, d };
        }
    }
    ASSERT(index == num_indices);

    // build tangents
    Geometry_computeTangents(gab);
}
