#include "stdafx.h"
#include "GenRock.h"
#include "ContentManager.h"
#include "DdsTextureResource.h"

GenRock::GenRock(float width, float height, float depth, int steps) :
	m_Width(width),
	m_Height(height),
	m_Depth(depth),
	m_Steps(steps),
	m_pVertexLayout(nullptr),
	m_pVertexBuffer(nullptr),
	m_pIndexBuffer(nullptr),
	m_pEffect(nullptr),
	m_pTechnique(nullptr),
	m_pMatWorldViewProjVariable(nullptr),
	m_pMatWorldVariable(nullptr),
	m_pMatViewInvVariable(nullptr),
	m_NumVertices(0),
	m_NumIndices(0)
{

}


GenRock::~GenRock(void)
{
	m_VecVertices.clear();
	m_VecIndices.clear();

	m_pVertexLayout->Release();
	m_pVertexBuffer->Release();
	m_pIndexBuffer->Release();
}


//BUILD ICOSPHERE
//*******************************************************************************************************************************
void  GenRock::BuildIco()
{
	int subdivisions = m_Steps;
	auto lists = MakeIcosphere(subdivisions);
	auto vertices = lists.first;
	auto indices = lists.second;

	//SUBDIVIDE TRIANGLES + ADD NEW VERTICES TO BUFFER
	//-----------------------------------------------------------------------------------------
	for (int i = 0; i < vertices.size(); i++)
	{
		auto vertice = vertices[i];
		auto vertVector = XMVector3Normalize(XMLoadFloat3(&vertice));
		XMFLOAT3 vert;
		DirectX::XMStoreFloat3(&vert, XMVector3Normalize(vertVector));

		float r = sqrt(vertice.x * vertice.x + vertice.y * vertice.y + vertice.z * vertice.z);
		float theta = acos(vertice.y / r); //lat
		float phi = atan(vertice.x / vertice.z); //long

		XMFLOAT3 newVert;
		newVert = vert;
		newVert.x *= m_Width;
		newVert.y *= m_Height;
		newVert.z *= m_Depth;

		XMFLOAT3 normal;
		auto normalVector = XMVector3Normalize(XMLoadFloat3(&newVert));
		DirectX::XMStoreFloat3(&normal, XMVector3Normalize(normalVector));

		XMFLOAT2 texcoord;
		texcoord = UVFromVector3(newVert);
		if (texcoord.y == 0)
			m_NorthIdx.insert(i);
		if (texcoord.y == 1)
			m_SouthIdx.insert(i);

		VertexBase base;
		base.Position = newVert;
		base.Normal = normal;
		base.Tangent = XMFLOAT3(0,0,0);
		base.TexCoord = texcoord;

		auto ver = VertexRock(base);

		m_VecVertices.push_back(ver);

	}
	m_NumVertices = m_VecVertices.size();

	//SET INDICES
	//-----------------------------------------------------------------------------------------
	for each (auto indice in indices)
	{
		m_VecIndices.push_back(indice.vertex[0]);
		m_VecIndices.push_back(indice.vertex[1]);
		m_VecIndices.push_back(indice.vertex[2]);
	}
	m_NumIndices = m_VecIndices.size();
}

//CONVERT ICOSPHERE INTO 'ROCK'
//*******************************************************************************************************************************
void GenRock::BuildRock()
{
	//FLATTEN BY 'PLANES'
	//-----------------------------------------------------------------------------------------
	for (UINT plane = 0; plane < m_MaxPlanes; plane++)
	{
		//Determine position of plane by angle on sphere
		XMFLOAT3 originPlane, radiusPlane;
		m_PrevAngles.x = m_PrevAngles.x + m_MinRandAngle / 180.0f * XM_PI;
		m_PrevAngles.y = m_PrevAngles.y + m_MinRandAngle / 180.0f * XM_PI;

		m_PrevAngles.x = rand() % ((int)m_MaxRandAngle - (int)m_MinRandAngle) + (int)m_MinRandAngle;
		m_PrevAngles.y = rand() % ((int)m_MaxRandAngle - (int)m_MinRandAngle) + (int)m_MinRandAngle;

		//Origin plane
		originPlane.x = m_Width * cos(m_PrevAngles.x) * cos(m_PrevAngles.y);
		originPlane.y = m_Height * cos(m_PrevAngles.x) * sin(m_PrevAngles.y);
		originPlane.z = m_Depth * sin(m_PrevAngles.x);

		radiusPlane.x = m_Width * cos(m_PrevAngles.x + XM_PI) * cos(m_PrevAngles.y + XM_PI);
		radiusPlane.y = m_Height * cos(m_PrevAngles.x + XM_PI) * sin(m_PrevAngles.y + XM_PI);
		radiusPlane.z = m_Depth * sin(m_PrevAngles.x + XM_PI);

		//Create plane
		XMFLOAT3 normalPlane;
		XMVECTOR origin = DirectX::XMLoadFloat3(&originPlane);
		float offset = rand() % (int)m_MaxOffsetPercent;
		origin *= (100.0f - offset) / 100.0f;
		DirectX::XMStoreFloat3(&originPlane, origin);
		XMVECTOR normal = XMVector3Normalize(origin);
		normal = XMVector3Normalize(normal);
		DirectX::XMStoreFloat3(&normalPlane, normal);

		//Flatten vertices onto plane
		for (UINT i = 0; i < m_NumVertices; i++)
		{
			//Check if vertice is in front of the plane
			auto vertice = m_VecVertices[i];
			auto point = XMLoadFloat3(&vertice.Position);
			auto vecP = point - XMLoadFloat3(&originPlane);
			auto dotV = XMVector3Dot(vecP, normal);
			float dot;
			XMStoreFloat(&dot, dotV);
			if (dot < 0) // dont proceed this one if dot is negative == more then 90 degree
				continue;

			//Project on plane
			XMFLOAT3 vectorFromPoint;
			DirectX::XMStoreFloat3(&vectorFromPoint, vecP);

			auto dist = vectorFromPoint.x*normalPlane.x + vectorFromPoint.y*normalPlane.y + vectorFromPoint.z*normalPlane.z;
			auto projected_point = point - dist * normal;
			XMFLOAT3 projectedPoint;
			DirectX::XMStoreFloat3(&projectedPoint, projected_point);

			//Create new vertice, make curved
			auto distToCenter = LengthBetweenPoints(projectedPoint, originPlane);
			auto diameter = LengthBetweenPoints(XMFLOAT3(0,0,0), radiusPlane) / 2.0f;
			auto strength = (1.0f / diameter)*distToCenter - 1.0f;

			projected_point = point - (dist / 2.0f) * normal * strength;
			DirectX::XMStoreFloat3(&projectedPoint, projected_point);

			//Update vertice
			m_VecVertices[i].Position = projectedPoint;
			m_VecVertices[i].Normal = normalPlane;
		}
	}
}

//PUSH VERTICES OUTWARDS TO COUNTER OVERLAP
//*******************************************************************************************************************************
void GenRock::Expand()
{
	float averageRadius = (m_Width + m_Height + m_Depth) / 3.0f;
	for (UINT i = 0; i < m_NumIndices; i += 3)
	{
		auto idx0 = m_VecIndices[i % m_NumIndices];
		auto idx1 = m_VecIndices[(i + 1) % m_NumIndices];
		auto idx2 = m_VecIndices[(i + 2) % m_NumIndices];

		auto v0 = &m_VecVertices[idx0];
		auto v1 = &m_VecVertices[idx1];
		auto v2 = &m_VecVertices[idx2];

		//Push all vertices out by every plane
		XMFLOAT3 triangle[3] = { v0->Position , v1->Position , v2->Position };
		XMFLOAT3 normal = ComputeNormal(v0->Position, v1->Position, v2->Position);

		(*v0).Position = AddXMFLOAT3(v0->Position, MultiplyXMFLOAT3(normal, averageRadius / 100.f));
		(*v1).Position = AddXMFLOAT3(v1->Position, MultiplyXMFLOAT3(normal, averageRadius / 100.f));
		(*v2).Position = AddXMFLOAT3(v2->Position, MultiplyXMFLOAT3(normal, averageRadius / 100.f));
	}
}

//BUILD NORMALS
//*******************************************************************************************************************************
void GenRock::BuildNormals()
{
	XMFLOAT3 normal;
	for (UINT idx = 0; idx + 2 < m_VecIndices.size(); idx += 3)
	{
		int idx0 = m_VecIndices[idx];
		int idx1 = m_VecIndices[idx + 1];
		int idx2 = m_VecIndices[idx + 2];

		normal = ComputeNormal(
			m_VecVertices[idx0].Position, m_VecVertices[idx1].Position, m_VecVertices[idx2].Position
		);

		m_VecVertices[idx0].Normal = AddXMFLOAT3(m_VecVertices[idx0].Normal, normal);
		m_VecVertices[idx1].Normal = AddXMFLOAT3(m_VecVertices[idx1].Normal, normal);
		m_VecVertices[idx2].Normal = AddXMFLOAT3(m_VecVertices[idx2].Normal, normal);
	}
	for (UINT i = 0; i < m_VecVertices.size(); i++)
	{
		m_VecVertices[i].Normal = NormalizeXMFLOAT3(m_VecVertices[i].Normal);
	}
}

//CORRECT UV SEAMS
//*******************************************************************************************************************************
void GenRock::CorrectUV()
{
	//Find seam vertices
	UINT countExtraVerts = 0;
	std::set<UINT> duplicatesIdx;
	for (int i = 0; i < m_NumIndices; i += 3)
	{
		//DATA --------------------------------------------
		#pragma region data
		auto idx0 = &m_VecIndices[i % m_NumIndices];
		auto idx1 = &m_VecIndices[(i + 1) % m_NumIndices];
		auto idx2 = &m_VecIndices[(i + 2) % m_NumIndices];

		auto v0 = &m_VecVertices[*idx0];
		auto v1 = &m_VecVertices[*idx1];
		auto v2 = &m_VecVertices[*idx2];

		XMFLOAT3 tex0 = XMFLOAT3(v0->TexCoord.x, v0->TexCoord.y, 0);
		XMFLOAT3 tex1 = XMFLOAT3(v1->TexCoord.x, v1->TexCoord.y, 0);
		XMFLOAT3 tex2 = XMFLOAT3(v2->TexCoord.x, v2->TexCoord.y, 0);

		XMFLOAT3 texNormal;
		XMVECTOR texN = XMVector3Cross(XMLoadFloat3(&tex1) - XMLoadFloat3(&tex0), XMLoadFloat3(&tex2) - XMLoadFloat3(&tex0));
		//Check uv to determine if new triangles are needed
		DirectX::XMStoreFloat3(&texNormal, texN);
		#pragma endregion

		//SIDES --------------------------------------------
		if (texNormal.z > 0)
		{
			if (tex0.x < 0.1f)
			{
				if (duplicatesIdx.find(*idx0) != duplicatesIdx.end())
				{
					m_VecIndices[i % m_NumIndices] = *duplicatesIdx.find(*idx0);
				}
				else
				{
					auto newV0 = *v0;
					newV0.TexCoord.x += 1.0f;
					m_VecVertices.push_back(newV0);
					m_VecIndices[i % m_NumIndices] = countExtraVerts + m_NumVertices;

					duplicatesIdx.insert(countExtraVerts + m_NumVertices);
					countExtraVerts++;
				}
			}

			if (tex1.x < 0.1f)
			{
				if (duplicatesIdx.find(*idx1) != duplicatesIdx.end())
				{
					m_VecIndices[(i + 1) % m_NumIndices] = *duplicatesIdx.find(*idx1);
				}
				else
				{
					auto newV1 = *v1;
					newV1.TexCoord.x += 1.0f;
					m_VecVertices.push_back(newV1);
					m_VecIndices[(i + 1) % m_NumIndices] = countExtraVerts + m_NumVertices;

					duplicatesIdx.insert(countExtraVerts + m_NumVertices);
					countExtraVerts++;
				}
			}

			if (tex2.x < 0.1f)
			{
				if (duplicatesIdx.find(*idx2) != duplicatesIdx.end())
				{
					m_VecIndices[(i + 2) % m_NumIndices] = *duplicatesIdx.find(*idx2);
				}
				else
				{
					auto newV2 = *v2;
					newV2.TexCoord.x += 1.0f;
					m_VecVertices.push_back(newV2);
					m_VecIndices[(i + 2) % m_NumIndices] = countExtraVerts + m_NumVertices;

					duplicatesIdx.insert(countExtraVerts + m_NumVertices);
					countExtraVerts++;
				}
			}
		}


		//POLES --------------------------------------------
		if (m_NorthIdx.find(*idx0) != m_NorthIdx.end() || m_SouthIdx.find(*idx0) != m_SouthIdx.end())
		{
			auto newV0 = *v0;
			newV0.TexCoord.x = (v1->TexCoord.x + v2->TexCoord.x) / 2.0f;
			m_VecVertices.push_back(newV0);
			m_VecIndices[(i) % m_NumIndices] = countExtraVerts + m_NumVertices;
			countExtraVerts++;
		}
		else if (m_NorthIdx.find(*idx1) != m_NorthIdx.end() || m_SouthIdx.find(*idx1) != m_SouthIdx.end())
		{
			auto newV1 = *v0;
			newV1.TexCoord.x = (v0->TexCoord.x + v2->TexCoord.x) / 2.0f;
			m_VecVertices.push_back(newV1);
			m_VecIndices[(i + 1) % m_NumIndices] = countExtraVerts + m_NumVertices;
			countExtraVerts++;
		}
		else if (m_NorthIdx.find(*idx2) != m_NorthIdx.end() || m_SouthIdx.find(*idx2) != m_SouthIdx.end())
		{
			auto newV2 = *v0;
			newV2.TexCoord.x = (v0->TexCoord.x + v1->TexCoord.x) / 2.0f;
			m_VecVertices.push_back(newV2);
			m_VecIndices[(i + 2) % m_NumIndices] = countExtraVerts + m_NumVertices;
			countExtraVerts++;
		}
	}

	m_NumVertices = m_VecVertices.size();
	m_NumIndices = m_VecIndices.size();
}

//BUILD TANGENTS
//*******************************************************************************************************************************
void GenRock::BuildTangents()
{
	XMFLOAT3 tangent;
	for (UINT idx = 0; idx + 2 < m_VecIndices.size(); idx += 3)
	{
		int idx0 = m_VecIndices[idx];
		int idx1 = m_VecIndices[idx + 1];
		int idx2 = m_VecIndices[idx + 2];

		tangent = ComputeTangent(
			m_VecVertices[idx0].Position, m_VecVertices[idx1].Position, m_VecVertices[idx2].Position,
			m_VecVertices[idx0].TexCoord, m_VecVertices[idx1].TexCoord, m_VecVertices[idx2].TexCoord
		);

		m_VecVertices[idx0].Tangent = AddXMFLOAT3(m_VecVertices[idx0].Tangent, tangent);
		m_VecVertices[idx1].Tangent = AddXMFLOAT3(m_VecVertices[idx1].Tangent, tangent);
		m_VecVertices[idx2].Tangent = AddXMFLOAT3(m_VecVertices[idx2].Tangent, tangent);
	}
	for(UINT i = 0; i < m_VecVertices.size(); i++)
	{
		m_VecVertices[i].Tangent = NormalizeXMFLOAT3(m_VecVertices[i].Tangent);
	}
}

//ENGINE
//***************************************************************************************************
void GenRock::Initialize(GameContext* pContext)
{
	//Effect
	m_pEffect = ContentManager::Load<ID3DX11Effect>(L"Shaders/Rock.fx");
	//m_pEffect = ContentManager::Load<ID3DX11Effect>(L"Shaders/PosNormTex3D.fx");
	m_pTechnique = m_pEffect->GetTechniqueByIndex(0);
	m_pMatWorldViewProjVariable = m_pEffect->GetVariableByName("gMatrixWVP")->AsMatrix();
	m_pMatWorldVariable = m_pEffect->GetVariableByName("gMatrixWorld")->AsMatrix();
	m_pMatViewInvVariable = m_pEffect->GetVariableByName("gMatrixViewInverse")->AsMatrix();

	//Diffuse
	m_pDiffuseVariable = m_pEffect->GetVariableByName("gTextureDiffuse")->AsShaderResource();
	//m_pDiffuseVariable = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();
	m_pUseDiffuseVariable = m_pEffect->GetVariableByName("gUseTextureDiffuse")->AsScalar();
	m_pColorDiffuseVariable = m_pEffect->GetVariableByName("gColorDiffuse")->AsVector();

	//Specular
	m_pSpecularVariable = m_pEffect->GetVariableByName("gTextureSpecularIntensity")->AsShaderResource();
	m_pUseSpecularVariable = m_pEffect->GetVariableByName("gUseTextureSpecularIntensity")->AsScalar();
	m_pColorSpecularVariable = m_pEffect->GetVariableByName("gColorSpecular")->AsVector();
	m_pSpecularIntensityVariable = m_pEffect->GetVariableByName("gSpecIntensity")->AsScalar();
	m_pShininessVariable = m_pEffect->GetVariableByName("gShininess")->AsScalar();

	//Specular model
	m_pUseBlinnVariable = m_pEffect->GetVariableByName("gUseSpecularBlinn")->AsScalar();
	m_pUsePhongVariable = m_pEffect->GetVariableByName("gUseSpecularPhong")->AsScalar();

	//Normal
	m_pNormalVariable = m_pEffect->GetVariableByName("gTextureNormal")->AsShaderResource();
	m_pUseNormalVariable = m_pEffect->GetVariableByName("gUseTextureNormal")->AsScalar();
	m_pFlipGreenChannelVariable = m_pEffect->GetVariableByName("gFlipGreenChannel")->AsScalar();

	//Opacity
	m_pOpacityVariable = m_pEffect->GetVariableByName("gTextureOpacity")->AsShaderResource();
	m_pOpacityIntensityVariable = m_pEffect->GetVariableByName("gOpacityIntensity")->AsScalar();
	m_pUseOpacityVariable = m_pEffect->GetVariableByName("gUseTextureOpacity")->AsScalar();

	//Ambient
	m_pAmbientIntensityVariable = m_pEffect->GetVariableByName("gAmbientIntensity")->AsScalar();
	m_pColorAmbientVariable = m_pEffect->GetVariableByName("gColorAmbient")->AsVector();

	BuildInputLayout(pContext);
}

void GenRock::Update(GameContext* pContext)
{
	if (m_PostInitialize == true)
	{
		if (m_pVertexBuffer != nullptr)
			m_pVertexBuffer->Release();
		if (m_pIndexBuffer != nullptr)
			m_pIndexBuffer->Release();
		m_VecVertices.clear();
		m_VecIndices.clear();
		m_NumVertices = 0;
		m_NumIndices = 0;

		BuildIco();
		BuildRock();
		Expand();
		BuildNormals();
		CorrectUV();
		BuildTangents();

		BuildVertexBuffer(pContext);
		BuildIndexBuffer(pContext);
		m_PostInitialize = false;

		Debug::LogWarning(L"Rock intialized");
	}
}

void GenRock::Draw(GameContext* pContext)
{
	XMMATRIX world = XMLoadFloat4x4(&m_WorldMatrix);
	XMMATRIX viewProj = XMLoadFloat4x4(&pContext->GetCamera()->GetViewProjection());
	XMMATRIX wvp = XMMatrixMultiply(world, viewProj);
	XMMATRIX viewInv = XMLoadFloat4x4(&pContext->GetCamera()->GetViewInverse());

	m_pMatWorldVariable->SetMatrix(reinterpret_cast<float*>(&world));
	m_pMatWorldViewProjVariable->SetMatrix(reinterpret_cast<float*>(&wvp));
	m_pMatViewInvVariable->SetMatrix(reinterpret_cast<float*>(&viewInv));

	//DIFFUSE
	//-----------------------------------------------------------------------------------------
	if (m_pDiffuseVariable)
	{
		m_pDiffuseVariable->SetResource(m_pDiffuseData->GetShaderResourceView());
		if (m_pDiffuseVariable->IsValid() == false)
		{
			Debug::LogError(L"m_pDiffuseVariable invalid");
		}
	}

	if (m_pUseDiffuseVariable)
	{
		m_pUseDiffuseVariable->SetRawValue(&m_pUseDiffuse, 0, sizeof(m_pUseDiffuse));
		if (m_pUseDiffuseVariable->IsValid() == false)
		{
			Debug::LogError(L"m_pUseDiffuseVariable invalid");
		}
	}

	if (m_pColorDiffuseVariable)
	{
		m_pColorDiffuseVariable->SetFloatVector(reinterpret_cast<float*>(&m_pColorDiffuse));
		if (m_pColorDiffuseVariable->IsValid() == false)
		{
			Debug::LogError(L"m_pColorDiffuseVariable invalid");
		}
	}


	//SPECULAR
	//-----------------------------------------------------------------------------------------
	if (m_pSpecularVariable)
	{
		m_pSpecularVariable->SetResource(m_pSpecularData->GetShaderResourceView());
		if (m_pSpecularVariable->IsValid() == false)
		{
			Debug::LogError(L"m_pSpecularVariable invalid");
		}
	}

	if (m_pUseSpecularVariable)
	{
		m_pUseSpecularVariable->SetRawValue(&m_pUseSpecular, 0, sizeof(m_pUseOpacity));
		if (m_pUseSpecularVariable->IsValid() == false)
		{
			Debug::LogError(L"m_pUseSpecularVariable invalid");
		}
	}

	if (m_pColorSpecularVariable)
	{
		m_pColorSpecularVariable->SetFloatVector(reinterpret_cast<float*>(&m_pColorSpecular));
		if (m_pColorSpecularVariable->IsValid() == false)
		{
			Debug::LogError(L"m_pColorSpecularVariable invalid");
		}
	}

	if (m_pSpecularIntensityVariable)
	{
		m_pSpecularIntensityVariable->SetFloat(m_pSpecIntensity);
		if (m_pSpecularIntensityVariable->IsValid() == false)
		{
			Debug::LogError(L"m_pSpecularIntensityVariable invalid");
		}
	}

	if (m_pShininessVariable)
	{
		m_pShininessVariable->SetFloat(m_pShininess);
		if (m_pShininessVariable->IsValid() == false)
		{
			Debug::LogError(L"m_pShininessVariable invalid");
		}
	}


	//MODEL
	//-----------------------------------------------------------------------------------------
	if (m_pUseBlinnVariable)
	{
		m_pUseBlinnVariable->SetRawValue(&m_pUseBlinn, 0, sizeof(m_pUseBlinn));
		if (m_pUseBlinnVariable->IsValid() == false)
		{
			Debug::LogError(L"m_pUseBlinnVariable invalid");
		}
	}

	if (m_pUsePhongVariable)
	{
		m_pUsePhongVariable->SetRawValue(&m_pUsePhong, 0, sizeof(m_pUsePhong));
		if (m_pUsePhongVariable->IsValid() == false)
		{
			Debug::LogError(L"m_pUsePhongVariable invalid");
		}
	}


	//NORMAL
	//-----------------------------------------------------------------------------------------
	if (m_pNormalVariable)
	{
		m_pNormalVariable->SetResource(m_pNormalData->GetShaderResourceView());
		if (m_pNormalVariable->IsValid() == false)
		{
			Debug::LogError(L"m_pNormalVariable invalid");
		}
	}

	if (m_pUseNormalVariable)
	{
		m_pUseNormalVariable->SetRawValue(&m_pUseNormal, 0, sizeof(m_pUseNormal));
		if (m_pUseNormalVariable->IsValid() == false)
		{
			Debug::LogError(L"m_pUseNormalVariable invalid");
		}
	}

	if (m_pFlipGreenChannelVariable)
	{
		m_pFlipGreenChannelVariable->SetRawValue(&m_pFlipGreenChannel, 0, sizeof(m_pFlipGreenChannel));
		if (m_pFlipGreenChannelVariable->IsValid() == false)
		{
			Debug::LogError(L"m_pFlipGreenChannelVariable invalid");
		}
	}


	//OPACITY
	//-----------------------------------------------------------------------------------------
	if (m_pOpacityVariable)
	{
		m_pOpacityVariable->SetResource(m_pOpacityData->GetShaderResourceView());
		if (m_pOpacityVariable->IsValid() == false)
		{
			Debug::LogError(L"m_pOpacityVariable invalid");
		}
	}

	if (m_pOpacityIntensityVariable)
	{
		m_pOpacityIntensityVariable->SetFloat(m_pOpacityIntensity);
		if (m_pOpacityIntensityVariable->IsValid() == false)
		{
			Debug::LogError(L"m_pOpacityIntensityVariable invalid");
		}
	}

	if (m_pUseOpacityVariable)
	{
		m_pUseOpacityVariable->SetRawValue(&m_pUseOpacity, 0, sizeof(m_pUseOpacity));
		if (m_pUseOpacityVariable->IsValid() == false)
		{
			Debug::LogError(L"m_pUseOpacityVariable invalid");
		}
	}


	//AMBIENT
	//-----------------------------------------------------------------------------------------
	if (m_pAmbientIntensityVariable)
	{
		m_pAmbientIntensityVariable->SetFloat(m_pAmbientIntensity);
		if (m_pAmbientIntensityVariable->IsValid() == false)
		{
			Debug::LogError(L"m_pAmbientIntensityVariable invalid");
		}
	}

	if (m_pColorAmbientVariable)
	{
		m_pColorAmbientVariable->SetFloatVector(reinterpret_cast<float*>(&m_pColorAmbient));
		if (m_pColorAmbientVariable->IsValid() == false)
		{
			Debug::LogError(L"m_pColorAmbientVariable invalid");
		}
	}


	// Set vertex buffer
	UINT stride = sizeof(VertexRock);
	UINT offset = 0;
	auto deviceContext = pContext->GetDeviceContext();
	deviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

	// Set index buffer
	deviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the input layout
	deviceContext->IASetInputLayout(m_pVertexLayout);

	// Set primitive topology
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Render a torus
	D3DX11_TECHNIQUE_DESC techDesc;
	m_pTechnique->GetDesc(&techDesc);
	for (UINT p = 0; p< techDesc.Passes; ++p)
	{
		m_pTechnique->GetPassByIndex(p)->Apply(0, deviceContext);
		deviceContext->DrawIndexed(m_NumIndices, 0, 0);
	}
}

void GenRock::BuildInputLayout(GameContext* pContext)
{
	//InputLayout
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numElements = sizeof(vertexDesc) / sizeof(vertexDesc[0]);

	// Create the input layout
	D3DX11_PASS_DESC passDesc;
	m_pTechnique->GetPassByIndex(0)->GetDesc(&passDesc);
	auto hr = pContext->GetDevice()->CreateInputLayout(
		vertexDesc,
		numElements,
		passDesc.pIAInputSignature,
		passDesc.IAInputSignatureSize,
		&m_pVertexLayout);
	Debug::LogHResult(hr, L"Failed to Create InputLayout");
}

void GenRock::BuildVertexBuffer(GameContext* pContext)
{
	//Vertexbuffer
	D3D11_BUFFER_DESC bd = {};
	D3D11_SUBRESOURCE_DATA initData = { 0 };
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(VertexRock) * m_NumVertices;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	initData.pSysMem = m_VecVertices.data();
	HRESULT hr = pContext->GetDevice()->CreateBuffer(&bd, &initData, &m_pVertexBuffer);
	Debug::LogHResult(hr, L"Failed to Create Vertexbuffer");
}

void GenRock::BuildIndexBuffer(GameContext* pContext)
{
	D3D11_BUFFER_DESC bd = {};
	D3D11_SUBRESOURCE_DATA initData = { 0 };
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(DWORD) * m_NumIndices;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	initData.pSysMem = m_VecIndices.data();
	HRESULT hr = pContext->GetDevice()->CreateBuffer(&bd, &initData, &m_pIndexBuffer);
	Debug::LogHResult(hr, L"Failed to Create Indexbuffer");
	m_NumIndices = m_VecIndices.size();
}

void GenRock::SetDiffuse(wstring diffuseFile, bool use, XMFLOAT4 color)
{
	m_pUseDiffuse = use;
	m_pColorDiffuse = color;
	m_pDiffuseData = ContentManager::Load<DdsTextureResource>(diffuseFile);
}
void GenRock::SetSpecular(wstring specularFile, bool use, XMFLOAT4 color, float intensity, float shininess)
{
	m_pUseSpecular = use;
	m_pColorSpecular = color;
	m_pSpecIntensity = intensity;
	m_pShininess = shininess;
	m_pSpecularData = ContentManager::Load<DdsTextureResource>(specularFile);
}
void GenRock::UsePhong(bool use)
{
	m_pUseBlinn = !use;
	m_pUsePhong = use;
}
void GenRock::SetNormal(wstring opacityFile, bool use, bool flip)
{
	m_pUseNormal = use;
	m_pFlipGreenChannel = flip;
	m_pNormalData = ContentManager::Load<DdsTextureResource>(opacityFile);
}
void GenRock::SetOpacity(wstring normalFile, bool use, float intensity)
{
	m_pUseOpacity = use;
	m_pOpacityIntensity = intensity;
	m_pOpacityData = ContentManager::Load<DdsTextureResource>(normalFile);
}
void GenRock::SetAmbient(float intensity, XMFLOAT4 color)
{
	m_pAmbientIntensity = intensity;
	m_pColorAmbient = color;
}

//bool GenRock::SameSide(XMVECTOR p1, XMVECTOR p2, XMVECTOR a, XMVECTOR b)
//{	
//	auto cp1 = XMVector3Cross(b - a, p1 - a);
//	auto cp2 = XMVector3Cross(b - a, p2 - a);
//	auto dotV = XMVector3Dot(cp1, cp2);
//	float dot;
//	XMStoreFloat(&dot, dotV);
//	if (dot >= 0)
//		return true;
//	return false;
//}
