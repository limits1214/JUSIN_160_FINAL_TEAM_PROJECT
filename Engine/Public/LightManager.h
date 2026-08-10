#pragma once
#include "Engine_Defines.h"
#include "Light.h"
#include "ResComputeShader.h"
#include "ResGeometryShader.h"
#include "ResViewPort.h"

NS_BEGIN(Engine)

class CLightPlacementEditor;

struct LightData {
	std::optional<CHandle> LightHandle;
	_float DistanceSQ;
};

struct SHADOW_ARRAY_2D {
	ComPtr<ID3D11Texture2D>			 TexBuffer{};
	ComPtr<ID3D11ShaderResourceView> SRV{};
	ComPtr<ID3D11DepthStencilView>	 DSVList[MAX_SHADOW_LIGHT_RENDER_COUNT];
};

struct SHADOW_ARRAY_CUBE {
	ComPtr<ID3D11Texture2D>			 TexBuffer{};
	ComPtr<ID3D11ShaderResourceView> SRV{};
	ComPtr<ID3D11DepthStencilView>	 DSVList[MAX_SHADOW_LIGHT_RENDER_COUNT];

	ComPtr<ID3D11DepthStencilView>	 FaceDSVList[MAX_SHADOW_LIGHT_RENDER_COUNT][POINT_SHADOW_MAPCOUNT];
};

class ENGINE_DLL CLightManager final : public CEngineBase {
private:
	CLightManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CLightManager() override;

public:
	HRESULT	Initialize_LightManager();

	HRESULT Initialize_PBRResources();
	HRESULT Initialize_ShadowResources();
	HRESULT Initialize_ShadowMapResources();

	VOID	Update(_float fTimeDelta);
	VOID	UpdateGUI();

	HRESULT	Capture_ShadowMap();

	HRESULT	Render_ObjectShadow();
	HRESULT	Render_ObjectNonShadow();

	std::optional<CHandle> Add_DirectionalLight(XMFLOAT3 _Direction, XMFLOAT3 _Color, _float _Intensity);
	std::optional<CHandle> Add_PointLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _InnerRange, _float _OuterRange);
	std::optional<CHandle> Add_SpotLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range, _float _InnerAtt, _float _OuterAtt);
	
	_bool	Evaluate_DirectionalLightCount();

	// LSY 변경: 배치 라이트를 핸들/레벨 그룹 단위로 관리하기 위한 API다.
	_bool	Remove_Light(const CHandle& hLight);
	size_t	Remove_PlacementLightGroup(std::string_view sGroup);
	void	SetActivePlacementLightGroup(std::string_view sGroup);
	// LSY 변경: 콘텐츠 코드가 레벨 그룹과 별칭으로 배치 라이트의 안전한 핸들을 찾는다.
	std::optional<CHandle> FindPlacementLightHandleByAlias(std::string_view sGroup,	std::string_view sAlias) const;
	const std::vector<std::optional<CHandle>>& GetLightHandles() const { return m_LightHandleList; }



	VOID	Clear_DynamicLightList();

	HRESULT	AddShadowRenderGroup(ACTORTYPE _ATYPE, IRenderable* pRenderObject);

	const	SPtr<CResDynamicTexture2D>& Get_CombinedResource()	{ return m_pUAVComBinedOutput; }

	VOID	Bind_ShadowResource();
	VOID	UnBind_ShadowResource();

	HRESULT Render_ShadowInstanced(const ComPtr<ID3D11DeviceContext>& pContext, std::optional<CHandle> _LightHandle, _bool _bStaticBatch, int32_t _PointFaceIndex = -1);

	VOID	Update_ActiveLights();
	VOID	Update_LightData();
	VOID	Allocate_ShadowSlot();

	VOID	Invalidate_DynamicShadowMaps();

	VOID	Build_StaticShadowCasterList(std::optional<CHandle> _LightHandle);	
	VOID	Notify_StaticShadowSceneChanged(const BoundingBox& ChangedBounds);

	VOID	Bind_VolumetricLocalLightResources();
	VOID	UnBind_VolumetricLocalLightResources();

	XMMATRIX	Get_CascadeShadowViewProj(uint32_t _Index);
	XMFLOAT4	Get_CascadeShadowSplits();
	CSM_DATA&	Get_MainDirectionalLightData() { return m_pMainDirectionalLight; }

public:		// Effect Light Fuction
	HRESULT	Initialize_EffectLight(uint32_t _PoolSize);
	std::optional<CHandle> Allocate_EffectLight(XMVECTOR _WorldPos, _float _Intensity, _float3 _Color, _float _InnerRange, _float _OuterRange, _float _LifeTime, _float3 _Velocity);

	_bool	IsActiveShadowLight(std::optional<CHandle>& _Handle);

	HRESULT Reset_EffectLight(const std::optional<CHandle>& _Handle);

	HRESULT Transform_EffectLight(const std::optional<CHandle>& _Handle, XMFLOAT3 _Position);
	HRESULT Transform_EffectLight(const std::optional<CHandle>& _Handle, XMVECTOR _Position);
	// LSY 변경: 이펙트 라이트 풀의 영향 범위를 DbgLineRender로 확인한다.
	void SetEffectLightDebugOptions(
		_bool bVisible,
		_bool bDepthTest)
	{
		m_bEffectLightDebugVisible = bVisible;
		m_bEffectLightDebugDepthTest = bDepthTest;
	}

private:	// Effect Light Variable
	VOID	Update_EffectLightData();

	void Clear_EffectLightPool();
	void DrawDebugEffectLights();

	std::vector<std::optional<CHandle>>		m_pEffectLightPool{};
	uint32_t								m_iEffectLightPoolSize{};
	uint32_t								m_iLastAllocatedIndex{};
	_bool									m_bEffectLightDebugVisible{};
	_bool									m_bEffectLightDebugDepthTest{};

private:
	HRESULT	Generate_ShadowArray2D(SHADOW_ARRAY_2D& _SHAR, uint32_t _ResolutionX, uint32_t _ResolutionY);
	HRESULT	Generate_ShadowArrayCube(SHADOW_ARRAY_CUBE& _SHAR, uint32_t _ResolutionX, uint32_t _ResolutionY);
	HRESULT	Generate_CSMShadowMap(CSM_DATA& _CDATA, uint32_t _ResolutionX, uint32_t _ResolutionY, uint32_t _CascadeCount);

	_bool	IsInFrustum(CLight* _LightOBJ);

	HRESULT Copy_StaticShadowToFinal(LIGHT_TYPE _LightType, uint32_t _ShadowSlot);


private:



	SPtr<CResDynamicTexture2D>			m_pUAVComBinedOutput = { nullptr };


	std::vector<IRenderable*>			m_pRenderable_StaticObjectList{};
	std::vector<IRenderable*>			m_pRenderable_DynamicObjectList{};





	CB_LIGHT							m_pLightConstantVariable{};
	CB_SHADOW							m_pShadowConstantVariable{};




	std::vector<IRenderable*>			m_pStaticShadowCasterScratch{};

private:	// PBR
	SPtr<CResComputeShader>				m_pNormalShadowPBRComputeShader		= { nullptr };
	SPtr<CResComputeShader>				m_pNonShadowPBRComputeShader		= { nullptr };
	SPtr<CResComputeShader>				m_pAlphaShadowPBRComputeShader		= { nullptr };
	
	SPtr<CResCBuffer>					m_pNormalLightConstantBuffer		= { nullptr };
	SPtr<CResCBuffer>					m_pShadowLightConstantBuffer		= { nullptr };
	SPtr<CResCBuffer>					m_pEffectLightConstantBuffer		= { nullptr };
	SPtr<CResCBuffer>					m_pPBRCSMConstantBuffer				= { nullptr };

private:	// PointLight Face
	SPtr<CResVertexShader>				m_pInstancedPointFaceVertexShader	= { nullptr };
	SPtr<CResVertexShader>				m_pNormalPointFaceVertexShader		= { nullptr };
	SPtr<CResPixelShader>				m_pNormalPointFacePixelShader		= { nullptr };

private:	// SpotLight Shader
	SPtr<CResVertexShader>				m_pNormalDirectionalVertexShader	= { nullptr };
	SPtr<CResPixelShader>				m_pNormalDirectionalPixelShader		= { nullptr };

private:	// Instanceing Shader
	SPtr<CResVertexShader>				m_pInstancedDirectionalVertexShader = { nullptr };

private:	// Light All (Active + DeActive), Main Directional Light
	std::vector<std::optional<CHandle>>	m_LightHandleList{};
	CSM_DATA							m_pMainDirectionalLight{};

private:	// Active Light Container(Normal, Normal + Shadow)
	std::vector<std::optional<CHandle>>	m_pActiveLightList{};
	std::vector<std::optional<CHandle>>	m_pActiveShadowLightList{};

private:	// Shadow Map List
	SHADOW_ARRAY_2D						m_pStaticDirectionalShadowList{};
	SHADOW_ARRAY_2D						m_pDynamicDirectionalShadowList{};

	SHADOW_ARRAY_CUBE					m_pStaticPointShadowList{};
	SHADOW_ARRAY_CUBE					m_pDynamicPointShadowList{};
	
private:	// Shadow Viewport
	SPtr<CResViewPort>					m_pSpotShadowViewPort{};
	SPtr<CResViewPort>					m_pPointShadowViewPort{};
	SPtr<CResViewPort>					m_pCSMShadowViewPort{};

private:	
	ComPtr<ID3D11Device>				m_pDevice			= { nullptr };
	ComPtr<ID3D11DeviceContext>			m_pContext			= { nullptr };

	UPtr<CLightPlacementEditor>			m_pPlacementEditor	= { nullptr };

private:	// Render 
	std::array<std::optional<CHandle>, MAX_SHADOW_LIGHT_RENDER_COUNT>	m_PointShadowSlotOwners{};
	std::array<std::optional<CHandle>, MAX_SHADOW_LIGHT_RENDER_COUNT>	m_2DShadowSlotOwners{};

public:
	static UPtr<CLightManager> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
};
NS_END


