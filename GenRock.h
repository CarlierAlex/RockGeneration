#pragma once
#include "GameObject.h"
#include "VertexStructs.h"
#include "RockHeader.h"

class DdsTextureResource;
class GenRock : public GameObject
{
public:
	GenRock(float width, float height, float depth, int steps);
	~GenRock(void);

	struct Plane
	{
		XMFLOAT3 origin;
		XMFLOAT3 normal;
		int size;
	};

	//Rockgen
	void Reset() { m_PostInitialize = true; }

	void SetRadiusWidth(float width) { m_Width = width; }
	void SetRadiusDepth(float depth) { m_Depth = depth; }
	void SetRadiusHeight(float height) { m_Height = height; }

	void SetRandAngleMax(float angle) { m_MaxRandAngle = angle; }
	void SetRandAngleMin(float angle) { m_MinRandAngle = angle; }
	void SetRandOffsetPercent(float percent) { m_MaxOffsetPercent = percent; }
	void SetRandShift(float shift) { m_MaxRandShift = shift; }

	void SetMaxPlaneVerts(UINT verts) { m_MaxPlaneVerts = verts; }
	void SetMinPlaneVerts(UINT verts) { m_MinPlaneVerts = verts; }
	void SetMaxPlanes(UINT planes) { m_MaxPlanes = planes; }

	void SetSteps(UINT steps) { m_Steps = steps; }

	//Shader
	void SetDiffuse(wstring diffuseFile, bool use, XMFLOAT4 color);
	void SetSpecular(wstring specularFile, bool use, XMFLOAT4 color, float intensity, float shininess);
	void UsePhong(bool use);
	void SetNormal(wstring opacityFile, bool use, bool flip);
	void SetOpacity(wstring normalFile, bool use, float intensity);
	void SetAmbient(float intensity, XMFLOAT4 color);

protected:

	virtual void Initialize(GameContext* pContext);
	virtual void Update(GameContext* pContext);
	virtual void Draw(GameContext* pContext);

private:
	void BuildInputLayout(GameContext* pContext);
	void BuildVertexBuffer(GameContext* pContext);
	void BuildIndexBuffer(GameContext* pContext);
	void BuildIco();

	void CorrectUV();

	void BuildRock();
	void Expand();
	void BuildNormals();
	void BuildTangents();


	//bool SameSide(XMVECTOR p1, XMVECTOR p2, XMVECTOR a, XMVECTOR b);

	bool m_PostInitialize = true;
	float m_Width, m_Height, m_Depth;
	float m_MinRandAngle, m_MaxRandAngle, m_MaxOffsetPercent, m_MaxRandShift;
	XMFLOAT2 m_PrevAngles = XMFLOAT2(0,0);
	UINT m_MaxPlaneVerts, m_MinPlaneVerts, m_MaxPlanes;
	UINT m_Steps;

	std::set<UINT> m_NorthIdx, m_SouthIdx;
	float m_MaxLineLength = 0;
	float m_MinLineLength = 9999999;

	std::vector<VertexRock> m_VecVertices;
	std::vector<DWORD> m_VecIndices;
	UINT m_NumVertices, m_NumIndices;

	//SHADER
	/******/
	ID3D11InputLayout*      m_pVertexLayout;
	ID3D11Buffer*           m_pVertexBuffer;
	ID3D11Buffer*           m_pIndexBuffer;
	ID3DX11Effect			*m_pEffect;
	ID3DX11EffectTechnique	*m_pTechnique;
	ID3DX11EffectMatrixVariable *m_pMatWorldViewProjVariable;
	ID3DX11EffectMatrixVariable *m_pMatWorldVariable;
	ID3DX11EffectMatrixVariable *m_pMatViewInvVariable;

	//DIFFUSE
	/*******/
	ID3DX11EffectShaderResourceVariable *m_pDiffuseVariable = nullptr;//TEXTURE
	DdsTextureResource* m_pDiffuseData;

	ID3DX11EffectScalarVariable* m_pUseDiffuseVariable = nullptr;//useTEXTURE
	bool m_pUseDiffuse = false;

	ID3DX11EffectVectorVariable* m_pColorDiffuseVariable = nullptr;//COLOR
	XMFLOAT4 m_pColorDiffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);


	//SPECULAR
	/********/
	ID3DX11EffectShaderResourceVariable *m_pSpecularVariable = nullptr;//TEXTURE
	DdsTextureResource* m_pSpecularData;

	ID3DX11EffectScalarVariable* m_pUseSpecularVariable = nullptr;//useTEXTURE
	bool m_pUseSpecular = false;

	ID3DX11EffectVectorVariable* m_pColorSpecularVariable = nullptr;//COLOR
	XMFLOAT4 m_pColorSpecular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	ID3DX11EffectScalarVariable* m_pSpecularIntensityVariable = nullptr;//INTENSITY
	float m_pSpecIntensity = 1.0f;

	ID3DX11EffectScalarVariable* m_pShininessVariable = nullptr;//SHININESS
	float m_pShininess = 1.0f;

	//MODEL
	/*****/
	ID3DX11EffectScalarVariable* m_pUseBlinnVariable = nullptr;//useBLINN
	bool m_pUseBlinn = false;

	ID3DX11EffectScalarVariable* m_pUsePhongVariable = nullptr;//usePHONG
	bool m_pUsePhong = false;

	//NORMAL
	/******/
	ID3DX11EffectShaderResourceVariable *m_pNormalVariable = nullptr;//TEXTURE
	DdsTextureResource* m_pNormalData;

	ID3DX11EffectScalarVariable* m_pFlipGreenChannelVariable = nullptr;//FLIP
	bool m_pFlipGreenChannel = false;

	ID3DX11EffectScalarVariable* m_pUseNormalVariable = nullptr;//useTEXTURE
	bool m_pUseNormal = false;

	//OPACITY
	/*******/
	ID3DX11EffectShaderResourceVariable *m_pOpacityVariable = nullptr;//TEXTURE
	DdsTextureResource* m_pOpacityData;

	ID3DX11EffectScalarVariable* m_pOpacityIntensityVariable = nullptr;//INTENSITY
	float m_pOpacityIntensity = 1.0f;

	ID3DX11EffectScalarVariable* m_pUseOpacityVariable = nullptr;//useTEXTURE
	bool m_pUseOpacity = false;

	//AMBIENT
	/*******/
	ID3DX11EffectVectorVariable* m_pColorAmbientVariable = nullptr;//COLOR
	XMFLOAT4 m_pColorAmbient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	ID3DX11EffectScalarVariable* m_pAmbientIntensityVariable;//INTENSITY
	float m_pAmbientIntensity = 1.0f;

private:

	// -------------------------
	// Disabling default copy constructor and default 
	// assignment operator.
	// -------------------------
	GenRock(const GenRock& yRef);
	GenRock& operator=(const GenRock& yRef);
};
