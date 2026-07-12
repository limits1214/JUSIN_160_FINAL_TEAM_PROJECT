#include "pch.h"
#include "ResModelAnim.h"
#include <fstream>

NS_USING(Engine)

CResModelAnim::CResModelAnim(const _string& sPath)
	: CResource{ sPath }
{
}

CResModelAnim::~CResModelAnim()
{
}

HRESULT CResModelAnim::Load(const std::any& arg)
{
    auto descArg = std::any_cast<DESC>(&arg);
    if (!descArg)
        return E_FAIL;

    if (m_eState == STATE::LOADED)
        return S_OK;

    m_eState = STATE::LOADING;

    m_AnimPath = descArg->path;
    auto& pModel = descArg->pModel;

    std::ifstream file(m_AnimPath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return E_FAIL;

    size_t fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(fileSize);

    file.read(buffer.get(), fileSize);
    if (!file)
        return E_FAIL;

    char* ptr = buffer.get();
    char* end = buffer.get() + fileSize;

    if (ptr + sizeof(MODEL_FILE_HEADER) > end)
        return E_FAIL;

    MODEL_FILE_HEADER* fh = reinterpret_cast<MODEL_FILE_HEADER*>(ptr);
    ptr += sizeof(MODEL_FILE_HEADER);

    if (ptr + sizeof(ChunkHeader) > end)
        return E_FAIL;

    ChunkHeader* chAnim = reinterpret_cast<ChunkHeader*>(ptr);
    ptr += sizeof(ChunkHeader);

    if (ptr + sizeof(_float) * 2 + sizeof(uint32_t) > end)
        return E_FAIL;

    memcpy(&m_fDuration, ptr, sizeof(_float));
    ptr += sizeof(_float);

    memcpy(&m_fTickPerSecond, ptr, sizeof(_float));
    ptr += sizeof(_float);

    memcpy(&m_iNumChannels, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    m_Channels.clear();
    m_Channels.reserve(m_iNumChannels);

    m_CurrentKeyFrameIndices.clear();
    m_CurrentKeyFrameIndices.resize(m_iNumChannels, 0);

	uint32_t keyFrameSize{};
    for (uint32_t i = 0; i < m_iNumChannels; ++i)
    {
        if (ptr + sizeof(uint32_t) > end)
            return E_FAIL;

        uint32_t channelSize = 0;
        memcpy(&channelSize, ptr, sizeof(uint32_t));
        ptr += sizeof(uint32_t);

        if (channelSize == 0)
            return E_FAIL;

        if (ptr + channelSize > end)
            return E_FAIL;

        //auto pChannel = CResModelChanel::Create();
        //if (nullptr == pChannel)
        //    return E_FAIL;

        //CResModelChanel::DESC channelDesc{};
        //channelDesc.ptr = ptr;
        //channelDesc.pModel = pModel;

        //if (FAILED(pChannel->Load(channelDesc)))
        //    return E_FAIL;

		CChannel channel{};
		channel.Intialize(ptr, pModel);

		keyFrameSize += sizeof(KEYFRAME)* channel.m_KeyFrames.size();
		keyFrameSize += sizeof(KEYFRAME)* channel.m_RootKeyFrames.size();
		

        m_Channels.push_back(channel);

        ptr += channelSize;
    
		m_iRootBoneIndex = pModel->Get_BoneIndex("Reference");
	
	}


    m_eState = STATE::LOADED;
    return S_OK;
}
HRESULT CResModelAnim::Unload(const std::any& arg)
{

	m_eState = STATE::UNLOAD;
	return S_OK;
}

//_bool CResModelAnim::Update_TransformationMatrices(_float fTimeDelta, const std::vector<SPtr<CResModelBone>>& Bones, _bool isLoop)
//{
//	_float fPrevTrackPosition = m_fCurrentTrackPosition;
//
//	m_fCurrentTrackPosition += m_fTickPerSecond * fTimeDelta;
//
//	if (m_fCurrentTrackPosition >= m_fDuration)
//	{
//		if (true == isLoop)
//			m_fCurrentTrackPosition = 0.f;
//		else
//			return true;
//	}
//
//	 ExtractRootMotionDelta(fPrevTrackPosition, m_fCurrentTrackPosition, m_iRootBoneIndex, m_vRootDelta);
//
//	for (uint32_t i = 0; i < m_iNumChannels; ++i)
//	{
//		m_Channels[i]->Update_TransformationMatrix(m_CurrentKeyFrameIndices[i], m_fCurrentTrackPosition, Bones, m_iRootBoneIndex);
//	}
//
//	return false;
//
//}

_bool CResModelAnim::ExtractRootMotionDelta(_float fPrevTrackPosition,_float fCurrTrackPosition,uint32_t iRootBoneIndex,_float3& vOutDelta)
{
	vOutDelta = _float3(0.f, 0.f, 0.f);
	// 본 채널 : 애니메이션의 처음 채널을 찾는다.
	auto pRootChannel = FindRootChannel(iRootBoneIndex);
	if (pRootChannel == nullptr)
		return false;

	//이전 루트 매트릭스 구함
	_matrix PrevRootMatrix =
		pRootChannel->Evaluate_TransformationMatrix(fPrevTrackPosition);


	// 최근 루트 매트리긋 구함
	_matrix CurrRootMatrix =
		pRootChannel->Evaluate_TransformationMatrix(fCurrTrackPosition);

	_float3 vPrevPos;
	_float3 vCurrPos;

	XMStoreFloat3(&vPrevPos, PrevRootMatrix.r[3]);
	XMStoreFloat3(&vCurrPos, CurrRootMatrix.r[3]);

	vOutDelta.x = vCurrPos.x - vPrevPos.x;
	vOutDelta.y = vCurrPos.y - vPrevPos.y;
	vOutDelta.z = vCurrPos.z - vPrevPos.z;

	return true;
}

void CResModelAnim::SetCurrentTrackPosition(float fPos)
{
	m_fCurrentTrackPosition = fPos;

	RebuildCurrentKeyFrameIndices();
}

CChannel* CResModelAnim::FindRootChannel(uint32_t iRootBoneIndex)
{
	for (auto& pChannel : m_Channels)
	{
		if (pChannel.Get_BoneIndex() == iRootBoneIndex)
			return &pChannel;
	}

	return nullptr;
}

void CResModelAnim::RebuildCurrentKeyFrameIndices()
{
	for (uint32_t i = 0; i < m_iNumChannels; ++i)
	{
		m_CurrentKeyFrameIndices[i] =
			m_Channels[i].FindKeyFrameIndex(
				m_fCurrentTrackPosition);
	}
}

SPtr<CResModelAnim> CResModelAnim::Create(const _string& sPath)
{
	return ToSPtr(new CResModelAnim{ sPath });
}
