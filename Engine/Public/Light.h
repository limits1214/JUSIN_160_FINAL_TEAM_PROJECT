#pragma once
#include "Engine_Defines.h"
#include "ResCBuffer.h"
#include "GameObject.h"
#include "ComConstantBuffer.h"
#include "ResVertexShader.h"
#include "ResPixelShader.h"
#include "ResQuadTexBuffer.h"
#include "ResTexture2D.h"
#include "ResDynamicTexture2D.h"
#include "ResSamplerState.h"
#include "ComCollider.h"

NS_BEGIN(Engine)

class ENGINE_DLL CLight final : public CGameObject {
private:
	CLight();
	~CLight() override;
public:
	typedef struct tagLightDesc : public CGameObject::GAMEOBJECT_DESC
	{

	}DESC;
public:
	DECLARE_DERIVED_TYPE(CLight, CGameObject)

public:
	HRESULT			InitializePrototype(VOID* pArg) override;
	HRESULT			Initialize(VOID* pArg) override;
	VOID			PriorityUpdate(E::_float _DT) override {};
	VOID			UpdateGUI() override;
	VOID			Update(E::_float _DT) override;
	VOID			LateUpdate(E::_float _DT) override {};
	HRESULT			Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) { return S_OK; };

	VOID			Update_ObjectConstantBuffer(ID3D11DeviceContext* pContext);
	VOID			Update_EffectLight(const _float& _DT);

	VOID			PrepareShadowMapMatrices();
	VOID			Update_PointShadowMatrices();
	VOID			Update_SpotShadowMatrices();
	VOID			Update_DirectionalShadowMatrices();

	std::vector<XMVECTOR>	Get_FrustumCorners(_float _SplitNear, _float _SplitFar);
	XMFLOAT4		Get_CascadeShadowSplits() const { return m_fCascadeShadowSplits; }

public:
	HRESULT 		Set_LightType(LIGHT_TYPE _LTYPE);
	LIGHT_TYPE		Get_LightType() { return static_cast<LIGHT_TYPE>(m_pDynamicLight.LightType); }

	// LSY 변경: 에디터에서 방향을 수정하면 그림자/컬링 데이터가 다시 계산되도록 Dirty 처리한다.
	VOID			Set_LightDirection(XMFLOAT3 _Direction);
	XMFLOAT3		Get_LightDirection()					{ return m_pDynamicLight.LightDirection; }

	VOID			Set_LightColor(XMFLOAT3 _Color) { m_pDynamicLight.LightColor = _Color;}
	XMFLOAT3		Get_LightColor() { return m_pDynamicLight.LightColor; }

	VOID			Set_LightIntensity(_float _Intensity) { m_pDynamicLight.LightIntensity = _Intensity; }
	_float			Get_LightIntensity() { return m_pDynamicLight.LightIntensity; }

	VOID			Set_LightRange(_float _Range);
	_float			Get_LightRange()				{ return m_pDynamicLight.LightRange; }

	VOID			Set_LightPosition(XMFLOAT3 _Position);
	VOID			Set_LightPosition(XMVECTOR _Position);
	XMFLOAT3		Get_LightPosition() { return m_pComTransform->GetPosition(); }

	// LSY 변경: Spot Light 감쇠각의 런타임 편집 결과를 즉시 반영하기 위해 Dirty 처리한다.
	VOID			Set_LightInnerAttenuation(_float _Attenuation) { m_pDynamicLight.InnerAttanuation = _Attenuation; }
	_float			Get_LightInnerAttenuation() { return m_pDynamicLight.InnerAttanuation; }

	VOID			Set_LightOuterAttenuation(_float _Attenuation);
	_float			Get_LightOuterAttenuation() { return m_pDynamicLight.OuterAttanuation; }

	VOID			Set_LightActivateState(_bool _State);
	_bool			Get_LightActivateState()				{ return m_bActivate_State;	}

	VOID			Set_LightShadowCast(_bool _State);
	_bool			Get_LightShadowCast() { return m_bCastShadow; }

	// LSY 변경: 에디터 표시용 별칭과 레벨별 저장/삭제 범위를 구분하는 배치 그룹 정보다.
	VOID			Set_LightAlias(std::string sAlias) { m_sAlias = std::move(sAlias); }
	const std::string& Get_LightAlias() const { return m_sAlias; }

	VOID			Set_LightPlacementGroup(std::string sGroup) { m_sPlacementGroup = std::move(sGroup); }
	const std::string& Get_LightPlacementGroup() const { return m_sPlacementGroup; }
	_bool			Is_PlacementLight() const { return !m_sPlacementGroup.empty(); }

	VOID			Set_LightViewProj(uint32_t _Index, XMMATRIX _LightViewProj) { XMStoreFloat4x4(&m_pDynamicLight.g_LightViewProj[_Index], _LightViewProj);	}
	XMFLOAT4X4		Get_LightViewProj(uint32_t _Index)							{ return m_pDynamicLight.g_LightViewProj[_Index];								}
	XMMATRIX		Get_LoadedLightViewProj(uint32_t _Index)					{ return XMLoadFloat4x4(&m_pDynamicLight.g_LightViewProj[_Index]);				}
	
	VOID			Set_LightLifeTime(_float _LifeTime)		{ m_fLifeTime = _LifeTime;	}
	_float			Get_LightLifeTime()						{ return m_fLifeTime;		}

	VOID			Set_LightVelocity(XMFLOAT3 _Velocity)	{ m_fVelocity = _Velocity;	}
	XMFLOAT3		Get_LightVelocity()						{ return m_fVelocity;		}

	VOID			Set_PointLightOuterAttenuation(_float _Attenuation);
	_float			Get_PointLightOuterAttenuation()					{ return m_fPointLightOuterAttenuation;			}

	VOID			Set_PointLightInnerAttenuation(_float _Attenuation) { m_fPointLightInnerAttenuation = _Attenuation; }
	_float			Get_PointLightInnerAttenuation()					{ return m_fPointLightInnerAttenuation;			}

	const SPtr<CCollider>& Get_SphereCollider()		{ return m_pColliderSphere; }
	const SPtr<CCollider>& Get_FrustumCollider()	{ return m_pColliderFrustum; }

	_bool			Is_StaticDirty()				{ return m_bStaticShadowDirty;  }
	VOID			Set_StaticDirty(_bool _Flag)	{ m_bStaticShadowDirty = _Flag; }

	_bool			Is_DynamicDirty()				{ return m_bDynamicShadowDirty; }
	VOID			Set_DynamicDirty(_bool _Flag)	{ m_bDynamicShadowDirty = _Flag; }

	_bool			Had_DynamicShadowCaster() const { return m_bHadDynamicShadowCaster; }
	VOID			Set_HadDynamicShadowCaster(_bool _Flag) { m_bHadDynamicShadowCaster = _Flag; }

	_bool			Is_EffectLight()				{ return m_bEffectLightFlag; }
	VOID			Set_EffectLight(_bool _Flag = true)	{ m_bEffectLightFlag = _Flag; }

	VOID			Set_ShadowSlotNumb(int32_t _Numb)	{ m_ShadowSlot = _Numb; }
	int32_t			Get_ShadowSlotNumb()				{ return m_ShadowSlot;  }

	VOID			Set_VolumetricScattering(_bool bEnable)		{ m_bVolumetricScattering = bEnable; }
	_bool			Get_VolumetricScattering() const			{ return m_bVolumetricScattering;	 }

	VOID			Set_VolumetricIntensity(_float fIntensity)	{ m_fVolumetricIntensity = fIntensity; }
	_float			Get_VolumetricIntensity() const				{ return m_bVolumetricScattering ? m_fVolumetricIntensity : 0.f; }

	VOID			InvalidateAllShadow()
	{
		m_bShadowMatrixDirty = true;

		m_bCullBoundsDirty = true;

		m_bStaticShadowDirty = true;
		m_bDynamicShadowDirty = true;
	}

private:
	DYNAMIC_LIGHT						m_pDynamicLight{};

	CComConstantBuffer*					m_pComCBufferPerObject{	};
	CComConstantBuffer*					m_pComCBufferPerPass  {	};

	SPtr<CCollider>						m_pColliderSphere{};
	SPtr<CCollider>						m_pColliderFrustum{};

	_bool								m_bShadowMatrixDirty  = { true };		// Shadow의 View 행렬 변경 플래그
	_bool								m_bCullBoundsDirty	  = { true };		// Light의 Collider 위치변경 플래그
	_bool								m_bStaticShadowDirty  = { true };		// Static쉐도우맵 변화 플래그
	_bool								m_bDynamicShadowDirty = { true };		// Dynamic쉐도우맵 변화 플래그

	_bool								m_bActivate_State			= { true };
	_bool								m_bCastShadow				= { true };
	_bool								m_bHadDynamicShadowCaster	= { false };
	_bool								m_bVolumetricScattering		= { true };

	_bool								m_bEffectLightFlag			= { false };
	// LSY 변경: 별칭은 식별 편의용이며, 배치 그룹은 런타임 로더 단위 정리에 사용한다.
	std::string							m_sAlias{};
	std::string							m_sPlacementGroup{};

	_float								m_fLifeTime = { 0.f };
	XMFLOAT3							m_fVelocity = { 0.f, 0.f, 0.f };

	_float								m_fPointLightInnerAttenuation{};
	_float								m_fPointLightOuterAttenuation{};

	_float								m_fVolumetricIntensity{ 10.f };

	XMFLOAT4 m_fCascadeShadowSplits{};

	XMFLOAT4X4 LightView{}, LightProj{};

private:	// PointLight
	XMVECTOR	DirectionVec[6], BaseUpVec[6];
	int32_t		m_ShadowSlot{-1};



public:


	HRESULT		Update_Collider();

	_bool	Intersects_ShadowBounds(const BoundingBox& _Bounds) const;

	HRESULT		Capture_ShadowMap(ID3D11DeviceContext* pContext, E::RENDER_CTX& ctx, const std::vector<IRenderable*>& _ObjectLis, int32_t _PointFaceIndex = -1);
	VOID		Reset_Light();

	_bool		Intersects_PointShadowFace(uint32_t _FaceIndex, const BoundingBox& _Bounds) const;

private:
	std::array<BoundingFrustum, POINT_SHADOW_MAPCOUNT> m_PointShadowFrustums{};


public:
	static UPtr<CLight> Create();
	UPtr<CPrototype>	Clone(void* pArg) override;
};
NS_END

