
#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CGameObject;
class CComAnimator;
class CComAnimMontage;

class CAnimEdit_Manager final : public CEngineBase
{
public:
	struct SPEED_KEY
	{
		float fTime = 0.f;
		float fSpeed = 1.f;

	};
	struct BAKE_SAMPLE
	{
		float fSourceTrackPosition = 0.f; // 원본에서 읽을 시간
		float fBakedTrackPosition = 0.f;  // 새 애니메이션에 저장할 시간
	};
private:
	CAnimEdit_Manager();
	~CAnimEdit_Manager();

public:
	HRESULT Initilize();


	HRESULT SetupTestModel();
public:

	void Update(_float fTimeDelta);

	void SetTestModelHandle(const CHandle& handle) { m_hTestModel = handle; }


public:
	int32_t GetAnimIndex(CHandle Handle);
public:
	
	//-------------------------------------------------------Anim---------------------------------------------------------
	void IMGUI_TopBar_Animation(CGameObject* pSampleObj, CComAnimator* pComAnimator);
	void IMGUI_Select_AnimType();
	void IMGUI_Slider_Animation();
	void IMGUI_Select_Animation();
	void IMGUI_Select_Detail_Data();

	void IMGUI_Speed_Animation();

	void IMGUI_File_Rename(const std::string& Path, const std::string& fileName, const std::string& newfileName);

	//------------------------------------------------------AnimMontage--------------------------------------------------------
	void IMGUI_Slider_AnimMontage();
	void IMGUI_Select_AnimMontage();
	void IMGUI_Detail_AnimMontage();
public:
	//-------------------------------------------------------Anim---------------------------------------------------------
	// helper 함수들
	_bool RenameAnimFile_Overwrite(const std::string& oldFullPath, const std::string& newAnimName, std::string& outNewFullPath);
	_bool IsSamePath(const std::filesystem::path& a, const std::filesystem::path& b);
	_bool IsAlreadyLoadedAnim(const std::vector<SPtr<CResModelAnim>>& animations, const std::filesystem::path& loadPath);

	_bool WriteSaveBakedBinary(const std::string& _path, const std::string& _Name);
	std::vector<BAKE_SAMPLE> BuildBakeSamples(float fSourceDuration, float fTickPerSecond, float fSampleFPS);
	KEYFRAME SampleChannelKeyFrame( CChannel& pChannel, float fTrackPosition);

	//-------------------------------------------------------Animmontage---------------------------------------------------------
private:
	std::unordered_map<std::string, CComAnimMontage*> m_mapAnimMontages;

	CComAnimMontage* m_pCurrentMontage = nullptr;
	std::string   m_strCurrentMontageName;


public:
	void UpdateGUI();



public:
	float GetSpeedAtTime(float fTrackPos);
	
	


public:
	
private:
	CHandle m_hTestModel{};
	_float	m_fTimeDelta{ 0.f };
	uint32_t m_iCurrentAnimIndex{};

private:
	bool bPopup_File_Open = false;

	std::string oldPath;
	std::string newPath;

	std::vector<SPEED_KEY> m_SpeedKeys;

public:
	static UPtr<CAnimEdit_Manager> Create();
};

NS_END

