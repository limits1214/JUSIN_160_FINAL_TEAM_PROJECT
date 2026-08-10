#include "pch.h"
#include "GameInstanceInitLoader.h"
#include "GameInstance.h"

#include "TimeProvider.h"
#include "ImGuiManager.h"
#include "DInputManager.h"
#include "GraphicDevice.h"
#include "LevelManager.h"
#include "Level.h"
#include "Prototype.h"
#include "Collider.h"
#include "ComConstantBuffer.h"
#include "FlyCamera.h"
#include "UICamera.h"
#include "CinematicCamera.h"
#include "ComBeHavior.h"
#include "ComModelInstance.h"
#include "ComStaticModelInstance.h"
#include "ComAnimator.h"
#include "ComSocket.h"
#include "Light.h"
#include "ComCollider.h"
#include "MapMeshObject.h"
#include "PhysXCollisionProxyObject.h"
#include "AmbientSound2DObject.h"
#include "AmbientSound3DObject.h"
#include "LightPlacementObject.h"

#include "ComPxBoxCollider.h"
#include "ComPxCapsuleCollider.h"
#include "ComPxSphereCollider.h"
#include "ComPxConvexCollider.h"
#include "ComPxCollider.h"
#include "ComPxRigidBody.h"
#include "ComPxDistanceJoint.h"
#include "ComPxD6Joint.h"
#include "ComPxFixedJoint.h"
#include "ComPxRagdoll.h"
#include "ComNvCloth.h"
#include "ComPxRevoluteJoint.h"
#include "ComPxTriMeshCollider.h"
#include "ComPxCharacterController.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
#include "ComSound.h"
#include "ComPathPlayback.h"
#include "StateMachine.h"

#include "ParticleManager.h"
#include "Particle.h"

#include "ButtonComponent.h"
#include "TweenComponent.h"
#include "ShadowCamera.h"


NS_BEGIN(Engine)

HRESULT CGameInstanceInitLoader::InitLoadStart()
{
	LOG_MEMORY("***start***");


	if (FAILED(LoadBuffer()))
	{
		return E_FAIL;
	}
	LOG_MEMORY("End LoadBuffer()");

	if (FAILED(LoadRenderState()))
	{
		return E_FAIL;
	}
	LOG_MEMORY("End LoadRenderState()");

	if (FAILED(LoadShader()))
	{
		return E_FAIL;
	}
	LOG_MEMORY("End LoadShader()");

	if (FAILED(LoadTexture()))
	{
		return E_FAIL;
	}
	LOG_MEMORY("End LoadTexture()");

	if (FAILED(LoadModel()))
	{
		return E_FAIL;
	}
	LOG_MEMORY("End LoadModel()");

	if (FAILED(LoadLua()))
	{
		return E_FAIL;
	}
	LOG_MEMORY("End LoadLua()");

	if (FAILED(LoadPrototype()))
	{
		return E_FAIL;
	}
	LOG_MEMORY("End LoadPrototype()");


	LOG_MEMORY("***end***");
	return S_OK;
}

HRESULT CGameInstanceInitLoader::LoadPrototype()
{
	if (FAILED(LoadPrototypeComponent()))
	{
		return E_FAIL;
	}

	if (FAILED(LoadPrototypeGameObject()))
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT CGameInstanceInitLoader::LoadPrototypeGameObject()
{
	if (CGameInstance::Get().AddPrototype(ES_EngineProtoMajorType::CAMERAS, ES_EngineProtoGameObject::Prototype_GameObject_FlyCamera, CFlyCamera::Create()))
	{
		return E_FAIL;
	}
	if (CGameInstance::Get().AddPrototype(ES_EngineProtoMajorType::CAMERAS, ES_EngineProtoGameObject::Prototype_GameObject_ShadowCamera, CShadowCamera::Create()))
	{
		return E_FAIL;
	}
	if (CGameInstance::Get().AddPrototype("CAMERAS", "Prototype_GameObject_UICamera", CUICamera::Create()))
	{
		return E_FAIL;
	}
	if (CGameInstance::Get().AddPrototype(ES_EngineProtoMajorType::CAMERAS, ES_EngineProtoGameObject::Prototype_GameObject_CinematicCamera, CCinematicCamera::Create()))
	{
		return E_FAIL;
	}
	

	if (CGameInstance::Get().AddPrototype("PERMANENT", "Prototype_GameObject_MapMeshObject", CMapMeshObject::Create()))
	{
		return E_FAIL;
	}

	if (CGameInstance::Get().AddPrototype(
		"PERMANENT", "Prototype_GameObject_PhysXCollisionProxy",
		CPhysXCollisionProxyObject::Create()))
	{
		return E_FAIL;
	}
	if (CGameInstance::Get().AddPrototype(
		ES_EngineProtoMajorType::PERMANENT,
		ES_EngineProtoGameObject::Prototype_GameObject_AmbientSound2D,
		CAmbientSound2DObject::Create()))
	{
		return E_FAIL;
	}
	if (CGameInstance::Get().AddPrototype(
		ES_EngineProtoMajorType::PERMANENT,
		ES_EngineProtoGameObject::Prototype_GameObject_AmbientSound3D,
		CAmbientSound3DObject::Create()))
	{
		return E_FAIL;
	}
	if (CGameInstance::Get().AddPrototype(
		ES_EngineProtoMajorType::PERMANENT,
		ES_EngineProtoGameObject::Prototype_GameObject_LightPlacement,
		CLightPlacementObject::Create()))
	{
		return E_FAIL;
	}
	//if (CGameInstance::Get().AddPrototype("CAMERAS", "Prototype_GameObject_PlayerCamera", CPlayerCamera::Create()))
	//{
	//	return E_FAIL;
	//}
	return S_OK;
}

HRESULT CGameInstanceInitLoader::LoadBuffer()
{
	if (FAILED(LoadBufferConstant()))
	{
		return E_FAIL;
	}
	if (FAILED(LoadBufferVertexIndex()))
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT CGameInstanceInitLoader::LoadBufferConstant()
{
	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PASS, E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_PER_PASS) })))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT, E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_PER_OBJECT) })))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PARTICLE, E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_PER_PARTICLE) })))
		{
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_SPAWN_PARTICLE, E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_PARTICLE_SPAWN) })))
		{
			return E_FAIL;
		}
	}


	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerUI", E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_PER_UI) })))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_MATERIAL", E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_MATERIAL) })))
		{
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_INIT_PARTICLE, E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_INIT_PARTICLE) })))
		{
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_BONE, E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(_float4x4) * 512 })))
		{
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_GPU_SKIN_MESH", E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(E::GPU_SKIN_MESH_CONSTANTS) })))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PART_ATTACHMENT, E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(E::CB_PART_ATTACHMENT) })))
		{
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON", E::CResStructuredBuffer::Create()))
		{
			E::CResStructuredBuffer::DESC Desc{};
	
		Desc.iNumElements = 512;
	
		Desc.iStructureByteStride = sizeof(E::GPU_ANIM_INSTANCE_DATA);
	
		Desc.pInitialData = nullptr;
	
		Desc.bAppendConsume = false;
	
		if (FAILED(res->Load(Desc)))
		{
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_PART_INSTANCE", E::CResStructuredBuffer::Create()))
	{
		E::CResStructuredBuffer::DESC Desc{};
		Desc.iNumElements = 512;
		Desc.iStructureByteStride = sizeof(E::GPU_PART_INSTANCE_DATA);
		Desc.pInitialData = nullptr;
		Desc.bAppendConsume = false;
		if (FAILED(res->Load(Desc))) return E_FAIL;
	}

	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_FINALBONEMATRIX", E::CResStructuredBuffer::Create()))
	{
		E::CResStructuredBuffer::DESC Desc{};

		Desc.iNumElements = 512 * 512;

		Desc.iStructureByteStride =sizeof(_float4x4);

		Desc.pInitialData =nullptr;

		Desc.bAppendConsume =false;


		if (FAILED(res->Load(Desc)))
		{
			return E_FAIL;
		}
	}

	// CPU/CPU+GPU evaluation 전용 bone palette.
	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_CPU_BONEMATRIX",E::CResStructuredBuffer::Create()))
	{
		E::CResStructuredBuffer::DESC Desc{};
		Desc.iNumElements = 512 * 512;
		Desc.iStructureByteStride = sizeof(_float4x4);
		Desc.pInitialData = nullptr;
		Desc.bAppendConsume = false;
		Desc.iBindFlags = D3D11_BIND_SHADER_RESOURCE;
		if (FAILED(res->Load(Desc)))
			return E_FAIL;
	}

	// UI용
	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_SpellMeter", E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_SPELLMETER) })))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_MiniMap", E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_MINIMAP) })))
		{
			return E_FAIL;
		}
	}

	return S_OK;
}

HRESULT CGameInstanceInitLoader::LoadBufferVertexIndex()
{
	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex", E::CResQuadTexBuffer::Create()))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	return S_OK;
}



HRESULT CGameInstanceInitLoader::LoadPrototypeComponent()
{
	if (CGameInstance::Get().AddPrototype(
		ES_EngineProtoMajorType::PERMANENT,
		ES_EngineProtoComponent::Prototype_Component_Transform, CComTransform::Create()))
	{
		return E_FAIL;
	}

	if (CGameInstance::Get().AddPrototype(
		"PERMANENT", "Prototype_Component_ConstantBuffer", CComConstantBuffer::Create()))
	{
		return E_FAIL;
	}

	if (CGameInstance::Get().AddPrototype(
		"PERMANENT", "Prototype_Component_ModelInstance", CComModelInstance::Create()))
	{
		return E_FAIL;
	}

	if (CGameInstance::Get().AddPrototype(
		"PERMANENT", "Prototype_Component_StaticModelInstance", CComStaticModelInstance::Create()))
	{
		return E_FAIL;
	}

	if (CGameInstance::Get().AddPrototype(
		"PERMANENT", "Prototype_Component_Animator", CComAnimator::Create()))
	{
		return E_FAIL;
	}

	if (CGameInstance::Get().AddPrototype(
		"PERMANENT", "Prototype_Component_Socket", CComSocket::Create()))
	{
		return E_FAIL;
	}

	if (CGameInstance::Get().AddPrototype("BEHAVIOR", "Prototype_Component_BeHavior", CComBeHavior::Create()))
	{
		return E_FAIL;
	}

	if (CGameInstance::Get().AddPrototype("COLLIDER", "Prototype_Component_Collider", CComCollider::Create()))
	{
		return E_FAIL;
	}

	if (CGameInstance::Get().AddPrototype(
		ES_EngineProtoMajorType::PERMANENT,
		ES_EngineProtoComponent::Prototype_Component_ComCharacterMoveIntent,
		CComCharacterMoveIntent::Create()))
	{
		return E_FAIL;
	}

	if (CGameInstance::Get().AddPrototype(
		ES_EngineProtoMajorType::PERMANENT,
		ES_EngineProtoComponent::Prototype_Component_ComCharacterMotor,
		CComCharacterMotor::Create()))
	{
		return E_FAIL;
	}

	if (CGameInstance::Get().AddPrototype(
		ES_EngineProtoMajorType::PERMANENT,
		ES_EngineProtoComponent::Prototype_Component_ComSound,
		CComSound::Create()))
	{
		return E_FAIL;
	}

	if (CGameInstance::Get().AddPrototype(
		ES_EngineProtoMajorType::PERMANENT,
		ES_EngineProtoComponent::Prototype_Component_ComPathPlayback,
		CComPathPlayback::Create()))
	{
		return E_FAIL;
	}

	if (CGameInstance::Get().AddPrototype("PERMANENT", "Prototype_Component_StateMachine", CStateMachine::Create()))
	{
		return E_FAIL;
	}

	// 피직스관련
	{

		if (CGameInstance::Get().AddPrototype(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxBoxCollider, CComPxBoxCollider::Create()))
		{
			return E_FAIL;
		}
		if (CGameInstance::Get().AddPrototype(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxCapsuleCollider, CComPxCapsuleCollider::Create()))
		{
			return E_FAIL;
		}
		if (CGameInstance::Get().AddPrototype(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxSphereCollider, CComPxSphereCollider::Create()))
		{
			return E_FAIL;
		}
		if (CGameInstance::Get().AddPrototype(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxConvexCollider, CComPxConvexCollider::Create()))
		{
			return E_FAIL;
		}
		if (CGameInstance::Get().AddPrototype(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxTriMeshCollider, CComPxTriMeshCollider::Create()))
		{
			return E_FAIL;
		}
		if (CGameInstance::Get().AddPrototype(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRigidBody, CComPxRigidBody::Create()))
		{
			return E_FAIL;
		}
		if (CGameInstance::Get().AddPrototype(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxCharacterController, CComPxCharacterController::Create()))
		{
			return E_FAIL;
		}
		if (CGameInstance::Get().AddPrototype(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxFixedJoint, CComPxFixedJoint::Create()))
		{
			return E_FAIL;
		}
		if (CGameInstance::Get().AddPrototype(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxDistanceJoint, CComPxDistanceJoint::Create()))
		{
			return E_FAIL;
		}
		if (CGameInstance::Get().AddPrototype(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRevoluteJoint, CComPxRevoluteJoint::Create()))
		{
			return E_FAIL;
		}
		if (CGameInstance::Get().AddPrototype(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxD6Joint, CComPxD6Joint::Create()))
		{
			return E_FAIL;
		}
		if (CGameInstance::Get().AddPrototype(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRagdoll, CComPxRagdoll::Create()))
		{
			return E_FAIL;
		}
		if (CGameInstance::Get().AddPrototype(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComNvCloth, CComNvCloth::Create()))
		{
			return E_FAIL;
		}
	}

	// 루아
	if (CGameInstance::Get().AddPrototype(ES_EngineProtoMajorType::UI, "Prototype_Component_ButtonUI", CButtonComponent::Create()))
	{
		return E_FAIL;
	}

	if (CGameInstance::Get().AddPrototype(ES_EngineProtoMajorType::UI, "Prototype_Component_Tween", TweenComponent::Create()))
	{
		return E_FAIL;
	}

	return S_OK;
}
NS_END

HRESULT CGameInstanceInitLoader::LoadRenderState()
{
	if (FAILED(LoadBlendState()))
	{
		return E_FAIL;
	}
	if (FAILED(LoadRasterizerState()))
	{
		return E_FAIL;
	}
	if (FAILED(LoadDepthStencilState()))
	{
		return E_FAIL;
	}
	if (FAILED(LoadSamplerState()))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CGameInstanceInitLoader::LoadBlendState()
{
	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_BLEND", E::CResBlendState::Create()))
	{
		D3D11_BLEND_DESC blendDesc{};
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		if (FAILED(res->Load(blendDesc))) return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, "BS_BLEND_NONE", E::CResBlendState::Create()))
	{
		D3D11_BLEND_DESC blendDesc{};
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		blendDesc.RenderTarget[0].BlendEnable = FALSE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		if (FAILED(res->Load(blendDesc))) return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_BLEND", E::CResBlendState::Create()))
	{
		D3D11_BLEND_DESC blendDesc{};
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		if (FAILED(res->Load(blendDesc))) return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_BLEND_ADD", E::CResBlendState::Create()))
	{
		D3D11_BLEND_DESC blendDesc{};
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		if (FAILED(res->Load(blendDesc))) return E_FAIL;
	}

		if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_EFFECT", E::CResBlendState::Create()))
		{
			D3D11_BLEND_DESC blendDesc{};
			blendDesc.AlphaToCoverageEnable = FALSE;
			blendDesc.IndependentBlendEnable = FALSE;
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			res->Load(blendDesc);
		}

		if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, "BS_ADDITIVE", E::CResBlendState::Create()))
		{
			D3D11_BLEND_DESC blendDesc{};
			blendDesc.AlphaToCoverageEnable = FALSE;
			blendDesc.IndependentBlendEnable = FALSE;
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;   // 텍스처 알파로 밝기(강도) 조절
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;        // 배경을 100% 보존하고 그 위에 더한다 ← 핵심
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			if (FAILED(res->Load(blendDesc))) return E_FAIL;
		}
	return S_OK;
}

HRESULT CGameInstanceInitLoader::LoadRasterizerState()
{
	
	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_BACKCULL, E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_BACK;
		desc.DepthClipEnable = TRUE;
		if (FAILED(res->Load(desc))) return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_FRONTCULL, E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_FRONT;
		desc.DepthClipEnable = TRUE;
		if (FAILED(res->Load(desc))) return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL, E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.DepthClipEnable = TRUE;
		desc.DepthBias = 10;
		desc.SlopeScaledDepthBias = 0.5f;
		desc.DepthBiasClamp = 0.0f;
		if (FAILED(res->Load(desc))) return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_WIREFRAME_NOCULL, E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_WIREFRAME;
		desc.CullMode = D3D11_CULL_NONE;
		desc.DepthClipEnable = TRUE;
		if (FAILED(res->Load(desc))) return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, "RS_SOLID_BACKCULL_DEPTHBIAS", E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_BACK;
		desc.DepthClipEnable = TRUE;
		desc.DepthBias = -100;
		desc.SlopeScaledDepthBias = -1.0f;
		desc.DepthBiasClamp = 0.0f;
		if (FAILED(res->Load(desc))) return E_FAIL;
	}

	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, "RS_SOLID_BLOCK_VOXEL_SHADOW", E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_FRONT;
		desc.DepthClipEnable = TRUE;
		if (FAILED(res->Load(desc))) return E_FAIL;
	}

	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, "RS_ALPHATEST_BLOCK_VOXEL_SHADOW", E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.DepthClipEnable = TRUE;
		desc.DepthBias = 10;
		desc.SlopeScaledDepthBias = 0.5f;
		desc.DepthBiasClamp = 0.0f;
		if (FAILED(res->Load(desc))) return E_FAIL;
	}

	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, "RS_MULTIPLE_SHADOW", E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.DepthClipEnable = TRUE;
		desc.DepthBias = 2;
		desc.SlopeScaledDepthBias = 0.1f;
		desc.DepthBiasClamp = 0.0f;
		if (FAILED(res->Load(desc))) return E_FAIL;
	}

	return S_OK;
}

HRESULT CGameInstanceInitLoader::LoadDepthStencilState()
{
	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, "DS_NO_DEPTHWRITE", E::CResDepthStencilState::Create()))
	{
		D3D11_DEPTH_STENCIL_DESC depthDesc{};
		depthDesc.DepthEnable = TRUE;
		depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
		if (FAILED(res->Load(depthDesc))) return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, "DS_DEPTHWRITE", E::CResDepthStencilState::Create()))
	{
		D3D11_DEPTH_STENCIL_DESC depthDesc{};
		depthDesc.DepthEnable = TRUE;
		depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
		depthDesc.StencilEnable = FALSE;
		depthDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		depthDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		if (FAILED(res->Load(depthDesc))) return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, "DS_DEPTHREAD", E::CResDepthStencilState::Create()))
	{
		D3D11_DEPTH_STENCIL_DESC depthDesc{};
		depthDesc.DepthEnable = TRUE;
		depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
		depthDesc.StencilEnable = FALSE;
		depthDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		depthDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		if (FAILED(res->Load(depthDesc))) return E_FAIL;
	}

	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, "DS_NO_DEPTHSTENCIL", E::CResDepthStencilState::Create()))
	{
		D3D11_DEPTH_STENCIL_DESC depthDesc{};
		depthDesc.DepthEnable = FALSE;
		depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
		depthDesc.StencilEnable = FALSE;
		if (FAILED(res->Load(depthDesc))) return E_FAIL;
	}

	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, "DS_DBG_LINE_DEPTH_ON", E::CResDepthStencilState::Create()))
	{
		D3D11_DEPTH_STENCIL_DESC depthDesc{};
		depthDesc.DepthEnable = TRUE;
		depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		depthDesc.StencilEnable = FALSE;
		if (FAILED(res->Load(depthDesc))) return E_FAIL;
	}

		if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_STATE, "DS_ALPHA_BLEND_DEPTH", E::CResDepthStencilState::Create()))
		{
			D3D11_DEPTH_STENCIL_DESC depthDesc{};
			depthDesc.DepthEnable = TRUE;
			depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	
			depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
			depthDesc.StencilEnable = FALSE;
			depthDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
			depthDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
			res->Load(depthDesc);
		}
	return S_OK;
}

HRESULT CGameInstanceInitLoader::LoadSamplerState()
{
	// LinearWrap
	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP, CResSamplerState::Create()))
	{
		if (FAILED(res->Load(D3D11_SAMPLER_DESC{
			.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			.AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
			.ComparisonFunc = D3D11_COMPARISON_NEVER,
			.MinLOD = 0,
			.MaxLOD = D3D11_FLOAT32_MAX,
			}))) {
			return E_FAIL;
		}
		CGameInstance::Get().GetGraphicDeviceContext()->VSSetSamplers(0, 1, res->GetSamplerState().GetAddressOf());
		CGameInstance::Get().GetGraphicDeviceContext()->PSSetSamplers(0, 1, res->GetSamplerState().GetAddressOf());
		CGameInstance::Get().GetGraphicDeviceContext()->CSSetSamplers(0, 1, res->GetSamplerState().GetAddressOf());

	}
	// LinearClamp
	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_CLAMP, CResSamplerState::Create()))
	{
		if (FAILED(res->Load(D3D11_SAMPLER_DESC{
			.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
			.ComparisonFunc = D3D11_COMPARISON_NEVER,
			.MinLOD = 0,
			.MaxLOD = D3D11_FLOAT32_MAX,
			}))) {
			return E_FAIL;
		}
		CGameInstance::Get().GetGraphicDeviceContext()->PSSetSamplers(1, 1, res->GetSamplerState().GetAddressOf());
		CGameInstance::Get().GetGraphicDeviceContext()->CSSetSamplers(1, 1, res->GetSamplerState().GetAddressOf());

	}
	// PointWrap
	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_POINT_WRAP, CResSamplerState::Create()))
	{
		D3D11_SAMPLER_DESC samplerDesc{};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

		samplerDesc.MinLOD = 0.f;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		if (FAILED(res->Load(samplerDesc))) return E_FAIL;
		CGameInstance::Get().GetGraphicDeviceContext()->VSSetSamplers(2, 1, res->GetSamplerState().GetAddressOf());
		CGameInstance::Get().GetGraphicDeviceContext()->PSSetSamplers(2, 1, res->GetSamplerState().GetAddressOf());
		CGameInstance::Get().GetGraphicDeviceContext()->CSSetSamplers(2, 1, res->GetSamplerState().GetAddressOf());

	}
	// PointClamp
	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_POINT_CLAMP, CResSamplerState::Create()))
	{
		if (FAILED(res->Load(D3D11_SAMPLER_DESC{
			.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
			.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
			.ComparisonFunc = D3D11_COMPARISON_NEVER,
			.MinLOD = 0,
			.MaxLOD = D3D11_FLOAT32_MAX,
			}))) {
			return E_FAIL;
		}
		CGameInstance::Get().GetGraphicDeviceContext()->VSSetSamplers(3, 1, res->GetSamplerState().GetAddressOf());
		CGameInstance::Get().GetGraphicDeviceContext()->PSSetSamplers(3, 1, res->GetSamplerState().GetAddressOf());
		CGameInstance::Get().GetGraphicDeviceContext()->CSSetSamplers(3, 1, res->GetSamplerState().GetAddressOf());

	}
	// PointWrapNoMip
	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_POINT_WRAP_NOMIP, CResSamplerState::Create()))
	{
		if (FAILED(res->Load(D3D11_SAMPLER_DESC{
			.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
			.AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
			.ComparisonFunc = D3D11_COMPARISON_NEVER,
			.MinLOD = 0,
			.MaxLOD = 0.0f,
			}))) {
			return E_FAIL;
		}
		CGameInstance::Get().GetGraphicDeviceContext()->PSSetSamplers(4, 1, res->GetSamplerState().GetAddressOf());
		CGameInstance::Get().GetGraphicDeviceContext()->CSSetSamplers(4, 1, res->GetSamplerState().GetAddressOf());

	}
	// AnisotropicWrap
	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_ANISOTROPIC_WRAP, CResSamplerState::Create()))
	{
		if (FAILED(res->Load(D3D11_SAMPLER_DESC{
			.Filter = D3D11_FILTER_ANISOTROPIC,
			.AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
			.MipLODBias = 0.f,
			.MaxAnisotropy = 16,
			.ComparisonFunc = D3D11_COMPARISON_NEVER,
			.MinLOD = 0,
			.MaxLOD = D3D11_FLOAT32_MAX,
			}))) {
			return E_FAIL;
		}
		CGameInstance::Get().GetGraphicDeviceContext()->PSSetSamplers(5, 1, res->GetSamplerState().GetAddressOf());
		CGameInstance::Get().GetGraphicDeviceContext()->CSSetSamplers(5, 1, res->GetSamplerState().GetAddressOf());

	}
	// ShadowSampler
	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_SAHDOW, CResSamplerState::Create()))
	{
		D3D11_SAMPLER_DESC sampDesc{};
		sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		
		sampDesc.BorderColor[0] = 1.f;
		sampDesc.BorderColor[1] = 1.f;
		sampDesc.BorderColor[2] = 1.f;
		sampDesc.BorderColor[3] = 1.f;

		sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
		if (FAILED(res->Load(sampDesc)))
		{
			return E_FAIL;
		}

		CGameInstance::Get().GetGraphicDeviceContext()->PSSetSamplers(6, 1, res->GetSamplerState().GetAddressOf());
		CGameInstance::Get().GetGraphicDeviceContext()->CSSetSamplers(6, 1, res->GetSamplerState().GetAddressOf());

	}
	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_STATE, "State_SS_LinearBorder", CResSamplerState::Create()))
	{
		D3D11_SAMPLER_DESC sampDesc{};
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;

		sampDesc.BorderColor[0] = 0.f; // R
		sampDesc.BorderColor[1] = 0.f; // G
		sampDesc.BorderColor[2] = 0.f; // B
		sampDesc.BorderColor[3] = 0.f; // A

		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

		if (FAILED(res->Load(sampDesc)))
		{
			return E_FAIL;
		}

		// 요청하신 7번 슬롯에 바인딩 (픽셀 셰이더)
		CGameInstance::Get().GetGraphicDeviceContext()->PSSetSamplers(7, 1, res->GetSamplerState().GetAddressOf());
	}
	return S_OK;
}

HRESULT CGameInstanceInitLoader::LoadShader()
{
	//ShaderFiles
	if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(
		TAG_RES_GRP_PERMANENT_SHADER, TAG_RES_VS_TERRAIN,
		"./ShaderFiles/Terrain/Shader_Terrain.hlsl"))
	{
		if (FAILED(res->Load())) return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(
		TAG_RES_GRP_PERMANENT_SHADER, TAG_RES_PS_TERRAIN,
		"./ShaderFiles/Terrain/Shader_Terrain.hlsl"))
	{
		if (FAILED(res->Load())) return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTex", "./ShaderFiles/QuadTex/QuadTex.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTex", "./ShaderFiles/QuadTex/QuadTex.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTexUI", "./ShaderFiles/UI/QuadTexUI.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTexUI", "./ShaderFiles/UI/QuadTexUI.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_DISOLVE", "./ShaderFiles/UI/DISOLVE.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_DISOLVE", "./ShaderFiles/UI/DISOLVE.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_GAMEOVERMASK", "./ShaderFiles/UI/GameOverMask.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_GAMEOVERMASK", "./ShaderFiles/UI/GameOverMask.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_VIDEOOBJECT", "./ShaderFiles/UI/VideoObject.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_VIDEOOBJECT", "./ShaderFiles/UI/VideoObject.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTexFlipBook", "./ShaderFiles/UI/QuadTexFlipBook.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTexFlipBook", "./ShaderFiles/UI/QuadTexFlipBook.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TwoTone", "./ShaderFiles/UI/TwoTone.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TwoTone", "./ShaderFiles/UI/TwoTone.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_Cursor", "./ShaderFiles/UI/Cursor.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_Cursor", "./ShaderFiles/UI/Cursor.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadCol", "./ShaderFiles/QuadCol/QuadCol.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadCol", "./ShaderFiles/QuadCol/QuadCol.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_UpdateParticle", "./ShaderFiles/Particle/Shader_Particle_Compute.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_SpawnParticle", "./ShaderFiles/Particle/Shader_Particle_Spawn_Compute.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_InitParticle", "./ShaderFiles/Particle/Shader_CS_Init.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_HizCopyDepth", "./ShaderFiles/Hiz/Shader_CS_HizCopyDepth.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_HizMipPyramid", "./ShaderFiles/Hiz/Shader_CS_HizMipPyramid.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_MapMeshGpuCull", "./ShaderFiles/Hiz/Shader_CS_MapMeshGlobalCull.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_MapMeshBuildIndirectArgs", "./ShaderFiles/Hiz/Shader_CS_MapMeshBuildIndirectArgs.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_ClearByOwner", "./ShaderFiles/Particle/CS_ClearByOwner.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_TransformOwner", "./ShaderFiles/Particle/CS_TransformOwner.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER,
		"CS_ChangeColorByOwner",
		"./ShaderFiles/Particle/CS_ChangeColorByOwner.hlsl"))
	{
		if (FAILED(res->Load()))
			return E_FAIL;
	}
			
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_Animation", "./ShaderFiles/TestModel/Shader_Animation_Compute.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_CPU_GPU_Skinning", "./ShaderFiles/TestModel/Shader_CPU_GPU_Skinning_Compute.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_SpellMeter", "./ShaderFiles/UI/SpellMeter.hlsl")) // 스펠이펙트
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_SpellMeter", "./ShaderFiles/UI/SpellMeter.hlsl"))
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_9SliceUI", "./ShaderFiles/UI/9SliceUI.hlsl")) // UI HP Bar
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_9SliceUI", "./ShaderFiles/UI/9SliceUI.hlsl"))
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_MiniMap", "./ShaderFiles/UI/MiniMap.hlsl"))
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_MiniMap", "./ShaderFiles/UI/MiniMap.hlsl"))
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}



	// model shader
	{
		if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(
			TAG_RES_GRP_PERMANENT_SHADER,
			"VS_NvCloth",
			"./ShaderFiles/TestModel/Shader_NvCloth.hlsl"))
		{
			if (FAILED(res->Load()))
				return E_FAIL;
		}
		if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(
			TAG_RES_GRP_PERMANENT_SHADER,
			"PS_NvCloth",
			"./ShaderFiles/TestModel/Shader_NvCloth.hlsl"))
		{
			if (FAILED(res->Load()))
				return E_FAIL;
		}
		if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(
			TAG_RES_GRP_PERMANENT_SHADER,
			"VS_NvClothShadow",
			"./ShaderFiles/TestModel/Shader_NvCloth.hlsl"))
		{
			if (FAILED(res->Load(CResShader::DESC{
				.sEntryPoint = "VSShadow",
				.sTarget = "vs_5_0" })))
			{
				return E_FAIL;
			}
		}
		if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(
			TAG_RES_GRP_PERMANENT_SHADER,
			"VS_NvClothPointShadow",
			"./ShaderFiles/TestModel/Shader_NvCloth.hlsl"))
		{
			if (FAILED(res->Load(CResShader::DESC{
				.sEntryPoint = "VSPointShadow",
				.sTarget = "vs_5_0" })))
			{
				return E_FAIL;
			}
		}

		if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim", "./ShaderFiles/TestModel/Shader_VtxMesh_NonInstanced.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}

		if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim", "./ShaderFiles/TestModel/Shader_VtxMesh_Instanced.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}

		if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim_Instanced", "./ShaderFiles/TestModel/Shader_VtxMesh_Instanced.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}

		if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim_Instanced_Foliage", "./ShaderFiles/TestModel/Shader_VtxMesh_Instanced_Foliage.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}

		if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim_Instanced", "./ShaderFiles/TestModel/Shader_VtxMesh_Instanced.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}

			if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim_Instanced", "./ShaderFiles/TestModel/Shader_VtxAnimMesh_Instanced.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
		if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim_CPU_GPU", "./ShaderFiles/TestModel/Shader_VtxAnimMesh_CPU_GPU.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}
		if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim_CPU_Skinning_Instanced","./ShaderFiles/TestModel/Shader_VtxAnimMesh_CPU_Skinning_Instanced.hlsl"))
		{
			if (FAILED(res->Load()))
				return E_FAIL;
		}

		if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim", "./ShaderFiles/TestModel/Shader_VtxAnimMesh.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}
		if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelAnim", "./ShaderFiles/TestModel/Shader_VtxAnimMesh.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}


		if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_PartObject", "./ShaderFiles/TestModel/Shader_VtxPartObject.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}
		if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PartObject", "./ShaderFiles/TestModel/Shader_VtxPartObject.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}

	}
	return S_OK;
}

HRESULT CGameInstanceInitLoader::LoadTexture()
{
	// 텍스쳐 없는 경우 대비, 대체 텍스쳐
	{
		if (auto res = E::CGameInstance::Get().AddResource("DEFAULT_TEXTURE", "TEX_DEFAULT_DIFFUSE", E::CResTexture2D::Create("./Resources/Engine/Texture/DefaultTexture/DefaultTex_Diffuse.png")))
		{
			if (FAILED(res->Load()))return E_FAIL;
		}
		if (auto res = E::CGameInstance::Get().AddResource("DEFAULT_TEXTURE", "TEX_DEFAULT_NORMAL", E::CResTexture2D::Create("./Resources/Engine/Texture/DefaultTexture/DefaultTex_Normal.png")))
		{
			if (FAILED(res->Load()))return E_FAIL;
		}
		if (auto res = E::CGameInstance::Get().AddResource("DEFAULT_TEXTURE", "TEX_DEFAULT_SMRO", E::CResTexture2D::Create("./Resources/Engine/Texture/DefaultTexture/DefaultTex_SMRO.png")))
		{
			if (FAILED(res->Load()))return E_FAIL;
		}
		if (auto res = E::CGameInstance::Get().AddResource("DEFAULT_TEXTURE", "TEX_DEFAULT_EMISSIVE", E::CResTexture2D::Create("./Resources/Engine/Texture/DefaultTexture/DefaultTex_Emissive.png")))
		{
			if (FAILED(res->Load()))return E_FAIL;
		}
		if (auto res = E::CGameInstance::Get().AddResource("DEFAULT_TEXTURE", "TEX_DEFAULT_WHITE", E::CResTexture2D::Create("./Resources/Engine/Texture/DefaultTexture/DefaultTex_White.png")))
		{
			if (FAILED(res->Load()))return E_FAIL;
		}
		if (auto res = E::CGameInstance::Get().AddResource("DEFAULT_TEXTURE", "TEX_DEFAULT_NOISE", E::CResTexture2D::Create("./Resources/Engine/Texture/DefaultTexture/DefaultNoise.png")))
		{
			if (FAILED(res->Load()))return E_FAIL;
		}
	}
	return S_OK;
}

HRESULT CGameInstanceInitLoader::LoadLua()
{
	if (!E::CGameInstance::Get().AddResource(ES_EngineResMajorType::PERMANENT_LUA, ES_EngineResLuaScript::LUA_TEST, CResLuaScript::CreateAndLoad("./LuaFiles/Test.lua")))
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT CGameInstanceInitLoader::LoadModel()
{
	if (FAILED(LoadAnimModel()))
	{
		return E_FAIL;
	}
	LOG_MEMORY("End LoadAnimModel()");

	if (FAILED(LoadStaticModel()))
	{
		return E_FAIL;
	}
	LOG_MEMORY("End LoadStaticModel()");
	return S_OK;
}

HRESULT CGameInstanceInitLoader::LoadAnimModel()
{

	//if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>("TEST", "Static_Model_Resource",
	//	CResStaticModel::Create("./Resources/SampleClient/Models/OriginData/Static/HorseStatue.fbx"))) {

	//	E::CResStaticModel::DESC pDesc{};
	//	pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f);

	//	if (FAILED(res->Load(pDesc)))
	//	{
	//		return E_FAIL;
	//	}
	//}
	return S_OK;
}

HRESULT CGameInstanceInitLoader::LoadStaticModel()
{
	
	return S_OK;
}
