#pragma once
#include "ResPhysXGeometry.h"

NS_BEGIN(physx)
class PxHeightField;
NS_END

NS_BEGIN(Engine)

class ENGINE_DLL CResPhysXHeightFieldGeometry : public CResPhysXGeometry
{
public:
	DECLARE_DERIVED_TYPE(CResPhysXHeightFieldGeometry, CResPhysXGeometry)

protected:
	explicit CResPhysXHeightFieldGeometry(const _string& sPath);
	~CResPhysXHeightFieldGeometry() override;

public:
	physx::PxHeightField* GetHeightField() const { return m_pHeightField; }
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

protected:
	HRESULT CreateFromCookedData(uint8_t* pData, size_t iDataSize);

private:
	physx::PxHeightField* m_pHeightField{};

public:
	static SPtr<CResPhysXHeightFieldGeometry> Create(const _string& sPath);
	static SPtr<CResPhysXHeightFieldGeometry> CreateAndLoad(const _string& sPath);

private:
	void Free() override;
};

NS_END
