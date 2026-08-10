// ParticleParamImGui.h
#pragma once
NS_BEGIN(Engine)

// ---- 타입별 그리기 함수 (전 패턴이 재사용) ----

inline void DrawBehaviorTypeFlags(uint32_t& flags)
{
	ImGui::Separator();


	bool bDistortion = (flags & BEHAVIOR_DISTORTION) != 0;
	bool bBillboard = (flags & BEHAVIOR_BILLBOARD) != 0;
	bool bGravity = (flags & BEHAVIOR_GRAVITY) != 0;
	bool bCircleWave = (flags & BEHAVIOR_CIRCLE_TO_WAVE) != 0;
	bool bSmoke = (flags & BEHAVIOR_SMOKE) != 0;
	bool bSmokeJump = (flags & BEHAVIOR_SMOKEJUMP) != 0;
	bool bSmokegv = (flags & BEHAVIOR_SMOKEGV) != 0;
	bool bSmokegw = (flags & BEHAVIOR_SMOKEGW) != 0;
	bool bLightning = (flags & BEHAVIOR_LIGHTNING) != 0;
	bool bSizeStop = (flags & BEHAVIOR_SIZESTOP) != 0;

	ImGui::Text("Common Pattern");
	if (ImGui::Checkbox("Distortion", &bDistortion))
		flags = bDistortion ? (flags | BEHAVIOR_DISTORTION) : (flags & ~BEHAVIOR_DISTORTION);
	ImGui::SameLine();
	if (ImGui::Checkbox("Billboard", &bBillboard))
		flags = bBillboard ? (flags | BEHAVIOR_BILLBOARD) : (flags & ~BEHAVIOR_BILLBOARD);

	if (ImGui::Checkbox("Gravity", &bGravity))
		flags = bGravity ? (flags | BEHAVIOR_GRAVITY) : (flags & ~BEHAVIOR_GRAVITY);
	ImGui::SameLine();
	if (ImGui::Checkbox("SizeStop", &bSizeStop))
		flags = bSizeStop ? (flags | BEHAVIOR_SIZESTOP) : (flags & ~BEHAVIOR_SIZESTOP);
	ImGui::Separator();


	ImGui::Text("Only For CPU Pattern");
	if (ImGui::Checkbox("CircleToWave", &bCircleWave))
		flags = bCircleWave ? (flags | BEHAVIOR_CIRCLE_TO_WAVE) : (flags & ~BEHAVIOR_CIRCLE_TO_WAVE);
	ImGui::SameLine();
	if (ImGui::Checkbox("Smoke", &bSmoke))
		flags = bSmoke ? (flags | BEHAVIOR_SMOKE) : (flags & ~BEHAVIOR_SMOKE);
	ImGui::SameLine();
	if (ImGui::Checkbox("SmokeJump", &bSmokeJump))
		flags = bSmokeJump ? (flags | BEHAVIOR_SMOKEJUMP) : (flags & ~BEHAVIOR_SMOKEJUMP);
	ImGui::SameLine();
	if (ImGui::Checkbox("Smokegv", &bSmokegv))
		flags = bSmokegv ? (flags | BEHAVIOR_SMOKEGV) : (flags & ~BEHAVIOR_SMOKEGV);
	ImGui::SameLine();
	if (ImGui::Checkbox("Smokegw", &bSmokegw))
		flags = bSmokegw ? (flags | BEHAVIOR_SMOKEGW) : (flags & ~BEHAVIOR_SMOKEGW);
	if (ImGui::Checkbox("Lightning", &bLightning))
		flags = bLightning ? (flags | BEHAVIOR_LIGHTNING) : (flags & ~BEHAVIOR_LIGHTNING);
	ImGui::Separator();


	ImGui::Text("Only For GPU Pattern");

}



inline void DrawField(const char* name, _float& v) { ImGui::DragFloat(name, &v, 0.05f); }
inline void DrawField(const char* name, _float2& v) { ImGui::DragFloat2(name, &v.x, 0.05f); }
inline void DrawField(const char* name, _float3& v) { ImGui::DragFloat3(name, &v.x, 0.05f); }
inline void DrawField(const char* name, _float4& v) { ImGui::ColorEdit4(name, &v.x, 0.05f); }
inline void DrawField(const char* name, uint32_t& v) { int tmp = (int)v; if (ImGui::DragInt(name, &tmp, 1, 0, 9999)) v = (uint32_t)std::max(0, tmp); }
inline void DrawField(const char* name, _bool& v){bool tmp = (bool)v;if (ImGui::Checkbox(name, &tmp))v = tmp;}



// ---- json 저장/로드 (타입별 헬퍼) ----

inline void SaveField(nlohmann::json& out, const char* name, const _float2& v) { out[name] = { v.x, v.y }; }
inline void SaveField(nlohmann::json& out, const char* name, const _float3& v) { out[name] = { v.x, v.y, v.z }; }
inline void SaveField(nlohmann::json& out, const char* name, const _float4& v) { out[name] = { v.x, v.y, v.z, v.w }; }
inline void SaveField(nlohmann::json& out, const char* name, const _float& v) { out[name] = v; }
inline void SaveField(nlohmann::json& out, const char* name, const uint32_t& v) { out[name] = v; }
inline void SaveField(nlohmann::json& out, const char* name, const _bool& v){out[name] = (bool)v;}
inline void LoadField(const nlohmann::json& in, const char* name, _float2& v)
{
	auto a = in.value(name, std::vector<float>{v.x, v.y});
	v = { a[0], a[1]};
}
inline void LoadField(const nlohmann::json& in, const char* name, _float3& v)
{
	auto a = in.value(name, std::vector<float>{v.x, v.y, v.z});
	v = { a[0], a[1], a[2] };
}
inline void LoadField(const nlohmann::json& in, const char* name, _float4& v)
{
	auto a = in.value(name, std::vector<float>{v.x, v.y, v.z, v.w});
	v = { a[0], a[1], a[2], a[3] };
}

inline void LoadField(const nlohmann::json& in, const char* name, _float& v) { v = in.value(name, v); }
inline void LoadField(const nlohmann::json& in, const char* name, uint32_t& v) { v = in.value(name, v); }
inline void LoadField(const nlohmann::json& in, const char* name, _bool& v)
{
	if (in.contains(name))
		v = in[name].get<bool>();
}

#define SAVE_PARAM_FIELD(type, name, defaultVal) SaveField(out, #name, p.name);
#define LOAD_PARAM_FIELD(type, name, defaultVal) LoadField(in, #name, p.name);
#define DRAW_PARAM_FIELD(type, name, defaultVal) DrawField(#name, p.name);

// 7.  draw imgui struct이름에 맞춰서 추가
inline void DrawImGui(SStairsParam& p) {
	STAIRS_FIELDS(DRAW_PARAM_FIELD)   DrawBehaviorTypeFlags(p.iBehaviorType);
}
inline void DrawImGui(SCircleParam& p) {
	CIRCLE_FIELDS(DRAW_PARAM_FIELD)  DrawBehaviorTypeFlags(p.iBehaviorType);
}
inline void DrawImGui(SCircleSpreadParam& p) {
	CIRCLE_SPREAD_FIELDS(DRAW_PARAM_FIELD)  DrawBehaviorTypeFlags(p.iBehaviorType);
}
inline void DrawImGui(SSpiralParam& p) {
	SPIRAL_FIELDS(DRAW_PARAM_FIELD)  DrawBehaviorTypeFlags(p.iBehaviorType);
}
inline void DrawImGui(SStraightGroundParam& p) {
	STRAIGHT_GROUND_FIELDS(DRAW_PARAM_FIELD) DrawBehaviorTypeFlags(p.iBehaviorType);
}
inline void DrawImGui(SMOKE& p) {
	SMOKE_FIELDS(DRAW_PARAM_FIELD) DrawBehaviorTypeFlags(p.iBehaviorType);
}
inline void DrawImGui(SLightning& p) {
	LIGHTNING_STREIGHT(DRAW_PARAM_FIELD) DrawBehaviorTypeFlags(p.iBehaviorType);
}
inline void DrawImGui(SConeParam& p) {
	CONE_FIELDS(DRAW_PARAM_FIELD) DrawBehaviorTypeFlags(p.iBehaviorType);
}
#undef DRAW_PARAM_FIELD

//8. save 추가
inline void SaveParam(const SStairsParam& p, nlohmann::json& out) { STAIRS_FIELDS(SAVE_PARAM_FIELD) }
inline void   SaveParam(const SCircleParam& p, nlohmann::json& out) { CIRCLE_FIELDS(SAVE_PARAM_FIELD) }
inline void SaveParam(const SCircleSpreadParam& p, nlohmann::json& out) { CIRCLE_SPREAD_FIELDS(SAVE_PARAM_FIELD) }
inline void SaveParam(const SSpiralParam& p, nlohmann::json& out) { SPIRAL_FIELDS(SAVE_PARAM_FIELD) }
inline void SaveParam(const SStraightGroundParam& p, nlohmann::json& out) { STRAIGHT_GROUND_FIELDS(SAVE_PARAM_FIELD) }
inline void SaveParam(const SMOKE& p, nlohmann::json& out) { SMOKE_FIELDS(SAVE_PARAM_FIELD) }
inline void SaveParam(const SLightning& p, nlohmann::json& out) { LIGHTNING_STREIGHT(SAVE_PARAM_FIELD) }
inline void SaveParam(const SConeParam& p, nlohmann::json& out) { CONE_FIELDS(SAVE_PARAM_FIELD) }

//9. 로드 추가
inline void LoadParam(SStairsParam& p, const nlohmann::json& in) { STAIRS_FIELDS(LOAD_PARAM_FIELD) }
inline void LoadParam(SCircleParam& p, const nlohmann::json& in) { CIRCLE_FIELDS(LOAD_PARAM_FIELD) }
inline void LoadParam(SCircleSpreadParam& p, const nlohmann::json& in) { CIRCLE_SPREAD_FIELDS(LOAD_PARAM_FIELD) }
inline void LoadParam(SSpiralParam& p, const nlohmann::json& in) { SPIRAL_FIELDS(LOAD_PARAM_FIELD) }
inline void LoadParam(SStraightGroundParam& p, const nlohmann::json& in) { STRAIGHT_GROUND_FIELDS(LOAD_PARAM_FIELD) }
inline void LoadParam(SMOKE& p, const nlohmann::json& in) { SMOKE_FIELDS(LOAD_PARAM_FIELD) }
inline void LoadParam(SLightning& p, const nlohmann::json& in) { LIGHTNING_STREIGHT(LOAD_PARAM_FIELD) }
inline void LoadParam(SConeParam& p, const nlohmann::json& in) { CONE_FIELDS(LOAD_PARAM_FIELD) }

#undef SAVE_PARAM_FIELD
#undef LOAD_PARAM_FIELD

// ---- variant 통합 버전 (커맨드 큐/프리셋에서 사용) ----
inline void DrawImGui(PatternParamVariant& v)
{
	std::visit([](auto& param) { DrawImGui(param); }, v);
}
inline void SaveParam(const PatternParamVariant& v, nlohmann::json& out)
{
	std::visit([&](const auto& param) { SaveParam(param, out); }, v);
}
// variant 로드는 kind 인덱스를 먼저 알아야 하므로 개별 LoadParam(struct&, json)을 직접 호출

NS_END
