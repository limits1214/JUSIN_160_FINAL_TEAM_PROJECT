#pragma once

#include "ResDynamicBuffer.h"
#include "ResCBuffer.h"
#include "ResCubeColBuffer.h"
#include "ResDynamicVIBuffer.h"
#include "ResQuadColBuffer.h"
#include "ResQuadFullscreenTexBuffer.h"
#include "ResQuadTexBuffer.h"
#include "ResStructuredBuffer.h"
#include "ComConstantBuffer.h"
#include "ResFmodSound.h"

#include "ResJson.h"

#include "ResComputeShader.h"
#include "ResGeometryShader.h"
#include "ResGeoShaderStreamOut.h"
#include "ResPixelShader.h"
#include "ResTessDomainShader.h"
#include "ResTessHullShader.h"
#include "ResVertexShader.h"

#include "ResBlendState.h"
#include "ResDepthStencilState.h"
#include "ResRasterizerState.h"
#include "ResSamplerState.h"

#include "ResTexture2D.h"
#include "ResDynamicTexture2D.h"
#include "ResOffscreenTexture.h"
#include "ResTexture2DArray.h"
#include "ResTextureCubeMap.h"

#include "ResViewPort.h"

#include "ResFont.h"
#include "ResFontCustom.h"

//#include "ResTestModel.h"
//#include "ResTestModelAnim.h"
//#include "ResTestModelMesh.h"
//#include "ResTestModelChanel.h"
//#include "ResTestModelMaterial.h"
//#include "ResTestModelBone.h"


#include "ResModel.h"
#include "ResStaticModel.h"
#include "ResModelAnim.h"
#include "ResModelMesh.h"
#include "ResStaticModelMesh.h"
#include "ResModelChanel.h"
#include "ResModelMaterial.h"
#include "ResModelBone.h"


#include "ResPhysXTriMeshGeometry.h"
#include "ResPhysXRTTriMeshGeometry.h"
#include "ResPhysXConvexGeometry.h"
#include "ResPhysXRTConvexGeometry.h"
#include "ResPhysXBoxGeometry.h"
#include "ResPhysXSphereGeometry.h"
#include "ResPhysXCapsuleGeometry.h"
#include "ResPhysXMaterial.h"
#include "ResPhysXHeightFieldGeometry.h"
#include "ResPhysXRTHeightFieldGeometry.h"

#include "ResLuaScript.h"

enum class ES_EngineResMajorType
{
	PERMANENT_BUFFER,
	PERMANENT_SHADER,
	PERMANENT_TEXTURE,
	PERMANENT_STATE,
	PERMANENT_VP,
	PERMANENT_LUA,
};

enum class ES_EngineResRasterizerState
{
	RS_SOLID_BACKCULL,
	RS_SOLID_FRONTCULL,
	RS_SOLID_NOCULL,
	RS_WIREFRAME_NOCULL,
};

enum class ES_EngineResSamplerState
{
	SS_LinearWrap,
	SS_PointWrap,
	SS_PointWrapNoMip,
	SS_SHADOW,
};

enum class ES_EngineResConstantBuffer
{
	CB_PerPass,
	CB_PerObject,
	CB_Material,
	CB_PerParticle,
	CB_SpawnParticle,
	CB_InitGPUParticle,
	CB_Bone,
	CB_PerUI,
};

enum class ES_EngineResVertexShader
{
	VS_QuadTex
};

enum class ES_EngineResPixelShader
{
	PS_QuadTex
};

enum class ES_EngineResLuaScript
{
	LUA_TEST,
};

/*
static const char* 선언했던것들은 전부
Deprecated 돼었으니 앞으로 위에서 enum설정하여 사용해주세요
*/

static const char* TAG_RES_GRP_PERMANENT_BUFFER = "PERMANENT_BUFFER";
static const char* TAG_RES_GRP_PERMANENT_SHADER = "PERMANENT_SHADER";
static const char* TAG_RES_GRP_PERMANENT_TEXTURE = "PERMANENT_TEXTURE";
static const char* TAG_RES_GRP_PERMANENT_STATE = "PERMANENT_STATE";
static const char* TAG_RES_GRP_PERMANENT_VP = "PERMANENT_VP";
static const char* TAG_RES_GRP_PERMANENT_PARTICLE_VSSHADER = "PERMANENT_PARTICLE_VSSHADER";
static const char* TAG_RES_GRP_PERMANENT_PARTICLE_PSSHADER = "PERMANENT_PARTICLE_PSSHADER";


// RS_: RasterazerState
// BS_: BlendState
// DS_: DepthStencilState
// SS_: SamplerState
constexpr static const char* TAG_RES_STATE_RS_SOLID_BACKCULL = "RS_SOLID_BACKCULL";
constexpr static const char* TAG_RES_STATE_RS_SOLID_FRONTCULL = "RS_SOLID_FRONTCULL";
constexpr static const char* TAG_RES_STATE_RS_SOLID_NOCULL = "RS_SOLID_NOCULL";
constexpr static const char* TAG_RES_STATE_RS_WIREFRAME_NOCULL = "RS_WIREFRAME_NOCULL";

constexpr static const char* TAG_RES_STATE_SS_LINEAR_WRAP = "SS_LinearWrap";
constexpr static const char* TAG_RES_STATE_SS_LINEAR_CLAMP = "SS_LinearClamp";
constexpr static const char* TAG_RES_STATE_SS_POINT_CLAMP = "SS_PointClamp";
constexpr static const char* TAG_RES_STATE_SS_POINT_WRAP = "SS_PointWrap";
constexpr static const char* TAG_RES_STATE_SS_POINT_WRAP_NOMIP = "SS_PointWrapNoMip";
constexpr static const char* TAG_RES_STATE_SS_ANISOTROPIC_WRAP = "SS_AnisotropicWrap";
constexpr static const char* TAG_RES_STATE_SS_SAHDOW = "SS_SHADOW";

constexpr static const char* TAG_RES_CBUFFER_PASS = "CB_PerPass";
constexpr static const char* TAG_RES_CBUFFER_OBJECT = "CB_PerObject";
constexpr static const char* TAG_RES_CBUFFER_MATERIAL = "CB_PerMaterial";
constexpr static const char* TAG_RES_CBUFFER_PARTICLE = "CB_PerParticle";
constexpr static const char* TAG_RES_CBUFFER_SPAWN_PARTICLE = "CB_SpawnParticle";
constexpr static const char* TAG_RES_CBUFFER_INIT_PARTICLE = "CB_InitGPUParticle";
constexpr static const char* TAG_RES_CBUFFER_BONE = "CB_Bone";
constexpr static const char* TAG_RES_CBUFFER_PART_ATTACHMENT = "CB_GPU_PART_ATTACHMENT";

