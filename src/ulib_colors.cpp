#include "ulib_colors.h"

t_CKBOOL init_chugl_colors(Chuck_DL_Query *QUERY)
{
	QUERY->begin_class(QUERY, "Color", "Object");
    QUERY->doc_class(QUERY, "ChuGL color utility class");

    // static colors ==========================================================

    QUERY->add_svar(QUERY, "vec3", "RED", true, (void *)&Color::RED);
    QUERY->doc_var(QUERY, "red: (1.0, 0.0, 0.0)");

    QUERY->add_svar(QUERY, "vec3", "GREEN", true, (void *)&Color::GREEN);
    QUERY->doc_var(QUERY, "green: (0.0, 1.0, 0.0)");

    QUERY->add_svar(QUERY, "vec3", "BLUE", true, (void *)&Color::BLUE);
    QUERY->doc_var(QUERY, "blue: (0.0, 0.0, 1.0)");

    QUERY->add_svar(QUERY, "vec3", "LIGHTGRAY", true, (void *)&Color::LIGHTGRAY);
    QUERY->doc_var(QUERY, "lightgray: (.784, .784, .784)");

    QUERY->add_svar(QUERY, "vec3", "GRAY", true, (void *)&Color::GRAY);
    QUERY->doc_var(QUERY, "gray: (.51, .51, .51)");

    QUERY->add_svar(QUERY, "vec3", "DARKGRAY", true, (void *)&Color::DARKGRAY);
    QUERY->doc_var(QUERY, "darkgray: (.3, .3, .3)");

    QUERY->add_svar(QUERY, "vec3", "YELLOW", true, (void *)&Color::YELLOW);
    QUERY->doc_var(QUERY, "yellow: (.9921, .976, 0)");

    QUERY->add_svar(QUERY, "vec3", "GOLD", true, (void *)&Color::GOLD);
    QUERY->doc_var(QUERY, "gold: (1.0, .796, 0)");

    QUERY->add_svar(QUERY, "vec3", "ORANGE", true, (void *)&Color::ORANGE);
    QUERY->doc_var(QUERY, "orange: (1.0, .631, 0)");

    QUERY->add_svar(QUERY, "vec3", "PINK", true, (void *)&Color::PINK);
    QUERY->doc_var(QUERY, "pink: (1.0, .427, .76)");

    QUERY->add_svar(QUERY, "vec3", "MAROON", true, (void *)&Color::MAROON);
    QUERY->doc_var(QUERY, "maroon: (.745, .129, .216)");

    QUERY->add_svar(QUERY, "vec3", "LIME", true, (void *)&Color::LIME);
    QUERY->doc_var(QUERY, "lime: (0, .619, .184)");

    QUERY->add_svar(QUERY, "vec3", "DARKGREEN", true, (void *)&Color::DARKGREEN);
    QUERY->doc_var(QUERY, "darkgreen: (0, .459, .173)");

    QUERY->add_svar(QUERY, "vec3", "SKYBLUE", true, (void *)&Color::SKYBLUE);
    QUERY->doc_var(QUERY, "skyblue: (.4, .749, 1.0)");

    QUERY->add_svar(QUERY, "vec3", "DARKBLUE", true, (void *)&Color::DARKBLUE);
    QUERY->doc_var(QUERY, "darkblue: (0, .322, .675)");

    QUERY->add_svar(QUERY, "vec3", "PURPLE", true, (void *)&Color::PURPLE);
    QUERY->doc_var(QUERY, "purple: (.784, .478, 1.0)");

    QUERY->add_svar(QUERY, "vec3", "VIOLET", true, (void *)&Color::VIOLET);
    QUERY->doc_var(QUERY, "violet: (.529, .235, .745)");

    QUERY->add_svar(QUERY, "vec3", "DARKPURPLE", true, (void *)&Color::DARKPURPLE);
    QUERY->doc_var(QUERY, "darkpurple: (.439, .122, .494)");

    QUERY->add_svar(QUERY, "vec3", "BEIGE", true, (void *)&Color::BEIGE);
    QUERY->doc_var(QUERY, "beige: (.827, .69, .514)");

    QUERY->add_svar(QUERY, "vec3", "BROWN", true, (void *)&Color::BROWN);
    QUERY->doc_var(QUERY, "brown: (.498, .416, .31)");

    QUERY->add_svar(QUERY, "vec3", "DARKBROWN", true, (void *)&Color::DARKBROWN);
    QUERY->doc_var(QUERY, "darkbrown: (.298, .247, .184)");

    QUERY->add_svar(QUERY, "vec3", "WHITE", true, (void *)&Color::WHITE);
    QUERY->doc_var(QUERY, "white: (1.0, 1.0, 1.0)");

    QUERY->add_svar(QUERY, "vec3", "BLACK", true, (void *)&Color::BLACK);
    QUERY->doc_var(QUERY, "black: (0.0, 0.0, 0.0)");

    QUERY->add_svar(QUERY, "vec3", "MAGENTA", true, (void *)&Color::MAGENTA);
    QUERY->doc_var(QUERY, "magenta: (1.0, 0.0, 1.0)");


    // helper color fns =======================================================
	QUERY->add_sfun(QUERY, chugl_color_hsv_to_rgb, "vec3", "hsv2rgb");
	QUERY->add_arg(QUERY, "vec3", "hsv");
    QUERY->doc_func(QUERY, "convert from hsv colorspace to rgb");

	QUERY->add_sfun(QUERY, chugl_color_rgb_to_hsv, "vec3", "rgb2hsv");
	QUERY->add_arg(QUERY, "vec3", "rgb");
    QUERY->doc_func(QUERY, 
        "convert from rgb colorspace to hsv"
        "hsv stands for hue, saturation, value, and is a more human-friendly color format"
        "hue takes on a value between 0 and 360, and represents the color itself"
        "saturation takes on a value between 0 and 1, and represents the amount of color"
        "value takes on a value between 0 and 1, and represents the brightness of the color"
    );

	QUERY->add_sfun(QUERY, chugl_color_grayscale_accurate, "vec3", "grayscale");
	QUERY->add_arg(QUERY, "vec3", "rgb");
    QUERY->doc_func(QUERY, "convert an rgb value to grayscale, scaled according to NTSC formula based on human perception");

    QUERY->add_sfun(QUERY, chugl_color_random_rgb, "vec3", "random");
    QUERY->doc_func(QUERY, "generate a random rgb color");




	QUERY->end_class(QUERY);

	return true;
}

CK_DLL_SFUN(chugl_color_rgb_to_hsv) {
    t_CKVEC3 rgb = GET_NEXT_VEC3(ARGS);
    glm::vec3 hsv = Color::rgb2hsv(
        glm::vec3(rgb.x, rgb.y, rgb.z)
    );
    RETURN->v_vec3 = {hsv.x, hsv.y, hsv.z};
}

CK_DLL_SFUN(chugl_color_hsv_to_rgb) {
    t_CKVEC3 hsv = GET_NEXT_VEC3(ARGS);
    glm::vec3 rgb = Color::hsv2rgb(
        glm::vec3(hsv.x, hsv.y, hsv.z)
    );
    RETURN->v_vec3 = {rgb.r, rgb.g, rgb.b};
}

CK_DLL_SFUN(chugl_color_grayscale_accurate)
{
	t_CKVEC3 color = GET_NEXT_VEC3(ARGS);
    float gsc = Color::GrayscaleAccurate(
        glm::vec3(color.x, color.y, color.z)
    );
    RETURN->v_vec3 = {gsc, gsc, gsc};
}

CK_DLL_SFUN(chugl_color_random_rgb)
{
    float r = (float)rand() / RAND_MAX;
    float g = (float)rand() / RAND_MAX;
    float b = (float)rand() / RAND_MAX;
    RETURN->v_vec3 = {r, g, b};
}

// COLOR class implementation =================================================

const t_CKVEC3 Color::RED = {1.0f, 0.0f, 0.0f};
const t_CKVEC3 Color::GREEN = {0.0f, 1.0f, 0.0f};
const t_CKVEC3 Color::BLUE = {0.0f, 0.0f, 1.0f};
const t_CKVEC3 Color::LIGHTGRAY  { 200.0 / 255.0, 200.0/255.0, 200.0/255.0 };
const t_CKVEC3 Color::GRAY       { 130.0 / 255.0, 130.0/255.0, 130.0/255.0 };
const t_CKVEC3 Color::DARKGRAY   { 80.0 / 255.0, 80.0/255.0, 80.0/255.0 };
const t_CKVEC3 Color::YELLOW     { 253.0 / 255.0, 249.0/255.0, 0.0/255.0 };
const t_CKVEC3 Color::GOLD       { 255.0 / 255.0, 203.0/255.0, 0.0/255.0 };
const t_CKVEC3 Color::ORANGE     { 255.0 / 255.0, 161.0/255.0, 0.0/255.0 };
const t_CKVEC3 Color::PINK       { 255.0 / 255.0, 109.0/255.0, 194.0/255.0 };
const t_CKVEC3 Color::MAROON     { 190.0 / 255.0, 33.0/255.0, 55.0/255.0 };
const t_CKVEC3 Color::LIME       { 0.0 / 255.0, 158.0/255.0, 47.0/255.0 };
const t_CKVEC3 Color::DARKGREEN  { 0.0 / 255.0, 117.0/255.0, 44.0/255.0 };
const t_CKVEC3 Color::SKYBLUE    { 102.0 / 255.0, 191.0/255.0, 255.0/255.0 };
const t_CKVEC3 Color::DARKBLUE   { 0.0 / 255.0, 82.0/255.0, 172.0/255.0 };
const t_CKVEC3 Color::PURPLE     { 200.0 / 255.0, 122.0/255.0, 255.0/255.0 };
const t_CKVEC3 Color::VIOLET     { 135.0 / 255.0, 60.0/255.0, 190.0/255.0 };
const t_CKVEC3 Color::DARKPURPLE { 112.0 / 255.0, 31.0/255.0, 126.0/255.0 };
const t_CKVEC3 Color::BEIGE      { 211.0 / 255.0, 176.0/255.0, 131.0/255.0 };
const t_CKVEC3 Color::BROWN      { 127.0 / 255.0, 106.0/255.0, 79.0/255.0 };
const t_CKVEC3 Color::DARKBROWN  { 76.0 / 255.0, 63.0/255.0, 47.0/255.0 };
const t_CKVEC3 Color::WHITE      { 255.0 / 255.0, 255.0/255.0, 255.0/255.0 };
const t_CKVEC3 Color::BLACK      { 0.0 / 255.0, 0.0/255.0, 0.0/255.0 };
const t_CKVEC3 Color::MAGENTA    { 255.0 / 255.0, 0.0/255.0, 255.0/255.0 };

glm::vec3 Color::hsv2rgb(const glm::vec3 &hsv)
{
    double r = 0, g = 0, b = 0;

	if (hsv.y < 0.0001f) {
		r = hsv.z;
		g = hsv.z;
		b = hsv.z;
	} else {
		int i;
		double hue, f, p, q, t;

		if (hsv.x == 360)
			hue = 0;
		else
			hue = hsv.x / 60;
        
		i = (int)glm::floor(hue);
		f = glm::fract(hue);

		p = hsv.z * (1.0 - hsv.y);
		q = hsv.z * (1.0 - (hsv.y * f));
		t = hsv.z * (1.0 - (hsv.y * (1.0 - f)));

		switch (i)
		{
		case 0:
			r = hsv.z;
			g = t;
			b = p;
			break;

		case 1:
			r = q;
			g = hsv.z;
			b = p;
			break;

		case 2:
			r = p;
			g = hsv.z;
			b = t;
			break;

		case 3:
			r = p;
			g = q;
			b = hsv.z;
			break;

		case 4:
			r = t;
			g = p;
			b = hsv.z;
			break;

		default:  // case 5
			r = hsv.z;
			g = p;
			b = q;
			break;
		}
	}
    return {r, g, b};
}

// convert sRGB with channels in range [0,1]
// to
// HSV with hue in range [0, 360] and saturation/value in range [0, 1]
glm::vec3 Color::rgb2hsv(const glm::vec3 &rgb)
{
    glm::vec3 hsv;
    const float& r = rgb.r;
    const float& g = rgb.g;
    const float& b = rgb.b;
    float max = glm::max(glm::max(r, g), b);
    float min = glm::min(glm::min(r, g), b);
    float delta = max - min;
    if (delta != 0)
    {
        float hue {0};
        if (r == max)
        {
            hue = (g - b) / delta;
        }
        else
        {
            if (g == max)
            {
                hue = 2 + (b - r) / delta;
            }
            else
            {
                hue = 4 + (r - g) / delta;
            }
        }
        hue *= 60;
        if (hue < 0) hue += 360;
        hsv.x = hue;
    }
    else
    {
        hsv.x = 0;
    }
    hsv.y = max == 0 ? 0 : (max - min) / max;
    hsv.z = max;
    return hsv;
}

float Color::GrayscaleAccurate(const glm::vec3 &rgb)
{
    return 0.299*rgb.r + 0.587*rgb.g + 0.114*rgb.b;
}

