#include "pch.h"
#include "Trail_CPU.h"
#include "GameInstance.h"
#include "Resources.h"

CTrail_CPU::CTrail_CPU()
{
}



CTrail_CPU::~CTrail_CPU()
{
}

HRESULT CTrail_CPU::Initialize(void* pArg)
{
    // CTrail_CPU는 CParticle_CPU와 같은 역할 - 버퍼/셰이더/텍스처 로딩 같은 공통 처리만 하고,
    // 실제 값(desc)은 자식 클래스(CTrail_Example 등)가 채워서 넘겨준다.
    auto pDesc = static_cast<DESC*>(pArg);
    if (pDesc == nullptr)
        return E_FAIL;

	m_pParticleShaderCache = pDesc->pShaderCache;

	if (!m_pParticleShaderCache)
		return E_FAIL;

    m_Desc = *pDesc;

	if (!m_Desc.bShrinkWidth)
		m_Desc.eAlignMode = TRAIL_ALIGN_MODE::VIEW;


    // 프레임 하나당 정점 2개(밑동/칼끝) - 정점 자체가 이미 폭의 양 끝
    uint32_t iMaxVertices = m_Desc.iMaxFrames * 2;

    if (auto res = CResDynamicBuffer::Create())
    {
        CResDynamicBuffer::DESC bufDesc{};
        bufDesc.desc = {
            .ByteWidth = (uint32_t)sizeof(TRAIL_VERTEX) * iMaxVertices,
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
            .MiscFlags = 0,
            .StructureByteStride = 0,
        };

        if (FAILED(res->Load(bufDesc)))
            return E_FAIL;

        m_pResVertexBuffer = res;
    }

	if (auto res = CResCBuffer::Create())
	{
		CResCBuffer::CBUFFER_DESC bufDesc{};
		bufDesc.byteWidth = sizeof(CB_SCROLL);
		if (FAILED(res->Load(bufDesc)))
			return E_FAIL;
		m_pScrollCBuffer = res;
	}
	switch (m_Desc.blendState) {
	case 0:
		m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ADDITIVE");
		break;
	case 1:
		m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_EFFECT");
		break;
	case 2:
		m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_BLEND_NONE");
		break;
	default:
		m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_EFFECT");
		break;
	}

	if (m_Desc.normalTextureID.first != "") {
		m_pNormalTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.normalTextureID.first, m_Desc.normalTextureID.second);
	}
	if (m_Desc.distortionTextureID.first != "") {
		m_pDistortionTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.distortionTextureID.first, m_Desc.distortionTextureID.second);
	}
	if (m_Desc.noiseTextureID.first != "") {
		m_pNoiseTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.noiseTextureID.first, m_Desc.noiseTextureID.second);
	}
	if (m_Desc.anyTextureID.second != "") {
		m_pAnyTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.anyTextureID.first, m_Desc.anyTextureID.second);
	}

	{
		m_pResVertexShader = m_pParticleShaderCache->GetVertexShader(pDesc->VSID.first, pDesc->VSID.second, m_Desc.sVEntryPoint);
		if (!m_pResVertexShader)
			return E_FAIL;
		m_pResPixelShader = m_pParticleShaderCache->GetPixelShader(pDesc->PSID.first, pDesc->PSID.second, m_Desc.sPEntryPoint);
		if (!m_pResPixelShader)
			return E_FAIL;
	}


    if (FAILED(LoadParticleTexture(m_Desc.textureID)))
        return E_FAIL;

    m_vecVertices.reserve(iMaxVertices);
	diffuseFrames = m_Desc.TexRows * m_Desc.TexColumns;
    return S_OK;
}

void CTrail_CPU::PriorityUpdate(_float fTimeDelta)
{
}

void CTrail_CPU::Update(_float fTimeDelta)
{
	m_fAccumulationTime += fTimeDelta;

	if (diffuseFrames > 1)
	{
		const _float fFlipbookFPS = 24.f;
		uint32_t frameIndex = static_cast<uint32_t>(m_fAccumulationTime * fFlipbookFPS);
		currentFrame = frameIndex % diffuseFrames;
	}
	else
	{
		currentFrame = 0;
	}

	for (auto& trailFrame : m_dequeFrames)
		trailFrame.fAge += fTimeDelta;

	while (!m_dequeFrames.empty() && m_dequeFrames.back().fAge >= m_Desc.fMaxDuration)
		m_dequeFrames.pop_back();

	m_fTimeSinceLastAdd += fTimeDelta;
	m_fIdleTime += fTimeDelta;

	if (m_fIdleTime >= m_fIdleThreshold && !m_dequeFrames.empty())
	{
		m_fTimeSinceLastRetract += fTimeDelta;

		while (m_fTimeSinceLastRetract >= m_fRetractInterval && !m_dequeFrames.empty())
		{
			m_dequeFrames.pop_back();
			m_fTimeSinceLastRetract -= m_fRetractInterval;
		}
	}

	BuildTrailGeometry();
}
void CTrail_CPU::LateUpdate(_float fTimeDelta)
{
}

_float CTrail_CPU::DistanceSq(const _float3& a, const _float3& b)
{
	float x = a.x - b.x;
	float y = a.y - b.y;
	float z = a.z - b.z;

	return x * x + y * y + z * z;
}
void CTrail_CPU::AddPoint(const _float3& vStart, const _float3& vEnd)
{
	if (m_bHasLastPoint && m_fTimeSinceLastAdd < m_fSampleInterval)
		return;


	if (m_bHasLastPoint)
	{
		_float3 prevCenter = { (m_vLastStart.x + m_vLastEnd.x) * 0.5f,
								(m_vLastStart.y + m_vLastEnd.y) * 0.5f,
								(m_vLastStart.z + m_vLastEnd.z) * 0.5f };
		_float3 currCenter = { (vStart.x + vEnd.x) * 0.5f,
								(vStart.y + vEnd.y) * 0.5f,
								(vStart.z + vEnd.z) * 0.5f };
		const float trailWidth = sqrtf(DistanceSq(vStart, vEnd));
		const float minSampleDistance = std::max(0.02f, trailWidth * 0.1f);
		const float centerDistance = sqrtf(DistanceSq(prevCenter, currCenter));
		const float deltaX = currCenter.x - prevCenter.x;
		const float deltaZ = currCenter.z - prevCenter.z;
		const float horizontalDistance = sqrtf(deltaX * deltaX + deltaZ * deltaZ);
		const float maxConnectDistance = std::max(2.f, trailWidth * 2.f);

		if (centerDistance < minSampleDistance)
			return;

		if (horizontalDistance > maxConnectDistance)
		{
			Clear();
			m_fTotalDistance = 0.f;
		}
		else
		{
			m_fTotalDistance += centerDistance;
		}
	}
	m_bHasLastPoint = true;
	m_vLastStart = vStart;
	m_vLastEnd = vEnd;
	m_fTimeSinceLastAdd = 0.f;
	m_fIdleTime = 0.f;
	m_fTimeSinceLastRetract = 0.f;


	TRAIL_FRAME frame;
	frame.vStart = vStart;
	frame.vEnd = vEnd;
	frame.fAge = 0.f;
	frame.fDistance = m_fTotalDistance;
	if (m_Desc.eAlignMode == TRAIL_ALIGN_MODE::VIEW)
	{
		XMMATRIX view = CGameInstance::Get().GetActiveCamera()->GetView();
		XMMATRIX invView = XMMatrixInverse(nullptr, view);

		XMVECTOR camPos = invView.r[3];
		XMVECTOR camRight = XMVector3Normalize(invView.r[0]);

		XMVECTOR start = XMLoadFloat3(&vStart);
		XMVECTOR end = XMLoadFloat3(&vEnd);
		XMVECTOR mid = (start + end) * 0.5f;

		XMVECTOR viewDir = XMVector3Normalize(mid - camPos);

		XMVECTOR pathDir;

		if (!m_dequeFrames.empty())
		{
			const auto& prev = m_dequeFrames.front();

			XMVECTOR prevMid =
				(XMLoadFloat3(&prev.vStart) +
					XMLoadFloat3(&prev.vEnd)) * 0.5f;

			XMVECTOR delta = mid - prevMid;

			// 움직이지 않았으면 이전 WidthDir 유지
			if (XMVectorGetX(XMVector3LengthSq(delta)) < 1e-6f)
			{
				frame.vWidthDir = prev.vWidthDir;
			}
			else
			{
				pathDir = XMVector3Normalize(delta);

				XMVECTOR widthDir = XMVector3Cross(viewDir, pathDir);
				
				if (XMVectorGetX(XMVector3LengthSq(widthDir)) < 1e-6f)
				{
					widthDir = camRight;
				}
				else
				{
					widthDir = XMVector3Normalize(widthDir);
					XMVECTOR prevWidthDir = XMLoadFloat3(&prev.vWidthDir);

					if (XMVectorGetX(XMVector3LengthSq(prevWidthDir)) > 1e-6f &&
						XMVectorGetX(XMVector3Dot(prevWidthDir, widthDir)) < 0.f)
					{
						widthDir = -widthDir;
					}
				}
				
				XMStoreFloat3(&frame.vWidthDir, widthDir);
			}
		}
		else
		{
			frame.vWidthDir = { 1.f, 0.f, 0.f };
		}
	}
	else
	{
		frame.vWidthDir = { 0.f,0.f,0.f };
	}

	m_dequeFrames.push_front(frame);

	while (m_dequeFrames.size() > m_Desc.iMaxFrames)
		m_dequeFrames.pop_back();
}

void CTrail_CPU::Clear()
{
	m_dequeFrames.clear();
	m_vecVertices.clear();

	m_bHasLastPoint = false;
}

void CTrail_CPU::SetPosition(const _float3& pos)
{
}

void CTrail_CPU::SetVelocity(const _float3& vel)
{
}

void CTrail_CPU::SetSize(const _float3& size)
{
}

void CTrail_CPU::SetColor(const _float4& color)
{
	m_vColor.x = color.x;
	m_vColor.y = color.y;
	m_vColor.z = color.z;
	m_vColor.w = color.w;
}
void CTrail_CPU::SetEmissive(const _float4& emissive)
{
	m_vEmissive = emissive;
}



void CTrail_CPU::TranslateOwner(uint32_t ownerId, const _float3& delta)
{
}

void CTrail_CPU::TransformOwner(uint32_t ownerId, const _float4x4& deltaMatrixData)
{
}


HRESULT CTrail_CPU::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
    if (m_vecVertices.size() < 4) // 최소 프레임 2개(=4정점)는 있어야 스윕 면이 성립
        return S_OK;

	auto Rasterizer = E::CGameInstance::GetConst().GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
	pContext->RSSetState(Rasterizer->GetRasterizerState().Get());
	pContext->OMSetBlendState(m_pBlendState->GetBlendState().Get(), nullptr, 0xffffffff);

	auto depthState =
		CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE,"DS_NO_DEPTHWRITE");
	if (!depthState)
		return E_FAIL;

	pContext->OMSetDepthStencilState(depthState->GetDepthStencilState().Get(),0);


	//초기화 버퍼 초기화
	{
		CB_SCROLL cb{};
		cb.g_fScrollOffset = m_ScrollOffset;
		cb.g_fAccumulationTime = m_fAccumulationTime;
		cb.g_iCurrentFrame = currentFrame;
		cb.g_iFlipbookRows = m_Desc.TexRows;
		cb.g_iFlipbookColumns = m_Desc.TexColumns;
		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (SUCCEEDED(pContext->Map(m_pScrollCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			memcpy(mapped.pData, &cb, sizeof(cb));
			pContext->Unmap(m_pScrollCBuffer->GetCBuffer().Get(), 0);
			pContext->PSSetConstantBuffers(10, 1, m_pScrollCBuffer->GetCBuffer().GetAddressOf());
		}
	}


    pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
    pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
    pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

	const auto& rasterizer = CGameInstance::Get().GetResourceFirst<CResRasterizerState>(
		TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
	pContext->RSSetState(rasterizer->GetRasterizerState().Get());


    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(pContext->Map(m_pResVertexBuffer->GetBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            std::memcpy(mapped.pData, m_vecVertices.data(),
                sizeof(TRAIL_VERTEX) * m_vecVertices.size());
            pContext->Unmap(m_pResVertexBuffer->GetBuffer().Get(), 0);
        }
    }

    ID3D11Buffer* vertexBuffers[] = { m_pResVertexBuffer->GetBuffer().Get() };
    uint32_t strides[] = { sizeof(TRAIL_VERTEX) };
    uint32_t offsets[] = { 0 };

    pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    pContext->PSSetShaderResources(1, 1, m_pParticleTexture->GetSRV().GetAddressOf());


	{
		ID3D11ShaderResourceView* pNormalSRV = m_pNormalTexture ? m_pNormalTexture->GetSRV().Get() : nullptr;
		pContext->PSSetShaderResources(2, 1, &pNormalSRV);
	}
	{
		ID3D11ShaderResourceView* pDistortionSRV = m_pDistortionTexture ? m_pDistortionTexture->GetSRV().Get() : nullptr;
		pContext->PSSetShaderResources(3, 1, &pDistortionSRV);
	}
	{
		ID3D11ShaderResourceView* pNoiseSRV = m_pNoiseTexture ? m_pNoiseTexture->GetSRV().Get() : nullptr;
		pContext->PSSetShaderResources(4, 1, &pNoiseSRV);
	}
	if (m_pAnyTexture)
	{
		ID3D11ShaderResourceView* pAnyTextureSRV = m_pAnyTexture->GetSRV().Get();
		pContext->PSSetShaderResources(8, 1, &pAnyTextureSRV);
	}

    pContext->Draw((UINT)m_vecVertices.size(), 0);

    ID3D11ShaderResourceView* nullSRV[] = { nullptr,nullptr ,nullptr,nullptr,nullptr ,nullptr };
    ID3D11ShaderResourceView* nullSRV2[] = { nullptr};
    pContext->PSSetShaderResources(0, 6, nullSRV);
    pContext->PSSetShaderResources(8, 1, nullSRV2);

	ID3D11Buffer* nullCBuffer[] = { nullptr };
	pContext->PSSetConstantBuffers(10, 1, nullCBuffer);

	pContext->RSSetState(nullptr);
	pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	pContext->OMSetDepthStencilState(nullptr, 0);
    return S_OK;
}

HRESULT CTrail_CPU::Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData)
{
    return E_FAIL; // AddPoint(start, end)로 직접 제어 - 이 인터페이스는 안 씀
}

void CTrail_CPU::ClearByOwner(uint32_t ownerID)
{
}

void CTrail_CPU::BuildTrailGeometry()
{
	m_vecVertices.clear();

	const uint32_t iCount = static_cast<uint32_t>(m_dequeFrames.size());

	if (iCount < 2 || !m_pParticleTexture)
		return;

	const _bool bBillboard = !m_Desc.bShrinkWidth || m_Desc.eAlignMode == TRAIL_ALIGN_MODE::VIEW;
	const float newestDistance = m_dequeFrames.front().fDistance;
	const float oldestDistance = m_dequeFrames.back().fDistance;
	const float visibleDistance = std::max(newestDistance - oldestDistance, 0.001f);

	for (uint32_t i = 0; i < iCount; ++i)
	{
		const auto& frame = m_dequeFrames[i];

		float fAgeRatio = frame.fAge / m_Desc.fMaxDuration;
		float fDeath = powf(fAgeRatio, 1.2f);
		float fLifeRatio = 1.f - fDeath;
		float t = (frame.fDistance - oldestDistance) / visibleDistance;
		float fWidthScale = m_Desc.bShrinkWidth ? fLifeRatio : 1.f;
		_float3 vTip, vBase;

		if (!bBillboard)
		{
			_float3 vMid =
			{
				(frame.vStart.x + frame.vEnd.x) * 0.5f,
				(frame.vStart.y + frame.vEnd.y) * 0.5f,
				(frame.vStart.z + frame.vEnd.z) * 0.5f
			};

			vTip =
			{
				vMid.x + (frame.vEnd.x - vMid.x) * fWidthScale,
				vMid.y + (frame.vEnd.y - vMid.y) * fWidthScale,
				vMid.z + (frame.vEnd.z - vMid.z) * fWidthScale
			};

			vBase =
			{
				vMid.x + (frame.vStart.x - vMid.x) * fWidthScale,
				vMid.y + (frame.vStart.y - vMid.y) * fWidthScale,
				vMid.z + (frame.vStart.z - vMid.z) * fWidthScale
			};
		}
		else // VIEW
		{
			XMVECTOR vStartVec = XMLoadFloat3(&frame.vStart);
			XMVECTOR vEndVec = XMLoadFloat3(&frame.vEnd);

			XMVECTOR vMidVec = (vStartVec + vEndVec) * 0.5f;

			float fHalfWidth =
				XMVectorGetX(
					XMVector3Length(vEndVec - vStartVec)) * 0.5f;

			XMVECTOR vWidthDir =
				XMLoadFloat3(&frame.vWidthDir);

			float fScaledHalfWidth =
				fHalfWidth * fWidthScale;

			XMVECTOR vTipVec =
				vMidVec + vWidthDir * fScaledHalfWidth;

			XMVECTOR vBaseVec =
				vMidVec - vWidthDir * fScaledHalfWidth;

			XMStoreFloat3(&vTip, vTipVec);
			XMStoreFloat3(&vBase, vBaseVec);
		}

		TRAIL_VERTEX vTop{};
		vTop.vPosition = vTip;
		vTop.vUV = { t, 0.f };
		vTop.vEmissive = m_vEmissive;
		const float alpha = m_vColor.w * fLifeRatio;

		XMStoreFloat4(&vTop.vColor, XMVectorSetW(XMLoadFloat4(&m_vColor), alpha));
		//XMStoreFloat4(&vTop.vColor, XMVectorSetW(XMLoadFloat4(&m_vColor), 1.f * fLifeRatio));

		TRAIL_VERTEX vBottom{};
		vBottom.vPosition = vBase;
		vBottom.vUV = { t, 1.f };
		vBottom.vEmissive = m_vEmissive;
		XMStoreFloat4(&vBottom.vColor, XMVectorSetW(XMLoadFloat4(&m_vColor), alpha));

		//XMStoreFloat4(&vBottom.vColor, XMVectorSetW(XMLoadFloat4(&m_vColor),1.f *fLifeRatio));

		m_vecVertices.push_back(vTop);
		m_vecVertices.push_back(vBottom);
	}
}
UPtr<CParticle> CTrail_CPU::Create(void* pArg)
{
	auto pInstance = E::ToUPtr(new CTrail_CPU{});
	if (FAILED(pInstance->Initialize(pArg)))
	{	
		MSG_BOX("Failed to Created : CTrail_CPU");
		return nullptr;
	}
	return  pInstance;
}
// 1. Quad Ease-Out (2차) - 가장 흔하고 무난함
float CTrail_CPU::EaseOutQuad(float x)
{
	return 1.f - (1.f - x) * (1.f - x);
}

// 2. Cubic Ease-Out (3차) - 더 급격한 초반 변화
float CTrail_CPU::EaseOutCubic(float x)
{
	float t = 1.f - x;
	return 1.f - t * t * t;
}

// 3. 범용 (지수를 파라미터로) - Cubic까지 아래처럼 일반화 가능
float CTrail_CPU::EaseOutPow(float x, float n)
{
	return 1.f - pow(1.f - x, n); // n=2면 Quad, n=3이면 Cubic
}

// 4. Expo Ease-Out - 아주 극적인 초반 변화, 끝에서 아주 천천히 수렴
float CTrail_CPU::EaseOutExpo(float x)
{
	return (x >= 1.f) ? 1.f : 1.f - exp2(-10.f * x);
}

// 5. Sine Ease-Out - 부드럽고 완만함
float CTrail_CPU::EaseOutSine(float x)
{
	return sin(x * 3.14159265f * 0.5f);
}
