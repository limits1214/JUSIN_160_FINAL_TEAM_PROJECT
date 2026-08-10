#include "pch.h"
#include "Particle_CPU.h"
#include "GameInstance.h"
#include "Resources.h"

NS_USING(Engine)

CParticle_CPU::CParticle_CPU()
{
}



CParticle_CPU::~CParticle_CPU()
{
}

HRESULT CParticle_CPU::Initialize(void* pArg)
{
    m_vecInstancedData.clear();



    auto pDesc = static_cast<DESC*>(pArg);
    if (pDesc == nullptr)
        return E_FAIL;

	m_pParticleShaderCache = pDesc->pShaderCache;

	if (!m_pParticleShaderCache)
		return E_FAIL;

    m_Desc = *pDesc;
    m_iNumElements = m_Desc.iMaxParticles;
    m_viBufferID = m_Desc.viBufferID;

    m_Particles.assign(m_iNumElements, PARTICLE_CPU_DATA{});

    if (auto res = CResDynamicBuffer::Create())
    {
        CResDynamicBuffer::DESC bufDesc{};
        bufDesc.desc = {
            .ByteWidth = (uint32_t)sizeof(VTX_PARTICLE_INSTANCED_DATA) * m_iNumElements,
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
            .MiscFlags = 0,
            .StructureByteStride = 0,
        };

        if (FAILED(res->Load(bufDesc)))
            return E_FAIL;

        m_pResInstancedBuffer = res;
    }

	if (auto res = CResCBuffer::Create())
	{
		CResCBuffer::CBUFFER_DESC bufDesc{};
		bufDesc.byteWidth = sizeof(CB_TIMEACCUMULATION);
		if (FAILED(res->Load(bufDesc)))
			return E_FAIL;
		m_pComTimeCBuffer = res;
	}

    m_pResSamplerState = CGameInstance::Get().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
    if (!m_pResSamplerState)
        return E_FAIL;
	m_pNoiseTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>("SAMPLE_CLINET_TEXTURE", "TEX_NOISE");
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

	{
		m_pResVertexShader = m_pParticleShaderCache->GetVertexShader(pDesc->VSID.first, pDesc->VSID.second, m_Desc.sVEntryPoint);
		if (!m_pResVertexShader)
			return E_FAIL;
		m_pResPixelShader = m_pParticleShaderCache->GetPixelShader(pDesc->PSID.first, pDesc->PSID.second, m_Desc.sPEntryPoint);
		if (!m_pResPixelShader)
			return E_FAIL;
	}


    if (m_Desc.whatKind == MESHORTEXTURE::TEX) {
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
        if (FAILED(LoadParticleTexture(m_Desc.textureID)))
            return E_FAIL;

    }
    else if (m_Desc.whatKind == MESHORTEXTURE::MESH) {

		if (m_Desc.noiseTextureID.first != "") {
			m_pNoiseTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.noiseTextureID.first, m_Desc.noiseTextureID.second);
		}
        m_pComCBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT);
        if (!m_pComCBuffer)
            return E_FAIL;

        {
            CComStaticModelInstance::DESC modelDesc{};
            modelDesc.sGroupTag = m_Desc.sGroupTag;   // 밖에서 주입
            modelDesc.sResTag = m_Desc.sResTag;     // 밖에서 주입


            auto pProto = CGameInstance::Get().ClonePrototype("PERMANENT", "Prototype_Component_StaticModelInstance", &modelDesc);
            if (pProto == nullptr)
                return E_FAIL;
            m_pComModelInstance = UPtr<CComStaticModelInstance>(static_cast<CComStaticModelInstance*>(pProto.release()));

            if (!m_pComModelInstance)
                return E_FAIL;
        }
		if (m_Desc.hdrPositionTextureID.first != "") {
			m_pHdrPositionTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.hdrPositionTextureID.first, m_Desc.hdrPositionTextureID.second);
		}
		if (m_Desc.hdrNormalTextureID.first != "") {
			m_pHdrNormalTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.hdrNormalTextureID.first, m_Desc.hdrNormalTextureID.second);
		}
		if (m_Desc.anyTextureID.second != "") {
			m_pAnyTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.anyTextureID.first, m_Desc.anyTextureID.second);
		}
    }

	{
		m_waveCb.g_fBurstSpeed = 1.5f;
		m_waveCb.g_fFlowSpeed = 2.5f;
		m_waveCb.g_fWaveAmplitude = 2.f;
		m_waveCb.g_fWaveFrequency = 0.5f;
		m_waveCb.g_fWaveSpeed = 1.5f;
		//m_waveCb.g_fBurstRatio = Randf(0.5f, 0.7f);
		//m_waveCb.g_fBurstSpeed = Randf(0.7f, 1.f);
		//m_waveCb.g_fFlowSpeed = Randf(1.f, 3.f);
		//m_waveCb.g_fTransitionRatio = Randf(0.4f, 1.6f);
		//m_waveCb.g_fWaveAmplitude = Randf(0.f, 3.f);
		//m_waveCb.g_fWaveFrequency = Randf(0.f, 3.f);
		//m_waveCb.g_fWaveSpeed = Randf(0.5f, 1.f);
		//m_waveCb.g_vFlowDirection = _float3(Randf(-1, 1), Randf(-1, 1), Randf(-1, 1));
	}		
    return S_OK;

}

void CParticle_CPU::PriorityUpdate(E::_float fTimeDelta)
{
}

void CParticle_CPU::Update(E::_float fTimeDelta)
{
	ProcessPendingSpawns(fTimeDelta);
    Simulate(fTimeDelta);

}

void CParticle_CPU::LateUpdate(E::_float fTimeDelta)
{
}

void CParticle_CPU::Simulate(E::_float fTimeDelta)
{
	m_fAccumulationTime += fTimeDelta;
	m_vecInstancedData.clear();
	uint32_t totalFrames = m_Desc.TexRows * m_Desc.TexColumns;

	for (auto& p : m_Particles)
	{
		if (!p.bAlive)
			continue;

		p.life += fTimeDelta; // "지난 시간" 방식 그대로 유지 (원래 Simulate 스타일)

		if (p.life >= p.fMaxLife)
		{
			if (p.loop)
			{
				p.life = 0.f;
				p.vPosition = p.originalPosition;
				p.vVelocity = p.originalVelocity;
				p.fGravityVelocity = 0.f;
				continue;
			}
			else
			{
				p.bAlive = false;
				continue;
			}
		}
		float ageRatio = std::clamp(p.life / p.fMaxLife, 0.f, 1.f);

		UpdateBehavior(p, fTimeDelta);
		XMStoreFloat3(&p.vPosition, XMLoadFloat3(&p.vPosition) + XMLoadFloat3(&p.vVelocity) * fTimeDelta);

		if (m_vecInstancedData.size() >= m_iNumElements)
			continue;

		if (totalFrames > 1)
		{
			uint32_t frame = (uint32_t)(ageRatio * totalFrames);
			p.iFrameIndex = std::min(frame, totalFrames - 1);
		}
		else
		{
			p.iFrameIndex = 0;
		}

		VTX_PARTICLE_INSTANCED_DATA inst{};
		inst.iBehaviorType = p.iBehaviorType;
		
		if ((p.iBehaviorType & CParticle::BEHAVIOR_SIZESTOP) != 0) {
			SizeLerp(p, fTimeDelta);
		}
		else {
			p.fSize.x = std::lerp(p.fStartSize.x, p.fEndSize.x, ageRatio);
			p.fSize.y = std::lerp(p.fStartSize.y, p.fEndSize.y, ageRatio);
			p.fSize.z = std::lerp(p.fStartSize.z, p.fEndSize.z, ageRatio);
		}
	

		
		_matrix matScale = XMMatrixScaling(p.fSize.x, p.fSize.y, p.fSize.z);
		_matrix matTrans = XMMatrixTranslation(p.vPosition.x, p.vPosition.y, p.vPosition.z);

		_matrix matWorld;
		if ((p.iBehaviorType & CParticle::BEHAVIOR_BILLBOARD) != 0 /*&& m_Desc.whatKind == MESHORTEXTURE::TEX*/)
		{
			auto camera = CGameInstance::Get().GetActiveCamera();
			if (!camera)
				return;

			_matrix matView = camera->GetView();
			_matrix matInvView = XMMatrixInverse(nullptr, matView);

			// 1. 카메라 회전(빌보드 기저 벡터) 추출
			XMVECTOR camRight = XMVector3Normalize(matInvView.r[0]);
			XMVECTOR camUp = XMVector3Normalize(matInvView.r[1]);
			XMVECTOR camForward = XMVector3Normalize(matInvView.r[2]);

			_matrix matBillboardRot = XMMatrixIdentity();
			matBillboardRot.r[0] = camRight;
			matBillboardRot.r[1] = camUp;
			matBillboardRot.r[2] = camForward;
			matBillboardRot.r[3] = XMVectorSet(0.f, 0.f, 0.f, 1.f);

			// 2. 파티클 자체의 회전 행렬 생성 (예: p.rotation이 Vector3(Pitch, Yaw, Roll)인 경우)
			_matrix matParticleRot = XMMatrixRotationRollPitchYaw(p.rotation.x, p.rotation.y, p.rotation.z);

			// 만약 p.rotation이 Quaternion(XMFLOAT4 / XMVECTOR)인 경우:
			// _matrix matParticleRot = XMMatrixRotationQuaternion(XMLoadFloat4(&p.rotation));

			// 3. 빌보드 회전과 파티클 오프셋 회전 결합
			// (로컬 공간 회전 후 카메라 방향으로 정렬)
			_matrix matFinalRot = matParticleRot * matBillboardRot;

			// 4. 최종 월드 행렬 계산
			matWorld = matScale * matFinalRot * matTrans;
		}
		else
		{
			_matrix matRotation = XMMatrixRotationRollPitchYaw(p.rotation.x, p.rotation.y, p.rotation.z);
			matWorld = matScale * matRotation * matTrans;
		}

		XMStoreFloat4x4(&inst.matWorld, matWorld);
		inst.vColor = p.vColor;
		inst.originalEmissive = p.originalEmissive;
		inst.emissive = p.emissive;
		inst.endEmissive = p.endEmissive;
		inst.life = p.life;
		inst.maxLife = p.fMaxLife;
		//inst.iBehaviorType = p.iBehaviorType;

		if (m_Desc.TexColumns > 0 && m_Desc.TexRows > 0)
		{
			uint32_t col = p.iFrameIndex % m_Desc.TexColumns;
			uint32_t row = p.iFrameIndex / m_Desc.TexColumns;
			inst.vUVSize = _float2(1.0f / m_Desc.TexColumns, 1.0f / m_Desc.TexRows);
			inst.vUVOffset = _float2(col * inst.vUVSize.x, row * inst.vUVSize.y);
		}
		else
		{
			inst.vUVSize = _float2(1.0f, 1.0f);
			inst.vUVOffset = _float2(0.0f, 0.0f);
		}

		m_vecInstancedData.push_back(inst);
	}
}
void CParticle_CPU::UpdateBehavior(PARTICLE_CPU_DATA& p, E::_float fTimeDelta)
{
	if ((p.iBehaviorType & CParticle::BEHAVIOR_DISTORTION) != 0) {
		int a = 0;
	}
	const bool hasCircleToWave = (p.iBehaviorType & BEHAVIOR_CIRCLE_TO_WAVE) != 0;
	const bool hasGravity = (p.iBehaviorType & BEHAVIOR_GRAVITY) != 0;
	if (hasCircleToWave)
	{
		const uint32_t particleIndex = static_cast<uint32_t>(&p - m_Particles.data());
		const uint32_t seed = p.iSpawnSeed ^ (particleIndex * 0x9E3779B9u);
		const float randomAngle = Hash01(seed ^ 0xA511E9B3u) * XM_2PI;
		const float random1 = Hash01(seed ^ 0x63D83595u);
		const float random2 = Hash01(seed ^ 0xB5297A4Du);
		const float random3 = Hash01(seed ^ 0x68E31DA4u);
		const float ageRatio = std::clamp(p.life / std::max(p.fMaxLife, 0.0001f), 0.f, 1.f);
		auto SmoothStep = [](float edge0, float edge1, float value) {
			float t = std::clamp((value - edge0) / std::max(edge1 - edge0, 0.0001f), 0.f, 1.f);
			return t * t * (3.f - 2.f * t);
			};
		const float ringHoldRatio = 0.45f;
		const float chaosStartRatio = 0.40f;
		const float chaosEndRatio = 0.8f;
		const float chaosT = SmoothStep(chaosStartRatio, chaosEndRatio, ageRatio);
		XMVECTOR planeRight = XMVectorSet(1.f, 0.f, 0.f, 0.f);
		XMVECTOR planeUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
		XMVECTOR planeNormal = XMVectorSet(0.f, 0.f, 1.f, 0.f);
		if (auto camera = CGameInstance::Get().GetActiveCamera())
		{
			const XMMATRIX cameraWorld = XMMatrixInverse(nullptr, camera->GetView());
			planeRight = XMVector3Normalize(cameraWorld.r[0]);
			planeUp = XMVector3Normalize(cameraWorld.r[1]);
			planeNormal = XMVector3Normalize(cameraWorld.r[2]);
		}
		const float radialX = cosf(randomAngle);
		const float radialY = sinf(randomAngle);
		const XMVECTOR radialDirection = XMVector3Normalize(planeRight * radialX + planeUp * radialY);
		const XMVECTOR tangentDirection = XMVector3Normalize(planeRight * -radialY + planeUp * radialX);
		const float ringSpeed = m_waveCb.g_fBurstSpeed;
		const float ringFade = 1.f - SmoothStep(ringHoldRatio, 0.75f, ageRatio);
		const float radialSpeed = ringSpeed * std::lerp(1.f, 0.25f, chaosT);
		const float phase = random1 * XM_2PI + p.life * m_waveCb.g_fWaveSpeed;
		const XMVECTOR position = XMLoadFloat3(&p.vPosition);
		const float planePositionX = XMVectorGetX(XMVector3Dot(position, planeRight));
		const float planePositionY = XMVectorGetX(XMVector3Dot(position, planeUp));
		const float turbulence1 = sinf(phase + planePositionX * m_waveCb.g_fWaveFrequency);
		const float turbulence2 = cosf(phase * 1.37f + planePositionY * m_waveCb.g_fWaveFrequency);
		const float noiseSpeed = m_waveCb.g_fWaveAmplitude;
		const XMVECTOR turbulenceVelocity = planeRight * (turbulence1 * noiseSpeed) + planeUp * (turbulence2 * noiseSpeed);
		const XMVECTOR ringVelocity = radialDirection * (radialSpeed * ringFade);
		XMVECTOR chaosVelocity = XMVectorZero();
		float depthVelocity = 0.f;
		switch (p.iPatternType)
		{
		case 0:
		{
			const float windStrength = m_waveCb.g_fFlowSpeed * (0.7f + random2 * 0.6f);
			chaosVelocity = planeRight * windStrength + planeUp * (windStrength * 0.2f) + turbulenceVelocity * 0.4f;
			depthVelocity = (random3 * 2.f - 1.f) * noiseSpeed * 0.3f;
			break;
		}
		case 1:
		{
			const float windStrength = m_waveCb.g_fFlowSpeed * (0.6f + random2 * 0.8f);
			chaosVelocity = planeRight * (-windStrength * 0.8f) + planeUp * (-windStrength * 0.35f) + turbulenceVelocity * 0.6f;
			depthVelocity = (random3 * 2.f - 1.f) * noiseSpeed * 0.5f;
			break;
		}
		case 2:
		{
			const float vortexSpeed = m_waveCb.g_fFlowSpeed * (0.7f + random2 * 0.6f);
			chaosVelocity = tangentDirection * vortexSpeed + turbulenceVelocity * 0.4f;
			depthVelocity = (random3 * 2.f - 1.f) * noiseSpeed * 0.4f;
			break;
		}
		case 3:
		default:
		{
			const float scatterAngle = random2 * XM_2PI;
			const XMVECTOR scatterDirection = planeRight * cosf(scatterAngle) + planeUp * sinf(scatterAngle);
			const float scatterSpeed = m_waveCb.g_fFlowSpeed * (0.5f + random3);
			chaosVelocity = scatterDirection * scatterSpeed + turbulenceVelocity;
			depthVelocity = (random1 * 2.f - 1.f) * noiseSpeed;
			break;
		}
		}
		const XMVECTOR finalVelocity = ringVelocity + chaosVelocity * chaosT + planeNormal * (depthVelocity * chaosT);
		XMStoreFloat3(&p.vVelocity, finalVelocity);
	}
	if (hasGravity)
	{
		constexpr float gravity = -9.8f;
		if (hasCircleToWave)
		{
			p.fGravityVelocity += gravity * fTimeDelta;
			p.vVelocity.y += p.fGravityVelocity;
		}
		else
		{
			p.vVelocity.y += gravity * fTimeDelta;
		}
	}
	
	if ((p.iBehaviorType & CParticle::BEHAVIOR_SMOKE) != 0) {
		MakeSmoke(p, fTimeDelta);
	}
	if ((p.iBehaviorType & CParticle::BEHAVIOR_SMOKEJUMP) != 0) {
		JumpSmoke(p, fTimeDelta);
	}
	if ((p.iBehaviorType & CParticle::BEHAVIOR_SMOKEGV) != 0) {
		GVBurstSmoke(p, fTimeDelta);
	}
	if ((p.iBehaviorType & CParticle::BEHAVIOR_SMOKEGW) != 0) {
		GWWaveSmoke(p, fTimeDelta);
	}
	if ((p.iBehaviorType & CParticle::BEHAVIOR_LIGHTNING) != 0) {
		Lightning(p, fTimeDelta);
	}
	if ((p.iBehaviorType & CParticle::BEHAVIOR_EXTRALIGHTNING) != 0) {
		ExtraLightning(p, fTimeDelta);
	}
	if ((p.iBehaviorType & CParticle::BEHAVIOR_KEEPROTATE) != 0) {
		KeepRotate(p, fTimeDelta);
	}
}
void CParticle_CPU::MakeSmoke(PARTICLE_CPU_DATA& p,_float fTimeDelta)
{
	_float fSpeed = 1.5f;
	
	_float t = p.life / p.fMaxLife;
	p.vColor.w = 0.7f + (0.f - 0.7f) * t;
	
	if(t >=0.5f)
		p.vVelocity.y += fSpeed * fTimeDelta;
	
	  
}
void CParticle_CPU::JumpSmoke(PARTICLE_CPU_DATA& p, _float fTimeDelta)
{
	_float t = p.life / p.fMaxLife;
	p.vColor.w = 0.7f + (0.f - 0.7f) * t;

	if (t >= 0.2f)
		p.vVelocity.y += p.vVelocity.y * fTimeDelta;
}

void CParticle_CPU::GVBurstSmoke(PARTICLE_CPU_DATA& p, _float fTimeDelta)
{
	XMStoreFloat3(&p.vVelocity,XMLoadFloat3(&p.vVelocity) * expf(-2.f * fTimeDelta));
}

void CParticle_CPU::GWWaveSmoke(PARTICLE_CPU_DATA& p, _float fTimeDelta)
{
	XMStoreFloat3(&p.vVelocity, XMLoadFloat3(&p.vVelocity) * expf(-4.f * fTimeDelta));

}
void CParticle_CPU::SizeLerp(PARTICLE_CPU_DATA& p, _float fTimeDelta)
{
	_float stopSizeTime = p.fStopSizeTime;

	float ageRatio = std::clamp(p.life / stopSizeTime, 0.f, 1.f);

	p.fSize.x = std::lerp(p.fStartSize.x, p.fEndSize.x, ageRatio);
	p.fSize.y = std::lerp(p.fStartSize.y, p.fEndSize.y, ageRatio);
	p.fSize.z = std::lerp(p.fStartSize.z, p.fEndSize.z, ageRatio);
}

void CParticle_CPU::KeepRotate(PARTICLE_CPU_DATA& p,_float fTimeDelta)
{
	const float deltaAngle = p.fRotationSpeed * fTimeDelta;

	p.rotation.x += p.roationAxis.x * deltaAngle;

	p.rotation.y += p.roationAxis.y * deltaAngle;

	p.rotation.z += p.roationAxis.z * deltaAngle;

	p.rotation.w += p.roationAxis.z * deltaAngle;
}

void CParticle_CPU::Lightning(PARTICLE_CPU_DATA& p, _float fTimeDelta){
	{
		///////////////////////////////////////////// Fade Out
		if (p.fMaxLife - 1.f <= p.life) {
			p.vColor.w -= fTimeDelta;
		}
	}
	{
		///////////////////////////////////////////// Stop Particle
		if (p.originalPosition.y > p.vPosition.y) {
			p.vVelocity = { 0.f, 0.f, 0.f };
			return;
		}
	}	
	{
		///////////////////////////////////////////// Velocity Control
		_float DragFactor = 2.f;
		XMVECTOR Velocity = XMLoadFloat3(&p.vVelocity);
		Velocity = XMVectorScale(Velocity, expf(-DragFactor * fTimeDelta));

		XMStoreFloat3(&p.vVelocity, Velocity);
	} 
	//{
	//	///////////////////////////////////////////// Gravity
	//	const float kGravity = -9.8f;
	//
	//	p.vVelocity.y += kGravity * fTimeDelta;
	//
	//	XMVECTOR vPos = XMLoadFloat3(&p.vPosition);
	//	XMVECTOR vVel = XMLoadFloat3(&p.vVelocity);
	//	vPos = XMVectorAdd(vPos, XMVectorScale(vVel, fTimeDelta));
	//	XMStoreFloat3(&p.vPosition, vPos);
	//}
	{
		///////////////////////////////////////////// Particle Spread Type
		//auto ActiveCam = CGameInstance::Get().GetActiveCamera();
		//if (nullptr == ActiveCam) return;
		//
		//XMVECTOR CamtoParticle = XMVectorSubtract(ActiveCam->GetTransform().GetLoadedPostion(), XMLoadFloat3(&p.vPosition));
		//
		//_float CamX = XMVectorGetX(CamtoParticle);
		//_float CamZ = XMVectorGetZ(CamtoParticle);
		//
		//if (CamX * CamX * CamZ * CamZ > 0.001f) {
		//	p.rotation.y = atan2f(CamX, CamZ);
		//}
	}

	{
		_float RotationSpeed = 0.5f;
		XMVECTOR Velocity = XMLoadFloat3(&p.vVelocity);
		if (fabsf(p.vVelocity.x) > 0.0001f || fabsf(p.vVelocity.z) > 0.0001f) {
			_float TargetAngle = atan2f(p.vVelocity.y, p.vVelocity.z) + XM_PIDIV2;
			_float CurrentAngle = p.rotation.x;

			_float DeltaAngle = TargetAngle - CurrentAngle;

			while (DeltaAngle > XM_PI)  DeltaAngle -= XM_2PI;
			while (DeltaAngle < -XM_PI) DeltaAngle += XM_2PI;

			const _float AngleVelocity = XM_PI * RotationSpeed;
			_float MaxStep = AngleVelocity * fTimeDelta;

			if (fabsf(DeltaAngle) > MaxStep) {
				DeltaAngle = DeltaAngle > 0.f ? MaxStep : -MaxStep;
			}
			p.rotation.x = CurrentAngle + DeltaAngle;

			if (p.rotation.x > +XM_PI) p.rotation.x -= XM_2PI;
			if (p.rotation.x < -XM_PI) p.rotation.x += XM_2PI;
		}

		if (XMVectorGetX(XMVector3LengthSq(Velocity)) > 0.1f) {
			_float TargetAngle = atan2f(p.vVelocity.y, p.vVelocity.x) + XM_PIDIV2;
			_float CurrentAngle = p.rotation.z;

			_float DeltaAngle = TargetAngle - CurrentAngle;

			while (DeltaAngle > XM_PI)  DeltaAngle -= XM_2PI;
			while (DeltaAngle < -XM_PI) DeltaAngle += XM_2PI;

			const _float AngleVelocity = XM_PI * RotationSpeed;
			_float MaxStep = AngleVelocity * fTimeDelta;

			if (fabsf(DeltaAngle) > MaxStep) {
				DeltaAngle = DeltaAngle > 0.f ? MaxStep : -MaxStep;
			}
			p.rotation.z = CurrentAngle + DeltaAngle;
			
			if (p.rotation.z > +XM_PI) p.rotation.z -= XM_2PI;
			if (p.rotation.z < -XM_PI) p.rotation.z += XM_2PI;
		}
	}
}
void	CParticle_CPU::ExtraLightning(PARTICLE_CPU_DATA& p, _float fTimeDelta) {
	
}

HRESULT CParticle_CPU::Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData)
{
	if (pSpawnData == nullptr || count == 0)
		return E_FAIL;
	uint32_t iSpawned = 0;
	const uint32_t spawnSeed = m_iSpawnSeed++;
	const uint32_t spawnPattern = spawnSeed % 4;
	for (uint32_t i = 0; i < m_Particles.size() && iSpawned < count; ++i)
	{
		if (m_Particles[i].bAlive)
			continue;
		const auto& src = pSpawnData[iSpawned];
		m_Particles[i].vPosition = src.position;
		m_Particles[i].vVelocity = src.velocity;
		m_Particles[i].originalPosition = src.originalPosition;
		m_Particles[i].originalVelocity = src.originalVelocity;
		m_Particles[i].life = 0.f;
		m_Particles[i].fMaxLife = src.life;
		m_Particles[i].bAlive = true;
		m_Particles[i].fSize = src.fSize;
		m_Particles[i].fStartSize = src.fSize;
		m_Particles[i].fEndSize = src.fEndSize;
		m_Particles[i].vColor = src.color;
		m_Particles[i].originalEmissive = src.originalEmissive;
		m_Particles[i].emissive = src.emissive;
		m_Particles[i].endEmissive = src.endEmissive;
		m_Particles[i].spawnDelay = src.spawnDelay;
		m_Particles[i].ownerID = src.ownerID;
		m_Particles[i].rotation = src.rotation;
		m_Particles[i].iBehaviorType = src.iBehaviorType;
		m_Particles[i].loop = src.loop;
		m_Particles[i].fStopSizeTime = src.fStopSizeTime;
		m_Particles[i].roationAxis = src.rotationAxis;
		m_Particles[i].fRotationSpeed = src.fRotationSpeed;
		m_Particles[i].fGravityVelocity = 0.f;
		m_Particles[i].iPatternType = spawnPattern;
		m_Particles[i].iSpawnSeed = spawnSeed;
		++iSpawned;
	}
	return iSpawned == count ? S_OK : E_FAIL;
}

HRESULT CParticle_CPU::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{


	CB_TIMEACCUMULATION cb{};
	cb.fAccumulationTime = m_fAccumulationTime;
	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (SUCCEEDED(pContext->Map(m_pComTimeCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
	{
		memcpy(mapped.pData, &cb, sizeof(cb));
		pContext->Unmap(m_pComTimeCBuffer->GetCBuffer().Get(), 0);
	}

	pContext->VSSetConstantBuffers(11, 1, m_pComTimeCBuffer->GetCBuffer().GetAddressOf());

	pContext->PSSetConstantBuffers(11, 1, m_pComTimeCBuffer->GetCBuffer().GetAddressOf());


    if (m_vecInstancedData.empty())
        return S_OK;

    if (m_Desc.whatKind == MESHORTEXTURE::MESH)
        Render_Mesh(pContext, ctx);
	else {
		Render_Texture(pContext, ctx);
	}
	ID3D11Buffer* nullBuffer = nullptr;

	pContext->PSSetConstantBuffers(11, 1, &nullBuffer);
	return  S_OK;// 기존 텍스처 파티클 렌더 코드
}


HRESULT CParticle_CPU::Render_Mesh(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	if (!pContext || m_vecInstancedData.empty() || !m_pComModelInstance || !m_pResInstancedBuffer)
		return S_OK;

	auto rasterizer = E::CGameInstance::GetConst().GetResourceFirst<E::CResRasterizerState>(
		TAG_RES_GRP_PERMANENT_STATE,
		TAG_RES_STATE_RS_SOLID_NOCULL);

	if (!rasterizer)
		return E_FAIL;

	pContext->RSSetState(rasterizer->GetRasterizerState().Get());

	if (!m_pBlendState)
		return E_FAIL;

	pContext->OMSetBlendState(m_pBlendState->GetBlendState().Get(), nullptr, 0xffffffff);

	auto depthState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(
		TAG_RES_GRP_PERMANENT_STATE,
		"DS_DEPTHREAD");

	if (!depthState)
		return E_FAIL;

	pContext->OMSetDepthStencilState(depthState->GetDepthStencilState().Get(), 0);

	const auto& vs = m_pResVertexShader;
	const auto& ps = m_pResPixelShader;

	if (!vs || !ps)
		return E_FAIL;

	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);
	m_pComModelInstance->Bind_Materials(pContext, { 0.f, 0.f, 0.f }, 0.f, { 0.f, 0.f, 0.f }, 0.f, 1.f);

	if (m_pHdrPositionTexture)
	{
		ID3D11ShaderResourceView* hdrPositionSRV = m_pHdrPositionTexture->GetSRV().Get();
		pContext->VSSetShaderResources(10, 1, &hdrPositionSRV);
	}

	if (m_pHdrNormalTexture)
	{
		ID3D11ShaderResourceView* hdrNormalSRV = m_pHdrNormalTexture->GetSRV().Get();
		pContext->VSSetShaderResources(11, 1, &hdrNormalSRV);
	}

	if (m_pNoiseTexture)
	{
		ID3D11ShaderResourceView* noiseSRV = m_pNoiseTexture->GetSRV().Get();
		pContext->PSSetShaderResources(5, 1, &noiseSRV);
		pContext->VSSetShaderResources(5, 1, &noiseSRV);
	}

	if (m_pDistortionTexture)
	{
		ID3D11ShaderResourceView* distortionSRV = m_pDistortionTexture->GetSRV().Get();
		pContext->PSSetShaderResources(6, 1, &distortionSRV);
		pContext->VSSetShaderResources(6, 1, &distortionSRV);
	}

	if (m_pAnyTexture)
	{
		ID3D11ShaderResourceView* anyTextureSRV = m_pAnyTexture->GetSRV().Get();
		pContext->PSSetShaderResources(8, 1, &anyTextureSRV);
	}

	D3D11_MAPPED_SUBRESOURCE mapped{};

	HRESULT hr = pContext->Map(
		m_pResInstancedBuffer->GetBuffer().Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mapped);

	if (FAILED(hr))
		return hr;

	std::memcpy(
		mapped.pData,
		m_vecInstancedData.data(),
		sizeof(VTX_PARTICLE_INSTANCED_DATA) * m_vecInstancedData.size());

	pContext->Unmap(m_pResInstancedBuffer->GetBuffer().Get(), 0);

	auto pModel = m_pComModelInstance->GetModel();

	if (!pModel)
		return E_FAIL;

	uint32_t iNumMeshes = pModel->Get_NumMeshes();

	for (uint32_t i = 0; i < iNumMeshes; ++i)
	{
		const auto& viBuffer = pModel->GetMeshes()[i];

		if (!viBuffer)
			continue;

		ID3D11Buffer* vertexBuffers[] = {
			viBuffer->GetVertexBuffer().Get(),
			m_pResInstancedBuffer->GetBuffer().Get()
		};

		uint32_t strides[] = {
			viBuffer->GetVertexStride(),
			static_cast<uint32_t>(sizeof(VTX_PARTICLE_INSTANCED_DATA))
		};

		uint32_t offsets[] = { 0, 0 };

		pContext->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
		pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		auto diffuseTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>(
			"DEFAULT_TEXTURE",
			"TEX_DEFAULT_DIFFUSE");

		if (auto resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_DIFFUSE, 0))
			diffuseTexture = resource;

		auto normalTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>(
			"DEFAULT_TEXTURE",
			"TEX_DEFAULT_NORMAL");

		if (auto resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_NORMALS, 0))
			normalTexture = resource;

		auto smroTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>(
			"DEFAULT_TEXTURE",
			"TEX_DEFAULT_SMRO");

		if (auto resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_METALNESS, 0))
			smroTexture = resource;

		auto emissiveTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>(
			"DEFAULT_TEXTURE",
			"TEX_DEFAULT_EMISSIVE");

		if (auto resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_EMISSIVE, 0))
			emissiveTexture = resource;

		if (!diffuseTexture || !normalTexture || !smroTexture || !emissiveTexture)
			continue;

		pContext->PSSetShaderResources(0, 1, diffuseTexture->GetSRV().GetAddressOf());
		pContext->PSSetShaderResources(1, 1, normalTexture->GetSRV().GetAddressOf());
		pContext->PSSetShaderResources(2, 1, smroTexture->GetSRV().GetAddressOf());
		pContext->PSSetShaderResources(3, 1, emissiveTexture->GetSRV().GetAddressOf());

		pContext->DrawIndexedInstanced(
			static_cast<UINT>(viBuffer->GetNumIndices()),
			static_cast<UINT>(m_vecInstancedData.size()),
			0,
			0,
			0);
	}

	ID3D11ShaderResourceView* nullSRV[] = { nullptr };

	pContext->PSSetShaderResources(0, 1, nullSRV);
	pContext->PSSetShaderResources(1, 1, nullSRV);
	pContext->PSSetShaderResources(2, 1, nullSRV);
	pContext->PSSetShaderResources(3, 1, nullSRV);
	pContext->PSSetShaderResources(5, 1, nullSRV);
	pContext->PSSetShaderResources(6, 1, nullSRV);
	pContext->PSSetShaderResources(8, 1, nullSRV);

	pContext->VSSetShaderResources(5, 1, nullSRV);
	pContext->VSSetShaderResources(10, 1, nullSRV);
	pContext->VSSetShaderResources(11, 1, nullSRV);

	pContext->OMSetDepthStencilState(nullptr, 0);
	pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

	return S_OK;
}

HRESULT CParticle_CPU::Render_Texture(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
    if (m_vecInstancedData.empty())
        return S_OK;

;
	pContext->OMSetBlendState(m_pBlendState->GetBlendState().Get(), nullptr, 0xffffffff);
	auto Rasterizer = E::CGameInstance::GetConst().GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
	pContext->RSSetState(Rasterizer->GetRasterizerState().Get());

	SPtr<CResDepthStencilState> DepthState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_ALPHA_BLEND_DEPTH");
	pContext->OMSetDepthStencilState(DepthState->GetDepthStencilState().Get(), 0);

    const auto& viBuffer = CGameInstance::Get().GetResourceFirst<CResVIBuffer>(m_viBufferID.first, m_viBufferID.second);

    pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
    pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
    pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

    ID3D11Buffer* vertexBuffers[] = {
        viBuffer->GetVertexBuffer().Get(),
        m_pResInstancedBuffer->GetBuffer().Get()
    };
    uint32_t strides[] = {
        viBuffer->GetVertexStride(),
        (uint32_t)sizeof(VTX_PARTICLE_INSTANCED_DATA),
    };
    uint32_t offsets[] = { 0, 0 };

    pContext->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
    pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
    pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(pContext->Map(m_pResInstancedBuffer->GetBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            std::memcpy(mapped.pData, m_vecInstancedData.data(),
                sizeof(VTX_PARTICLE_INSTANCED_DATA) * m_vecInstancedData.size());
            pContext->Unmap(m_pResInstancedBuffer->GetBuffer().Get(), 0);
        }
    }

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
		pContext->PSSetShaderResources(5, 1, &pAnySRV);

	}


    pContext->DrawIndexedInstanced((UINT)viBuffer->GetNumIndices(), (UINT)m_vecInstancedData.size(), 0, 0, 0);

    ID3D11ShaderResourceView* nullSRV[] = { nullptr,nullptr,nullptr,nullptr,nullptr,nullptr };
    pContext->PSSetShaderResources(0, 6, nullSRV);

	pContext->OMSetDepthStencilState(nullptr, 0);
	pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);


    return S_OK;
}
UPtr<CParticle> CParticle_CPU::Create(void* pArg)
{
	auto pInstance = E::ToUPtr(new CParticle_CPU{});
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Created : CParticle_CPU");
		return nullptr;
	}
	return  pInstance;
}
void CParticle_CPU::ClearByOwner(uint32_t ownerID)
{
	for (auto& p : m_Particles)
	{
		if (p.bAlive && p.ownerID == ownerID)
		{
			p = PARTICLE_CPU_DATA{}; // 통째로 기본값으로 리셋 (bAlive=false 포함)
		}
	}
}
void CParticle_CPU::SetPosition(const _float3& pos)
{
	if (m_Particles.empty())
		return;
	for (auto& particle : m_Particles) {
		particle.vPosition = pos;
	}
}

void CParticle_CPU::SetVelocity(const _float3& vel)
{
	if (m_Particles.empty())
		return;

	for (auto& particle : m_Particles) {
		particle.vVelocity = vel;
	}
}

void CParticle_CPU::SetSize(const _float3& size)
{
	if (m_Particles.empty())
		return;

	for (auto& particle : m_Particles) {
		particle.fSize = size;
	}
}
void CParticle_CPU::SetColor(const _float4& color)
{
	if (m_Particles.empty())
		return;
	for (auto& particle : m_Particles) {
		particle.vColor = color;
	}
}
void CParticle_CPU::SetColorByOwner(uint32_t ownerId, const _float4& color)
{
	for (auto& particle : m_Particles)
	{
		if (!particle.bAlive)
			continue;

		if (particle.ownerID != ownerId)
			continue;

		particle.vColor = color;
	}
}
void CParticle_CPU::SetEmissive(const _float4& emissive)
{
	if (m_Particles.empty())
		return;
	for (auto& particle : m_Particles) {
		particle.emissive = emissive;
		particle.originalEmissive = emissive;
	}
}
void CParticle_CPU::TranslateOwner(uint32_t ownerId,const _float3& delta)
{
	for (auto& particle : m_Particles)
	{
		if (!particle.bAlive ||
			particle.ownerID != ownerId)
		{
			continue;
		}

		particle.vPosition.x += delta.x;
		particle.vPosition.y += delta.y;
		particle.vPosition.z += delta.z;

		particle.originalPosition.x += delta.x;
		particle.originalPosition.y += delta.y;
		particle.originalPosition.z += delta.z;
	}
}
void CParticle_CPU::TransformOwner(uint32_t ownerId, const _float4x4& deltaMatrixData)
{
	TransformPendingOwner(ownerId, deltaMatrixData);

	const XMMATRIX deltaMatrix = XMLoadFloat4x4(&deltaMatrixData);

	XMVECTOR scale{};
	XMVECTOR deltaRotation{};
	XMVECTOR translation{};

	if (!XMMatrixDecompose(&scale, &deltaRotation, &translation, deltaMatrix))
		return;

	XMFLOAT4X4 rotationMatrix{};
	XMStoreFloat4x4(&rotationMatrix, XMMatrixRotationQuaternion(deltaRotation));

	const float deltaYaw = -std::atan2(rotationMatrix._31, rotationMatrix._33);

	for (auto& particle : m_Particles)
	{
		if (!particle.bAlive || particle.ownerID != ownerId)
			continue;

		XMStoreFloat3(&particle.vPosition, XMVector3TransformCoord(XMLoadFloat3(&particle.vPosition), deltaMatrix));
		XMStoreFloat3(&particle.originalPosition, XMVector3TransformCoord(XMLoadFloat3(&particle.originalPosition), deltaMatrix));

		XMStoreFloat3(&particle.vVelocity, XMVector3Rotate(XMLoadFloat3(&particle.vVelocity), deltaRotation));
		XMStoreFloat3(&particle.originalVelocity, XMVector3Rotate(XMLoadFloat3(&particle.originalVelocity), deltaRotation));

		particle.rotation.y = std::remainder(particle.rotation.y + deltaYaw, XM_2PI);
	}
}
