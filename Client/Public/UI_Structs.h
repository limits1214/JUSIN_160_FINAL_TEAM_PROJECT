#pragma once

#include "Client_Defines.h"
#include "UI_Enums.h"

#include <string>
#include <vector>

NS_BEGIN(Client)

struct MINIMAP_PROFILE
{
	MINIMAP_MODE Mode{ MINIMAP_MODE::WORLD_MAP };
	std::string TextureTag{ "TEX_UI_T_MapMini_Sanctuary_03_D" };
	_float2 WorldMinXZ{};
	_float2 WorldMaxXZ{};
	_float2 UVMin{};
	_float2 UVMax{ 1.f, 1.f };
	_float MapScale{ 0.6f };
	_float SmokeIntensity{ 0.65f };
	_float SmokeSpeed{ 1.f };
	_float FogAlphaMultiplier{ 1.f };
	_float PlayerScrollScale{ 0.003f };
};

struct BATTLE_ZONE_INFO
{
	QUEST_UI_GROUP Group{ QUEST_UI_GROUP::NONE };
	_bool Enabled{ true };
	_float3 Center{};
	_float WorldRadius{ 15.f };
	_float VisibleDistance{ 60.f };
	_float Alpha{ 0.25f };
	uint32_t LevelID{ static_cast<uint32_t>(-1) };
};

struct OBJECTIVE_VISUAL_PHASE
{
	_float MinDistance{};
	_float MaxDistance{}; // 0: no upper limit
	std::string TextureTag;
	std::string PrototypeTag{ "Prototype_GameObject_TextureUI" };
	_float IconSize{ 24.f };
	_float3 TintColor{}; // zero keeps the original texture color
	int WeightOffset{ 4 };
	_float FadeInTime{ 0.5f };
	_float FadeOutTime{ 0.5f };
	_float DistanceHysteresis{ 1.f };
	_bool ShowScreenMarker{ false };
	_float ScreenMarkerSize{ 36.f };
	_float3 ScreenMarkerWorldOffset{ 0.f, 2.5f, 0.f };
	int ScreenMarkerWeight{ 100 };
	_float ScreenMarkerOffscreenAlpha{ 0.5f };
	_float ScreenMarkerEdgePadding{ 32.f };
	_bool DesiredVisible{ false };
	_float MarkerAlphaRatio{};
	CHandle MarkerHandle{};
	_bool ScreenMarkerDesiredVisible{ false };
	_float ScreenMarkerAlpha{};
	CHandle ScreenMarkerHandle{};
};

struct MINIMAP_OBJECTIVE_INFO
{
	QUEST_UI_GROUP Group{ QUEST_UI_GROUP::NONE };
	_bool Enabled{ true };
	std::string Key;
	_float3 WorldPosition{};
	uint32_t LevelID{ static_cast<uint32_t>(-1) };
	OBJECTIVE_ACTIVE_RULE ActiveRule{ OBJECTIVE_ACTIVE_RULE::MANUAL };
	_float AutoActivateDistance{};
	_float ActivationHysteresis{ 5.f };
	_bool ManualActive{ false };
	_bool ProximityActive{ false };
	std::vector<OBJECTIVE_VISUAL_PHASE> VisualPhases;
};

NS_END

