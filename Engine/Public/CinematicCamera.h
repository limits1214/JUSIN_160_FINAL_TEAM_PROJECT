#pragma once
#include "CameraObject.h"

NS_BEGIN(Engine)

class ENGINE_DLL CCinematicCamera final : public CCameraObject
{
public:
	DECLARE_DERIVED_TYPE(CCinematicCamera, CCameraObject)

protected:
	CCinematicCamera();
	CCinematicCamera(const CCinematicCamera& Prototype);
	~CCinematicCamera() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void LateUpdate(_float fTimeDelta) override;

public:
	HRESULT ApplyPose(const _float3& vPosition, const _float4& vRotation, _float fFovY);

	static UPtr<CCinematicCamera> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

};

NS_END
