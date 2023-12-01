// Note: See "main.cpp" for headers.
// This file was extracted to improve the organization of the code,
// but it is still compiled in the "main.cpp" translation unit,
// because both files have mostly the same dependencies (OpenGL, GLM, FreeType).

#include "chugl_pch.h"

class CHGL_Text;
class Shader;

class Font {
	struct Glyph {
		FT_UInt index;
		int32_t bufferIndex;

		int32_t curveCount;

		// Important glyph metrics in font units.
		FT_Pos width, height;
		FT_Pos bearingX;
		FT_Pos bearingY;
		FT_Pos advance;
	};

	struct BufferGlyph {
		int32_t start, count; // range of bezier curves belonging to this glyph
	};

	struct BufferCurve {
		float x0, y0, x1, y1, x2, y2;
	};

public:
	struct BufferVertex {
		float   x, y, u, v;
		int32_t bufferIndex;
	};

	static FT_Face loadFace(FT_Library library, const std::string& filename, std::string& error);

	// If hinting is enabled, worldSize must be an integer and defines the font size in pixels used for hinting.
	// Otherwise, worldSize can be an arbitrary floating-point value.
	Font(FT_Face face, float worldSize = 1.0f, bool hinting = false);
	~Font();

public:
	void setWorldSize(float worldSize);
	void prepareGlyphsForText(const std::string& text);

private:
	void uploadBuffers();
	void buildGlyph(uint32_t charcode, FT_UInt glyphIndex);

	// This function takes a single contour (defined by firstIndex and
	// lastIndex, both inclusive) from outline and converts it into individual
	// quadratic bezier curves, which are added to the curves vector.
	void convertContour(std::vector<BufferCurve>& curves, const FT_Outline* outline, short firstIndex, short lastIndex, float emSize);

public:

    // given text and a starting model-space coordinate (x,y)
    // reconstructs the vertex and index buffers for the text
    // (used to batch draw a single GText object)
    void RebuildBuffers(
        const std::string& mainText, float x, float y,
        std::vector<BufferVertex>& vertices, std::vector<int32_t>& indices,
        float verticalScale = 1.0f
    );

	// Decodes the first Unicode code point from the null-terminated UTF-8 string *text and advances *text to point at the next code point.
	// If the encoding is invalid, advances *text by one byte and returns 0.
	// *text should not be empty, because it will be advanced past the null terminator.
	uint32_t decodeCharcode(const char** text);

	void BindTextures(Shader* fontShader);

	struct BoundingBox {
		float minX, minY, maxX, maxY;
	};

	BoundingBox measure(
        float x, float y, const std::string& text, float verticalScale = 1.0f
    );

private:
	FT_Face face;

	// Whether hinting is enabled for this instance.
	// Note that hinting changes how we operate FreeType:
	// If hinting is not enabled, we scale all coordinates ourselves (see comment for emSize).
	// If hinting is enabled, we must let FreeType scale the outlines for the hinting to work properly.
	// The variables loadFlags and kerningMode are set in the constructor and control this scaling behavior.
	bool hinting;
	FT_Int32 loadFlags;
	FT_Kerning_Mode kerningMode;

	// Size of the em square used to convert metrics into em-relative values,
	// which can then be scaled to the worldSize. We do the scaling ourselves in
	// floating point to support arbitrary world sizes (whereas the fixed-point
	// numbers used by FreeType do not have enough resolution if the world size
	// is small).
	// Following the FreeType convention, if hinting (and therefore scaling) is enabled,
	// this value is in 1/64th of a pixel (compatible with 26.6 fixed point numbers).
	// If hinting/scaling is not enabled, this value is in font units.
	float emSize;

	float worldSize;

	unsigned int glyphTexture, curveTexture;
	unsigned int glyphBuffer, curveBuffer;

	std::vector<BufferGlyph> bufferGlyphs;
	std::vector<BufferCurve> bufferCurves;
	std::unordered_map<uint32_t, Glyph> glyphs;

public:
	// The glyph quads are expanded by this amount to enable proper
	// anti-aliasing. Value is relative to emSize.
	float dilation = 0;
};

class RendererText
{
private:  // statics
    static Shader* s_FontShader;

    // need to lazy evaluate bc OpenGL context is not initialized

private:  // member vars
    CHGL_Text* m_chugl_text;
    Font::BoundingBox bb;

    // GPU Buffers 
    unsigned int vao, vbo, ebo;

    // buffer data
    std::vector<Font::BufferVertex> m_Vertices;
    std::vector<int32_t> m_Indices;
public:
    RendererText(CHGL_Text* chugl_text);
	virtual ~RendererText();

    static Shader* GetFontShader();

    void Draw(Font* font);
};
