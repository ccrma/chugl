#pragma once

// arc camera impl with velocity / dampening
// https://webgpu.github.io/webgpu-samples/?sample=cameras#camera.ts
// Original Arcball camera paper?
// https://www.talisman.org/~erlkonig/misc/shoemake92-arcball.pdf

#include "entity.h"

struct Spherical {
    f32 radius;
    f32 theta; // polar (radians)
    f32 phi;   // azimuth (radians)

    // Left handed system
    // (1, 0, 0) maps to cartesion coordinate (0, 0, 1)
    static glm::vec3 toCartesian(Spherical s)
    {
        f32 v = s.radius * cos(s.phi);
        return glm::vec3(v * sin(s.theta),      // x
                         s.radius * sin(s.phi), // y
                         v * cos(s.theta)       // z
        );
    }
};

struct Camera {
    Entity entity;
    Spherical spherical;

    // camera
    f32 fovDegrees;
    f32 nearPlane;
    f32 farPlane;

    // controls
    bool mouseDown;
    f64 lastMouseX;
    f64 lastMouseY;

    static void init(Camera* camera)
    {
        Entity::init(&camera->entity, NULL, NULL);

        // set spherical radius so camera is looking at origin
        camera->spherical.radius = 6.0f;

        // init camera params
        camera->fovDegrees = 45.0f;
        camera->nearPlane  = 0.1f;
        camera->farPlane   = 1000.0f;
    }

    static glm::mat4 projectionMatrix(Camera* camera, f32 aspect)
    {
        return glm::perspective(glm::radians(camera->fovDegrees), aspect,
                                camera->nearPlane, camera->farPlane);
    }

    static void update(Camera* camera, f32 dt)
    {
        UNUSED_VAR(dt);

        camera->entity.pos = Spherical::toCartesian(camera->spherical);
        // camera lookat arcball origin
        Entity::lookAt(&camera->entity, VEC_ORIGIN);

        // TODO group all inputs after glfwPollEvents into a single Input struct
        // pass the input struct to update function as part of frameContext
    }

    static void onMouseButton(Camera* camera, i32 button, i32 action, i32 mods)
    {
        UNUSED_VAR(mods);

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            // double xpos, ypos;
            // glfwGetCursorPos(window, &xpos, &ypos);
            camera->mouseDown = true;
        } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
            camera->mouseDown = false;
        }
    }

    static void onScroll(Camera* camera, f64 xoffset, f64 yoffset)
    {
        UNUSED_VAR(xoffset);
        camera->spherical.radius
          = MAX(0.1f, camera->spherical.radius -= yoffset);
    }

    static void onCursorPosition(Camera* camera, f64 xpos, f64 ypos)
    {
        f64 deltaX = xpos - camera->lastMouseX;
        f64 deltaY = ypos - camera->lastMouseY;

        const f64 speed = 0.02;

        if (camera->mouseDown) {
            camera->spherical.theta -= (speed * deltaX);
            camera->spherical.phi -= (speed * deltaY);

            // clamp phi
            camera->spherical.phi
              = CLAMP(camera->spherical.phi, -PI / (2.0f + EPSILON),
                      PI / (2.0f + EPSILON));
            // clamp theta
            camera->spherical.theta = fmod(camera->spherical.theta, 2.0f * PI);
        }

        camera->lastMouseX = xpos;
        camera->lastMouseY = ypos;
    }
};