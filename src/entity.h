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

#include "core/macros.h"

#include "geometry.h"
#include "graphics.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp> // quatToMat4

#define QUAT_IDENTITY (glm::quat(1.0, 0.0, 0.0, 0.0))
#define MAT_IDENTITY (glm::mat4(1.0))

#define VEC_ORIGIN (glm::vec3(0.0f, 0.0f, 0.0f))
#define VEC_UP (glm::vec3(0.0f, 1.0f, 0.0f))
#define VEC_DOWN (glm::vec3(0.0f, -1.0f, 0.0f))
#define VEC_LEFT (glm::vec3(-1.0f, 0.0f, 0.0f))
#define VEC_RIGHT (glm::vec3(1.0f, 0.0f, 0.0f))
#define VEC_FORWARD (glm::vec3(0.0f, 0.0f, -1.0f))
#define VEC_BACKWARD (glm::vec3(0.0f, 0.0f, 1.0f))

enum EntityType {
    ENTITY_TYPE_NONE = 0,
    ENTITY_TYPE_COUNT,
};

struct Entity {
    u64 _id;
    EntityType type;

    // transform
    glm::vec3 pos;
    glm::quat rot;
    glm::vec3 sca;

    // cpu geometry (TODO share across entities)
    Vertices vertices;
    // gpu geometry (renderable) (TODO share across entities)
    VertexBuffer gpuVertices;
    IndexBuffer gpuIndices;

    // BindGroupEntry to hold model uniform buffer
    // currently only model matrices
    BindGroup bindGroup;

    // material (TODO share across entities)

    static void init(Entity* entity, GraphicsContext* ctx,
                     WGPUBindGroupLayout bindGroupLayout);

    static void setVertices(Entity* entity, Vertices* vertices, GraphicsContext* ctx);

    static glm::mat4 modelMatrix(Entity* entity); // TODO cache
    static glm::mat4 viewMatrix(Entity* entity);
    static glm::mat4 projectionMatrix(Entity* entity, f32 aspect);

    static void rotateOnLocalAxis(Entity* entity, glm::vec3 axis, f32 deg);
    static void rotateOnWorldAxis(Entity* entity, glm::vec3 axis, f32 deg);

    static void lookAt(Entity* entity, glm::vec3 target);

    // Idea: just store world matrix and decompose it when needed?
    // TODO: for getting world position, rotation, scale, etc.
    // glm::decompose(matrix, decomposedScale, decomposedRotationQuat,
    // decomposedPosition, skewUnused, perspectiveUnused);
    // https://stackoverflow.com/questions/17918033/glm-decompose-mat4-into-translation-and-rotation
    /* OR
    void decomposeMatrix(const glm::mat4& m, glm::vec3& pos, glm::quat& rot,
    glm::vec3& scale)
    {
        pos = m[3];
        for(int i = 0; i < 3; i++)
            scale[i] = glm::length(vec3(m[i]));

        // PROBLEM: doesn't handle negative scale
        // negate sx if determinant is negative

        const glm::mat3 rotMtx(
            glm::vec3(m[0]) / scale[0],
            glm::vec3(m[1]) / scale[1],
            glm::vec3(m[2]) / scale[2]);
        rot = glm::quat_cast(rotMtx);
    }
    // THREEjs impl here:
    https://github.com/mrdoob/three.js/blob/ef80ac74e6716a50104a57d8add6c8a950bff8d7/src/math/Matrix4.js#L732
    */
};
