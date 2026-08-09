#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)

class CCollider;
class CCollFrustum;
class CCollOrientedBox;

class ENGINE_DLL CCameraObject : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CCameraObject, CGameObject)

public:
	enum class PROJ { PERSPECTIVE, ORTHOGRAPHIC, END };
	typedef struct tagCameraDesc : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vEye{};
		_float3 vAt{};
		_float3 vUp{ 0.f, 1.f, 0.f };
		_float fFovY{};
		_float fAspect{};
		_float fNear{};
		_float fFar{};
		CCameraObject::PROJ eProj{ CCameraObject::PROJ::END };
		_float fWidth{};
		_float fHeight{};
	}CAMERA_DESC;


protected:
	explicit CCameraObject();
	explicit CCameraObject(const CCameraObject& Prototype);
	~CCameraObject() override;

public:
	_matrix GetView() const { return XMLoadFloat4x4(&m_matView); }
	_matrix GetProj() const { return XMLoadFloat4x4(&m_matProj); }
	_float GetFovY() const { return m_cameraDesc.fFovY; }
	/*----------- 광윤 추가 -----------*/
	_float GetAspect()	const { return m_cameraDesc.fAspect; }
	_float GetNear()	const { return m_cameraDesc.fNear;	 }
	_float GetFar()		const { return m_cameraDesc.fFar;	 }
	/*---------------------------------*/
	const CCollider* GetViewVolumeCollider() const { return m_pViewVolumeCollider.get(); }
	const CCollFrustum* GetFrustumCollider() const;
	const CCollOrientedBox* GetOrientedBoxCollider() const;
	ContainmentType ContainsViewVolume(const BoundingBox& bounds) const;
	_bool IntersectsViewVolume(const BoundingBox& bounds) const;
	const _float4* GetFrustumFarCorner() const { return m_FrustumFarCorner; }
	

	std::pair<_float3, _float3> GetRay() const
	{
		//RECT rect;
		//GetClientRect(CGameInstance::Get().GetHwnd(), &rect);

		E::_float4x4 P;
		XMStoreFloat4x4(&P, GetProj());

		E::_vector rayOrigin = XMVectorSet(0.f, 0.f, 0.f, 1.f);
		E::_vector rayDir = XMVectorSet(0.f, 0.f, 1.f, 0.f);

		E::_matrix V = GetView();
		auto detV = XMMatrixDeterminant(V);
		E::_matrix invView = XMMatrixInverse(&detV, V);

		_float3 vecrayOrigin;
		_float3 vecrayDir;
		XMStoreFloat3(&vecrayOrigin, XMVector3TransformCoord(rayOrigin, invView));
		XMStoreFloat3(&vecrayDir, XMVector3Normalize(XMVector3TransformNormal(rayDir, invView)));
		return { vecrayOrigin ,vecrayDir };
	}
	std::pair<_float3, _float3> GetRayFromScreenPixel(
		const _float2& vScreenPixel,
		const _float2& vViewportSize) const;

public:
	HRESULT Initialize(void* pArg) override;
	//void PriorityUpdate(E::_float fTimeDelta) override;
	//void Update(E::_float fTimeDelta) override;
	//void LateUpdate(E::_float fTimeDelta) override;
	//HRESULT Render(ComPtr<ID3D11Device>& ppDevice, ComPtr<ID3D11DeviceContext>& ppContext) override;

public:
	HRESULT UpdateViewMatrix();
	HRESULT UpdateProjMatrix();

protected:
	CCollider* GetMutableViewVolumeCollider() { return m_pViewVolumeCollider.get(); }
	HRESULT UpdateViewVolume();

public:
	void FSRCameraJitter();
	std::pair<float, float> GetFSRCameraJitter() const { return m_pairCameraJitter; }
private:
	std::pair<float, float> m_pairCameraJitter{};

protected:
	CAMERA_DESC m_cameraDesc{};
	_float4x4 m_matView{};
	_float4x4 m_matProj{};
	UPtr<CCollider> m_pViewVolumeCollider{};

	_float4 m_FrustumFarCorner[4]{};
};

NS_END
