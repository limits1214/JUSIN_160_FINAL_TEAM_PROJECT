#pragma once
namespace Engine
{
	enum class ES_EngineProtoMajorType
	{
		PERMANENT,
		CAMERAS,
		PHYSX,
		UI,
	};

	enum class ES_EngineProtoComponent
	{
		Prototype_Component_Transform,
		Prototype_Component_ConstantBuffer,
		Prototype_Component_ModelInstance,
		Prototype_Component_StaticModelInstance,
		Prototype_Component_Animator,
		Prototype_Component_ComCharacterMoveIntent,
		Prototype_Component_ComCharacterMotor,
		Prototype_Component_ComSound,
		Prototype_Component_ComPathPlayback
	};

	enum class ES_EngineProtoPhysXComponent
	{
		Prototype_Component_ComPxBoxCollider,
		Prototype_Component_ComPxCapsuleCollider,
		Prototype_Component_ComPxSphereCollider,
		Prototype_Component_ComPxConvexCollider,
		Prototype_Component_ComPxTriMeshCollider,
		Prototype_Component_ComPxRigidBody,
		Prototype_Component_ComPxCharacterController,
		Prototype_Component_ComPxFixedJoint,
		Prototype_Component_ComPxDistanceJoint,
		Prototype_Component_ComPxRevoluteJoint,
		Prototype_Component_ComPxD6Joint,
		Prototype_Component_ComPxRagdoll,
		Prototype_Component_ComNvCloth
	};

	enum class ES_EngineProtoGameObject
	{
		Prototype_GameObject_FlyCamera,
		Prototype_GameObject_ShadowCamera,
		Prototype_GameObject_UICamera,
		Prototype_GameObject_CinematicCamera,
		Prototype_GameObject_AmbientSound2D,
		Prototype_GameObject_AmbientSound3D,
		Prototype_GameObject_LightPlacement
	};
}
