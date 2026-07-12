#include "pch.h"
#include "Resource.h"

NS_USING(Engine)

CResource::CResource(const _string& sPath)
	: m_sPath{ sPath }
{
	LogMemoryUsage();
}

CResource::~CResource()
{
}

//_wstring CAsset::GetTypeStr() const
//{
//	switch (m_eType)
//	{
//	case TYPE::FMOD_SOUND: return L"FMOD_SOUND";
//	case TYPE::JSON: return L"JSON";
//	case TYPE::PIXEL_SHADER: return L"PIXEL_SHADER";
//	case TYPE::VERTEX_SHADER: return L"VERTEX_SHADER";
//	}
//	
//	return {};
//}

_string CResource::GetStateStr() const
{
	switch (m_eState)
	{
	case STATE::LOADED: return "LOADED";
	case STATE::UNLOAD: return "UNLOAD";
	case STATE::LOADFAIL: return "LOADFAIL";
	case STATE::LOADING: return "LOADING";
	}

	return {};
}

void CResource::UpdateGUI()
{
	ImGui::Text("Type: %s", GetTypeString().data());
	ImGui::Text("State: %s", GetStateStr().c_str());
	ImGui::Text("Path: %s", m_sPath.c_str());
}
