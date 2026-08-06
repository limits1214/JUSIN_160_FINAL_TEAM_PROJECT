#pragma once
#include "ResPhysXHeightFieldGeometry.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResPhysXRTHeightFieldGeometry final : public CResPhysXHeightFieldGeometry
{
public:
	static constexpr uint8_t MATERIAL_HOLE = 127;

	struct SAMPLE
	{
		int16_t iHeight{};
		uint8_t iMaterialIndex0{};
		uint8_t iMaterialIndex1{};
		_bool bTessFlag{};
	};

	struct DESC
	{
		const SAMPLE* pSamples{};
		size_t iSampleCount{};
		uint32_t iRowCount{};
		uint32_t iColumnCount{};
		_float fConvexEdgeThreshold{};
		_bool bNoBoundaryEdges{};
	};

public:
	DECLARE_DERIVED_TYPE(CResPhysXRTHeightFieldGeometry, CResPhysXHeightFieldGeometry)

private:
	explicit CResPhysXRTHeightFieldGeometry(const _string& sPath);
	~CResPhysXRTHeightFieldGeometry() override;

public:
	static DESC MakeDesc(
		const std::vector<SAMPLE>& samples,
		uint32_t iRowCount,
		uint32_t iColumnCount,
		_float fConvexEdgeThreshold = 0.f,
		_bool bNoBoundaryEdges = false)
	{
		DESC desc{};
		desc.pSamples = samples.data();
		desc.iSampleCount = samples.size();
		desc.iRowCount = iRowCount;
		desc.iColumnCount = iColumnCount;
		desc.fConvexEdgeThreshold = fConvexEdgeThreshold;
		desc.bNoBoundaryEdges = bNoBoundaryEdges;
		return desc;
	}

public:
	HRESULT Load(const std::any& arg = {}) override;
	static HRESULT CookToMemory(const DESC& desc, std::vector<uint8_t>& outCookedData);
	static HRESULT CookToFile(const DESC& desc, const _string& sOutputPath);

public:
	static SPtr<CResPhysXRTHeightFieldGeometry> Create();
	static SPtr<CResPhysXRTHeightFieldGeometry> CreateAndLoad(const DESC& desc);
};

NS_END
