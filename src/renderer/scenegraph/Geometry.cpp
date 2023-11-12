#include "Geometry.h"
/* =============================================================================
									Base Geo
===============================================================================*/

// constants
const t_CKUINT Geometry::POSITION_ATTRIB_IDX = 0;
const t_CKUINT Geometry::NORMAL_ATTRIB_IDX = 1;
const t_CKUINT Geometry::COLOR_ATTRIB_IDX = 2;
const t_CKUINT Geometry::UV0_ATTRIB_IDX = 3;

// methods
void Geometry::AddVertex(Vertex v)
{
	auto& posAttrib = m_Attributes[POSITION_ATTRIB_IDX];
	auto& normAttrib = m_Attributes[NORMAL_ATTRIB_IDX];
    auto& colorAttrib = m_Attributes[COLOR_ATTRIB_IDX];
	auto& uvAttrib = m_Attributes[UV0_ATTRIB_IDX];

	posAttrib.data.push_back(v.Position.x);
	posAttrib.data.push_back(v.Position.y);
	posAttrib.data.push_back(v.Position.z);

	normAttrib.data.push_back(v.Normal.x);
	normAttrib.data.push_back(v.Normal.y);
	normAttrib.data.push_back(v.Normal.z);

    colorAttrib.data.push_back(v.Color.x);
    colorAttrib.data.push_back(v.Color.y);
    colorAttrib.data.push_back(v.Color.z);
    colorAttrib.data.push_back(v.Color.w);

	uvAttrib.data.push_back(v.TexCoords.x);
	uvAttrib.data.push_back(v.TexCoords.y);
}

void Geometry::AddTriangleIndices(size_t i1, size_t i2, size_t i3)
{
	m_Indices.push_back(i1);
	m_Indices.push_back(i2);
	m_Indices.push_back(i3);
}

// ck class names
Geometry::CkTypeMap Geometry::s_CkTypeMap = {
	{GeometryType::Base, "Geometry"},
	{GeometryType::Box, "BoxGeometry"},
	{GeometryType::Sphere, "SphereGeometry"},
	{GeometryType::Triangle, "TriangleGeometry"},
	{GeometryType::Circle, "CircleGeometry"},
	{GeometryType::Cylinder, "CylinderGeometry"},
	{GeometryType::Capsule, "CapsuleGeometry"},
	{GeometryType::Lathe, "LatheGeometry"},
	{GeometryType::Cone, "ConeGeometry"},
	{GeometryType::Plane, "PlaneGeometry"},
	{GeometryType::Quad, "QuadGeometry"},
	{GeometryType::Torus, "TorusGeometry"},
	{GeometryType::Custom, "CustomGeometry"}
};

const char * Geometry::CKName(GeometryType type) {
    return s_CkTypeMap[type].c_str();
}

/* =============================================================================
									Sphere Geo
===============================================================================*/

void SphereGeometry::BuildGeometry()
{
	ResetVertexData();
	m_Dirty = false;

	const float pi = 3.14159265358979323846f;
	const float epsilon = .00001f; // tolerance
	m_Params.widthSeg = std::max(3, m_Params.widthSeg);
	m_Params.heightSeg = std::max(2, m_Params.heightSeg);

	const float thetaEnd = std::min(m_Params.thetaStart + m_Params.thetaLength, pi);

	unsigned int index = 0;
	std::vector<unsigned int> grid;


	// generate vertices, normals and uvs
	for (int iy = 0; iy <= m_Params.heightSeg; iy++) {

		const float v = (float)iy / (float)m_Params.heightSeg;

		// special case for the poles
		float uOffset = 0;
		if (iy == 0 && glm::epsilonEqual(m_Params.thetaStart, 0.0f, epsilon)) {
			uOffset = 0.5f / m_Params.widthSeg;
		}
		else if (iy == m_Params.heightSeg&& glm::epsilonEqual(thetaEnd, pi, epsilon)) {
			uOffset = -0.5 / m_Params.widthSeg;
		}

		for (int ix = 0; ix <= m_Params.widthSeg; ix++) {

			const float u = (float)ix / (float)m_Params.widthSeg;

			Vertex vert;

			// vertex
			vert.Position.x = -m_Params.radius * glm::cos(m_Params.phiStart + u * m_Params.phiLength) * glm::sin(m_Params.thetaStart + v * m_Params.thetaLength);
			vert.Position.y = m_Params.radius * glm::cos(m_Params.thetaStart + v * m_Params.thetaLength);
			vert.Position.z = m_Params.radius * glm::sin(m_Params.phiStart + u * m_Params.phiLength) * glm::sin(m_Params.thetaStart + v * m_Params.thetaLength);

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

	const size_t rowSize = (size_t)m_Params.widthSeg+ 1;
	for (size_t iy = 0; iy < m_Params.heightSeg; iy++) {
		for (size_t ix = 0; ix < m_Params.widthSeg; ix++) {

			const unsigned int a = grid[(iy * rowSize) + ix + 1];
			const unsigned int b = grid[(iy * rowSize) + ix];
			const unsigned int c = grid[(rowSize * (iy + 1)) + ix];
			const unsigned int d = grid[rowSize * (iy + 1) + (ix + 1)];

			if (iy != 0 || m_Params.thetaStart > epsilon)
				AddTriangleIndices(a, b, d);
			if (iy != (size_t)m_Params.heightSeg- 1 || thetaEnd < pi - epsilon)
				AddTriangleIndices(b, c, d);
		}
	}

}

/* =============================================================================
									Box Geo
===============================================================================*/

void BoxGeometry::BuildGeometry()
{
	ResetVertexData();
	m_Dirty = false;

	buildPlane('z', 'y', 'x', -1, -1, m_Params.depth, m_Params.height, m_Params.width, m_Params.depthSeg, m_Params.heightSeg, 0); // px
	buildPlane('z', 'y', 'x', 1, -1, m_Params.depth, m_Params.height, -m_Params.width, m_Params.depthSeg, m_Params.heightSeg, 1); // nx
	buildPlane('x', 'z', 'y', 1, 1, m_Params.width, m_Params.depth, m_Params.height, m_Params.widthSeg, m_Params.depthSeg, 2); // py
	buildPlane('x', 'z', 'y', 1, -1, m_Params.width, m_Params.depth, -m_Params.height, m_Params.widthSeg, m_Params.depthSeg, 3); // ny
	buildPlane('x', 'y', 'z', 1, -1, m_Params.width, m_Params.height, m_Params.depth, m_Params.widthSeg, m_Params.heightSeg, 4); // pz
	buildPlane('x', 'y', 'z', -1, -1, m_Params.width, m_Params.height, -m_Params.depth, m_Params.widthSeg, m_Params.heightSeg, 5); // nz

}

void BoxGeometry::buildPlane(char u, char v, char w, int udir, int vdir, float width, float height, float depth, int gridX, int gridY, int materialIndex) {

	const float segmentWidth = width / (float)gridX;
	const float segmentHeight = height / (float)gridY;

	const float widthHalf = width / 2;
	const float heightHalf = height / 2;
	const float depthHalf = depth / 2;

	const int gridX1 = gridX + 1;
	const int gridY1 = gridY + 1;

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
		}

	}
}

/* =============================================================================
									Circle Geo
===============================================================================*/

void CircleGeometry::BuildGeometry()
{
	ResetVertexData();
	m_Dirty = false;

	Vertex initVert;
	initVert.Position = glm::vec3(0, 0, 0);
	initVert.Normal = glm::vec3(0, 0, 1);
	initVert.TexCoords.x = 0.5;
	initVert.TexCoords.y = 0.5;
	AddVertex(initVert);

	auto& positions = GetPositions().data;

	Vertex v;
	for ( int s = 0, i = 3; s <= m_Params.segments; s++, i += 3 ) {

		const float segment = m_Params.thetaStart + (float) s / (float) m_Params.segments * m_Params.thetaLength;

		// vertex
		v.Position.x = m_Params.radius * glm::cos( segment );
		v.Position.y = m_Params.radius * glm::sin( segment );
		v.Position.z = 0;

		// normal
		v.Normal = glm::vec3( 0, 0, 1.0 );

		// uvs
		// v.TexCoords.x = ( positions[ i ] / m_Params.radius + 1.0 ) / 2.0;
		// v.TexCoords.y = ( positions[ i + 1 ] / m_Params.radius + 1.0 ) / 2.0;
		v.TexCoords.x = ( v.Position.x / m_Params.radius + 1.0 ) / 2.0;
		v.TexCoords.y = ( v.Position.y / m_Params.radius + 1.0 ) / 2.0;


		AddVertex(v);

	}

	// indices

	for ( size_t i = 1; i <= m_Params.segments; i ++ )
		AddTriangleIndices( i, i + 1, 0 );
	
};

/* =============================================================================
									Plane Geo
===============================================================================*/

void PlaneGeometry::BuildGeometry()
{
	ResetVertexData();
	m_Dirty = false;

	const float width_half = m_Params.width / 2.0f;
	const float height_half = m_Params.height / 2.0f;

	const int gridX = m_Params.widthSegments;
	const int gridY = m_Params.heightSegments;

	const int gridX1 = gridX + 1;
	const int gridY1 = gridY + 1;

	const float segment_width = m_Params.width / gridX;
	const float segment_height = m_Params.height / gridY;

	Vertex vert;
	for ( int iy = 0; iy < gridY1; iy++ ) {
		const float y = iy * segment_height - height_half;
		for ( int ix = 0; ix < gridX1; ix++ ) {
			const float x = ix * segment_width - width_half;

			vert.Position = glm::vec3( x, - y, 0 );
			vert.Normal = glm::vec3( 0, 0, 1 );
			vert.TexCoords.x = (float) ix / (float) gridX;
			vert.TexCoords.y = 1.0 - ((float) iy / (float) gridY);

			AddVertex(vert);
		}
	}

	for ( int iy = 0; iy < gridY; iy ++ ) {
		for ( int ix = 0; ix < gridX; ix ++ ) {
			const int a = ix + gridX1 * iy;
			const int b = ix + gridX1 * ( iy + 1 );
			const int c = ( ix + 1 ) + gridX1 * ( iy + 1 );
			const int d = ( ix + 1 ) + gridX1 * iy;

			AddTriangleIndices( a, b, d );
			AddTriangleIndices( b, c, d );
		}
	}
}

/* =============================================================================
									Torus Geo
===============================================================================*/

void TorusGeometry::BuildGeometry()
{
	ResetVertexData();
	m_Dirty = false;

	glm::vec3 center;
	center.z = 0;
	Vertex vert;

	auto tubularSegments = m_Params.tubularSegments;
	auto radialSegments = m_Params.radialSegments;

	for ( int j = 0; j <= m_Params.radialSegments; j++ ) {
		for ( int i = 0; i <= m_Params.tubularSegments; i ++ ) {
			const float u = (float) i / (float) m_Params.tubularSegments * m_Params.arcLength;
			const float v = (float) j / (float) m_Params.radialSegments * glm::pi<float>() * 2.0;

			// vertex
			vert.Position.x = ( m_Params.radius + m_Params.tubeRadius * glm::cos( v ) ) * glm::cos( u );
			vert.Position.y = ( m_Params.radius + m_Params.tubeRadius * glm::cos( v ) ) * glm::sin( u );
			vert.Position.z = m_Params.tubeRadius * glm::sin( v );

			// normal
			center.x = m_Params.radius * glm::cos( u );
			center.y = m_Params.radius * glm::sin( u );
			vert.Normal = glm::normalize(vert.Position - center);

			// uv
			vert.TexCoords.x = (float) i / (float) m_Params.tubularSegments;
			vert.TexCoords.y = (float) j / (float) m_Params.radialSegments;

			AddVertex(vert);
		}
	}

	// generate indices
	for ( int j = 1; j <= m_Params.radialSegments; j ++ ) {
		for ( int i = 1; i <= m_Params.tubularSegments; i ++ ) {
			// indices
			const int a = ( tubularSegments + 1 ) * j + i - 1;
			const int b = ( tubularSegments + 1 ) * ( j - 1 ) + i - 1;
			const int c = ( tubularSegments + 1 ) * ( j - 1 ) + i;
			const int d = ( tubularSegments + 1 ) * j + i;

			AddTriangleIndices( a, b, d );
			AddTriangleIndices( b, c, d );
		}
	}
}

/* =============================================================================
									Lathe Geo
===============================================================================*/

void LatheGeometry::UpdateParams(int segments, float phiStart, float phiLength)
{
	m_Params.segments = segments;
	m_Params.phiStart = phiStart;
	m_Params.phiLength = phiLength;
	m_Dirty = true;
}

void LatheGeometry::UpdateParams(
	std::vector<double> points, int segments, float phiStart, float phiLength
) {
	// convert points to glm::vec2
	m_Params.points.clear();
	for (int i = 0; i < points.size(); i += 2) {
		if (i + 1 >= points.size()) break;
		m_Params.points.push_back(glm::vec2(points[i], points[i+1]));
	}
	m_Params.segments = segments;
	m_Params.phiStart = phiStart;
	m_Params.phiLength = phiLength;
	m_Dirty = true;
}

void LatheGeometry::BuildGeometry()
{
	ResetVertexData();
	m_Dirty = false;

	int segments = m_Params.segments;
	auto& points = m_Params.points;
	float phiStart = m_Params.phiStart;


	float phiLength = glm::clamp<float>( m_Params.phiLength, 0, glm::pi<float>() * 2.0f );

		// buffers
		std::vector<float> initNormals;

		// helper variables

		const float inverseSegments = 1.0f / segments;
		auto normal = glm::vec3(0.0);
		auto curNormal = glm::vec3(0.0);
		auto prevNormal = glm::vec3(0.0);
		float dx = 0;
		float dy = 0;

		// pre-compute normals for initial "meridian"

		for ( size_t j = 0; j <= ( points.size() - 1 ); j ++ ) {
				if (j == 0) {				// special handling for 1st vertex on path

					dx = points[ j + 1 ].x - points[ j ].x;
					dy = points[ j + 1 ].y - points[ j ].y;

					normal.x = dy * 1.0f;
					normal.y = - dx;
					normal.z = dy * 0.0f;

					prevNormal = normal;

					normal = glm::normalize(normal);

					// initNormals.push_back(normal);
					initNormals.push_back(normal.x);
					initNormals.push_back(normal.y);
					initNormals.push_back(normal.z);

				} else if  (j ==  points.size() - 1 )	{// special handling for last Vertex on path

					// initNormals.push_back( prevNormal.x, prevNormal.y, prevNormal.z );
					// initNormals.push_back(prevNormal);
					initNormals.push_back(prevNormal.x);
					initNormals.push_back(prevNormal.y);
					initNormals.push_back(prevNormal.z);
				} else {


					dx = points[ j + 1 ].x - points[ j ].x;
					dy = points[ j + 1 ].y - points[ j ].y;

					normal.x = dy * 1.0f;
					normal.y = - dx;
					normal.z = dy * 0.0f;

					curNormal = normal;

					normal.x += prevNormal.x;
					normal.y += prevNormal.y;
					normal.z += prevNormal.z;

					normal = glm::normalize(normal);

					// initNormals.push_back( normal.x, normal.y, normal.z );
					// initNormals.push_back( normal);
					initNormals.push_back(normal.x);
					initNormals.push_back(normal.y);
					initNormals.push_back(normal.z);
					prevNormal = curNormal;
				}

			}

		// generate vertices, uvs and normals
		Vertex vert;
		for ( int i = 0; i <= segments; i++ ) {

			const float phi = phiStart + i * inverseSegments * phiLength;

			const float sin = glm::sin( phi );
			const float cos = glm::cos( phi );

			for ( size_t j = 0; j <= ( points.size() - 1 ); j ++ ) {

				// vertex

				vert.Position.x = points[ j ].x * sin;
				vert.Position.y = points[ j ].y;
				vert.Position.z = points[ j ].x * cos;

				// uv
				vert.TexCoords.x = (float) i / (float) segments;
				vert.TexCoords.y = (float) j / (float) ( points.size() - 1 );

				// normal
				// auto initNorm = initNormals[ j ];
				// vert.Normal.x = initNorm.x * sin;
				// vert.Normal.y = initNorm.y;
				// vert.Normal.z = initNorm.x * cos;

				vert.Normal.x = initNormals[3 * j + 0] * sin;
				vert.Normal.y = initNormals[3 * j + 1];
				vert.Normal.z = initNormals[3 * j + 0] * cos;

				AddVertex(vert);
			}
		}

	// indices
	for ( size_t i = 0; i < segments; i ++ ) {
		for ( size_t j = 0; j < ( points.size() - 1 ); j ++ ) {

			const size_t base = j + i * points.size();

			const size_t a = base;
			const size_t b = base + points.size();
			const size_t c = base + points.size() + 1;
			const size_t d = base + 1;

			// faces
			AddTriangleIndices( a, b, d );
			AddTriangleIndices( c, d, b );
		}
	}
}

/* =============================================================================
									Cylinder Geo
===============================================================================*/

void CylinderGeometry::BuildGeometry()
{
	ResetVertexData();
	m_Dirty = false;

	auto& p = m_Params;

	// helper variables
	unsigned int index = 0;

	// generate torso
	GenerateTorso(index);

	if ( p.openEnded == false ) {
		if ( p.radiusTop > 0 ) GenerateCap( true, index );
		if ( p.radiusBottom > 0 ) GenerateCap( false, index );
	}
}

void CylinderGeometry::GenerateTorso(unsigned int& index)
{
	auto& p = m_Params;
	const float halfHeight = p.height / 2.0f;

	std::vector<std::vector<unsigned int>> indexArray;

	// this will be used to calculate the normal
	const float slope = ( p.radiusBottom - p.radiusTop ) / p.height;

	// generate vertices, normals and uvs
	for ( unsigned int y = 0; y <= p.heightSegments; y++ ) {

		std::vector<unsigned int> indexRow;

		const float v = (float) y / (float) p.heightSegments;

		// calculate the radius of the current row
		const float radius = v * ( p.radiusBottom - p.radiusTop ) + p.radiusTop;


		for ( unsigned int x = 0; x <= p.radialSegments; x++ ) {
			Vertex vert;

			const float u = (float) x / (float) p.radialSegments;

			const float theta = u * p.thetaLength + p.thetaStart;

			const float sinTheta = glm::sin( theta );
			const float cosTheta = glm::cos( theta );

			// vertex
			vert.Position.x = radius * sinTheta;
			vert.Position.y = - v * p.height + halfHeight;
			vert.Position.z = radius * cosTheta;
			// vertices.push( vertex.x, vertex.y, vertex.z );

			// normal
			vert.Normal = glm::normalize(glm::vec3( sinTheta, slope, cosTheta ));

			// uv
			vert.TexCoords.x = u;
			vert.TexCoords.y = 1.0 - v;

			// save index of vertex in respective row
			AddVertex(vert);

			indexRow.push_back( index ++ );
		}

		// now save vertices of the row in our index array
		indexArray.push_back( indexRow );

	}

	// generate indices
	for ( unsigned int x = 0; x < p.radialSegments; x ++ ) {
		for ( unsigned int y = 0; y < p.heightSegments; y ++ ) {

			// we use the index array to access the correct indices

			const unsigned int a = indexArray[ y ][ x ];
			const unsigned int b = indexArray[ y + 1 ][ x ];
			const unsigned int c = indexArray[ y + 1 ][ x + 1 ];
			const unsigned int d = indexArray[ y ][ x + 1 ];

			// faces
			AddTriangleIndices( a, b, d );
			AddTriangleIndices( b, c, d );
		}
	}
}

void CylinderGeometry::GenerateCap(bool top, unsigned int& index)
{
	auto& p = m_Params;
	const float halfHeight = p.height / 2.0f;

	// save the index of the first center vertex
	const auto centerIndexStart = index;

	const float radius = top ? p.radiusTop : p.radiusBottom;
	const float sign = top ? 1.0f : -1.0f;

	// first we generate the center vertex data of the cap.
	// because the geometry needs one set of uvs per face,
	// we must generate a center vertex per face/segment
	for (unsigned int x = 1; x <= p.radialSegments; x++)
	{
		Vertex vert;

		vert.Position = glm::vec3(0.0, halfHeight * sign, 0.0);
		vert.Normal = glm::vec3(0.0, sign, 0.0);
		vert.TexCoords = glm::vec2(0.5, 0.5);

		AddVertex(vert);

		// increase index
		index++;
	}

	// save the index of the last center vertex
	const unsigned int centerIndexEnd = index;

	// now we generate the surrounding vertices, normals and uvs

	for (unsigned int x = 0; x <= p.radialSegments; x++)
	{
		Vertex vert;

		const float u = (float) x / (float) p.radialSegments;
		const float theta = u * p.thetaLength + p.thetaStart;

		const float cosTheta = glm::cos(theta);
		const float sinTheta = glm::sin(theta);

		// vertex
		vert.Position.x = radius * sinTheta;
		vert.Position.y = halfHeight * sign;
		vert.Position.z = radius * cosTheta;

		// normal
		vert.Normal = glm::vec3(0, sign, 0);

		// uv
		vert.TexCoords.x = (cosTheta * 0.5) + 0.5;
		vert.TexCoords.y = (sinTheta * 0.5 * sign) + 0.5;

		AddVertex(vert);

		// increase index
		index++;
	}

	// generate indices

	for (unsigned int x = 0; x < p.radialSegments; x++)
	{

		const unsigned int c = centerIndexStart + x;
		const unsigned int i = centerIndexEnd + x;

		if (top)
		{
			// face top
			AddTriangleIndices(i, i + 1, c);
		}
		else
		{
			// face bottom
			AddTriangleIndices(i + 1, i, c);
		}
	}
}

// void CapsuleGeometry::BuildGeometry()
// {
// 	ResetVertexData();
// 	m_Dirty = false;
// }

/* =============================================================================
								Triangle Geo
===============================================================================*/

void TriangleGeometry::BuildGeometry()
{
	ResetVertexData();
	m_Dirty = false;

	// first calculate position data, assuming bottom left point is centered at 0,0
	glm::vec3 p1 = glm::vec3(0, 0, 0);   		  // bottom left
	glm::vec3 p2 = glm::vec3(m_Params.width, 0, 0);   // bottom right
	glm::vec3 p3 = glm::vec3(					  // top
		m_Params.height / glm::tan(m_Params.theta),
		m_Params.height,
		0
	);

	// then modify position data to center triangle at 0,0
	glm::vec3 center = (p1 + p2 + p3) / 3.0f;
	p1 -= center;
	p2 -= center;
	p3 -= center;

	// add vertex data
	Vertex vert;
	vert.Position = p1;
	vert.Normal = glm::vec3(0, 0, 1);
	vert.TexCoords = glm::vec2(0, 0);
	AddVertex(vert);

	vert.Position = p2;
	vert.Normal = glm::vec3(0, 0, 1);
	vert.TexCoords = glm::vec2(1, 0);
	AddVertex(vert);

	vert.Position = p3;
	vert.Normal = glm::vec3(0, 0, 1);
	vert.TexCoords = glm::vec2(0, 1);
	AddVertex(vert);

	AddTriangleIndices(0, 1, 2);
}
