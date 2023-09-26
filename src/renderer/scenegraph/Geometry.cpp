#include "Geometry.h"
/* =============================================================================
									Base Geo
===============================================================================*/

// constants
const unsigned int Geometry::POSITION_ATTRIB_IDX = 0;
const unsigned int Geometry::NORMAL_ATTRIB_IDX = 1;
const unsigned int Geometry::COLOR_ATTRIB_IDX = 2;
const unsigned int Geometry::UV0_ATTRIB_IDX = 3;

// methods
void Geometry::AddVertex(Vertex v)
{
	auto& posAttrib = m_Attributes[POSITION_ATTRIB_IDX];
	auto& normAttrib = m_Attributes[NORMAL_ATTRIB_IDX];
	auto& uvAttrib = m_Attributes[UV0_ATTRIB_IDX];

	posAttrib.data.push_back(v.Position.x);
	posAttrib.data.push_back(v.Position.y);
	posAttrib.data.push_back(v.Position.z);

	normAttrib.data.push_back(v.Normal.x);
	normAttrib.data.push_back(v.Normal.y);
	normAttrib.data.push_back(v.Normal.z);

	uvAttrib.data.push_back(v.TexCoords.x);
	uvAttrib.data.push_back(v.TexCoords.y);
}

void Geometry::AddTriangleIndices(unsigned int i1, unsigned int i2, unsigned int i3)
{
	m_Indices.push_back(i1);
	m_Indices.push_back(i2);
	m_Indices.push_back(i3);
}

/* =============================================================================
									Sphere Geo
===============================================================================*/

SphereGeometry::SphereGeometry(float radius, int widthSegments, int heightSegments, float phiStart, float phiLength, float thetaStart, float thetaLength) : 
m_Radius(radius), m_WidthSeg(widthSegments), m_HeightSeg(heightSegments),
m_PhiStart(phiStart), m_PhiLength(phiLength),
m_ThetaStart(thetaStart), m_ThetaLength(thetaLength)
{
	BuildGeometry();
}

void SphereGeometry::BuildGeometry()
{
	ResetVertexData();
	m_Dirty = false;

	const float pi = 3.14159265358979323846f;
	const float epsilon = .00001f; // tolerance
	m_WidthSeg = std::max(3, m_WidthSeg);
	m_HeightSeg = std::max(2, m_HeightSeg);

	const float thetaEnd = std::min(m_ThetaStart + m_ThetaLength, pi);

	unsigned int index = 0;
	std::vector<unsigned int> grid;


	// generate vertices, normals and uvs
	for (int iy = 0; iy <= m_HeightSeg; iy++) {

		const float v = (float)iy / (float)m_HeightSeg;

		// special case for the poles
		float uOffset = 0;
		if (iy == 0 && glm::epsilonEqual(m_ThetaStart, 0.0f, epsilon)) {
			uOffset = 0.5f / m_WidthSeg;
		}
		else if (iy == m_HeightSeg&& glm::epsilonEqual(thetaEnd, pi, epsilon)) {
			uOffset = -0.5 / m_WidthSeg;
		}

		for (int ix = 0; ix <= m_WidthSeg; ix++) {

			const float u = (float)ix / (float)m_WidthSeg;

			Vertex vert;

			// vertex
			vert.Position.x = -m_Radius * glm::cos(m_PhiStart + u * m_PhiLength) * glm::sin(m_ThetaStart + v * m_ThetaLength);
			vert.Position.y = m_Radius * glm::cos(m_ThetaStart + v * m_ThetaLength);
			vert.Position.z = m_Radius * glm::sin(m_PhiStart + u * m_PhiLength) * glm::sin(m_ThetaStart + v * m_ThetaLength);

			// normal
			vert.Normal = glm::normalize(vert.Position);

			// uv
			vert.TexCoords.x = u + uOffset;
			vert.TexCoords.y = 1 - v;

			AddVertex(vert);

			grid.push_back(index++);
		}
	}

	// indices

	const size_t rowSize = (size_t)m_WidthSeg+ 1;
	for (size_t iy = 0; iy < m_HeightSeg; iy++) {
		for (size_t ix = 0; ix < m_WidthSeg; ix++) {

			const unsigned int a = grid[(iy * rowSize) + ix + 1];
			const unsigned int b = grid[(iy * rowSize) + ix];
			const unsigned int c = grid[(rowSize * (iy + 1)) + ix];
			const unsigned int d = grid[rowSize * (iy + 1) + (ix + 1)];

			if (iy != 0 || m_ThetaStart > epsilon)
				AddTriangleIndices(a, b, d);
			if (iy != (size_t)m_HeightSeg- 1 || thetaEnd < pi - epsilon)
				AddTriangleIndices(b, c, d);
		}
	}

}

/* =============================================================================
									Box Geo
===============================================================================*/

BoxGeometry::BoxGeometry(float width, float height, float depth, int widthSeg, int heightSeg, int depthSeg) : m_Width(width), m_Height(height), m_Depth(depth),
m_WidthSeg(widthSeg), m_HeightSeg(heightSeg), m_DepthSeg(depthSeg)
{
	std::cout << "calling boxgeo constructor\n";
	BuildGeometry();
}

void BoxGeometry::BuildGeometry()
{
	ResetVertexData();
	m_Dirty = false;

	buildPlane('z', 'y', 'x', -1, -1, m_Depth, m_Height, m_Width, m_DepthSeg, m_HeightSeg, 0); // px
	buildPlane('z', 'y', 'x', 1, -1, m_Depth, m_Height, -m_Width, m_DepthSeg, m_HeightSeg, 1); // nx
	buildPlane('x', 'z', 'y', 1, 1, m_Width, m_Depth, m_Height, m_WidthSeg, m_DepthSeg, 2); // py
	buildPlane('x', 'z', 'y', 1, -1, m_Width, m_Depth, -m_Height, m_WidthSeg, m_DepthSeg, 3); // ny
	buildPlane('x', 'y', 'z', 1, -1, m_Width, m_Height, m_Depth, m_WidthSeg, m_HeightSeg, 4); // pz
	buildPlane('x', 'y', 'z', -1, -1, m_Width, m_Height, -m_Depth, m_WidthSeg, m_HeightSeg, 5); // nz

}

void BoxGeometry::buildPlane(char u, char v, char w, int udir, int vdir, float width, float height, float depth, int gridX, int gridY, int materialIndex) {

	const float segmentWidth = width / (float)gridX;
	const float segmentHeight = height / (float)gridY;

	const float widthHalf = width / 2;
	const float heightHalf = height / 2;
	const float depthHalf = depth / 2;

	const int gridX1 = gridX + 1;
	const int gridY1 = gridY + 1;

	// unsigned int vertexCounter = 0;
	unsigned int groupCount = 0;

	const glm::vec3 vector = glm::vec3(0.0);

	auto& posAttrib = m_Attributes[POSITION_ATTRIB_IDX];
	auto& normAttrib = m_Attributes[NORMAL_ATTRIB_IDX];
	auto& uvAttrib = m_Attributes[UV0_ATTRIB_IDX];

	// save number of vertices BEFORE adding any this round
	// used to figure out indices
	const int numberOfVertices = posAttrib.data.size() / posAttrib.numComponents;

	// generate vertices, normals and uvs
	for (int iy = 0; iy < gridY1; iy++) {
		const float y = iy * segmentHeight - heightHalf;
		for (int ix = 0; ix < gridX1; ix++) {
			const float x = ix * segmentWidth - widthHalf;

			// prepare new vertex
			Vertex vert;

			// set position
			vert.Pos(u) = x * udir;
			vert.Pos(v) = y * vdir;
			vert.Pos(w) = depthHalf;

			// set normals
			vert.Norm(u) = 0;
			vert.Norm(v) = 0;
			vert.Norm(w) = depth > 0 ? 1 : -1;

			// set uvs
			vert.TexCoords.x = (ix / gridX);
			vert.TexCoords.y = (1 - (iy / gridY));

			// copy to list
			AddVertex(vert);

			// counters
			// vertexCounter += 1;
		}
	}

	// indices

	// 1. you need three indices to draw a single face
	// 2. a single segment consists of two faces
	// 3. so we need to generate six (2*3) indices per segment

	for (int iy = 0; iy < gridY; iy++) {
		for (int ix = 0; ix < gridX; ix++) {

			unsigned int a = numberOfVertices + ix + gridX1 * iy;
			unsigned int b = numberOfVertices + ix + gridX1 * (iy + 1);
			unsigned int c = numberOfVertices + (ix + 1) + gridX1 * (iy + 1);
			unsigned int d = numberOfVertices + (ix + 1) + gridX1 * iy;

			// faces
			AddTriangleIndices(a, b, d);
			AddTriangleIndices(b, c, d);

			// increase group counter
			groupCount += 6;
		}

	}

	// add a group to the geometry. this will ensure multi material support
	// TODO: add this later. too complex for now. assume a single geometry is rendererd with a single mat
	// scope.addGroup(groupStart, groupCount, materialIndex);

	// calculate new start value for groups
	// groupStart += groupCount;
}
