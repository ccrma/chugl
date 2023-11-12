#pragma once

#include "chugl_pch.h"

// Color class functions
t_CKBOOL init_chugl_colors(Chuck_DL_Query *QUERY);

CK_DLL_SFUN(chugl_color_hsv_to_rgb);
CK_DLL_SFUN(chugl_color_rgb_to_hsv);
CK_DLL_SFUN(chugl_color_grayscale_accurate);
CK_DLL_SFUN(chugl_color_random_rgb);
// CK_DLL_SFUN(chugl_color_grayscale_fast);


class Color 
{
public:  // rgb color constants
    static const t_CKVEC3 RED;
    static const t_CKVEC3 GREEN;
    static const t_CKVEC3 BLUE;

    static const t_CKVEC3 LIGHTGRAY;
    static const t_CKVEC3 GRAY;
    static const t_CKVEC3 DARKGRAY;
    static const t_CKVEC3 YELLOW;
    static const t_CKVEC3 GOLD;
    static const t_CKVEC3 ORANGE;
    static const t_CKVEC3 PINK;
    static const t_CKVEC3 MAROON;
    static const t_CKVEC3 LIME;
    static const t_CKVEC3 DARKGREEN;
    static const t_CKVEC3 SKYBLUE;
    static const t_CKVEC3 DARKBLUE;
    static const t_CKVEC3 PURPLE;
    static const t_CKVEC3 VIOLET;
    static const t_CKVEC3 DARKPURPLE;
    static const t_CKVEC3 BEIGE;
    static const t_CKVEC3 BROWN;
    static const t_CKVEC3 DARKBROWN;
    static const t_CKVEC3 WHITE;
    static const t_CKVEC3 BLACK;
    static const t_CKVEC3 MAGENTA;

public:  // helper fns
    static glm::vec3 hsv2rgb(const glm::vec3& hsv);
    static glm::vec3 rgb2hsv(const glm::vec3& rgb);
    static float GrayscaleAccurate(const glm::vec3& rgb);
};
