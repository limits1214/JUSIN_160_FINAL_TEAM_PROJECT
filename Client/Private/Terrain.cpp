#include "pch.h"
#include "Terrain.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "Resources.h"
#include "GameInstance.h"

#include "ComPxRigidBody.h"
#include "ComPxTriMeshCollider.h"
#include "Level_Defines.h"
#include "ComPxHeightFieldCollider.h"

NS_USING(Client)

CTerrain::CTerrain()
	: CGameObject{}
{
}

CTerrain::~CTerrain()
{
}

HRESULT CTerrain::InitializePrototype(void* pArg)
{
	
	m_pResTerrainVIBuffer = CGameInstance::Get().GetResourceFirst<CResTerrainVIBuffer>(LEVEL::TERRAIN, "VIBUFFER_Terrain");
	if (!m_pResTerrainVIBuffer)
	{
		return E_FAIL;
	}

	m_pResTerrainTexture2D = CGameInstance::Get().GetResourceFirst<CResTexture2D>(LEVEL::TERRAIN, "TEX2D_Terrain_Tile0");
	if (!m_pResTerrainTexture2D)
	{
		return E_FAIL;
	}

	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>("CLIENT_SHADER", "VS_VTX_NOR_TEX");
	if (FAILED(m_pResVertexShader->Load()))
	{
		return E_FAIL;
	}
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>("CLIENT_SHADER", "PS_VTX_NOR_TEX");
	if (FAILED(m_pResPixelShader->Load()))
	{
		return E_FAIL;
	}

	//if (FAILED(BuildPxRuntimeTriMesh()))
	//{
	//	MSG_BOX("Terrain BuildPxRuntimeTriMesh Failed");
	//	return E_FAIL;
	//}

	if (FAILED(BuildPxRuntimeHeightField()))
	{
		MSG_BOX("Terrain BuildPxRuntimeHeightField Failed");
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CTerrain::Initialize(void* pArg)
{
	m_RenderPassFlags = ETOUI(RENDERPASS::DEFAULT) | ETOUI(RENDERPASS::SHADOW);
	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		};
	}

	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::STATIC;
		if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxRigidBody", "ComPxRigidBody", &Desc, &m_pComPxRigidBody)))
		{
			return E_FAIL;
		};
	}


	//{
	//	CComPxTriMeshCollider::DESC Desc{};
	//	Desc.pComPxRigidBody = m_pComPxRigidBody;
	//	Desc.pResTriMesh = m_pResTriMesh;
	//	Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
	//	Desc.tFilter = PX_FILTER_DESC{
	//		.iLayer = ETOUI(COLLISION_LAYER::WORLD_STATIC),
	//		.iSimulationMask = PX_ALL_LAYERS,
	//		.iQueryMask = PX_ALL_LAYERS };
	//	if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxTriMeshCollider", "ComPxTriMeshCollider", &Desc, &m_pComPxTriMeshCollider)))
	//	{
	//		return E_FAIL;
	//	};
	//}

	{
		CComPxHeightFieldCollider::DESC Desc{};

		Desc.pComPxRigidBody = m_pComPxRigidBody;
		Desc.pResHeightField = m_pResHeightField;
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});

		Desc.fHeightScale = m_fPxHeightScale;
		Desc.fRowScale = m_fPxRowScale;
		Desc.fColumnScale = m_fPxColumnScale;
		Desc.vLocalOffset = m_vPxHeightFieldOffset;

		Desc.tFilter = PX_FILTER_DESC{
			.iLayer = ETOUI(COLLISION_LAYER::WORLD_STATIC),
			.iSimulationMask = PX_ALL_LAYERS,
			.iQueryMask = PX_ALL_LAYERS
		};

		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxHeightFieldCollider,
			"ComPxHeightFieldCollider",
			&Desc,
			&m_pComPxHeightFieldCollider)))
		{
			return E_FAIL;
		}
	}

	return S_OK;
}

void CTerrain::PriorityUpdate(E::_float fTimeDelta)
{
}

void CTerrain::Update(E::_float fTimeDelta)
{
}

void CTerrain::LateUpdate(E::_float fTimeDelta)
{
	GetTransform().Update();
	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
}

HRESULT CTerrain::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	{
		E::CB_PER_OBJECT cbPerObject{};
		cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
		XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
		if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))	return E_FAIL;

		pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	}
	const auto& vs = m_pResVertexShader;
	const auto& ps = m_pResPixelShader;

	const auto& viBuffer = m_pResTerrainVIBuffer;
	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);

	if (ctx.pass == RENDERPASS::DEFAULT) {			// 오류 메세지 ID3D11DeviceContext::DrawIndexed 제거용
		pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);
	}

	ID3D11Buffer* vertexBuffers[] = {
		viBuffer->GetVertexBuffer().Get()
	};
	uint32_t strides[] = {
		viBuffer->GetVertexStride()
	};
	uint32_t offsets[] = {
		0
	};
	pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
	pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

	SPtr<CResTexture2D> DiffuseTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_DIFFUSE");
	if (auto Resource = m_pResTerrainTexture2D) {
		DiffuseTexture = Resource;
	}
	pContext->PSSetShaderResources(0, 1, DiffuseTexture->GetSRV().GetAddressOf());

	auto MaterialConstantBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_MATERIAL");
	D3D11_MAPPED_SUBRESOURCE MRES;
	if (SUCCEEDED(pContext->Map(MaterialConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
	{
		CB_MATERIAL   CMMAT;
		CMMAT.EmissiveColor = _float3(1.f, 1.f, 1.f);
		CMMAT.EmissiveIntensity = 0.f;
		CMMAT.ObjectAlpha = 1.f;

		memcpy(MRES.pData, &CMMAT, sizeof(CB_MATERIAL));
		pContext->Unmap(MaterialConstantBuffer->GetCBuffer().Get(), 0);
	}
	pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::MATERIAL), 1, MaterialConstantBuffer->GetCBuffer().GetAddressOf());
	pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);

	ID3D11ShaderResourceView* pSRVs[1] = { nullptr };
	pContext->PSSetShaderResources(0, 1, pSRVs);
	pContext->PSSetShaderResources(1, 1, pSRVs);
	pContext->PSSetShaderResources(2, 1, pSRVs);
	pContext->PSSetShaderResources(3, 1, pSRVs);

	CGameInstance::Get().Reset_DefaultShader(RENDERGROUP::NONBLEND);
	// 오브젝트 렌더 할 떄, VSSetShader, PSSetShader 를 해야한다면,
	// 다시 원래 쉐이더로 돌려놓아야 이후에 렌더하는 오브젝트들이 정상적으로 렌더 됨.
	// 나중에 오브젝트들 정리해서 배칭으로 전환할 때 삭제 예정.

	return S_OK;
}

HRESULT CTerrain::BuildPxRuntimeTriMesh()
{
	const auto& v = m_pResTerrainVIBuffer->GetVertices();
	const auto& indices = m_pResTerrainVIBuffer->GetIndices();

	m_pResTriMesh = CResPhysXRTTriMeshGeometry::Create();
	const auto triMeshDesc = CResPhysXRTTriMeshGeometry::MakeDesc(
		v, indices, offsetof(VTX_NORMAL_TEX, pos));
	if (FAILED(m_pResTriMesh->Load(triMeshDesc)))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CTerrain::BuildPxRuntimeHeightField()
{
	using HEIGHT_FIELD_RES = CResPhysXRTHeightFieldGeometry;
	using SAMPLE = HEIGHT_FIELD_RES::SAMPLE;

	constexpr _float COORD_EPSILON = 1.e-4f;
	constexpr _float MIN_HEIGHT_SCALE = 1.e-6f;

	const auto& vertices = m_pResTerrainVIBuffer->GetVertices();
	const auto& indices = m_pResTerrainVIBuffer->GetIndices();

	if (vertices.size() < 4 || indices.size() < 6 ||
		indices.size() % 3 != 0)
	{
		DEBUG_LOG("[PX][TerrainHF] Invalid terrain mesh.\n");
		return E_FAIL;
	}

	/*
	 * 1. X축과 Z축의 고유 좌표를 구합니다.
	 *
	 * PhysX HeightField:
	 *   row    = local X
	 *   column = local Z
	 */
	std::vector<_float> xAxis{};
	std::vector<_float> zAxis{};

	xAxis.reserve(vertices.size());
	zAxis.reserve(vertices.size());

	for (const auto& vertex : vertices)
	{
		xAxis.push_back(vertex.pos.x);
		zAxis.push_back(vertex.pos.z);
	}

	const auto BuildUniformAxis =
		[COORD_EPSILON](
			std::vector<_float>& axis,
			_float& outScale) -> _bool
		{
			std::sort(axis.begin(), axis.end());

			axis.erase(
				std::unique(
					axis.begin(),
					axis.end(),
					[COORD_EPSILON](_float lhs, _float rhs)
					{
						return std::abs(lhs - rhs) <= COORD_EPSILON;
					}),
				axis.end());

			if (axis.size() < 2)
				return false;

			outScale = axis[1] - axis[0];
			if (outScale <= 0.f)
				return false;

			const _float tolerance =
				std::max(COORD_EPSILON, std::abs(outScale) * 1.e-3f);

			for (size_t i = 1; i < axis.size(); ++i)
			{
				const _float expected =
					axis[0] + static_cast<_float>(i) * outScale;

				if (std::abs(axis[i] - expected) > tolerance)
					return false;
			}

			return true;
		};

	if (!BuildUniformAxis(xAxis, m_fPxRowScale) ||
		!BuildUniformAxis(zAxis, m_fPxColumnScale))
	{
		DEBUG_LOG(
			"[PX][TerrainHF] Terrain vertices are not a uniform X/Z grid.\n");
		return E_FAIL;
	}

	if (xAxis.size() >
		static_cast<size_t>(std::numeric_limits<uint32_t>::max()) ||
		zAxis.size() >
		static_cast<size_t>(std::numeric_limits<uint32_t>::max()))
	{
		return E_FAIL;
	}

	const uint32_t rowCount =
		static_cast<uint32_t>(xAxis.size());

	const uint32_t columnCount =
		static_cast<uint32_t>(zAxis.size());

	const size_t sampleCount =
		static_cast<size_t>(rowCount) * columnCount;

	/*
	 * HeightField는 한 X/Z 좌표마다 버텍스가 하나씩 있어야 합니다.
	 * 렌더 메쉬가 UV seam 등의 이유로 버텍스를 중복했다면
	 * 이 조건이 실패합니다.
	 */
	if (sampleCount != vertices.size())
	{
		DEBUG_LOG(
			"[PX][TerrainHF] Vertex count does not match row * column.\n");
		return E_FAIL;
	}

	/*
	 * 2. HeightField sample index -> 렌더 버텍스 index 매핑
	 */
	constexpr uint32_t INVALID_INDEX =
		std::numeric_limits<uint32_t>::max();

	std::vector<uint32_t> gridToVertex(
		sampleCount,
		INVALID_INDEX);

	_float minHeight = std::numeric_limits<_float>::max();
	_float maxHeight = std::numeric_limits<_float>::lowest();

	const _float originX = xAxis.front();
	const _float originZ = zAxis.front();

	for (uint32_t vertexIndex = 0;
		vertexIndex < static_cast<uint32_t>(vertices.size());
		++vertexIndex)
	{
		const auto& position = vertices[vertexIndex].pos;

		const _float rowValue =
			(position.x - originX) / m_fPxRowScale;

		const _float columnValue =
			(position.z - originZ) / m_fPxColumnScale;

		const int64_t row =
			static_cast<int64_t>(std::llround(rowValue));

		const int64_t column =
			static_cast<int64_t>(std::llround(columnValue));

		if (row < 0 ||
			column < 0 ||
			row >= static_cast<int64_t>(rowCount) ||
			column >= static_cast<int64_t>(columnCount) ||
			std::abs(rowValue - static_cast<_float>(row)) > 1.e-3f ||
			std::abs(columnValue - static_cast<_float>(column)) > 1.e-3f)
		{
			DEBUG_LOG(
				"[PX][TerrainHF] Vertex is outside the uniform grid.\n");
			return E_FAIL;
		}

		const size_t sampleIndex =
			static_cast<size_t>(row) * columnCount +
			static_cast<size_t>(column);

		if (gridToVertex[sampleIndex] != INVALID_INDEX)
		{
			DEBUG_LOG(
				"[PX][TerrainHF] Duplicate X/Z terrain vertex.\n");
			return E_FAIL;
		}

		gridToVertex[sampleIndex] = vertexIndex;

		minHeight = std::min(minHeight, position.y);
		maxHeight = std::max(maxHeight, position.y);
	}

	for (const uint32_t vertexIndex : gridToVertex)
	{
		if (vertexIndex == INVALID_INDEX)
		{
			DEBUG_LOG(
				"[PX][TerrainHF] Missing terrain grid vertex.\n");
			return E_FAIL;
		}
	}

	/*
	 * 3. 실제 float 높이를 signed int16으로 양자화합니다.
	 *
	 * 실제 높이:
	 *   localOffset.y + sample.iHeight * heightScale
	 */
	const _float heightCenter =
		minHeight + (maxHeight - minHeight) * 0.5f;

	m_fPxHeightScale = std::max(
		(maxHeight - minHeight) / 65534.f,
		MIN_HEIGHT_SCALE);

	m_vPxHeightFieldOffset = {
		originX,
		heightCenter,
		originZ
	};

	std::vector<SAMPLE> samples(sampleCount);

	for (size_t sampleIndex = 0;
		sampleIndex < sampleCount;
		++sampleIndex)
	{
		const auto& position =
			vertices[gridToVertex[sampleIndex]].pos;

		const int64_t quantizedHeight =
			static_cast<int64_t>(std::llround(
				(position.y - heightCenter) /
				m_fPxHeightScale));

		auto& sample = samples[sampleIndex];

		sample.iHeight = static_cast<int16_t>(
			std::clamp<int64_t>(
				quantizedHeight,
				std::numeric_limits<int16_t>::min(),
				std::numeric_limits<int16_t>::max()));

		// 현재 Shape에 재질을 하나만 전달하므로 0번 재질 사용
		sample.iMaterialIndex0 = 0;
		sample.iMaterialIndex1 = 0;
		sample.bTessFlag = false;
	}

	/*
	 * 4. 렌더 메쉬 인덱스에서 각 사각형의 대각선을 구합니다.
	 *
	 * tessFlag == true:
	 *   (row, column) → (row + 1, column + 1)
	 *
	 * tessFlag == false:
	 *   나머지 두 꼭짓점 사이를 연결
	 */
	const auto MakeEdgeKey =
		[](uint32_t lhs, uint32_t rhs) -> uint64_t
		{
			if (lhs > rhs)
				std::swap(lhs, rhs);

			return
				(static_cast<uint64_t>(lhs) << 32) |
				static_cast<uint64_t>(rhs);
		};

	std::unordered_set<uint64_t> meshEdges{};
	meshEdges.reserve(indices.size());

	for (size_t i = 0; i < indices.size(); i += 3)
	{
		const uint32_t i0 = static_cast<uint32_t>(indices[i]);
		const uint32_t i1 = static_cast<uint32_t>(indices[i + 1]);
		const uint32_t i2 = static_cast<uint32_t>(indices[i + 2]);

		if (i0 >= vertices.size() ||
			i1 >= vertices.size() ||
			i2 >= vertices.size())
		{
			DEBUG_LOG(
				"[PX][TerrainHF] Terrain index is out of range.\n");
			return E_FAIL;
		}

		meshEdges.insert(MakeEdgeKey(i0, i1));
		meshEdges.insert(MakeEdgeKey(i1, i2));
		meshEdges.insert(MakeEdgeKey(i2, i0));
	}

	for (uint32_t row = 0; row + 1 < rowCount; ++row)
	{
		for (uint32_t column = 0;
			column + 1 < columnCount;
			++column)
		{
			const size_t sample00 =
				static_cast<size_t>(row) * columnCount + column;

			const size_t sample10 =
				static_cast<size_t>(row + 1) * columnCount + column;

			const size_t sample01 =
				static_cast<size_t>(row) * columnCount + column + 1;

			const size_t sample11 =
				static_cast<size_t>(row + 1) * columnCount + column + 1;

			const uint32_t vertex00 = gridToVertex[sample00];
			const uint32_t vertex10 = gridToVertex[sample10];
			const uint32_t vertex01 = gridToVertex[sample01];
			const uint32_t vertex11 = gridToVertex[sample11];

			const _bool hasCurrentToOpposite =
				meshEdges.find(
					MakeEdgeKey(vertex00, vertex11)) !=
				meshEdges.end();

			const _bool hasOtherDiagonal =
				meshEdges.find(
					MakeEdgeKey(vertex10, vertex01)) !=
				meshEdges.end();

			/*
			 * 정상적인 grid triangle mesh라면
			 * 두 대각선 중 정확히 하나만 존재해야 합니다.
			 */
			if (hasCurrentToOpposite == hasOtherDiagonal)
			{
				DEBUG_LOG(
					"[PX][TerrainHF] Could not determine cell tessellation.\n");
				return E_FAIL;
			}

			samples[sample00].bTessFlag =
				hasCurrentToOpposite;
		}
	}

	/*
	 * 5. 런타임 쿠킹 및 PxHeightField 생성
	 */
	const auto heightFieldDesc =
		HEIGHT_FIELD_RES::MakeDesc(
			samples,
			rowCount,
			columnCount,
			0.f,
			false);

	m_pResHeightField =
		HEIGHT_FIELD_RES::CreateAndLoad(heightFieldDesc);

	if (!m_pResHeightField)
	{
		DEBUG_LOG(
			"[PX][TerrainHF] Failed to create HeightField resource.\n");
		return E_FAIL;
	}

	DEBUG_LOG(
		"[PX][TerrainHF] HeightField created successfully.\n");

	return S_OK;
}

E::UPtr<CTerrain> CTerrain::Create()
{
	auto pInstance = E::ToUPtr(new CTerrain{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTerrain");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTerrain::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTerrain{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTerrain");
		return nullptr;
	}

	return pInstance;
}
