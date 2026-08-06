#include "pch.h"
#include "ResPhysXHeightFieldGeometry.h"
#include "GameInstance.h"

#pragma push_macro("new")
#undef new
#include "PxPhysicsAPI.h"
#pragma pop_macro("new")

#include "PhysXCookedMeshFile.h"

using namespace physx;
NS_USING(Engine)

CResPhysXHeightFieldGeometry::CResPhysXHeightFieldGeometry(const _string& sPath)
	: CResPhysXGeometry{ sPath }
{
}

CResPhysXHeightFieldGeometry::~CResPhysXHeightFieldGeometry() = default;

HRESULT CResPhysXHeightFieldGeometry::Load(const std::any& arg)
{
	if (m_eState == STATE::LOADED)
		return S_OK;

	m_eState = STATE::LOADING;
	std::vector<uint8_t> cookedData{};
	if (FAILED(PhysXCookedMeshFile::Read(
		m_sPath,
		PhysXCookedMeshFile::TYPE::HEIGHT_FIELD,
		cookedData)) ||
		FAILED(CreateFromCookedData(cookedData.data(), cookedData.size())))
	{
		m_eState = STATE::LOADFAIL;
		DEBUG_LOG("[PX][HeightField] Failed to load cooked height-field file.\n");
		return E_FAIL;
	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResPhysXHeightFieldGeometry::Unload(const std::any& arg)
{
	return S_OK;
}

HRESULT CResPhysXHeightFieldGeometry::CreateFromCookedData(
	uint8_t* pData,
	size_t iDataSize)
{
	if (!pData || iDataSize == 0 ||
		iDataSize > std::numeric_limits<PxU32>::max() ||
		m_pHeightField)
		return E_INVALIDARG;

	auto* pPhysics = CGameInstance::Get().PxGetPhysics();
	if (!pPhysics)
		return E_FAIL;

	PxDefaultMemoryInputData input{ pData, static_cast<PxU32>(iDataSize) };
	m_pHeightField = pPhysics->createHeightField(input);
	if (!m_pHeightField)
	{
		DEBUG_LOG("[PX][HeightField] createHeightField failed.\n");
		return E_FAIL;
	}

	return S_OK;
}

SPtr<CResPhysXHeightFieldGeometry> CResPhysXHeightFieldGeometry::Create(const _string& sPath)
{
	return ToSPtr(new CResPhysXHeightFieldGeometry{ sPath });
}

SPtr<CResPhysXHeightFieldGeometry> CResPhysXHeightFieldGeometry::CreateAndLoad(const _string& sPath)
{
	auto pInstance = Create(sPath);
	if (!pInstance || FAILED(pInstance->Load()))
		return nullptr;

	return pInstance;
}

void CResPhysXHeightFieldGeometry::Free()
{
	if (m_pHeightField)
	{
		m_pHeightField->release();
		m_pHeightField = nullptr;
	}
	CResPhysXGeometry::Free();
}
