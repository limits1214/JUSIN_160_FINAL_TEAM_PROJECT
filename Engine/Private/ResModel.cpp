#include "pch.h"
#include "ResModel.h"
#include "ResModelMesh.h"
#include "ResModelBone.h"
#include "ResModelMaterial.h"
#include "ResModelAnim.h"
#include <fstream>

NS_USING(Engine)

CResModel::CResModel(const _string& sPath)
	: CResource{ sPath }
{
}

CResModel::~CResModel()
{
}

HRESULT CResModel::Load(const std::any& arg)
{
	auto descArg = std::any_cast<DESC>(&arg);
	if (!descArg)
		return E_FAIL;

	if (m_eState == STATE::LOADED)
		return S_OK;

	m_eState = STATE::LOADING;
	XMStoreFloat4x4(&m_PreTransformMatrix, descArg->PreTransformMatrix);

	LogMemoryUsage("CResModel-Load-Start");
	{
		if (!std::filesystem::exists(m_sPath))
			return E_FAIL;

		std::ifstream file(m_sPath, std::ios::binary | std::ios::ate);
		if (!file.is_open())
			return E_FAIL;

		size_t size = static_cast<size_t>(file.tellg());
		file.seekg(0, std::ios::beg);

		std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size);
		file.read(buffer.get(), size);

		if (!file)
			return E_FAIL;

		char* ptr = buffer.get();

		MODEL_FILE_HEADER* fh = reinterpret_cast<MODEL_FILE_HEADER*>(ptr);
		ptr += sizeof(MODEL_FILE_HEADER);

		m_iNumMeshes = fh->MeshCount;
		m_iAnimCnt = fh->AnimationCount;
		m_iNumMaterials = fh->MaterialCount;
		m_iNumBones = fh->BoneCount;

		char* base = buffer.get();
		char* end = base + size;

		while (ptr < end)
		{
			CHUCKHEADER* chunk = reinterpret_cast<CHUCKHEADER*>(ptr);
			ptr += sizeof(CHUCKHEADER);

			if (ptr + chunk->size > end)
				return E_FAIL;

			switch (chunk->type)
			{
			case CHUNCK_TYPE::CHUNK_BONE:
				if (FAILED(Ready_Bones(ptr)))
					return E_FAIL;
				LogMemoryUsage("CResModel-Ready-BONE");
				break;
				

			case CHUNCK_TYPE::CHUNK_MESH:
				if (FAILED(Ready_Meshes(ptr)))
					return E_FAIL;
				LogMemoryUsage("CResModel-Ready-MESH");
				break;

			case CHUNCK_TYPE::CHUNK_MATERIAL:
				if (FAILED(Ready_Materials(m_sPath, ptr)))
					return E_FAIL;
				LogMemoryUsage("CResModel-Ready-MATERIAL");
				break;
			}

			ptr += chunk->size;
		}

	}
	LogMemoryUsage("CResModel-Ready-BONE,MESH,MATERIAL");


	if (FAILED(Ready_Animation()))
		return E_FAIL;
	LogMemoryUsage("CResModel-Ready-ANIMATION");
	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResModel::Unload(const std::any& arg)
{

	m_eState = STATE::UNLOAD;
	return S_OK;
}

HRESULT CResModel::Ready_Bones(_char* ptr)
{
	//----------------------------------------------------------------------------
	for (uint32_t i = 0; i < m_iNumBones; ++i) {
		uint32_t consumed = *(uint32_t*)ptr;
		ptr += sizeof(uint32_t);

		auto    pBone = CResModelBone::Create();
		if (nullptr == pBone)
			return E_FAIL;

		E::CResModelBone::DESC pDesc{};
		pDesc.ptr = ptr;
		if (FAILED(pBone->Load(pDesc))) {
			return E_FAIL;
		}

		m_Bones.push_back(pBone);

		ptr += consumed;
	}

	return S_OK;
}

HRESULT CResModel::Ready_Materials(const _string& strModelFilePath, _char* ptr)
{
	m_Materials.resize(m_iNumMaterials);
	for (size_t i = 0; i < m_iNumMaterials; i++)
	{

		uint32_t consumed = *(uint32_t*)ptr;
		ptr += sizeof(uint32_t);

		auto  pMaterial = CResModelMaterial::Create(strModelFilePath);

		if (FAILED(pMaterial->Load(CResModelMaterial::DESC{ .ptr = ptr }))) {
			return E_FAIL;
		}

		m_Materials[pMaterial->GetMaterialTypeNum()] = (pMaterial);


		ptr += consumed;
	}

	return S_OK;
}

HRESULT CResModel::Ready_Meshes(_char* ptr)
{

	for (size_t i = 0; i < m_iNumMeshes; i++)
	{
	
		uint32_t consumed = *(uint32_t*)ptr;
		ptr += sizeof(uint32_t);


		auto    pMesh = CResModelMesh::Create();
		if (nullptr == pMesh)
			return E_FAIL;

		E::CResModelMesh::DESC pDesc{};
		pDesc.eType = m_eModelType;
		pDesc.ptr = ptr;
		pDesc.pModel = this;
		pDesc.PreTransformMatrix = XMLoadFloat4x4(&m_PreTransformMatrix);
	
		if (FAILED(pMesh->Load(pDesc))) {
			return E_FAIL;
		}

		m_Meshes.push_back(pMesh);

		ptr += consumed;
	}

	return S_OK;
}

HRESULT CResModel::Ready_Animation()
{

	std::filesystem::path modelPath(m_sPath);


	std::filesystem::path folderPath = modelPath.parent_path();

	for (const auto& entry : std::filesystem::directory_iterator(folderPath))
	{
		if (!entry.is_regular_file())
			continue;

		const auto& path = entry.path();

		std::string fileName = path.filename().string();

		if (fileName.rfind("AN_", 0) != 0)
			continue;

		std::string animPath = path.string();

		auto pAnimation = CResModelAnim::Create();
		if (nullptr == pAnimation) {
			return E_FAIL;
		}
				

		if (FAILED(pAnimation->Load(CResModelAnim::DESC{.pModel = this, .path = animPath }))){
				return E_FAIL;
		}

		pAnimation->SetAnimName(fileName);
		m_Animations.push_back(pAnimation);
	
	}


	return S_OK;
}



int32_t CResModel::Get_BoneIndex(const _char* pBoneName)
{
	int32_t iBoneIndex = { 0 };
	auto    iter = find_if(m_Bones.begin(), m_Bones.end(), [&](SPtr<CResModelBone> pBone)->_bool
		{
			if (true == pBone->Compare_Name(pBoneName))
				return true;

			++iBoneIndex;

			return false;
		});

	if (iter == m_Bones.end())
		return -1;

	return iBoneIndex;
}

const _float4x4* CResModel::Get_BoneMatrixPtr(const _char* pBoneName)
{
	auto    iter = find_if(m_Bones.begin(), m_Bones.end(), [&](SPtr<CResModelBone> pBone)->_bool
		{
			if (true == pBone->Compare_Name(pBoneName))
				return true;

			return false;
		});

	if (iter == m_Bones.end())
		return nullptr;

	return (*iter)->Get_CombinedTransformationMatrixPtr();
}

SPtr<CResModel> CResModel::Create(const _string& sPath)
{
	return ToSPtr(new CResModel{ sPath });
}

