#pragma once
#include "ComPxCollider.h"

namespace physx
{
	class PxHeightField;
}

NS_BEGIN(Engine)

class CResPhysXHeightFieldGeometry;

class ENGINE_DLL CComPxHeightFieldCollider final : public CComPxCollider
{
public:
	struct DESC : CComPxCollider::DESC
	{
		SPtr<CResPhysXHeightFieldGeometry> pResHeightField{};

		// PhysX HeightField local axes:
		//   row    -> X
		//   height -> Y
		//   column -> Z
		_float fHeightScale{ 1.f };
		_float fRowScale{ 1.f };
		_float fColumnScale{ 1.f };
	};

public:
	DECLARE_DERIVED_TYPE(CComPxHeightFieldCollider, CComPxCollider)

public:
	void UpdateGUI() override;

private:
	explicit CComPxHeightFieldCollider();
	~CComPxHeightFieldCollider() override;

public:
	HRESULT Initialize(void* pArg) override;

public:
	_float GetHeightScale() const { return m_fHeightScale; }
	_float GetRowScale() const { return m_fRowScale; }
	_float GetColumnScale() const { return m_fColumnScale; }

public:
	static UPtr<CComPxHeightFieldCollider> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	SPtr<CResPhysXHeightFieldGeometry> m_pResHeightField{};
	_float m_fHeightScale{ 1.f };
	_float m_fRowScale{ 1.f };
	_float m_fColumnScale{ 1.f };

private:
	void Free() override;
};

NS_END
