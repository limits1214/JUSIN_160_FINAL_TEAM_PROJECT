#include "pch.h"
#include "Particle_GPU.h"
#include "GameInstance.h"
#include "Resources.h"
#include "Particle_CPU.h"

NS_USING(Engine)

CParticle_GPU::CParticle_GPU()
{
}



CParticle_GPU::~CParticle_GPU()
{
}

HRESULT CParticle_GPU::Initialize(void* pArg)
{
    auto context = CGameInstance::Get().GetGraphicDeviceContext();

    auto pDesc = static_cast<DESC*>(pArg);
    if (pDesc == nullptr)
        return E_FAIL;

    m_Desc = *pDesc;


	m_pParticleShaderCache = pDesc->pShaderCache;

	if (!m_pParticleShaderCache)
		return E_FAIL;


    m_iNumElements = m_Desc.iMaxParticles;

    // 파티클을 다 죽은 상태로 초기화
    std::vector<PARTICLE> initParticles(m_iNumElements);
    for (uint32_t i = 0; i < m_iNumElements; i++)
    {
        initParticles[i].position = _float3(0.f, 0.f, 0.f);
        initParticles[i].velocity = _float3(0.f, 0.f, 0.f);
        initParticles[i].life = 0.f;
        initParticles[i].maxLife = 0.f;
		initParticles[i].size = _float3(0.f, 0.f, 0.f);
		initParticles[i].startSize = _float3(0.f, 0.f, 0.f);
		initParticles[i].endSize = _float3(0.f, 0.f, 0.f);
		initParticles[i].rotation = { 0,0,0,0 };
		initParticles[i].color = _float4(1.f, 1.f, 1.f, 0.f);
		initParticles[i].alive = false;
		initParticles[i].loop = false;
		initParticles[i].emissive = { 0,0,0,0 };
		initParticles[i].endEmissive = { 0,0,0,0 };
		initParticles[i].frameIndex = 0;
		initParticles[i].iBehaviorType = 0;
		initParticles[i].originalPosition = _float3(0.f, 0.f, 0.f);
		initParticles[i].originalEmissive = _float4(0.f, 0.f, 0.f,0.f);
		initParticles[i].fStopSizeTime = 0;
	}

	std::vector<uint32_t> initDeadIndices(m_iNumElements);
	for (uint32_t i = 0; i < m_iNumElements; i++)
		initDeadIndices[i] = i;


	if (auto res = CResCBuffer::Create())
	{
		CResCBuffer::CBUFFER_DESC bufDesc{};
		bufDesc.byteWidth = sizeof(CB_INIT_PARTICLE);
		if (FAILED(res->Load(bufDesc)))
			return E_FAIL;
		m_pComInitCBuffer = res;
	}

	// 파티클 구조체 버퍼
	if (auto res = CResStructuredBuffer::Create())
	{
		CResStructuredBuffer::DESC bufDesc{};
		bufDesc.iNumElements = m_iNumElements;
		bufDesc.iStructureByteStride = sizeof(PARTICLE);
		bufDesc.pInitialData = initParticles.data();
		bufDesc.bAppendConsume = false;
		if (FAILED(res->Load(bufDesc)))
			return E_FAIL;
		m_pParticleStructuredBuffer = res;
	}


	// 죽은 파티클 인덱스 버퍼
	if (auto res = CResStructuredBuffer::Create())
	{
		CResStructuredBuffer::DESC bufDesc{};
		bufDesc.iNumElements = m_iNumElements;
		bufDesc.iStructureByteStride = sizeof(uint32_t);
		bufDesc.pInitialData = initDeadIndices.data();
		bufDesc.bAppendConsume = false;
		if (FAILED(res->Load(bufDesc)))
			return E_FAIL;
		m_pDeadListBuffer = res;
	}
	if (auto res = CResStructuredBuffer::Create())
	{
		uint32_t initialDeadCount = m_iNumElements;

		CResStructuredBuffer::DESC desc{};
		desc.iNumElements = 1;
		desc.iStructureByteStride = sizeof(uint32_t);
		desc.pInitialData = &initialDeadCount;
		desc.bAppendConsume = false;

		if (FAILED(res->Load(desc)))
			return E_FAIL;

		m_pDeadCountBuffer = res;
	}

	// 스폰 데이터 버퍼
	if (auto res = CResStructuredBuffer::Create())
	{
		std::vector<PARTICLE_SPAWN_DATA> initSpawnData(m_iNumElements);

		CResStructuredBuffer::DESC bufDesc{};
		bufDesc.iNumElements = m_iNumElements;
		bufDesc.iStructureByteStride = sizeof(PARTICLE_SPAWN_DATA);
		bufDesc.pInitialData = initSpawnData.data();
		bufDesc.bAppendConsume = false;
		if (FAILED(res->Load(bufDesc)))
			return E_FAIL;
		m_pSpawnListBuffer = res;
	}

	if (auto res = CResCBuffer::Create())
	{
		CResCBuffer::CBUFFER_DESC bufDesc{};
		bufDesc.byteWidth = sizeof(CB_PER_PARTICLE);
		if (FAILED(res->Load(bufDesc)))
			return E_FAIL;
		m_pComCBuffer = res;
	}


	{

		if (auto res = CResCBuffer::Create())
		{
			CResCBuffer::CBUFFER_DESC bufDesc{};
			bufDesc.byteWidth = sizeof(CB_PARTICLE_SPAWN);
			if (FAILED(res->Load(bufDesc)))
				return E_FAIL;
			m_pComSpawnCBuffer = res;
		}
	}

	switch (m_Desc.blendState) {
	case 0:
		m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_EFFECT");
		break;
	case 1:
		m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ADDITIVE");
		break;
	case 2:
		m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_BLEND_NONE");
		break;
	default:
		m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_EFFECT");
		break;
	}


	m_pResClearByOwnerCS = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_ClearByOwner");
	m_pResTransformOwnerCS = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_TransformOwner");
	m_pResChangeColorByOwnerCS = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_ChangeColorByOwner");
	//if (FAILED(m_pResClearByOwnerCS->Load()))
	//	return E_FAIL;

	if (auto res = CResCBuffer::Create())
	{
		CResCBuffer::CBUFFER_DESC bufDesc{};
		bufDesc.byteWidth = sizeof(CB_OWNER_OPERATION);
		if (FAILED(res->Load(bufDesc)))
			return E_FAIL;
		m_pComOwnerOperationCBuffer = res;
	}

	{
		m_pResVertexShader = m_pParticleShaderCache->GetVertexShader(pDesc->VSID.first, pDesc->VSID.second, m_Desc.sVEntryPoint);
		if (!m_pResVertexShader)
			return E_FAIL;
		m_pResPixelShader = m_pParticleShaderCache->GetPixelShader(pDesc->PSID.first, pDesc->PSID.second, m_Desc.sPEntryPoint);
		if (!m_pResPixelShader)
			return E_FAIL;
	}
	
	if (m_Desc.distortionTextureID.first != "") {
		m_pDistortionTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.distortionTextureID.first, m_Desc.distortionTextureID.second);
	}
	if (m_Desc.anyTextureID.second != "") {
		m_pAnyTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.anyTextureID.first, m_Desc.anyTextureID.second);
	}


    if (m_Desc.whatKind == MESHORTEXTURE::TEX) {


		if (m_Desc.normalTextureID.first != "") {
			m_pNormalTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.normalTextureID.first, m_Desc.normalTextureID.second);
		}
	
		if (m_Desc.noiseTextureID.first != "") {
			m_pNoiseTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.noiseTextureID.first, m_Desc.noiseTextureID.second);
		}

        if (FAILED(LoadParticleTexture(m_Desc.textureID)))
            return E_FAIL;

    }
    else if (m_Desc.whatKind == MESHORTEXTURE::MESH) {
		if (m_pNoiseTexture) {
			m_pNoiseTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.noiseTextureID.first, m_Desc.noiseTextureID.second);
		}
		else {
			m_pNoiseTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>("SAMPLE_CLINET_TEXTURE", "TEX_NOISE");

		}
		if (m_Desc.hdrPositionTextureID.first != "") {
			m_pHdrPositionTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.hdrPositionTextureID.first, m_Desc.hdrPositionTextureID.second);
		}
		if (m_Desc.hdrNormalTextureID.first != "") {
			m_pHdrNormalTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.hdrNormalTextureID.first, m_Desc.hdrNormalTextureID.second);
		}
        // 모델 인스턴스는 컴포넌트 프로토타입 clone이 필요하다면 아래처럼
        // (AddComponentFromProto 대신, GameObject 없이도 쓸 수 있는 형태로)
        {

            CComStaticModelInstance::DESC Desc{};
            Desc.sGroupTag = m_Desc.sGroupTag;
            Desc.sResTag = m_Desc.sResTag;
            //Desc.sGroupTag = "TEST";
            //Desc.sResTag = "Static_Model_Resource";

            auto pProto = CGameInstance::Get().ClonePrototype("PERMANENT", "Prototype_Component_StaticModelInstance", &Desc);
            if (pProto == nullptr)
                return E_FAIL;
            m_pComModelInstance = UPtr<CComStaticModelInstance>(static_cast<CComStaticModelInstance*>(pProto.release()));

            if (!m_pComModelInstance)
                return E_FAIL;

        }
		if (m_pHdrPositionTexture && m_pComModelInstance)
		{
			auto pModel = m_pComModelInstance->GetModel();
			if (pModel && pModel->Get_NumMeshes() > 0)
			{
				uint32_t iMeshVertexCount = pModel->GetMeshes()[0]->GetNumVertices();

				auto pTex = m_pHdrPositionTexture->GetTexture(); // ID3D11Texture2D 접근자 — 이름 확인 필요
				if (pTex)
				{
					D3D11_TEXTURE2D_DESC texDesc{};
					pTex->GetDesc(&texDesc);
					{
						char buf[256];
						sprintf_s(buf,
							"Width\n",
							texDesc.Width, iMeshVertexCount,
							m_Desc.sGroupTag.GetDbgStr(), m_Desc.sResTag.GetDbgStr());
						OutputDebugStringA(buf);
					}
					{
						char buf[256];
						sprintf_s(buf,
							"Height\n",
							texDesc.Height, iMeshVertexCount,
							m_Desc.sGroupTag.GetDbgStr(), m_Desc.sResTag.GetDbgStr());
						OutputDebugStringA(buf);
					}
				
					if (texDesc.Width != iMeshVertexCount)
					{
						char buf[256];
						sprintf_s(buf,
							"[VAT 경고] HdrPosition 텍스처 폭(%u)이 메쉬 정점 개수(%u)와 다릅니다! GroupTag=%s ResTag=%s\n",
							texDesc.Width, iMeshVertexCount,
							m_Desc.sGroupTag.GetDbgStr(), m_Desc.sResTag.GetDbgStr());
						OutputDebugStringA(buf);
					}
				}
			}
		}
		if (m_pHdrNormalTexture && m_pComModelInstance)
		{
			auto pModel = m_pComModelInstance->GetModel();
			if (pModel && pModel->Get_NumMeshes() > 0)
			{
				uint32_t iMeshVertexCount = pModel->GetMeshes()[0]->GetNumVertices();

				auto pTex = m_pHdrNormalTexture->GetTexture(); // ID3D11Texture2D 접근자 — 이름 확인 필요
				if (pTex)
				{
					D3D11_TEXTURE2D_DESC texDesc{};
					pTex->GetDesc(&texDesc);
					{
						char buf[256];
						sprintf_s(buf,
							"Width\n",
							texDesc.Width, iMeshVertexCount,
							m_Desc.sGroupTag.GetDbgStr(), m_Desc.sResTag.GetDbgStr());
						OutputDebugStringA(buf);
					}
					{
						char buf[256];
						sprintf_s(buf,
							"Height\n",
							texDesc.Height, iMeshVertexCount,
							m_Desc.sGroupTag.GetDbgStr(), m_Desc.sResTag.GetDbgStr());
						OutputDebugStringA(buf);
					}
				
					if (texDesc.Width != iMeshVertexCount)
					{
						char buf[256];
						sprintf_s(buf,
							"[VAT 경고] HdrPosition 텍스처 폭(%u)이 메쉬 정점 개수(%u)와 다릅니다! GroupTag=%s ResTag=%s\n",
							texDesc.Width, iMeshVertexCount,
							m_Desc.sGroupTag.GetDbgStr(), m_Desc.sResTag.GetDbgStr());
						OutputDebugStringA(buf);
					}
				}
			}
		}
    }


 



    // 셰이더 3종(VS/PS) + CS 3종은 모든 GPU 파티클이 공유하는 범용 파이프라인이라 고정


    m_pResUpdateComputeShader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_UpdateParticle");
    if (FAILED(m_pResUpdateComputeShader->Load()))
        return E_FAIL;

    m_pResSpawnComputeShader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_SpawnParticle");
    if (FAILED(m_pResSpawnComputeShader->Load()))
        return E_FAIL;



    m_pResSamplerState = CGameInstance::Get().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
    if (!m_pResSamplerState)
        return E_FAIL;


    //초기화 버퍼 초기화
    {
        CB_INIT_PARTICLE cb{};
        cb.g_iMaxParticles = m_iNumElements;

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(context->Map(m_pComInitCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, &cb, sizeof(cb));
            context->Unmap(m_pComInitCBuffer->GetCBuffer().Get(), 0);
            context->CSSetConstantBuffers(10, 1, m_pComInitCBuffer->GetCBuffer().GetAddressOf());
        }
    }




    return S_OK;
}


void CParticle_GPU::PriorityUpdate(E::_float fTimeDelta)
{
}

void CParticle_GPU::Update(E::_float fTimeDelta)
{

    UINT initialCounts[] = { (UINT)-1, (UINT)-1 };
    ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr,nullptr };


	auto pContext = CGameInstance::Get().GetGraphicDeviceContext();
	m_fTime += fTimeDelta;

    // 1. 스폰
    if (m_iCurrentSpawnCount > 0)
    {

        pContext->CSSetConstantBuffers(12, 1, m_pComSpawnCBuffer->GetCBuffer().GetAddressOf());

        ID3D11ShaderResourceView* spawnSRV = m_pSpawnListBuffer->GetSRV().Get();
        pContext->CSSetShaderResources(6, 1, &spawnSRV);

		ID3D11UnorderedAccessView* spawnUAVs[] = {
		  m_pDeadListBuffer->GetUAV().Get(),
		  m_pParticleStructuredBuffer->GetUAV().Get(),
		  m_pDeadCountBuffer->GetUAV().Get()
		};

        UINT spawnInitialCounts[] = { (UINT)-1, (UINT)-1 };
        pContext->CSSetUnorderedAccessViews(0, 3, spawnUAVs, spawnInitialCounts);

        pContext->CSSetShader(m_pResSpawnComputeShader->GetComputeShader().Get(), nullptr, 0);

        uint32_t spawnGroup = (m_iCurrentSpawnCount + 255) / 256;
        pContext->Dispatch(spawnGroup, 1, 1);

        ID3D11UnorderedAccessView* nullUAVs2[] = { nullptr, nullptr, nullptr };
        pContext->CSSetUnorderedAccessViews(0, 3, nullUAVs2, nullptr);

        ID3D11ShaderResourceView* nullSRV[] = { nullptr };
        pContext->CSSetShaderResources(6, 1, nullSRV);

        m_iCurrentSpawnCount = 0;
    }

    // 2. Update
    CB_PER_PARTICLE cb{};
    cb.g_fTimeDelta = fTimeDelta;
    cb.g_iNumInstances = m_iNumElements;
	cb.g_iFlipbookColumns = m_Desc.TexColumns;
	cb.g_iFlipbookRows = m_Desc.TexRows;
	cb.g_iTotalFrames = m_Desc.TexRows * m_Desc.TexColumns;
	cb.g_fTime = m_fTime;



    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(pContext->Map(m_pComCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, &cb, sizeof(cb));
            pContext->Unmap(m_pComCBuffer->GetCBuffer().Get(), 0);
            pContext->CSSetConstantBuffers(11, 1, m_pComCBuffer->GetCBuffer().GetAddressOf());
        }
    }
	
	

    ID3D11UnorderedAccessView* updateUAVs[] = {
        m_pDeadListBuffer->GetUAV().Get(),
        m_pParticleStructuredBuffer->GetUAV().Get(),
		m_pDeadCountBuffer->GetUAV().Get()
    };
    pContext->CSSetUnorderedAccessViews(0, 3, updateUAVs, initialCounts);

    pContext->CSSetShader(m_pResUpdateComputeShader->GetComputeShader().Get(), nullptr, 0);

    uint32_t groupX = (m_iNumElements + 255) / 256;
    pContext->Dispatch(groupX, 1, 1);

    pContext->CSSetUnorderedAccessViews(0, 3, nullUAVs, nullptr);
    pContext->CSSetShader(nullptr, nullptr, 0);
	ProcessPendingSpawns(fTimeDelta);
}

void CParticle_GPU::LateUpdate(E::_float fTimeDelta)
{
}

HRESULT CParticle_GPU::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{

	if (m_Desc.whatKind == MESHORTEXTURE::MESH) {
		if (FAILED(Render_Mesh(pContext, ctx))) {
			return E_FAIL;
		}
	}
	else {
		if(FAILED(Render_Texture(pContext, ctx)))
			return E_FAIL;

	}
	ID3D11Buffer* nullCBuffer[] = { nullptr, nullptr };
	pContext->CSSetConstantBuffers(11, 2, nullCBuffer);
	return S_OK;

}

HRESULT CParticle_GPU::Render_Mesh(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	auto Rasterizer = E::CGameInstance::GetConst().GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_BACKCULL);
	pContext->RSSetState(Rasterizer->GetRasterizerState().Get());

	pContext->OMSetBlendState(m_pBlendState->GetBlendState().Get(), nullptr, 0xffffffff);
	SPtr<CResDepthStencilState> DepthState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_DEPTHREAD");
	pContext->OMSetDepthStencilState(DepthState->GetDepthStencilState().Get(), 0);
    ID3D11ShaderResourceView* pParticleSRV = m_pParticleStructuredBuffer->GetSRV().Get();
    pContext->VSSetShaderResources(4, 1, &pParticleSRV);
	pContext->VSSetConstantBuffers(11, 1, m_pComCBuffer->GetCBuffer().GetAddressOf());
	pContext->PSSetConstantBuffers(11, 1, m_pComCBuffer->GetCBuffer().GetAddressOf());
    const auto& vs = m_pResVertexShader; // 인스턴싱용 신규 VS 필요
    const auto& ps = m_pResPixelShader;
	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);
	m_pComModelInstance->Bind_Materials(pContext, { 0.f, 0.f, 0.f }, 0.f, { 0.f, 0.f, 0.f }, 0.f, 1.f);



	if (m_pHdrPositionTexture) {
		ID3D11ShaderResourceView* pHdrSRV = m_pHdrPositionTexture->GetSRV().Get();
		pContext->VSSetShaderResources(10, 1, &pHdrSRV);
	}

	if (m_pHdrNormalTexture) {
		ID3D11ShaderResourceView* pHdrSRV = m_pHdrNormalTexture->GetSRV().Get();
		pContext->VSSetShaderResources(11, 1, &pHdrSRV);
	}

	if (m_pNoiseTexture)
	{
		ID3D11ShaderResourceView* pNoiseSRV = m_pNoiseTexture->GetSRV().Get();
		pContext->PSSetShaderResources(5, 1, &pNoiseSRV);
		pContext->VSSetShaderResources(5, 1, &pNoiseSRV);
	}

	if (m_pDistortionTexture)
	{
		ID3D11ShaderResourceView* pDistortionSRV = m_pDistortionTexture->GetSRV().Get();
		pContext->PSSetShaderResources(6, 1, &pDistortionSRV);
	}
	if (m_pAnyTexture)
	{
		ID3D11ShaderResourceView* pAnyTextureSRV = m_pAnyTexture->GetSRV().Get();
		pContext->PSSetShaderResources(8, 1, &pAnyTextureSRV);
	}
    auto pModel = m_pComModelInstance->GetModel();
    uint32_t iNumMeshes = pModel->Get_NumMeshes();

    for (uint32_t i = 0; i < iNumMeshes; ++i)
    {
        const auto& viBuffer = pModel->GetMeshes()[i];

        ID3D11Buffer* vertexBuffers[] = { viBuffer->GetVertexBuffer().Get() };
        uint32_t strides[] = { viBuffer->GetVertexStride() };
        uint32_t offsets[] = { 0 };
        pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
        pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
        pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		SPtr<CResTexture2D> DiffuseTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_DIFFUSE");
		if (auto Resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_DIFFUSE, 0)) {
			DiffuseTexture = Resource;
		}
		pContext->PSSetShaderResources(0, 1, DiffuseTexture->GetSRV().GetAddressOf());
		SPtr<CResTexture2D> NormalTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_NORMAL");
		if (auto Resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_NORMALS, 0)) {
			NormalTexture = Resource;
		}
		pContext->PSSetShaderResources(1, 1, NormalTexture->GetSRV().GetAddressOf());

		SPtr<CResTexture2D> SMROTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_SMRO");
		if (auto Resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_METALNESS, 0)) {
			SMROTexture = Resource;
		}
		pContext->PSSetShaderResources(2, 1, SMROTexture->GetSRV().GetAddressOf());

		SPtr<CResTexture2D> EmissiveTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_EMISSIVE");
		if (auto Resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_EMISSIVE, 0)) {
			EmissiveTexture = Resource;
		}
		pContext->PSSetShaderResources(3, 1, EmissiveTexture->GetSRV().GetAddressOf());

        // 핵심: DrawIndexed → DrawIndexedInstanced
        pContext->DrawIndexedInstanced(viBuffer->GetNumIndices(), m_iNumElements, 0, 0, 0);
    }

	ID3D11ShaderResourceView* pSRVs[1] = { nullptr };
	pContext->PSSetShaderResources(0, 1, pSRVs);
	pContext->PSSetShaderResources(1, 1, pSRVs);
	pContext->PSSetShaderResources(2, 1, pSRVs);
	pContext->PSSetShaderResources(3, 1, pSRVs);
	pContext->PSSetShaderResources(5, 1, pSRVs);
	pContext->PSSetShaderResources(6, 1, pSRVs);
	pContext->PSSetShaderResources(8, 1, pSRVs);
	pContext->VSSetShaderResources(10, 1, pSRVs);
	pContext->VSSetShaderResources(11, 1, pSRVs);
	ID3D11Buffer* nullCB[] = { nullptr };

	pContext->VSSetConstantBuffers(11, 1, nullCB);
	pContext->PSSetConstantBuffers(11, 1, nullCB);
	{
		ID3D11ShaderResourceView* nullSRV[] = { nullptr };
		pContext->VSSetShaderResources(4, 1, nullSRV);
		pContext->VSSetShaderResources(5, 1, nullSRV);
	}
	pContext->OMSetDepthStencilState(nullptr, 0);

	pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

    return S_OK;
}

HRESULT CParticle_GPU::Render_Texture(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{


	auto Rasterizer = E::CGameInstance::GetConst().GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
	pContext->RSSetState(Rasterizer->GetRasterizerState().Get());
	SPtr<CResDepthStencilState> DepthState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_ALPHA_BLEND_DEPTH");
	pContext->OMSetDepthStencilState(DepthState->GetDepthStencilState().Get(), 0);
	pContext->OMSetBlendState(m_pBlendState->GetBlendState().Get(), nullptr, 0xffffffff);

	pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

	pContext->IASetInputLayout(nullptr);
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	ID3D11ShaderResourceView* pSRV = m_pParticleStructuredBuffer->GetSRV().Get();
	pContext->VSSetShaderResources(0, 1, &pSRV);
	pContext->VSSetConstantBuffers(11, 1, m_pComCBuffer->GetCBuffer().GetAddressOf());
	pContext->PSSetConstantBuffers(11, 1, m_pComCBuffer->GetCBuffer().GetAddressOf());


	SPtr<CResTexture2D> DiffuseTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_DIFFUSE");
	if (m_pParticleTexture) {
		DiffuseTexture = m_pParticleTexture;
	}
	pContext->PSSetShaderResources(1, 1, DiffuseTexture->GetSRV().GetAddressOf());
	SPtr<CResTexture2D> NormalTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_NORMAL");
	if (m_pNormalTexture) {
		NormalTexture = m_pNormalTexture;
	}
	pContext->PSSetShaderResources(2, 1, NormalTexture->GetSRV().GetAddressOf());

	if (m_pDistortionTexture)
	{
		ID3D11ShaderResourceView* pDistortionSRV = m_pDistortionTexture->GetSRV().Get();
		pContext->PSSetShaderResources(3, 1, &pDistortionSRV);
	}
	if (m_pNoiseTexture)
	{
		ID3D11ShaderResourceView* pNoiseSRV = m_pNoiseTexture->GetSRV().Get();
		pContext->PSSetShaderResources(4, 1, &pNoiseSRV);
	}
	if (m_pAnyTexture)
	{
		ID3D11ShaderResourceView* pAnySRV = m_pAnyTexture->GetSRV().Get();
		pContext->PSSetShaderResources(8, 1, &pAnySRV);

	}
	pContext->DrawInstanced(4, m_iNumElements, 0, 0);

	ID3D11ShaderResourceView* nullSRV[] = { nullptr };
    ID3D11ShaderResourceView* nullSRV1[] = { nullptr, nullptr };
    ID3D11ShaderResourceView* nullSRV2[] = { nullptr,nullptr ,nullptr,nullptr,nullptr,nullptr};
    pContext->VSSetShaderResources(0, 2, nullSRV1);
    pContext->PSSetShaderResources(0, 6, nullSRV2);
    pContext->PSSetShaderResources(8, 1, nullSRV);

	ID3D11Buffer* nullCB[] = { nullptr };
	pContext->VSSetConstantBuffers(11, 1, nullCB);
	pContext->PSSetConstantBuffers(11, 1, nullCB);
	pContext->OMSetDepthStencilState(nullptr, 0);

	pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);


    return S_OK;
}


HRESULT CParticle_GPU::Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData)
{
	if (pSpawnData == nullptr || count == 0)
		return E_FAIL;

	if (m_iCurrentSpawnCount != 0)
		return E_FAIL;


	count = std::min(count, m_iNumElements);

	if (count == 0)
		return E_FAIL;

	auto context = CGameInstance::Get().GetGraphicDeviceContext();

	std::vector<PARTICLE_SPAWN_DATA> fullData(m_iNumElements);
	memcpy(fullData.data(), pSpawnData, sizeof(PARTICLE_SPAWN_DATA) * count);

	context->UpdateSubresource(
		m_pSpawnListBuffer->GetBuffer().Get(),
		0,
		nullptr,
		fullData.data(),
		0,
		0);

	CB_PARTICLE_SPAWN scb{};
	scb.g_iSpawnCount = count;
	scb.g_iMaxParticles = m_iNumElements;

	D3D11_MAPPED_SUBRESOURCE mapped{};

	if (FAILED(context->Map(
		m_pComSpawnCBuffer->GetCBuffer().Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mapped)))
	{
		return E_FAIL;
	}

	memcpy(mapped.pData, &scb, sizeof(scb));

	context->Unmap(
		m_pComSpawnCBuffer->GetCBuffer().Get(),
		0);

	m_iCurrentSpawnCount = count;

	return S_OK;
}

UPtr<CParticle> CParticle_GPU::Create(void* pArg)
{
	auto pInstance = E::ToUPtr(new CParticle_GPU{});
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Created : CParticle_GPU");
		return nullptr;
	}
	return  pInstance;
}
void CParticle_GPU::ClearByOwner(uint32_t ownerID)
{
	auto context = CGameInstance::Get().GetGraphicDeviceContext();

	CB_OWNER_OPERATION cb{};
	cb.iTargetOwnerID = ownerID;
	cb.iMaxParticles = m_iNumElements;
	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (SUCCEEDED(context->Map(m_pComOwnerOperationCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
	{
		memcpy(mapped.pData, &cb, sizeof(cb));
		context->Unmap(m_pComOwnerOperationCBuffer->GetCBuffer().Get(), 0);
		context->CSSetConstantBuffers(13, 1, m_pComOwnerOperationCBuffer->GetCBuffer().GetAddressOf());
	}

	ID3D11UnorderedAccessView* clearUAVs[] = {
		m_pParticleStructuredBuffer->GetUAV().Get(),  // u1
		m_pDeadListBuffer->GetUAV().Get(),// u0
		m_pDeadCountBuffer->GetUAV().Get()
	};
	UINT initialCounts[] = { (UINT)-1, (UINT)-1 };
	context->CSSetUnorderedAccessViews(0, 3, clearUAVs, initialCounts);

	context->CSSetShader(m_pResClearByOwnerCS->GetComputeShader().Get(), nullptr, 0);

	uint32_t group = (m_iNumElements + 255) / 256;
	context->Dispatch(group, 1, 1);

	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr,nullptr };
	context->CSSetUnorderedAccessViews(0, 3, nullUAVs, nullptr);
	context->CSSetShader(nullptr, nullptr, 0);

	ID3D11Buffer* nullCB[] = { nullptr };
	context->CSSetConstantBuffers(13, 1, nullCB);

}
void CParticle_GPU::TranslateOwner(uint32_t ownerId,const _float3& delta)
{
	_float4x4 deltaMatrix{};

	XMStoreFloat4x4(&deltaMatrix,XMMatrixTranslation(delta.x,delta.y,delta.z));

	TransformOwner(ownerId, deltaMatrix);
}
void CParticle_GPU::TransformOwner(uint32_t ownerId, const _float4x4& deltaMatrixData)
{
	if (ownerId == INVALID_PARTICLE_OWNER_ID)
		return;

	auto context = CGameInstance::Get().GetGraphicDeviceContext();

	if (!context ||
		!m_pComOwnerOperationCBuffer ||
		!m_pResTransformOwnerCS ||
		!m_pParticleStructuredBuffer)
	{
		return;
	}

	CB_OWNER_OPERATION cb{};
	cb.iTargetOwnerID = ownerId;
	cb.iMaxParticles = m_iNumElements;
	cb.matDelta = deltaMatrixData;

	D3D11_MAPPED_SUBRESOURCE mapped{};

	if (FAILED(context->Map(
		m_pComOwnerOperationCBuffer->GetCBuffer().Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mapped)))
	{
		return;
	}

	memcpy(mapped.pData, &cb, sizeof(cb));

	context->Unmap(m_pComOwnerOperationCBuffer->GetCBuffer().Get(), 0);

	context->CSSetConstantBuffers(13,1,
		m_pComOwnerOperationCBuffer->GetCBuffer().GetAddressOf());

	ID3D11UnorderedAccessView* particleUAV = m_pParticleStructuredBuffer->GetUAV().Get();

	context->CSSetUnorderedAccessViews(0, 1, &particleUAV, nullptr);
	context->CSSetShader(m_pResTransformOwnerCS->GetComputeShader().Get(), nullptr, 0);

	const uint32_t groupCount = (m_iNumElements + 255) / 256;
	context->Dispatch(groupCount, 1, 1);

	ID3D11UnorderedAccessView* nullUAV = nullptr;
	context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

	ID3D11Buffer* nullCB = nullptr;
	context->CSSetConstantBuffers(13, 1, &nullCB);

	context->CSSetShader(nullptr, nullptr, 0);

	TransformPendingOwner(ownerId, deltaMatrixData);
}

void CParticle_GPU::SetColorByOwner(uint32_t ownerId, const _float4& color)
{
	if (ownerId == INVALID_PARTICLE_OWNER_ID)
		return;

	if (!m_pComOwnerOperationCBuffer ||
		!m_pResChangeColorByOwnerCS ||
		!m_pParticleStructuredBuffer)
	{
		return;
	}

	auto context = CGameInstance::Get().GetGraphicDeviceContext();

	if (!context)
		return;

	CB_OWNER_OPERATION cb{};
	cb.iTargetOwnerID = ownerId;
	cb.iMaxParticles = m_iNumElements;
	cb.vColor = color;

	D3D11_MAPPED_SUBRESOURCE mapped{};

	if (FAILED(context->Map(
		m_pComOwnerOperationCBuffer->GetCBuffer().Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mapped)))
	{
		return;
	}

	memcpy(mapped.pData, &cb, sizeof(cb));
	context->Unmap(m_pComOwnerOperationCBuffer->GetCBuffer().Get(), 0);

	ID3D11Buffer* ownerCBuffer =
		m_pComOwnerOperationCBuffer->GetCBuffer().Get();

	context->CSSetConstantBuffers(13, 1, &ownerCBuffer);

	ID3D11UnorderedAccessView* particleUAV =
		m_pParticleStructuredBuffer->GetUAV().Get();

	context->CSSetUnorderedAccessViews(0, 1, &particleUAV, nullptr);
	context->CSSetShader(m_pResChangeColorByOwnerCS->GetComputeShader().Get(), nullptr, 0);

	uint32_t groupCount = (m_iNumElements + 255) / 256;
	context->Dispatch(groupCount, 1, 1);

	ID3D11UnorderedAccessView* nullUAV = nullptr;
	ID3D11Buffer* nullCBuffer = nullptr;

	context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
	context->CSSetConstantBuffers(13, 1, &nullCBuffer);
	context->CSSetShader(nullptr, nullptr, 0);
}
