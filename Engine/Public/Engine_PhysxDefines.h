#pragma once

#include "Handle.h"
namespace Engine
{
	class CGameObject;

	enum class PX_ACTOR_TYPE : uint8_t
	{
		RIGID_BODY,
		CHARACTER_CONTROLLER,
		RAGDOLL_BONE,
		ARTICULATION_LINK
	};

	enum class PX_SHAPE_TYPE : uint8_t
	{
		BOX,
		SPHERE,
		CAPSULE,
		CONVEX_MESH,
		TRIANGLE_MESH,
		RAGDOLL,
		HEIGHT_FIELD
	};

	struct PX_ACTOR_USER_DATA
	{
		CHandle hGameObject{};
		PX_ACTOR_TYPE eType{ PX_ACTOR_TYPE::RIGID_BODY };
		uint32_t iSubIndex{ std::numeric_limits<uint32_t>::max() };
	};

	struct PX_SHAPE_USER_DATA
	{
		CHandle hGameObject{};
		PX_SHAPE_TYPE eType{ PX_SHAPE_TYPE::BOX };
		uint32_t iSubIndex{ std::numeric_limits<uint32_t>::max() };
	};

	struct PX_JOINT_USER_DATA
	{
		CHandle hJointOwner{};
		CHandle hActorA{};
		CHandle hActorB{};
		uint32_t iJointSubIndex{ std::numeric_limits<uint32_t>::max() };
	};

	inline constexpr uint32_t PX_DEFAULT_LAYER = 1u;
	inline constexpr uint32_t PX_ALL_LAYERS = std::numeric_limits<uint32_t>::max();
	inline constexpr char PX_UNIT_CYLINDER_CONVEX_PATH[] =
		"./Resources/PhysX/Primitives/UnitCylinder12.pxconvex";
	inline constexpr _float PX_UNIT_CYLINDER_RADIUS = 0.5f;
	inline constexpr _float PX_UNIT_CYLINDER_HALF_HEIGHT = 0.5f;
	inline constexpr int32_t PX_UNIT_CYLINDER_SEGMENTS = 12;
	inline constexpr char PX_UNIT_OCTAGONAL_PRISM_CONVEX_PATH[] =
		"./Resources/PhysX/Primitives/UnitOctagonalPrism.pxconvex";
	inline constexpr int32_t PX_UNIT_OCTAGONAL_PRISM_SEGMENTS = 8;
	inline constexpr char PX_UNIT_WEDGE_CONVEX_PATH[] =
		"./Resources/PhysX/Primitives/UnitWedge.pxconvex";
	inline constexpr _float PX_UNIT_WEDGE_HALF_EXTENT = 0.5f;

	struct PX_FILTER_DESC
	{
		uint32_t iLayer{ PX_DEFAULT_LAYER };
		uint32_t iSimulationMask{ PX_ALL_LAYERS };
		uint32_t iQueryMask{ PX_ALL_LAYERS };
	};

	enum class PX_CCT_BEHAVIOR : uint8_t
	{
		NONE = 0,
		CAN_RIDE = 1 << 0,
		SLIDE = 1 << 1,
		USER_DEFINED_RIDE = 1 << 2
	};

	struct PX_CCT_HIT_DATA
	{
		CGameObject* pGameObject{};
		PX_SHAPE_TYPE eOtherShapeType{ PX_SHAPE_TYPE::BOX };
		uint32_t iOtherShapeSubIndex{ std::numeric_limits<uint32_t>::max() };
		_float3 vWorldPosition{};
		_float3 vWorldNormal{};
		_float3 vMoveDirection{};
		_float fMoveLength{};
	};

	struct PX_CCT_STANDING_DATA
	{
		CHandle hGameObject{};
		PX_SHAPE_TYPE eShapeType{ PX_SHAPE_TYPE::BOX };
		uint32_t iShapeSubIndex{ std::numeric_limits<uint32_t>::max() };
		_bool bHasShapeData{};
	};

	struct PX_CCT_OBSTACLE_HIT_DATA
	{
		const void* pUserData{};
		_float3 vWorldPosition{};
		_float3 vWorldNormal{};
		_float3 vMoveDirection{};
		_float fMoveLength{};
	};

	struct PX_SYNC_DATA
	{
		_float3 vPos{};
		_float4 vQuat{};
	};

	inline constexpr uint32_t PX_MAX_CONTACT_POINTS = 8u;

	struct PX_CONTACT_POINT_DATA
	{
		_float3 vWorldPosition{};
		_float3 vWorldNormal{};
		_float3 vImpulse{};
		_float fSeparation{};
	};

	struct PX_ON_COLLISION_DATA
	{
		std::array<PX_CONTACT_POINT_DATA, PX_MAX_CONTACT_POINTS> Contacts{};
		uint32_t iContactCount{};
		PX_SHAPE_TYPE eSelfShapeType{ PX_SHAPE_TYPE::BOX };
		PX_SHAPE_TYPE eOtherShapeType{ PX_SHAPE_TYPE::BOX };
		uint32_t iSelfShapeSubIndex{ std::numeric_limits<uint32_t>::max() };
		uint32_t iOtherShapeSubIndex{ std::numeric_limits<uint32_t>::max() };
	};

	struct PX_ON_TRIGGER_DATA
	{
		_bool bSelfIsTrigger{};
		PX_SHAPE_TYPE eSelfShapeType{ PX_SHAPE_TYPE::BOX };
		PX_SHAPE_TYPE eOtherShapeType{ PX_SHAPE_TYPE::BOX };
		uint32_t iSelfShapeSubIndex{ std::numeric_limits<uint32_t>::max() };
		uint32_t iOtherShapeSubIndex{ std::numeric_limits<uint32_t>::max() };
	};

	struct PX_ON_JOINT_BREAK_DATA
	{
		CHandle hJointOwner{};
		CHandle hActorA{};
		CHandle hActorB{};
		uint32_t iJointSubIndex{ std::numeric_limits<uint32_t>::max() };
	};

	struct PX_RAYCAST_RESULT
	{
		_bool bHit{ false };
		_float3 vHitpos{}; // 충돌 지점
		_float3 vHitNormal{}; // 충돌 표면의 법선 벡터
		_float fDistance{}; // 시작점으로부터의 거리
		CHandle hGameObject{};
		CGameObject* pGameObject{};
		PX_SHAPE_TYPE eShapeType{ PX_SHAPE_TYPE::BOX };
		uint32_t iShapeSubIndex{ std::numeric_limits<uint32_t>::max() };
	};

	struct PX_QUERY_FILTER_DESC
	{
		uint32_t iQueryMask{ PX_ALL_LAYERS };
		CHandle hIgnoreGameObject{};
		_bool bQueryStatic{ true };
		_bool bQueryDynamic{ true };
		_bool bIncludeTrigger{ false };
	};

	struct PX_RAYCAST_DESC
	{
		_float3 vOrigin{};
		_float3 vDirection{ 0.f, 0.f, 1.f };
		_float fMaxDistance{};
		_bool bHitMeshBothSides{ false };
		PX_QUERY_FILTER_DESC tFilter{};
	};

	enum class PX_QUERY_GEOMETRY_TYPE : uint8_t
	{
		BOX,
		SPHERE,
		CAPSULE
	};

	struct PX_QUERY_GEOMETRY_DESC
	{
		PX_QUERY_GEOMETRY_TYPE eType{ PX_QUERY_GEOMETRY_TYPE::BOX };
		_float3 vBoxHalfExtents{ 0.5f, 0.5f, 0.5f };
		_float fRadius{ 0.5f };
		_float fCapsuleHalfHeight{ 0.5f };
	};

	struct PX_QUERY_POSE
	{
		_float3 vPosition{};
		_float4 vRotation{ 0.f, 0.f, 0.f, 1.f };
	};

	struct PX_SWEEP_DESC
	{
		PX_QUERY_GEOMETRY_DESC tGeometry{};
		PX_QUERY_POSE tPose{};
		_float3 vDirection{ 0.f, 0.f, 1.f };
		_float fMaxDistance{};
		PX_QUERY_FILTER_DESC tFilter{};
	};

	using PX_SWEEP_RESULT = PX_RAYCAST_RESULT;

	struct PX_OVERLAP_DESC
	{
		PX_QUERY_GEOMETRY_DESC tGeometry{};
		PX_QUERY_POSE tPose{};
		PX_QUERY_FILTER_DESC tFilter{};
	};

	struct PX_OVERLAP_RESULT
	{
		_bool bHit{ false };
		CHandle hGameObject{};
		CGameObject* pGameObject{};
		PX_SHAPE_TYPE eShapeType{ PX_SHAPE_TYPE::BOX };
		uint32_t iShapeSubIndex{ std::numeric_limits<uint32_t>::max() };
	};

	struct PX_QUERY_MULTIPLE_STATUS
	{
		_bool bTruncated{};
	};

}
