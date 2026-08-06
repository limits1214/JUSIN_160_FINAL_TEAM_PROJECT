#include "pch.h"
#include "ResPhysXRTHeightFieldGeometry.h"

#pragma push_macro("new")
#undef new
#include "PxPhysicsAPI.h"
#pragma pop_macro("new")

#include "PhysXCookedMeshFile.h"

#include <cmath>

using namespace physx;
NS_USING(Engine)

namespace
{
	_bool IsValidDesc(const CResPhysXRTHeightFieldGeometry::DESC& desc)
	{
		if (!desc.pSamples ||
			desc.iRowCount < 2 ||
			desc.iColumnCount < 2 ||
			!std::isfinite(desc.fConvexEdgeThreshold) ||
			desc.fConvexEdgeThreshold < 0.f)
		{
			return false;
		}

		const uint64_t iRequiredSampleCount =
			static_cast<uint64_t>(desc.iRowCount) *
			static_cast<uint64_t>(desc.iColumnCount);

		if (iRequiredSampleCount > std::numeric_limits<size_t>::max() ||
			iRequiredSampleCount > std::numeric_limits<PxU32>::max() ||
			desc.iSampleCount != static_cast<size_t>(iRequiredSampleCount))
		{
			return false;
		}

		for (size_t i = 0; i < desc.iSampleCount; ++i)
		{
			const auto& sample = desc.pSamples[i];
			if (sample.iMaterialIndex0 > CResPhysXRTHeightFieldGeometry::MATERIAL_HOLE ||
				sample.iMaterialIndex1 > CResPhysXRTHeightFieldGeometry::MATERIAL_HOLE)
			{
				return false;
			}
		}

		return true;
	}
}

CResPhysXRTHeightFieldGeometry::CResPhysXRTHeightFieldGeometry(const _string& sPath)
	: CResPhysXHeightFieldGeometry{ sPath }
{
}

CResPhysXRTHeightFieldGeometry::~CResPhysXRTHeightFieldGeometry() = default;

HRESULT CResPhysXRTHeightFieldGeometry::Load(const std::any& arg)
{
	const auto* pDesc = std::any_cast<DESC>(&arg);
	if (!pDesc)
		return E_INVALIDARG;

	if (m_eState == STATE::LOADED)
		return S_OK;

	m_eState = STATE::LOADING;
	std::vector<uint8_t> cookedData{};
	if (FAILED(CookToMemory(*pDesc, cookedData)) ||
		FAILED(CreateFromCookedData(cookedData.data(), cookedData.size())))
	{
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResPhysXRTHeightFieldGeometry::CookToMemory(
	const DESC& desc,
	std::vector<uint8_t>& outCookedData)
{
	outCookedData.clear();
	if (!IsValidDesc(desc))
		return E_INVALIDARG;

	std::vector<PxHeightFieldSample> pxSamples(desc.iSampleCount);
	for (size_t i = 0; i < desc.iSampleCount; ++i)
	{
		const auto& src = desc.pSamples[i];
		auto& dst = pxSamples[i];

		dst.height = static_cast<PxI16>(src.iHeight);
		dst.materialIndex0 = PxBitAndByte(
			static_cast<PxU8>(src.iMaterialIndex0),
			src.bTessFlag);
		dst.materialIndex1 = PxBitAndByte(
			static_cast<PxU8>(src.iMaterialIndex1));
	}

	PxHeightFieldDesc heightFieldDesc{};
	heightFieldDesc.nbRows = static_cast<PxU32>(desc.iRowCount);
	heightFieldDesc.nbColumns = static_cast<PxU32>(desc.iColumnCount);
	heightFieldDesc.samples.data = pxSamples.data();
	heightFieldDesc.samples.stride = sizeof(PxHeightFieldSample);
	heightFieldDesc.convexEdgeThreshold = desc.fConvexEdgeThreshold;
	if (desc.bNoBoundaryEdges)
		heightFieldDesc.flags |= PxHeightFieldFlag::eNO_BOUNDARY_EDGES;

	if (!heightFieldDesc.isValid())
	{
		DEBUG_LOG("[PX][HeightField] Invalid height-field descriptor.\n");
		return E_INVALIDARG;
	}

	PxDefaultMemoryOutputStream output{};
	if (!PxCookHeightField(heightFieldDesc, output))
	{
		DEBUG_LOG("[PX][HeightField] Cooking failed.\n");
		return E_FAIL;
	}

	outCookedData.assign(output.getData(), output.getData() + output.getSize());
	return outCookedData.empty() ? E_FAIL : S_OK;
}

HRESULT CResPhysXRTHeightFieldGeometry::CookToFile(
	const DESC& desc,
	const _string& sOutputPath)
{
	std::vector<uint8_t> cookedData{};
	if (FAILED(CookToMemory(desc, cookedData)))
		return E_FAIL;

	return PhysXCookedMeshFile::Write(
		sOutputPath,
		PhysXCookedMeshFile::TYPE::HEIGHT_FIELD,
		cookedData.data(),
		cookedData.size());
}

SPtr<CResPhysXRTHeightFieldGeometry> CResPhysXRTHeightFieldGeometry::Create()
{
	return ToSPtr(new CResPhysXRTHeightFieldGeometry{ "" });
}

SPtr<CResPhysXRTHeightFieldGeometry> CResPhysXRTHeightFieldGeometry::CreateAndLoad(const DESC& desc)
{
	auto pInstance = Create();
	if (!pInstance || FAILED(pInstance->Load(desc)))
		return nullptr;

	return pInstance;
}
