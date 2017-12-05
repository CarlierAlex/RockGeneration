#pragma once
#include "VertexStructs.h"
#include <set>

struct Triangle
{
	UINT vertex[3];
};

using TriangleList = std::vector<Triangle>;
using VertexList = std::vector<XMFLOAT3>;
using Lookup = std::map<std::pair<UINT, UINT>, UINT>;
using IndexedMesh = std::pair<VertexList, TriangleList>;

namespace icosahedron
{
	const float X = .525731112119133606f;
	const float Z = .850650808352039932f;
	const float N = 0.f;

	static const VertexList vertices =
	{
		{ -X,N,Z },{ X,N,Z },{ -X,N,-Z },{ X,N,-Z },
		{ N,Z,X },{ N,Z,-X },{ N,-Z,X },{ N,-Z,-X },
		{ Z,X,N },{ -Z,X, N },{ Z,-X,N },{ -Z,-X, N }
	};

	static const TriangleList triangles =
	{
		{ 0,4,1 },{ 0,9,4 },{ 9,5,4 },{ 4,5,8 },{ 4,8,1 },
		{ 8,10,1 },{ 8,3,10 },{ 5,3,8 },{ 5,2,3 },{ 2,7,3 },
		{ 7,10,3 },{ 7,6,10 },{ 7,11,6 },{ 11,0,6 },{ 0,1,6 },
		{ 6,1,10 },{ 9,0,11 },{ 9,11,2 },{ 9,2,5 },{ 7,2,11 }
	};
}

struct VertexRock
{
	VertexRock() :
		Position(XMFLOAT3(0,0,0)),
		Normal(XMFLOAT3(0, 0, 0)),
		Tangent(XMFLOAT3(0, 0, 0)),
		TexCoord(XMFLOAT2(0, 0))
	{}

	VertexRock(VertexBase& vertex) :
		Position(vertex.Position),
		Normal(vertex.Normal),
		Tangent(vertex.Normal),
		TexCoord(vertex.TexCoord)
	{}

	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT3 Tangent;
	XMFLOAT2 TexCoord;
};

const auto UVFromVector3 = [](const XMFLOAT3 position)
{
	XMFLOAT3 normalizedPos;
	XMVECTOR norm = XMLoadFloat3(&position);
	XMStoreFloat3(&normalizedPos, XMVector3Normalize(norm));
	float angle, u, v;

	angle = (float)atan2(normalizedPos.z, normalizedPos.x);
	u = (angle / XM_PI) * 0.5f;
	angle = (float)acos(normalizedPos.y);
	v = (angle / XM_PI);

	return XMFLOAT2(u, v);
};

const auto DotProduct = [](const XMFLOAT3 v1, const XMFLOAT3 v2)
{
	auto vec1 = XMVector3Normalize(XMLoadFloat3(&v1));
	auto vec2 = XMVector3Normalize(XMLoadFloat3(&v2));

	auto T = XMVector3Dot(vec1, vec2);
	float t;
	XMStoreFloat(&t, T);

	return t;
};

const auto CrossProduct = [](const XMFLOAT3 v1, const XMFLOAT3 v2)
{
	XMVECTOR vec1 = XMLoadFloat3(&v1);
	XMVECTOR vec2 = XMLoadFloat3(&v2);

	XMVECTOR cross = XMVector3Cross(vec1, vec2);

	XMFLOAT3 crossResult;
	XMStoreFloat3(&crossResult, cross);

	return crossResult;
};

const auto WeldVertices = [](std::vector<VertexRock> points)
{
	VertexRock weldedPoint;

	XMVECTOR pos = XMLoadFloat3(&XMFLOAT3(0, 0, 0)), 
		norm = XMLoadFloat3(&XMFLOAT3(0, 0, 0)), 
		tex = XMLoadFloat3(&XMFLOAT3(0, 0, 0)), 
		tang = XMLoadFloat3(&XMFLOAT3(0, 0, 0));
	UINT size = points.size();
	for each (auto point in points)
	{
		pos += XMLoadFloat3(&point.Position);
		norm += XMLoadFloat3(&point.Normal);
		tex += XMLoadFloat2(&point.TexCoord);
		tang += XMLoadFloat3(&point.Tangent);
	}

	pos = pos / size;
	norm = norm / size;
	tex = tex / size;
	tang = tang / size;

	XMStoreFloat3(&weldedPoint.Position, pos);
	XMStoreFloat3(&weldedPoint.Normal, norm);
	XMStoreFloat2(&weldedPoint.TexCoord, tex);
	XMStoreFloat3(&weldedPoint.Tangent, tang);

	return weldedPoint;
};

const auto WeldVerticesProto = [](std::vector<VertexPosNormTex> points)
{
	VertexPosNormTex weldedPoint;

	XMVECTOR pos, norm, tex;
	UINT size = points.size();
	for each (auto point in points)
	{
		pos += XMLoadFloat3(&point.Position);
		norm += XMLoadFloat3(&point.Normal);
		tex += XMLoadFloat2(&point.TexCoord);
	}

	pos = pos / size;
	norm = norm / size;
	tex = tex / size;

	XMStoreFloat3(&weldedPoint.Position, pos);
	XMStoreFloat3(&weldedPoint.Normal, norm);
	XMStoreFloat2(&weldedPoint.TexCoord, tex);
	return weldedPoint;
};

const auto VertexForEdge = [](Lookup& lookup, VertexList& vertices, UINT first, UINT second)
{
	Lookup::key_type key(first, second);
	if (key.first>key.second)
		std::swap(key.first, key.second);

	auto inserted = lookup.insert({ key, vertices.size() });
	if (inserted.second)
	{
		auto& edge0 = XMLoadFloat3(&vertices[first]);
		auto& edge1 = XMLoadFloat3(&vertices[second]);
		auto point = edge0 + edge1;
		XMFLOAT3 newVert;
		XMStoreFloat3(&newVert, XMVector3Normalize(point));
		vertices.push_back(newVert);
	}

	return inserted.first->second;
};

const auto SubdivideTriangle = [](VertexList& vertices, TriangleList triangles)
{
	Lookup lookup;
	TriangleList result;

	for (auto&& each : triangles)
	{
		UINT mid[3];
		for (int edge = 0; edge<3; ++edge)
		{
			mid[edge] = VertexForEdge(lookup, vertices, each.vertex[edge], each.vertex[(edge + 1) % 3]);
		}
		result.push_back({ each.vertex[0], mid[0], mid[2] });
		result.push_back({ each.vertex[1], mid[1], mid[0] });
		result.push_back({ each.vertex[2], mid[2], mid[1] });
		result.push_back({ mid[0], mid[1], mid[2] });
	}

	return result;
};

const auto MakeIcosphere = [](int subdivisions)
{
	VertexList vertices = icosahedron::vertices;
	TriangleList triangles = icosahedron::triangles;

	for (int i = 0; i<subdivisions; ++i)
	{
		triangles = SubdivideTriangle(vertices, triangles);
	}

	IndexedMesh result{ vertices, triangles };

	return result;
};

const auto CalculateTangent = [](const XMFLOAT3& P1, const XMFLOAT3& P2, const XMFLOAT3& P3, const XMFLOAT2& UV1, const XMFLOAT2& UV2, const XMFLOAT2& UV3)
{
	XMFLOAT3 tangent;
	XMFLOAT3 normal;
	XMVECTOR tangentV;
	XMVECTOR normalV;

	XMFLOAT3 v2v1, v3v1;
	XMStoreFloat3(&v2v1, XMLoadFloat3(&P2) - XMLoadFloat3(&P1));
	XMStoreFloat3(&v3v1, XMLoadFloat3(&P3) - XMLoadFloat3(&P1));

	normalV = XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&v2v1), XMLoadFloat3(&v3v1)));
	XMStoreFloat3(&normal, normalV);

	tangentV = XMVector3Normalize(XMVector3Cross(normalV, XMLoadFloat3(&XMFLOAT3(0,1,0))));
	tangentV = XMVector3Normalize(tangentV);
	XMStoreFloat3(&tangent, tangentV);

	return tangent;
};

const auto ComputeNormal = [](const XMFLOAT3& P0, const XMFLOAT3& P1, const XMFLOAT3& P2)
{
	XMFLOAT3 normal;
	XMVECTOR normalV;

	XMFLOAT3 P, Q;
	XMStoreFloat3(&P, XMLoadFloat3(&P1) - XMLoadFloat3(&P0));
	XMStoreFloat3(&Q, XMLoadFloat3(&P2) - XMLoadFloat3(&P0));

	normalV = XMVector3Cross(XMLoadFloat3(&Q), XMLoadFloat3(&P));

	normalV = XMVector3Normalize(normalV);
	XMStoreFloat3(&normal, normalV);
	return normal;
};

const auto ComputeTangent = [](const XMFLOAT3& P0, const XMFLOAT3& P1, const XMFLOAT3& P2,
	const XMFLOAT2& UV0, const XMFLOAT2& UV1, const XMFLOAT2& UV2)
{
	XMFLOAT3 tangent;
	//XMFLOAT3 normal;
	XMVECTOR tangentV;
	//XMVECTOR normalV;

	XMFLOAT3 P, Q;
	XMStoreFloat3(&P, XMLoadFloat3(&P1) - XMLoadFloat3(&P0));
	XMStoreFloat3(&Q, XMLoadFloat3(&P2) - XMLoadFloat3(&P0));

	//normalV = XMVector3Cross(XMLoadFloat3(&P), XMLoadFloat3(&Q));
	//using Eric Lengyel's approach with a few modifications
	//from Mathematics for 3D Game Programmming and Computer Graphics
	// want to be able to trasform a vector in Object Space to Tangent Space
	// such that the x-axis cooresponds to the 's' direction and the
	// y-axis corresponds to the 't' direction, and the z-axis corresponds
	// to <0,0,1>, straight up out of the texture map


	float s1 = UV1.x - UV0.x;
	float t1 = UV1.y - UV0.y;
	float s2 = UV2.x - UV0.x;
	float t2 = UV2.y - UV0.y;


	//we need to solve the equation
	// P = s1*T + t1*B
	// Q = s2*T + t2*B
	// for T and B


	//this is a linear system with six unknowns and six equatinos, for TxTyTz BxByBz
	//[px,py,pz] = [s1,t1] * [Tx,Ty,Tz]
	// qx,qy,qz     s2,t2     Bx,By,Bz

	//multiplying both sides by the inverse of the s,t matrix gives
	//[Tx,Ty,Tz] = 1/(s1t2-s2t1) *  [t2,-t1] * [px,py,pz]
	// Bx,By,Bz                      -s2,s1	    qx,qy,qz  

	//solve this for the unormalized T and B to get from tangent to object space

	float tmp = 0.0f;
	if (fabsf(s1*t2 - s2*t1) <= 0.0001f)
	{
		tmp = 1.0f;
	}
	else
	{
		tmp = 1.0f / (s1*t2 - s2*t1);
	}

	tangent.x = (t2*P.x - t1*Q.x);
	tangent.y = (t2*P.y - t1*Q.y);
	tangent.z = (t2*P.z - t1*Q.z);
	tangentV = XMLoadFloat3(&tangent);
	tangentV = tangentV*tmp;

	tangentV = XMVector3Normalize(tangentV);
	XMStoreFloat3(&tangent, tangentV);
	return tangent;
};

const auto AddXMFLOAT3 = [](const XMFLOAT3& original, const XMFLOAT3& addition)
{
	XMVECTOR origin = XMLoadFloat3(&original);
	XMVECTOR add = XMLoadFloat3(&addition);

	XMFLOAT3 newTangent;
	XMStoreFloat3(&newTangent, origin + add);

	return newTangent;
};

const auto NormalizeXMFLOAT3 = [](const XMFLOAT3& tangent)
{
	XMFLOAT3 newTangent;
	XMVECTOR origin = XMLoadFloat3(&tangent);
	XMStoreFloat3(&newTangent, XMVector3Normalize(origin));
	return newTangent;
};

const auto MultiplyXMFLOAT3 = [](const XMFLOAT3 v0, const float scalar)
{
	XMVECTOR p0 = XMLoadFloat3(&v0);

	XMFLOAT3 vector;
	XMVECTOR newV = p0 * scalar;
	XMStoreFloat3(&vector, newV);

	return vector;
};

const auto SubstractXMFLOAT3 = [](const XMFLOAT3 v0, const XMFLOAT3 v1)
{
	XMVECTOR p0 = XMLoadFloat3(&v0);
	XMVECTOR p1 = XMLoadFloat3(&v1);

	XMFLOAT3 vector;
	XMVECTOR newV = XMVectorSubtract(p0, p1);
	XMStoreFloat3(&vector, newV);

	return vector;
};

const auto LengthBetweenPoints = [](const XMFLOAT3 v0, const XMFLOAT3 v1)
{
	float distance = 0.0f;

	XMVECTOR p0 = XMLoadFloat3(&v0);
	XMVECTOR p1 = XMLoadFloat3(&v1);

	XMVECTOR length = XMVector3Length(XMVectorSubtract(p0, p1));
	XMStoreFloat(&distance, length);

	return distance;
};

const auto AngleBetweenVectors = [](const XMFLOAT3 v0, const XMFLOAT3 v1)
{
	float angle = 0.0f;

	XMVECTOR p0 = XMLoadFloat3(&v0);
	XMVECTOR p1 = XMLoadFloat3(&v1);

	XMVECTOR length = XMVector3AngleBetweenVectors(p0, p1);
	XMStoreFloat(&angle, length);

	return angle;
};

const auto MaxLengthTriangleLine = [](const XMFLOAT3& v0, const XMFLOAT3& v1, const XMFLOAT3& v2)
{
	float maxDistance = 0.0f;
	float distance = 0.0f;

	distance = LengthBetweenPoints(v0, v1);
	if (maxDistance < distance)
		maxDistance = distance;

	distance = LengthBetweenPoints(v0, v2);
	if (maxDistance < distance)
		maxDistance = distance;

	distance = LengthBetweenPoints(v1, v2);
	if (maxDistance < distance)
		maxDistance = distance;

	return distance;
};

const auto MinLengthTriangleLine = [](const XMFLOAT3& v0, const XMFLOAT3& v1, const XMFLOAT3& v2)
{
	float maxDistance = 9999999;
	float distance = 0.0f;

	distance = LengthBetweenPoints(v0, v1);
	if (maxDistance > distance)
		maxDistance = distance;

	distance = LengthBetweenPoints(v0, v2);
	if (maxDistance > distance)
		maxDistance = distance;

	distance = LengthBetweenPoints(v1, v2);
	if (maxDistance > distance)
		maxDistance = distance;

	if(maxDistance < 0.1f)
		maxDistance = 0.1f;

	return distance;
};

const auto AverageXmfloat3 = [](std::vector<XMFLOAT3> points)
{
	XMFLOAT3 average;
	XMVECTOR pos;
	UINT size = points.size();
	for each (auto point in points)
	{
		pos += XMLoadFloat3(&point);
	}
	pos = pos / size;

	XMStoreFloat3(&average, pos);
	return average;
};

const auto CheckUVForDistance = [](std::vector<VertexRock*> points)
{
	auto threshHold = 0.07f;
	std::vector<XMFLOAT2> UVlist;
	UINT size = points.size();
	UVlist.resize(size);

	for each (auto point in points)
	{
		auto uv = point->TexCoord;
		while (uv.x > 0.99)
			uv.x -= 1;
		while (uv.y > 0.99)
			uv.y -= 1;
		UVlist.push_back(uv);
	}

	for (UINT i = 0; i < size; i += 3)
	{
		auto distance = abs(UVlist[i % size].x - UVlist[(i + 1) % size].x);
		if (distance < threshHold)
			return true;

		distance = UVlist[i % size].y - UVlist[(i + 1) % size].y;
		if (distance < threshHold)
			return true;
	}

	return false;
};

const auto SameSide = [](const XMFLOAT3 p1, const XMFLOAT3 p2, const XMFLOAT3 A, const XMFLOAT3 B)
{
	XMVECTOR vecP1 = XMLoadFloat3(&p1);
	XMVECTOR vecP2 = XMLoadFloat3(&p2);
	XMVECTOR vecA = XMLoadFloat3(&A);
	XMVECTOR vecB = XMLoadFloat3(&B);

	XMVECTOR cp1 = XMVector3Cross((vecA - vecB), (vecA - vecP1));
	XMVECTOR cp2 = XMVector3Cross((vecA - vecB), (vecA - vecP1));

	auto d = XMVector3Dot(cp1, cp2);
	float dot;
	XMStoreFloat(&dot, d);

	if (dot >= 0) return true;
	return false;
};

const auto PointInTriangle = [](const XMFLOAT3 TriangleVectors [3], const XMFLOAT3 P)
{
	XMFLOAT3 A = TriangleVectors[0], B = TriangleVectors[1], C = TriangleVectors[2];
	XMVECTOR vecA = XMLoadFloat3(&A);
	XMVECTOR vecB = XMLoadFloat3(&B);
	XMVECTOR vecC = XMLoadFloat3(&A);
	XMVECTOR vecP = XMLoadFloat3(&B);

	if (SameSide(P, A, B, C) && SameSide(P, B, A, C) && SameSide(P, C, A, B))
	{
		XMVECTOR vc1 = XMVector3Cross(vecB - vecA, vecC - vecA);
		auto d = XMVector3Dot((vecP - vecA), vc1);
		float dot;
		XMStoreFloat(&dot, d);

		if (abs(dot) <= .01f)
			return true;
	}

	return false;
};

const auto LinePlaneIntersection = [](const XMFLOAT3 triangle[3], const XMFLOAT3 triangleNormal, const XMFLOAT3 rayStart, const XMFLOAT3 rayEnd)
{
	auto triangleN = XMLoadFloat3(&triangleNormal);
	auto pointOnPlane = XMLoadFloat3(&AverageXmfloat3({ triangle[0] ,triangle[1] ,triangle[2] }));
	auto rayOrigin = XMLoadFloat3(&rayStart);
	auto rayDirection = XMLoadFloat3(&rayEnd) - rayOrigin;


	auto T = XMVector3Dot(triangleN,(pointOnPlane - rayOrigin))
		/ XMVector3Dot(triangleN,rayDirection);

	float t;
	XMStoreFloat(&t, T);

	auto pointInPlane = rayOrigin + rayDirection * t;
	XMFLOAT3 planePoint;
	XMStoreFloat3(&planePoint, pointInPlane);


	auto pS = SubstractXMFLOAT3(rayStart, planePoint);
	auto pE = SubstractXMFLOAT3(rayEnd, planePoint);

	if (AngleBetweenVectors(pS, pE) != XM_PI)
		return false;
	
	return PointInTriangle(triangle, planePoint);

	return false;
};

const auto ProjectedPointOnPlane = [](const XMFLOAT3 originPlane, const XMFLOAT3 vertice, const XMFLOAT3 normalPlane)
{
	auto point = XMLoadFloat3(&vertice);
	auto vecP = point - XMLoadFloat3(&originPlane);
	auto normal = XMLoadFloat3(&normalPlane);
	auto dotV = XMVector3Dot(vecP, normal);
	float dot;
	XMStoreFloat(&dot, dotV);
	if (dot < 0) // dont proceed this one if dot is negative == more then 90 degree
		return XMFLOAT3(0,0,0);

	XMFLOAT3 vectorFromPoint;
	DirectX::XMStoreFloat3(&vectorFromPoint, vecP);

	auto dist = LengthBetweenPoints(vectorFromPoint, normalPlane);
	//dist = dot;
	auto projected_point = point - dist * normal;// +origin;
	XMFLOAT3 projectedPoint;
	DirectX::XMStoreFloat3(&projectedPoint, projected_point);

	return projectedPoint;
};