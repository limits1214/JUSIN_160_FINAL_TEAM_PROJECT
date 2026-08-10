#include "pch.h"
#include "Light.h"
#include "GameInstance.h"
#include "Engine_Base.h"
#include "Collider.h"
#include "CollSphere.h"
#include "CollFrustum.h"

CLight::CLight() : CGameObject{} {}
CLight::~CLight() {}

VOID	CLight::UpdateGUI() {
	CGameObject::UpdateGUI();
}

HRESULT CLight::InitializePrototype(VOID* pArg) {
	{
		m_pDynamicLight.LightDirection = { 1.f, -1.f, 1.f };
		m_pDynamicLight.LightIntensity = { 10.f };
		m_pDynamicLight.LightColor = { 1.f, 1.f, 1.f };
		m_pDynamicLight.LightRange = { 100.f };

		m_pDynamicLight.Position = { 0.f, 0.f, 0.f };
		m_pDynamicLight.LightType = ETOUI(LIGHT_TYPE::DIRECTIONAL);

		m_pDynamicLight.InnerAttanuation = { 20.f };
		m_pDynamicLight.OuterAttanuation = { 30.f };

		for (uint32_t i = 0; i < POINT_SHADOW_MAPCOUNT; ++i)
			XMStoreFloat4x4(&m_pDynamicLight.g_LightViewProj[i], XMMatrixIdentity());
	}
	{
		// CubeMap 그림자 촬영 View의 Look벡터
		DirectionVec[0] = { XMVectorSet(+1.f, 0.f, 0.f, 0.f) }; // +X (Right)
		DirectionVec[1] = { XMVectorSet(-1.f, 0.f, 0.f, 0.f) }; // -X (Left)
		DirectionVec[2] = { XMVectorSet(0.f, +1.f, 0.f, 0.f) }; // +Y (Up)
		DirectionVec[3] = { XMVectorSet(0.f, -1.f, 0.f, 0.f) }; // -Y (Down)
		DirectionVec[4] = { XMVectorSet(0.f, 0.f, +1.f, 0.f) }; // +Z (Forward)
		DirectionVec[5] = { XMVectorSet(0.f, 0.f, -1.f, 0.f) }; // -Z (Backward)

		// CubeMap 그림자 촬영 View의 Up벡터
		BaseUpVec[0] = { XMVectorSet(0.f, +1.f, 0.f, 0.f) }; // +X (Up: +Y)
		BaseUpVec[1] = { XMVectorSet(0.f, +1.f, 0.f, 0.f) }; // -X (Up: +Y)
		BaseUpVec[2] = { XMVectorSet(0.f, 0.f, -1.f, 0.f) }; // +Y (Up: -Z)
		BaseUpVec[3] = { XMVectorSet(0.f, 0.f, +1.f, 0.f) }; // -Y (Up: +Z)
		BaseUpVec[4] = { XMVectorSet(0.f, +1.f, 0.f, 0.f) }; // +Z (Up: +Y)
		BaseUpVec[5] = { XMVectorSet(0.f, +1.f, 0.f, 0.f) }; // -Z (Up: +Y)
	}
	
	return S_OK;
}

HRESULT CLight::Initialize(VOID* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))			return E_FAIL;

	{
		m_pColliderSphere = CCollSphere::Create(m_pComTransform->GetPosition(), 10.f);
		if (nullptr == m_pColliderSphere) return E_FAIL;

		m_pColliderFrustum = CCollFrustum::Create(XMMatrixIdentity());
		if (nullptr == m_pColliderFrustum) return E_FAIL;
	}
	{
		CComConstantBuffer::DESC PerObjectDesc{};
		PerObjectDesc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &PerObjectDesc, &m_pComCBufferPerObject)))	return E_FAIL;
		
		CComConstantBuffer::DESC PerPassDesc{};
		PerPassDesc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PASS };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerPass", &PerPassDesc, &m_pComCBufferPerPass)))			return E_FAIL;
	}

	return S_OK;
}

VOID CLight::Update(E::_float _DT) {
	if (m_bActivate_State == false) return;

	if (m_bEffectLightFlag) 
		Update_EffectLight(_DT);

	if (m_pComTransform->Update()) 
		InvalidateAllShadow();

	if (m_bCullBoundsDirty && SUCCEEDED(Update_Collider()))
		m_bCullBoundsDirty = false;
}

VOID CLight::Update_ObjectConstantBuffer(ID3D11DeviceContext* pContext){
	if (m_bActivate_State == false) return;

	XMMATRIX LightWorldMatrix = XMLoadFloat4x4(m_pComTransform->GetWorldMatrix());
	XMMATRIX LightViewMatrix = XMLoadFloat4x4(&LightView);
	XMMATRIX LightProjMatrix = XMLoadFloat4x4(&LightProj);

	if (m_pDynamicLight.LightType == ETOUI(LIGHT_TYPE::POINT))
	{
		XMVECTOR PositionOffset = XMVectorSet(0.f, 0.0001f, 0.f, 0.f);

		XMVECTOR LightPosition  = LightWorldMatrix.r[3] + PositionOffset;
		XMVECTOR LightDirection = XMVectorSet(1.f, 0.f, 0.f, 0.f);
		XMVECTOR WorldUpVector  = XMVectorSet(0.f, 1.f, 0.f, 0.f);

		_float	 fNearZ = 0.01f;
		LightViewMatrix = XMMatrixLookAtLH(LightPosition, LightPosition + LightDirection, WorldUpVector);

		const _float OuterRange = std::max(m_fPointLightOuterAttenuation, 0.02f);
		LightProjMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, fNearZ, OuterRange);
	}
	{
		E::CB_PER_PASS pCbPerPass{};

		XMMATRIX LightViewProj{};
		LightViewProj = m_pDynamicLight.LightType == ETOUI(LIGHT_TYPE::DIRECTIONAL) ?
			XMLoadFloat4x4(&m_pDynamicLight.g_LightViewProj[0]) :
			XMMatrixMultiply(LightViewMatrix, LightProjMatrix);

		XMStoreFloat4x4(&pCbPerPass.matView, LightViewMatrix);
		XMStoreFloat4x4(&pCbPerPass.matProj, LightProjMatrix);
		XMStoreFloat4x4(&pCbPerPass.matViewProj, LightViewProj);
		XMStoreFloat4x4(&pCbPerPass.matInvView, XMMatrixInverse(nullptr, LightViewMatrix));
		XMStoreFloat4x4(&pCbPerPass.matInvProj, XMMatrixInverse(nullptr, LightProjMatrix));
		XMStoreFloat4x4(&pCbPerPass.matInvViewProj, XMMatrixInverse(nullptr, LightViewProj));
		XMStoreFloat4x4(&pCbPerPass.matShadowLightViewProj, LightViewProj);
		pCbPerPass.fDeltaTime = 0.f;
		pCbPerPass.fTimeAccumulation = 0.f;

		pCbPerPass.vCamPos = m_pComTransform->GetPosition();

		if (FAILED(m_pComCBufferPerPass->MapDiscard(pContext, &pCbPerPass, sizeof(pCbPerPass))))	return;

		pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_PASS), 1, m_pComCBufferPerPass->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_PASS), 1, m_pComCBufferPerPass->GetAdressOfBuffer());
		pContext->GSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_PASS), 1, m_pComCBufferPerPass->GetAdressOfBuffer());
	}
}

VOID CLight::Update_EffectLight(const _float& _DT){
	if (m_fLifeTime > 0.f) {
		m_fLifeTime -= _DT;
		XMVECTOR VelocityVec = XMLoadFloat3(&m_fVelocity);
		
		_float length = XMVectorGetX(XMVector3Length(VelocityVec));
		if (length != 0) {
			XMVECTOR CurrentPosition = m_pComTransform->GetLoadedPostion();
			m_pComTransform->SetPosition(CurrentPosition + XMLoadFloat3(&m_fVelocity) * _DT);
		}
	}
	else {
		Reset_Light(); 
	}
}

VOID CLight::PrepareShadowMapMatrices() {
	LIGHT_TYPE LTYPE = static_cast<LIGHT_TYPE>(m_pDynamicLight.LightType);

	if (LTYPE == LIGHT_TYPE::DIRECTIONAL) {
		Update_DirectionalShadowMatrices();
		m_bShadowMatrixDirty = false;
		return;
	}

	if (!m_bShadowMatrixDirty) return;

	if		(LTYPE == LIGHT_TYPE::POINT) {
		Update_PointShadowMatrices();
	}
	else if (LTYPE == LIGHT_TYPE::SPOTLIGHT) {
		Update_SpotShadowMatrices();
	}
	
	m_bShadowMatrixDirty = false;
}

VOID CLight::Update_PointShadowMatrices() {
	XMVECTOR LightPosition = m_pComTransform->GetState(STATE::POSITION);

	const _float OuterRange = std::max(m_fPointLightOuterAttenuation, 0.02f);
	XMMATRIX	 ProjMat = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, 0.01f, OuterRange);

	BoundingFrustum LocalFrustum{};
	BoundingFrustum::CreateFromMatrix(LocalFrustum, ProjMat);

	for (uint32_t i = 0; i < POINT_SHADOW_MAPCOUNT; ++i) {
		XMMATRIX ViewMat = XMMatrixLookAtLH(LightPosition, XMVectorAdd(LightPosition, DirectionVec[i]), BaseUpVec[i]);
		XMStoreFloat4x4(&m_pDynamicLight.g_LightViewProj[i], XMMatrixMultiply(ViewMat, ProjMat));
		LocalFrustum.Transform(m_PointShadowFrustums[i], XMMatrixInverse(nullptr, ViewMat));
	}
}

VOID CLight::Update_SpotShadowMatrices() {
	_float	 fNearZ = 0.1f;

	m_pDynamicLight.OuterAttanuation = std::clamp(m_pDynamicLight.OuterAttanuation, 1.f, 75.f);
	m_pDynamicLight.LightRange = std::clamp(m_pDynamicLight.LightRange, 0.01f + 0.01f, 100.f);

	XMVECTOR	LightPosition = m_pComTransform->GetState(STATE::POSITION);
	XMVECTOR	WorldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	_float		FOVAngle = m_pDynamicLight.OuterAttanuation * 2.f * 1.02f;
	if (FOVAngle > 150.f) FOVAngle = 150.f;

	XMStoreFloat4x4(&LightView, MakeSafeLookToLH(LightPosition, XMLoadFloat3(&m_pDynamicLight.LightDirection), WorldUp));
	XMStoreFloat4x4(&LightProj, XMMatrixPerspectiveFovLH(XMConvertToRadians(FOVAngle), 1.f, fNearZ, m_pDynamicLight.LightRange));
	XMStoreFloat4x4(&m_pDynamicLight.g_LightViewProj[0], XMMatrixMultiply(XMLoadFloat4x4(&LightView), XMLoadFloat4x4(&LightProj)));
}

VOID CLight::Update_DirectionalShadowMatrices() {
	auto MainCamera = CGameInstance::Get().GetActiveCamera();
	if (nullptr == MainCamera)	return;

	_float	 CameraNear = MainCamera->GetNear();

	const _float ShadowMaxDistance = static_cast<_float>(VOLUME_MAXFAR);		// 카메라 Far가 변하면 그림자 품질도 달라지므로 최대 그림자 Casting 거리 제한
	_float	 CameraFar	= std::min(MainCamera->GetFar(), ShadowMaxDistance);

	_float	 Lambda		= 0.7f;

	_float CSM_SplitDistance[MAX_CASCADE_COUNT + 1];	// Cascade 한 절두체의 시작거리(i) ~ 끝 거리(i+1)
	CSM_SplitDistance[0] = CameraNear;

	for (uint32_t i = 1; i <= MAX_CASCADE_COUNT; ++i) {
		_float FID = static_cast<_float>(i) / static_cast<_float>(MAX_CASCADE_COUNT);

		_float LogValue		= CameraNear * powf(CameraFar / CameraNear, FID);
		_float LinearValue	= CameraNear + (CameraFar - CameraNear) * FID;

		CSM_SplitDistance[i] = Lambda * LogValue + (1.f - Lambda) * LinearValue;
	}
	
	m_fCascadeShadowSplits = XMFLOAT4(CSM_SplitDistance[1], CSM_SplitDistance[2], CSM_SplitDistance[3], CSM_SplitDistance[4]);

	XMVECTOR LightDirection = XMLoadFloat3(&m_pDynamicLight.LightDirection);
	_float LightDirectionLength = XMVectorGetX(XMVector3Length(LightDirection));

	if (LightDirectionLength <= 0.0001f)
	{
		LightDirection = XMVectorSet(0.f, -1.f, 0.f, 0.f);
	}
	else
	{
		LightDirection = XMVector3Normalize(LightDirection);
	}

	for (uint32_t i = 0; i < MAX_CASCADE_COUNT; ++i) {
		_float NearSplit = CSM_SplitDistance[i];
		_float FarSplit  = CSM_SplitDistance[i+1];

		// Get Frustum 8 Corners Position
		std::vector<XMVECTOR> FrustumWorldCorners = Get_FrustumCorners(NearSplit, FarSplit);

		// Get Frustum Center Position
		XMVECTOR FrustumCenterPos = XMVectorZero();
		for (int j = 0; j < 8; ++j) 
			FrustumCenterPos += FrustumWorldCorners[j];
		FrustumCenterPos /= 8.f;

		_float FrustumRadius = 0.f;
		for (int j = 0; j < 8; ++j)
		{
			XMVECTOR CornerOffset = FrustumWorldCorners[j] - FrustumCenterPos;

			_float CornerDistance = XMVectorGetX(XMVector3Length(CornerOffset));

			FrustumRadius = std::max(FrustumRadius, CornerDistance);
		}

		// Set Light Position / ViewMat
		const _float CasterSearchDistance = 1000.f;
		_float DistanceFromCenter = FrustumRadius + CasterSearchDistance;
		XMVECTOR LightPosition = FrustumCenterPos - (LightDirection * DistanceFromCenter);
		XMMATRIX LightViewMat = XMMatrixLookAtLH(LightPosition, FrustumCenterPos, XMVectorSet(0.f, 1.f, 0.f, 0.f));

		_float MinX = FLT_MAX, MaxX = -FLT_MAX;
		_float MinY = FLT_MAX, MaxY = -FLT_MAX;
		_float MinZ = FLT_MAX, MaxZ = -FLT_MAX;

		for (int j = 0; j < 8; ++j) {
			XMVECTOR vLightSpacePos = XMVector3TransformCoord(FrustumWorldCorners[j], LightViewMat);

			XMFLOAT3 Pos;
			XMStoreFloat3(&Pos, vLightSpacePos);

			MinX = std::min(MinX, Pos.x); MaxX = std::max(MaxX, Pos.x);
			MinY = std::min(MinY, Pos.y); MaxY = std::max(MaxY, Pos.y);
			MinZ = std::min(MinZ, Pos.z); MaxZ = std::max(MaxZ, Pos.z);
		}
		const _float CascadeXYMargin = FrustumRadius * 0.1f;

		MinX -= CascadeXYMargin;
		MaxX += CascadeXYMargin;
		MinY -= CascadeXYMargin;
		MaxY += CascadeXYMargin;

		MinZ = 0.01f;
		const _float ReceiverDepthPadding = 100.f;
		MaxZ += ReceiverDepthPadding;
		MaxZ = std::max(MaxZ, MinZ + 1.f);

		XMMATRIX mLightProj = XMMatrixOrthographicOffCenterLH(MinX, MaxX, MinY, MaxY, MinZ, MaxZ);
		XMStoreFloat4x4(&m_pDynamicLight.g_LightViewProj[i], XMMatrixMultiply(LightViewMat, mLightProj));
	}
}

std::vector<XMVECTOR> CLight::Get_FrustumCorners(_float _SplitNear, _float _SplitFar){
	XMVECTOR vNDC[8] = {
		XMVectorSet(-1.f, +1.f, 0.f, 1.f),
		XMVectorSet(+1.f, +1.f, 0.f, 1.f),
		XMVectorSet(+1.f, -1.f, 0.f, 1.f),
		XMVectorSet(-1.f, -1.f, 0.f, 1.f),
		XMVectorSet(-1.f, +1.f, 1.f, 1.f),
		XMVectorSet(+1.f, +1.f, 1.f, 1.f),
		XMVectorSet(+1.f, -1.f, 1.f, 1.f),
		XMVectorSet(-1.f, -1.f, 1.f, 1.f)
	};

	auto MainCamera = CGameInstance::Get().GetActiveCamera();
	_float fCameraAspect = MainCamera->GetAspect();
	_float fCameraFovY = MainCamera->GetFovY();

	XMMATRIX mCamView = MainCamera->GetView();
	XMMATRIX mCamProj = XMMatrixPerspectiveFovLH(XMConvertToRadians(fCameraFovY), fCameraAspect, _SplitNear, _SplitFar);

	XMMATRIX mInvViewProj = XMMatrixInverse(nullptr, mCamView * mCamProj);

	std::vector<XMVECTOR> vWorldCorners(8);
	for (int i = 0; i < 8; ++i) {
		vWorldCorners[i] = XMVector3TransformCoord(vNDC[i], mInvViewProj);
	}

	return vWorldCorners;
}

HRESULT CLight::Update_Collider() {
	// LSY 변경: 라이트 배치 에디터는 ColliderManager 피킹을 사용하지 않는다.
	// 라이트 바운드는 전역 충돌 그룹에 등록하지 않고 컬링/디버그 계산용으로만 갱신한다.
	if (m_bActivate_State == false) return E_FAIL;
	XMVECTOR PosVec = XMLoadFloat3(&m_pComTransform->GetPosition());

	if		(m_pDynamicLight.LightType == ETOUI(LIGHT_TYPE::SPOTLIGHT)) {
		if (nullptr == m_pColliderFrustum) return E_FAIL;

		auto FrustumCollider = static_pointer_cast<CCollFrustum>(m_pColliderFrustum);
		if (nullptr == FrustumCollider) return E_FAIL;

		_float		FOVAngle = m_pDynamicLight.OuterAttanuation * 2.f * 1.2f;
		if (FOVAngle > 150.f) FOVAngle = 150.f;
		XMMATRIX CullProj = XMMatrixPerspectiveFovLH(XMConvertToRadians(FOVAngle), 1.f, 0.01f, m_pDynamicLight.LightRange * 1.1f);

		FrustumCollider->SetLocalFrustum(CullProj);

		XMMATRIX CullView = E::MakeSafeLookAtLH(PosVec, PosVec + XMLoadFloat3(&m_pDynamicLight.LightDirection), XMVectorSet(0.f, 1.f, 0.f, 0.f));
		m_pColliderFrustum->Transform(XMMatrixInverse(nullptr, CullView));
	}
	else if (m_pDynamicLight.LightType == ETOUI(LIGHT_TYPE::POINT)) {
		if (nullptr == m_pColliderSphere)	return E_FAIL;

		auto SphereCollider = std::static_pointer_cast<CCollSphere>(m_pColliderSphere);
		if (nullptr == SphereCollider)		return E_FAIL;

		// 컬링용 확장
		SphereCollider->SetLocalBoundingSphere({}, m_fPointLightOuterAttenuation * 1.1f);

		m_pColliderSphere->Transform(XMMatrixTranslationFromVector(PosVec));
	}

	return S_OK;
}

_bool CLight::Intersects_ShadowBounds(const BoundingBox& _Bounds) const{
	switch (static_cast<LIGHT_TYPE>(m_pDynamicLight.LightType)) {
		case LIGHT_TYPE::POINT :{
			const auto Sphere = std::static_pointer_cast<CCollSphere>(m_pColliderSphere);

			return Sphere->GetBoundingSphere().Intersects(_Bounds);
		}
		case LIGHT_TYPE::SPOTLIGHT: {
			const auto Frustum = std::static_pointer_cast<CCollFrustum>(m_pColliderFrustum);
			return Frustum->GetBoundingFrustum().Intersects(_Bounds);
		}
		case LIGHT_TYPE::DIRECTIONAL: {
			return true;
		}
	}
	return true;
}

HRESULT CLight::Capture_ShadowMap(ID3D11DeviceContext* pContext, E::RENDER_CTX& RCTX, const std::vector<IRenderable*>& _ObjectHandleList, int32_t _PointFaceIndex) {
	if (m_bActivate_State == false) return E_FAIL;

	if (_PointFaceIndex < -1 || _PointFaceIndex >= static_cast<int32_t>(POINT_SHADOW_MAPCOUNT))		return E_FAIL;

	const LIGHT_TYPE LightType = Get_LightType();

	RCTX.eye = XMLoadFloat3(&m_pComTransform->GetPosition());

	if (LightType == LIGHT_TYPE::POINT) {

		RCTX.PointShadowFaceIndex = _PointFaceIndex;
		XMMATRIX Identity = XMMatrixIdentity();
		RCTX.matView = Identity;
		RCTX.matProj = Identity;
		RCTX.matViewProj = XMLoadFloat4x4(&m_pDynamicLight.g_LightViewProj[_PointFaceIndex]);
	}
	else if (LightType == LIGHT_TYPE::SPOTLIGHT){
		RCTX.PointShadowFaceIndex = -1;

		RCTX.matView = XMLoadFloat4x4(&LightView);
		RCTX.matProj = XMLoadFloat4x4(&LightProj);
		RCTX.matViewProj = RCTX.matView * RCTX.matProj;
	}
	else {
		RCTX.PointShadowFaceIndex = _PointFaceIndex;

		XMMATRIX Identity = XMMatrixIdentity();
		RCTX.matView = Identity;
		RCTX.matProj = Identity;
		RCTX.matViewProj = XMLoadFloat4x4(&m_pDynamicLight.g_LightViewProj[_PointFaceIndex]);
	}

	for (auto& GOBJ : _ObjectHandleList) {
		if (nullptr == GOBJ) continue;
	
		BoundingBox ShadowBound{};
	
		if (GOBJ->GetShadowBounds(ShadowBound)) {
			const _float ShadowCullPadding = 1.f;

			ShadowBound.Extents.x += ShadowCullPadding;
			ShadowBound.Extents.y += ShadowCullPadding;
			ShadowBound.Extents.z += ShadowCullPadding;

			_bool bVisibleToLight = true;
	
			if (bVisibleToLight && LightType == LIGHT_TYPE::POINT && _PointFaceIndex >= 0) {
				bVisibleToLight = Intersects_PointShadowFace(static_cast<uint32_t>(_PointFaceIndex), ShadowBound);
			}
			else {
				bVisibleToLight = Intersects_ShadowBounds(ShadowBound);
			}
			if (false == bVisibleToLight) continue;
		}
	
		if (FAILED(GOBJ->Render_Shadow(pContext, RCTX))) return E_FAIL;
	}
	
	return S_OK;
}

VOID	CLight::Reset_Light(){
	m_pDynamicLight.LightDirection	 = { 1.f, -1.f, 1.f };
	m_pDynamicLight.LightIntensity	 = { 0.f };
	m_pDynamicLight.LightColor		 = { 1.f, 1.f, 1.f };
	m_pDynamicLight.LightRange		 = { 10.f };
									 
	m_pDynamicLight.Position		 = { 0.f, 0.f, 0.f };
	m_pDynamicLight.LightType		 = ETOUI(LIGHT_TYPE::POINT);

	m_pDynamicLight.InnerAttanuation = { 20.f };
	m_pDynamicLight.OuterAttanuation = { 30.f };

	m_fLifeTime = 0.f;
	m_fVelocity = { 0.f, 0.f, 0.f };
	m_bActivate_State = false;
	m_bHadDynamicShadowCaster = false;

	InvalidateAllShadow();
}

_bool	CLight::Intersects_PointShadowFace(uint32_t _FaceIndex, const BoundingBox& _Bounds) const {
	if (_FaceIndex >= POINT_SHADOW_MAPCOUNT)	return false;

	return m_PointShadowFrustums[_FaceIndex].Intersects(_Bounds);
}

HRESULT	CLight::Set_LightType(LIGHT_TYPE _LTYPE) {
	if (m_pDynamicLight.LightType == ETOUI(_LTYPE))
		return E_FAIL;

	m_pDynamicLight.LightType = ETOUI(_LTYPE);

	m_bShadowMatrixDirty = true;
	m_bCullBoundsDirty = true;
	m_bStaticShadowDirty = true;
	m_bDynamicShadowDirty = true;

	m_fPointLightInnerAttenuation = 0.f;
	m_fPointLightOuterAttenuation = 0.02f;

	return S_OK;
}
VOID	CLight::Set_LightDirection(XMFLOAT3 _Direction) {
	m_pDynamicLight.LightDirection = _Direction;

	m_bShadowMatrixDirty = true;
	m_bCullBoundsDirty = true;
	m_bStaticShadowDirty = true;
	m_bDynamicShadowDirty = true;
}
VOID	CLight::Set_LightPosition(XMFLOAT3 _Position) {
	m_pComTransform->SetPosition(_Position);

	m_bShadowMatrixDirty = true;
	m_bCullBoundsDirty = true;
	m_bStaticShadowDirty = true;
	m_bDynamicShadowDirty = true;
}
VOID	CLight::Set_LightPosition(XMVECTOR _Position) {
	m_pComTransform->SetPosition(_Position);

	m_bShadowMatrixDirty = true;
	m_bCullBoundsDirty = true;
	m_bStaticShadowDirty = true;
	m_bDynamicShadowDirty = true;
}
VOID	CLight::Set_LightActivateState(_bool _State) {
	if (_State && !m_bActivate_State) {
		m_bShadowMatrixDirty = true;
		m_bCullBoundsDirty = true;
		m_bStaticShadowDirty = true;
		m_bDynamicShadowDirty = true;
	}

	m_bActivate_State = _State;
}
VOID	CLight::Set_LightShadowCast(_bool _State) {
	if (_State && !m_bCastShadow)
	{
		m_bShadowMatrixDirty = true;
		m_bStaticShadowDirty = true;
		m_bDynamicShadowDirty = true;
	}

	m_bCastShadow = _State;
}
VOID	CLight::Set_LightRange(_float _Range) {
	// LSY 변경: 0 이하 Range로 투영행렬의 Near/Far가 무효가 되거나
	// 디버그 바운드가 깨지는 것을 막기 위해 안전한 최소값을 보장한다.
	constexpr _float MIN_LIGHT_RANGE = 0.02f;
	const _float safeRange = std::max(_Range, MIN_LIGHT_RANGE);

	if (m_pDynamicLight.LightType == static_cast<uint32_t>(LIGHT_TYPE::POINT))
	{
		m_fPointLightOuterAttenuation = safeRange;
	}
	else
	{
		m_pDynamicLight.LightRange = safeRange;
	}

	InvalidateAllShadow();
}
VOID	CLight::Set_PointLightOuterAttenuation(_float _Attenuation) {
	m_fPointLightOuterAttenuation = std::max(_Attenuation, 0.02f);

	m_bShadowMatrixDirty = true;
	m_bCullBoundsDirty = true;
	m_bStaticShadowDirty = true;
	m_bDynamicShadowDirty = true;
}
VOID	CLight::Set_LightOuterAttenuation(_float _Attenuation) {
	m_pDynamicLight.OuterAttanuation = std::clamp(_Attenuation, 1.f, 75.f);

	m_bShadowMatrixDirty = true;
	m_bCullBoundsDirty = true;
	m_bStaticShadowDirty = true;
	m_bDynamicShadowDirty = true;
}

UPtr<CLight>	 CLight::Create()	{
	auto pInstance = ToUPtr(new CLight{});
	if (FAILED(pInstance->InitializePrototype(nullptr))) {
		MSG_BOX("Failed to Create: CLight");
		return nullptr;
	}

	return pInstance;
}
UPtr<CPrototype> CLight::Clone(void* pArg)	{
	auto pInstance = ToUPtr(new CLight{ *this });
	if (FAILED(pInstance->Initialize(pArg))) {
		MSG_BOX("Failed to Cloned: CLight");
		return nullptr;
	}

	return pInstance;
}
