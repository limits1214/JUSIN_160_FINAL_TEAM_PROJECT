#include "pch.h"
#include "ResStaticModel.h"
#include "ResStaticModelMesh.h"
#include "ResModelMaterial.h"
#include <fstream>
#include "ComStaticModelInstance.h"

NS_USING(Engine)

CResStaticModel::CResStaticModel(const _string& sPath)
	: CResource{ sPath }
{
}

CResStaticModel::~CResStaticModel()
{
}

HRESULT CResStaticModel::Load(const std::any& arg)
{
	auto descArg = std::any_cast<DESC>(&arg);
	if (!descArg)
	{
		return E_FAIL;
	}

	if (m_eState == STATE::LOADED)
	{
		return S_OK;
	}

	LogMemoryUsage("CResStaticModel_Load_Start");

	m_eState = STATE::LOADING;
	XMStoreFloat4x4(&m_PreTransformMatrix, descArg->PreTransformMatrix);

	std::filesystem::path fsPath(m_sPath);
	std::string ext = fsPath.extension().string();

	if(ext == ".bin")
	{
		std::ifstream file(m_sPath, std::ios::binary | std::ios::ate);


		if (!file.is_open())
		{
			return E_FAIL;
		}

		file.seekg(0, std::ios::end);
		size_t size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::shared_ptr<char[]> buffer = std::make_shared<char[]>(size);
		file.read(buffer.get(), size);

		file.close();

		char* ptr = buffer.get();

		MODEL_FILE_HEADER* fh = (MODEL_FILE_HEADER*)ptr;
		ptr += sizeof(MODEL_FILE_HEADER);

		m_iNumMeshes = fh->MeshCount;
		m_iNumMaterials = fh->MaterialCount;

		char* base = buffer.get();
		char* end = base + size;

		while (ptr < end)
		{
			CHUCKHEADER* chunk = (CHUCKHEADER*)ptr;
			ptr += sizeof(CHUCKHEADER);

			switch (chunk->type)
			{
			case CHUNCK_TYPE::CHUNK_MESH:
			{
				char* meshData = ptr;

				if (FAILED(Ready_Meshes(meshData)))
					return E_FAIL;
				//LogMemoryUsage("CResStaticModel-Ready-MESH");

				ptr += chunk->size;
			}
			break;

			case CHUNCK_TYPE::CHUNK_MATERIAL:
			{
				char* matData = ptr;

				if (FAILED(Ready_Materials(m_sPath, matData)))
					return E_FAIL;
				//LogMemoryUsage("CResStaticModel-Ready-MATERIAL");
				ptr += chunk->size;
			}
			break;
			}
		};


	}

	LogMemoryUsage("InitializeResources---Static_Load_END");
	if (ext == ".fbx") {
		LoadAssimp();
	}
	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResStaticModel::Unload(const std::any& arg)
{

	m_eState = STATE::UNLOAD;
	return S_OK;
}




HRESULT CResStaticModel::LoadAssimp()
{
	if (m_eState == STATE::LOADED)
		return S_OK;

	m_eState = STATE::LOADING;

	uint32_t iFlag = 0;
	iFlag |= aiProcess_ConvertToLeftHanded;
	iFlag |= aiProcess_PopulateArmatureData;
	iFlag |= aiProcess_GlobalScale;
	iFlag |= aiProcess_ImproveCacheLocality;
	iFlag |= aiProcessPreset_TargetRealtime_Fast;

	Assimp::Importer importer;

	const aiScene* pScene = importer.ReadFile(m_sPath, iFlag);

	if (pScene == nullptr)
	{
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	m_iNumMeshes = pScene->mNumMeshes;
	m_iNumMaterials = pScene->mNumMaterials;

	if (pScene->HasMaterials())
	{
		if (FAILED(AssimpMaterials(pScene)))
		{
			m_eState = STATE::LOADFAIL;
			return E_FAIL;
		}
	}

	if (pScene->HasMeshes())
	{
		ProcessAssimpNode(pScene->mRootNode, pScene);
	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResStaticModel::Ready_Meshes(_char* ptr)
{

	for (size_t i = 0; i < m_iNumMeshes; i++)
	{

		uint32_t consumed = *(uint32_t*)ptr;
		ptr += sizeof(uint32_t);


		auto    pMesh = CResStaticModelMesh::Create();
		if (nullptr == pMesh)
			return E_FAIL;

		E::CResStaticModelMesh::DESC pDesc{};
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


HRESULT CResStaticModel::Ready_Materials(const _string& strModelFilePath, _char* ptr)
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



SPtr<CResStaticModel> CResStaticModel::Create(const _string& sPath)
{
	return ToSPtr(new CResStaticModel{ sPath });
}

void CResStaticModel::ProcessAssimpMesh(aiMesh* mesh, const aiScene* scene)
{
	if (mesh == nullptr || mesh->mNumVertices == 0)
		return;

	const UINT vertexCount = mesh->mNumVertices;
	const UINT faceCount = mesh->mNumFaces;

	std::vector<VTXMESH> vertices;
	std::vector<uint32_t> indices;

	vertices.resize(vertexCount);
	indices.reserve(faceCount * 3);

	const bool hasNormal = mesh->HasNormals();
	const bool hasUV = mesh->HasTextureCoords(0);
	const bool hasTangent = mesh->HasTangentsAndBitangents();

	const aiVector3D* positions = mesh->mVertices;
	const aiVector3D* normals = mesh->mNormals;
	const aiVector3D* uvs = hasUV ? mesh->mTextureCoords[0] : nullptr;
	const aiVector3D* tangents = mesh->mTangents;
	const aiVector3D* binormals = mesh->mBitangents;

	XMFLOAT3 minPos = { FLT_MAX,  FLT_MAX,  FLT_MAX };
	XMFLOAT3 maxPos = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for (UINT i = 0; i < vertexCount; ++i)
	{
		const aiVector3D& pos = positions[i];

		VTXMESH& v = vertices[i];

		v.vPosition = { pos.x, pos.y, pos.z };

		if (hasNormal)
		{
			const aiVector3D& n = normals[i];
			v.vNormal = { n.x, n.y, n.z };
		}

		if (hasUV)
		{
			const aiVector3D& uv = uvs[i];
			v.vTexcoord = { uv.x, uv.y };
		}

		if (hasTangent)
		{
			const aiVector3D& t = tangents[i];
			const aiVector3D& b = binormals[i];

			v.vTangent = { t.x, t.y, t.z };
			v.vBinormal = { b.x, b.y, b.z };
		}

		minPos.x = std::min(minPos.x, pos.x);
		minPos.y = std::min(minPos.y, pos.y);
		minPos.z = std::min(minPos.z, pos.z);

		maxPos.x = std::max(maxPos.x, pos.x);
		maxPos.y = std::max(maxPos.y, pos.y);
		maxPos.z = std::max(maxPos.z, pos.z);
	}

	for (UINT i = 0; i < faceCount; ++i)
	{
		const aiFace& face = mesh->mFaces[i];

		for (UINT j = 0; j < face.mNumIndices; ++j)
		{
			indices.emplace_back(face.mIndices[j]);
		}
	}

	std::string meshName;

	if (mesh->mName.length > 0)
		meshName = mesh->mName.C_Str();


	const uint32_t materialIndex = mesh->mMaterialIndex;


	auto    pMesh = CResStaticModelMesh::Create();
	if (nullptr == pMesh)
		return;

	E::CResStaticModelMesh::DESC pDesc{};
	pDesc.eType = m_eModelType;
	pDesc.pModel = this;
	pDesc.PreTransformMatrix = XMLoadFloat4x4(&m_PreTransformMatrix);

	if (FAILED(pMesh->LoadAssimp(std::move(meshName),materialIndex,minPos,maxPos,std::move(vertices),std::move(indices), XMLoadFloat4x4(&m_PreTransformMatrix)))) {
		return ;
	}

	m_Meshes.push_back(pMesh);
}
void CResStaticModel::ProcessAssimpNode(aiNode* node, const aiScene* scene)
{
	for (UINT i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessAssimpMesh(mesh, scene);
	}

	for (UINT i = 0; i < node->mNumChildren; ++i)
		ProcessAssimpNode(node->mChildren[i], scene);
}

HRESULT CResStaticModel::AssimpMaterials(const aiScene* scene)
{
	if (scene == nullptr)
		return E_FAIL;

	m_iNumMaterials = scene->mNumMaterials;

	m_Materials.clear();
	m_Materials.resize(m_iNumMaterials);

	for (UINT i = 0; i < m_iNumMaterials; ++i)
	{
		aiMaterial* aiMat = scene->mMaterials[i];

		if (aiMat == nullptr)
			continue;

		auto pMaterial = CResModelMaterial::Create(m_sPath);

		if (pMaterial == nullptr)
			return E_FAIL;

		if (FAILED(pMaterial->LoadAssimp(aiMat, i)))
			return E_FAIL;

		m_Materials[pMaterial->GetMaterialTypeNum()] = pMaterial;
	}

	return S_OK;
}
