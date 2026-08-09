#include "pch.h"
#include "LevelTerrain.h"
#include "GameInstance.h"
#include "LevelLoading.h"
#include "FlyCamera.h"
#include "ResCBuffer.h"
#include "BackGround.h"
#include "UiCamera.h"
#include "Terrain.h"
#include "Particle.h"
#include "Player.h"
#include "PlayerThirdPersonCamera.h"
#include "Mon_Weapon.h"
#include "Client_Defines.h"
#include "ComPxCharacterController.h"
#include "ComPxD6Joint.h"
#include "ComPxDistanceJoint.h"
#include "ComPxRevoluteJoint.h"
#include "ComPxRigidBody.h"
#include "OilBarrel.h"
#include "TestPathPlaybackObject.h"
#include "TombBossBullet.h"
#include "RagdollTest.h"
#include "NvClothCape.h"

#include "EnderDragon.h"
#include "BossTMB.h"
#include "TmbGurdian.h"
#include "LightPlacementObject.h"
#include "StarBurst.h"
#include "AmbientSound3DObject.h"
// UI
#include "UIController.h"
#include "UIManager.h"
NS_USING(Client)

CLevelTerrain::CLevelTerrain()
	:CLevel{ ETOUI(LEVEL::TERRAIN) }
{
}

CLevelTerrain::~CLevelTerrain()
{
}

HRESULT CLevelTerrain::Initialize()
{
	Engine::CGameInstance::Get().GameObjectAllResetExceptLayers({
		"00_ENGINE_CINEMATIC_CAMERA"
	});

	GET_SINGLE(UIManager)->CreateFadeOut();

	std::array<CHandle, 6> hOilBarrels{};
	if (FAILED(
		CGameInstance::Get().
			Initialize_EffectLight(15)))
	{
		return E_FAIL;
	}

	{
		CRagdollTest::DESC tDesc{};
		tDesc.sObjectTag = "RagdollTest";
		tDesc.vInitialPosition = { 10.f, 5.f, 10.f };
		if (!E::CGameInstance::Get().AddGameObjectToLayer(
			LEVEL::TERRAIN,
			PROTO_GAMEOBJECT::Prototype_GameObject_RagdollTest,
			"03_PhysXTest",
			&tDesc))
		{
			return E_FAIL;
		}
	}

	{
		
		CGameObject::GAMEOBJECT_DESC Desc{};
		Desc.sObjectTag = "Terrain";
		if (auto h = E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Terrain,
			"01_Terrain", &Desc))
		{
			int x = 0;
		}
	}

	if (FAILED(InitializePathPlaybackTests()))
		return E_FAIL;

	if (FAILED(InitializeOilBarrelPool()))
		return E_FAIL;

	if(false)
	{
		for (uint32_t i = 0; i < 6; ++i)
		{
			COilBarrel::DESC desc{};
			desc.sObjectTag = "OilBarrel";
			desc.vInitialPosition = {
				5.f + (static_cast<_float>(i) + 1.f) * 3.f,
				96.f,
				5.f
			};
			desc.vConvexScale = { 300.f, 300.f, 300.f };
			const auto hOilBarrel =
				E::CGameInstance::Get().AddGameObjectToLayer(
				LEVEL::TERRAIN,
				PROTO_GAMEOBJECT::Prototype_GameObject_OilBarrel,
				"03_PhysXTest",
				&desc);
			if (!hOilBarrel)
				return E_FAIL;

			hOilBarrels[i] = *hOilBarrel;
		}
	}


	const auto hPlayer = SpawnPlayer();
	if (!hPlayer)
		return E_FAIL;
	m_hPlayer = *hPlayer;

	{
		CNvClothCape::DESC Desc{};
		Desc.sObjectTag = "NvClothCape";
		Desc.hTarget = *hPlayer;
		Desc.sResourceGroup = LEVEL::TERRAIN;
		Desc.sModelResourceTag =
			"PLAYER_CAPE_MODEL_RESOURCE";
		Desc.sClothMeshResourceTag =
			"PLAYER_CAPE_CLOTH_RESOURCE";
		Desc.sTargetModelComponentTag =
			"ComCModelIntance";
		Desc.sAttachBoneName =
			"Spine3";
		Desc.vLocalPosition =
			{ 0.05f, 0.08f, 0.f };
		E::CGameInstance::Get().JsonDeSerialize(
			"./Resources/NvCloth/CollisionRigs/"
			"ProfessorCape.nvclothcollision.json",
			Desc.tBodyCollisionRig,
			E::NVCLOTH_COLLISION_RIG_ROOT,
			false);
		if (!E::CGameInstance::Get().
			AddGameObjectToLayer(
				LEVEL::TERRAIN,
				PROTO_GAMEOBJECT::
					Prototype_GameObject_NvClothCape,
				"03_Player",
				&Desc))
		{
			return E_FAIL;
		}
	}

	//if (FAILED(InitializeJointTests(*hPlayer, hOilBarrels)))
	//	return E_FAIL;

	if (FAILED(InitializeCamerasAndLighting(hPlayer)))
		return E_FAIL;


	//{
	//	CAmbientSound3DObject::DESC desc{};
	//	desc.sObjectTag = "Ambient_Wind";
	//
	//	desc.tSoundData.sName = "Wind";
	//	desc.tSoundData.sSoundPath =
	//		"./Resources/SampleClient/Sound/PowerSong.mp3";
	//	desc.tSoundData.vPosition = { 10.f, 2.f, 5.f };
	//	desc.tSoundData.fMinDistance = 3.f;
	//	desc.tSoundData.fMaxDistance = 30.f;
	//	desc.tSoundData.fVolume = 0.8f;
	//	desc.tSoundData.bLoop = true;
	//	desc.tSoundData.bAutoPlay = true;
	//	desc.tSoundData.eRolloff = SOUND_3D_ROLLOFF::LINEAR;
	//
	//	CGameInstance::Get().AddGameObjectToLayer(
	//		ES_EngineProtoMajorType::PERMANENT,
	//		ES_EngineProtoGameObject::Prototype_GameObject_AmbientSound3D,
	//		"Layer_AmbientSound",
	//		&desc);
	//}
	//
	if (FAILED(SpawnMonster(hPlayer)))
		return E_FAIL;
	return S_OK;
}

HRESULT CLevelTerrain::InitializePathPlaybackTests()
{
	using TEST_CASE = CTestPathPlaybackObject::TEST_CASE;

	struct TEST_PLACEMENT
	{
		const char* pObjectTag{};
		TEST_CASE eTestCase{};
		_float3 vPosition{};
		_float3 vRotation{};
		_float fPlaybackRate{ 1.f };
	};

	const std::array<TEST_PLACEMENT, 7> Placements{
		TEST_PLACEMENT{
			"PathTest_StartLocal",
			TEST_CASE::START_LOCAL_LINEAR,
			{ 13.f, 0.f, 5.f },
			{ 0.f, 45.f, 0.f } },
		TEST_PLACEMENT{
			"PathTest_World",
			TEST_CASE::WORLD_LINEAR,
			{ 20.f, 0.f, 5.f } },
		TEST_PLACEMENT{
			"PathTest_Catmull",
			TEST_CASE::CATMULL_FACE_DIRECTION,
			{ 34.f, 0.f, 5.f } },
		TEST_PLACEMENT{
			"PathTest_Loop",
			TEST_CASE::LOOP_RECTANGLE,
			{ 44.f, 0.f, 5.f } },
		TEST_PLACEMENT{
			"PathTest_PingPong",
			TEST_CASE::PING_PONG_ROTATION,
			{ 54.f, 0.f, 5.f } },
		TEST_PLACEMENT{
			"PathTest_EventCustom",
			TEST_CASE::EVENT_TO_CUSTOM,
			{ 64.f, 0.f, 5.f } },
		TEST_PLACEMENT{
			"PathTest_EasingSegments",
			TEST_CASE::EASING_SEGMENTS,
			{ 74.f, 0.f, 5.f } }
	};

	for (const TEST_PLACEMENT& Placement : Placements)
	{
		CTestPathPlaybackObject::DESC Desc{};
		Desc.sObjectTag = Placement.pObjectTag;
		Desc.eTestCase = Placement.eTestCase;
		Desc.vInitialPosition = Placement.vPosition;
		Desc.vInitialRotation = Placement.vRotation;
		Desc.fPlaybackRate = Placement.fPlaybackRate;
		if (!CGameInstance::Get().AddGameObjectToLayer(
			LEVEL::TERRAIN,
			PROTO_GAMEOBJECT::Prototype_GameObject_TestPathPlayback,
			"03_PathPlaybackTest",
			&Desc))
		{
			return E_FAIL;
		}
	}

	return S_OK;
}

HRESULT CLevelTerrain::InitializeTombBossBulletTest(CHandle hPlayer)
{
	CTombBossBullet::DESC Desc{};
	Desc.sObjectTag = "TombBossBullet_PathTest";
	Desc.vInitialPosition = { 5.f, 10.f, 10.f };
	XMStoreFloat4(
		&Desc.vInitialRotation,
		XMQuaternionRotationAxis(
			XMVectorSet(0.f, 1.f, 0.f, 0.f),
			XMConvertToRadians(
				m_fTombBossBulletSpawnYawDegrees)));
	Desc.hTarget = hPlayer;
	Desc.fArcMoveSpeed = 68.f;
	Desc.fArcHeight = 2.f;
	Desc.fArcLifeTime = 6.f;
	Desc.fRadius = 0.5f;
	Desc.fPlaybackRate = 1.f;
	// 실제 보스 구체 이펙트가 확정되면 이 이름만 교체하면 된다.
	Desc.sEffectName = "Boss_StarBurst_A";

	return CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::TERRAIN,
		PROTO_GAMEOBJECT::Prototype_GameObject_TombBossBullet,
		"03_PathPlaybackTest",
		&Desc)
		? S_OK
		: E_FAIL;
}

HRESULT CLevelTerrain::InitializeJointTests(
	CHandle hPlayer,
	const std::array<CHandle, 6>& hOilBarrels)
{
	auto& gameInstance = E::CGameInstance::Get();
	auto* pPlayer =
		gameInstance.GetGameObjectByHandleT<CPlayer>(hPlayer);
	auto* pCharacterController =
		pPlayer
		? pPlayer->GetComponent<CComPxCharacterController>(
			"ComPxCharacterController")
		: nullptr;
	if (!pPlayer || !pCharacterController)
		return E_FAIL;

	std::array<COilBarrel*, 6> pOilBarrels{};
	for (size_t i = 0; i < hOilBarrels.size(); ++i)
	{
		pOilBarrels[i] =
			gameInstance.GetGameObjectByHandleT<COilBarrel>(
				hOilBarrels[i]);
		if (!pOilBarrels[i] || !pOilBarrels[i]->GetRigidBody())
			return E_FAIL;
	}

	CComPxDistanceJoint::DESC tHeadJointDesc{};
	tHeadJointDesc.pCharacterControllerA = pCharacterController;
	tHeadJointDesc.pRigidBodyB = pOilBarrels[0]->GetRigidBody();
	tHeadJointDesc.fMinDistance = 0.f;
	tHeadJointDesc.fMaxDistance = 5.f;
	tHeadJointDesc.fTolerance = 0.025f;
	tHeadJointDesc.bMinDistanceEnabled = false;
	tHeadJointDesc.bMaxDistanceEnabled = true;
	tHeadJointDesc.bSpringEnabled = false;
	tHeadJointDesc.bCollisionEnabled = false;
	tHeadJointDesc.bVisualizationEnabled = true;
	tHeadJointDesc.iJointSubIndex = 100u;

	//CComPxDistanceJoint* pHeadJoint =
	//	gameInstance.AddPxJoint<CComPxDistanceJoint>(
	//		*pPlayer,
	//		"ComPxDistanceJoint_OilBarrelHead",
	//		tHeadJointDesc);
	//if (!pHeadJoint)
	//{
	//	return E_FAIL;
	//}

	for (size_t i = 0; i + 1 < pOilBarrels.size(); ++i)
	{
		if (!pOilBarrels[i]->CreateDistanceJoint(
			pOilBarrels[i + 1],
			3.f,
			static_cast<uint32_t>(101u + i)))
		{
			return E_FAIL;
		}
	}

	{
		COilBarrel::DESC tBarrelDesc{};
		tBarrelDesc.sObjectTag = "OilBarrel_RevoluteTest";
		tBarrelDesc.vInitialPosition = { 22.f, 3.f, 18.f };
		tBarrelDesc.vConvexScale = { 300.f, 300.f, 300.f };

		const auto hRevoluteBarrel =
			gameInstance.AddGameObjectToLayer(
				LEVEL::TERRAIN,
				PROTO_GAMEOBJECT::Prototype_GameObject_OilBarrel,
				"03_PhysXTest",
				&tBarrelDesc);
		if (!hRevoluteBarrel)
			return E_FAIL;

		auto* pRevoluteBarrel =
			gameInstance.GetGameObjectByHandleT<COilBarrel>(
				*hRevoluteBarrel);
		if (!pRevoluteBarrel ||
			!pRevoluteBarrel->GetRigidBody())
		{
			return E_FAIL;
		}

		CComPxRevoluteJoint::DESC tJointDesc{};
		tJointDesc.pRigidBodyB =
			pRevoluteBarrel->GetRigidBody();
		tJointDesc.bPreserveCurrentPose = false;
		tJointDesc.tLocalFrameA.vPosition =
			tBarrelDesc.vInitialPosition;
		tJointDesc.tLocalFrameA.vRotation = {
			0.f,
			0.f,
			0.70710678f,
			0.70710678f
		};
		tJointDesc.tLocalFrameB.vRotation =
			tJointDesc.tLocalFrameA.vRotation;
		tJointDesc.fLowerLimitDegrees = 0.f;
		tJointDesc.fUpperLimitDegrees = 179.9f;
		tJointDesc.bLimitEnabled = true;
		tJointDesc.fDriveVelocityDegreesPerSecond = 0.f;
		tJointDesc.fDriveForceLimit = 0.f;
		tJointDesc.fDriveGearRatio = 1.f;
		tJointDesc.bDriveEnabled = false;
		tJointDesc.bDriveFreeSpin = false;
		tJointDesc.bCollisionEnabled = false;
		tJointDesc.bVisualizationEnabled = true;
		tJointDesc.iJointSubIndex = 200u;

		CComPxRevoluteJoint* pRevoluteJoint =
			gameInstance.AddPxJoint<CComPxRevoluteJoint>(
				*pRevoluteBarrel,
				"ComPxRevoluteJoint_WorldTest",
				tJointDesc);
		if (!pRevoluteJoint)
		{
			return E_FAIL;
		}
	}

	{
		constexpr size_t D6_TEST_COUNT = 6;
		constexpr _float D6_TEST_START_X = 30.f;
		constexpr _float D6_TEST_SPACING = 4.f;
		constexpr _float D6_TEST_Y = 5.f;
		constexpr _float D6_TEST_Z = 5.f;

		std::array<COilBarrel*, D6_TEST_COUNT> pD6Barrels{};
		std::array<_float3, D6_TEST_COUNT> vD6Positions{};

		for (size_t i = 0; i < D6_TEST_COUNT; ++i)
		{
			vD6Positions[i] = {
				D6_TEST_START_X +
					static_cast<_float>(i) * D6_TEST_SPACING,
				D6_TEST_Y,
				D6_TEST_Z
			};

			COilBarrel::DESC tBarrelDesc{};
			tBarrelDesc.sObjectTag =
				"OilBarrel_D6Test_" + std::to_string(i);
			tBarrelDesc.vInitialPosition = vD6Positions[i];
			tBarrelDesc.vConvexScale = { 300.f, 300.f, 300.f };

			const auto hD6Barrel =
				gameInstance.AddGameObjectToLayer(
					LEVEL::TERRAIN,
					PROTO_GAMEOBJECT::Prototype_GameObject_OilBarrel,
					"03_PhysXTest",
					&tBarrelDesc);
			if (!hD6Barrel)
				return E_FAIL;

			pD6Barrels[i] =
				gameInstance.GetGameObjectByHandleT<COilBarrel>(
					*hD6Barrel);
			if (!pD6Barrels[i] ||
				!pD6Barrels[i]->GetRigidBody())
			{
				return E_FAIL;
			}
		}

		const auto AddD6WorldJoint =
			[](
				COilBarrel* pBarrel,
				const char* pComponentTag,
				CComPxD6Joint::DESC& tJointDesc)
			-> CComPxD6Joint*
			{
				if (!pBarrel || !pBarrel->GetRigidBody())
					return nullptr;

				tJointDesc.pRigidBodyB =
					pBarrel->GetRigidBody();
				tJointDesc.bCollisionEnabled = false;
				tJointDesc.bVisualizationEnabled = true;

				return CGameInstance::Get()
					.AddPxJoint<CComPxD6Joint>(
						*pBarrel,
						pComponentTag,
						tJointDesc);
			};

		// D6 0: ��� �� ���. Fixed Joint�� ���� ���� ������ Ȯ���Ѵ�.
		{
			CComPxD6Joint::DESC tJointDesc{};
			tJointDesc.bPreserveCurrentPose = true;
			tJointDesc.iJointSubIndex = 300u;

			if (!AddD6WorldJoint(
				pD6Barrels[0],
				"ComPxD6Joint_AllLocked",
				tJointDesc))
			{
				return E_FAIL;
			}
		}

		// D6 1: X�ุ -3~3 �̵��ϰ� �Ѱ������� �ݹ��ϴ� �����̴�.
		{
			CComPxD6Joint::DESC tJointDesc{};
			tJointDesc.bPreserveCurrentPose = true;
			tJointDesc.eMotions[
				static_cast<size_t>(CComPxD6Joint::AXIS::X)] =
				CComPxD6Joint::MOTION::LIMITED;
			tJointDesc.tLinearLimits[
				static_cast<size_t>(CComPxD6Joint::AXIS::X)] = {
				.fLower = -3.f,
				.fUpper = 3.f,
				.tResponse = {
					.fRestitution = 0.75f,
					.fBounceThreshold = 0.1f
				}
			};
			tJointDesc.iJointSubIndex = 301u;

			if (!AddD6WorldJoint(
				pD6Barrels[1],
				"ComPxD6Joint_LinearLimit",
				tJointDesc) ||
				!pD6Barrels[1]->GetRigidBody()
					->SetLinearVelocity({ 6.f, 0.f, 0.f }))
			{
				return E_FAIL;
			}
		}

		// D6 2: Y�� ���� �ȿ��� ��ǥ ��ġ�� �̵��ϴ� ���� ����.
		{
			CComPxD6Joint::DESC tJointDesc{};
			tJointDesc.bPreserveCurrentPose = true;
			tJointDesc.eMotions[
				static_cast<size_t>(CComPxD6Joint::AXIS::Y)] =
				CComPxD6Joint::MOTION::LIMITED;
			tJointDesc.tLinearLimits[
				static_cast<size_t>(CComPxD6Joint::AXIS::Y)] = {
				.fLower = -2.f,
				.fUpper = 2.f
			};
			tJointDesc.tDrives[
				static_cast<size_t>(CComPxD6Joint::DRIVE::Y)] = {
				.fStiffness = 80.f,
				.fDamping = 15.f,
				.fForceLimit = 1000.f,
				.bAcceleration = true
			};
			tJointDesc.tDrivePose.vPosition = {
				0.f,
				1.5f,
				0.f
			};
			tJointDesc.iJointSubIndex = 302u;

			if (!AddD6WorldJoint(
				pD6Barrels[2],
				"ComPxD6Joint_LinearDrive",
				tJointDesc))
			{
				return E_FAIL;
			}
		}

		// D6 3: ���� X��(Twist)�� -60~60���� ������ �ӵ� ����.
		{
			CComPxD6Joint::DESC tJointDesc{};
			tJointDesc.bPreserveCurrentPose = true;
			tJointDesc.eMotions[
				static_cast<size_t>(
					CComPxD6Joint::AXIS::TWIST)] =
				CComPxD6Joint::MOTION::LIMITED;
			tJointDesc.tTwistLimit = {
				.fLowerDegrees = -60.f,
				.fUpperDegrees = 60.f,
				.tResponse = {
					.fRestitution = 0.25f,
					.fBounceThreshold = 5.f
				}
			};
			tJointDesc.tDrives[
				static_cast<size_t>(
					CComPxD6Joint::DRIVE::TWIST)] = {
				.fStiffness = 0.f,
				.fDamping = 20.f,
				.fForceLimit = 1000.f,
				.bAcceleration = true
			};
			tJointDesc.vDriveAngularVelocityDegreesPerSecond = {
				120.f,
				0.f,
				0.f
			};
			tJointDesc.iJointSubIndex = 303u;

			if (!AddD6WorldJoint(
				pD6Barrels[3],
				"ComPxD6Joint_TwistDrive",
				tJointDesc))
			{
				return E_FAIL;
			}
		}

		// D6 4: ������ ���忡 �����ϰ� Swing Cone �ȿ��� ��鸮�� ����.
		{
			constexpr _float PENDULUM_ANCHOR_HEIGHT = 3.f;

			CComPxD6Joint::DESC tJointDesc{};
			tJointDesc.bPreserveCurrentPose = false;
			tJointDesc.tLocalFrameA.vPosition = {
				vD6Positions[4].x,
				vD6Positions[4].y + PENDULUM_ANCHOR_HEIGHT,
				vD6Positions[4].z
			};
			tJointDesc.tLocalFrameB.vPosition = {
				0.f,
				PENDULUM_ANCHOR_HEIGHT,
				0.f
			};
			tJointDesc.eMotions[
				static_cast<size_t>(
					CComPxD6Joint::AXIS::SWING_Y)] =
				CComPxD6Joint::MOTION::LIMITED;
			tJointDesc.eMotions[
				static_cast<size_t>(
					CComPxD6Joint::AXIS::SWING_Z)] =
				CComPxD6Joint::MOTION::LIMITED;
			tJointDesc.tSwingLimit = {
				.fYDegrees = 60.f,
				.fZDegrees = 40.f,
				.tResponse = {
					.fRestitution = 0.1f,
					.fBounceThreshold = 1.f
				}
			};
			tJointDesc.iJointSubIndex = 304u;

			if (!AddD6WorldJoint(
				pD6Barrels[4],
				"ComPxD6Joint_SwingCone",
				tJointDesc) ||
				!pD6Barrels[4]->GetRigidBody()
					->AddImpulse({ 8.f, 0.f, 0.f }))
			{
				return E_FAIL;
			}
		}

		// D6 5: �� ȸ������ SLERP�� ��ǥ �ڼ��� ���ߴ� ���� ����.
		{
			CComPxD6Joint::DESC tJointDesc{};
			tJointDesc.bPreserveCurrentPose = true;
			tJointDesc.eMotions[
				static_cast<size_t>(
					CComPxD6Joint::AXIS::TWIST)] =
				CComPxD6Joint::MOTION::FREE;
			tJointDesc.eMotions[
				static_cast<size_t>(
					CComPxD6Joint::AXIS::SWING_Y)] =
				CComPxD6Joint::MOTION::FREE;
			tJointDesc.eMotions[
				static_cast<size_t>(
					CComPxD6Joint::AXIS::SWING_Z)] =
				CComPxD6Joint::MOTION::FREE;
			tJointDesc.eAngularDriveMode =
				CComPxD6Joint::ANGULAR_DRIVE_MODE::SLERP;
			tJointDesc.tDrives[
				static_cast<size_t>(
					CComPxD6Joint::DRIVE::SLERP)] = {
				.fStiffness = 80.f,
				.fDamping = 15.f,
				.fForceLimit = 1000.f,
				.bAcceleration = true
			};

			XMStoreFloat4(
				&tJointDesc.tDrivePose.vRotation,
				XMQuaternionRotationRollPitchYaw(
					XMConvertToRadians(35.f),
					XMConvertToRadians(50.f),
					XMConvertToRadians(20.f)));
			tJointDesc.iJointSubIndex = 305u;

			if (!AddD6WorldJoint(
				pD6Barrels[5],
				"ComPxD6Joint_SlerpDrive",
				tJointDesc))
			{
				return E_FAIL;
			}
		}
	}

	return S_OK;
}

HRESULT CLevelTerrain::InitializeCamerasAndLighting(
	const std::optional<CHandle>& hPlayer)
{
	if (FAILED(SpawnPlayerCamera(hPlayer)))
		return E_FAIL;

	{
		E::CCameraObject::CAMERA_DESC Desc{};
		Desc.eProj = E::CCameraObject::PROJ::ORTHOGRAPHIC;
		Desc.fNear = 0.f;
		Desc.fFar = 1.f;
		Desc.fWidth = g_iWinSizeX;
		Desc.fHeight = g_iWinSizeY;
		Desc.sObjectTag = "UICam";
		Desc.vEye = { 0.f, 0.f, -0.1f };

		if (auto uiCam = E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_UICamera",
			"99_CAMERA", &Desc))
		{
			if (FAILED(E::CGameInstance::Get().RegistCamera("UI", uiCam.value())))
			{
				MSG_BOX("FailedToRegistCamera");
				return E_FAIL;
			}
		}
	}
	{
		E::CCameraObject::CAMERA_DESC Desc{};
		Desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
		Desc.vAt = { 0.f, 0.f, 0.f };
		Desc.vEye = { 0.f, 0.f, -5.f };
		Desc.fAspect = { g_iWinSizeX / (E::_float)g_iWinSizeY };
		Desc.fFovY = 75.f;
		Desc.fNear = 0.01f;
		Desc.fFar = 100.f;
		Desc.sObjectTag = "FlyCam";

		if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_FlyCamera",
			"99_CAMERA", &Desc))
		{
			if (FAILED(E::CGameInstance::Get().RegistCamera("FLY", flyCam.value())))
			{
				int x = 0;
			}
			CGameInstance::Get().SetActiveCamera("FLY");
		}
	}
	_float3(0.577f, -0.577f, 0.577f);
	CGameInstance::Get().Add_DirectionalLight({ 0.577f, -0.577f, 0.577f }, { 1.f, 1.f, 1.f }, 10.f);

	return S_OK;
}

HRESULT CLevelTerrain::SpawnMonster(const std::optional<CHandle>& hPlayer)
{
	{
		CBossTMB::TMB_DESC TmbDesc{};
		TmbDesc.sObjectTag = "BossTmb";
		TmbDesc.TargetHandle = hPlayer.value();
		TmbDesc.LevelTag = MagicEnumToStringView(LEVEL::TERRAIN);
		XMStoreFloat3(&TmbDesc.vPos, XMVectorSet(5, 5, 5, 1));
		TmbDesc.ReSourceTag = "Model_Resource_TombBoss";
		TmbDesc.BeHaviorTag = "./Resources/json/BeHavior/BossDef.json";
		XMStoreFloat3(&TmbDesc.vScale, XMVectorSet(6.f, 6.f, 6.f, 1));
		auto BossTmb = E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_BossTMB, "02_BossTmb", &TmbDesc);

		if (!BossTmb)
		{
			MSG_BOX("Create BossTmb Failed in TERRAIN");
			return E_FAIL;
		}
	}
	{
		CTmbGurdian::TMBGURDIAN_DESC TmbGurdianDesc{};
		TmbGurdianDesc.sObjectTag = "TmbGurdian";
		TmbGurdianDesc.TargetHandle = hPlayer.value();
		TmbGurdianDesc.LevelTag = MagicEnumToStringView(LEVEL::TERRAIN);
		XMStoreFloat3(&TmbGurdianDesc.vPos, XMVectorSet(44.f, 15.f, 65.f, 1.f));
		TmbGurdianDesc.ReSourceTag = "Model_Resource_TMBGurdian";

		TmbGurdianDesc.resBeHaviorMajor = "BTJSON";
		TmbGurdianDesc.resBeHaviorMinor = "TOMB_BT_GURDIAN3";
		TmbGurdianDesc.WeaponProtoName = MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_Mace);
		TmbGurdianDesc.WeaponResourceName = "Model_Resource_Mace";
		TmbGurdianDesc.MonType = MONSTER_TYPE::NORMAL;

		XMStoreFloat3(&TmbGurdianDesc.vScale, XMVectorSet(2.f, 2.f, 2.f, 1));
		auto BossTmb = E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_TMBGurdian, "02_TmbGurdian", &TmbGurdianDesc);

		if (!BossTmb)
		{
			MSG_BOX("Create TmbGurdian Failed in Terrain");
			return E_FAIL;
		}
	}

	{
		CEnderDragon::DRAGON_DESC Dragon{};
		Dragon.sObjectTag = "Dragon";
		Dragon.TargetHandle = hPlayer.value();
		Dragon.LevelTag = MagicEnumToStringView(LEVEL::TERRAIN);
		XMStoreFloat3(&Dragon.vPos, XMVectorSet(44.f, 15.f, 65.f, 1.f));
		Dragon.ReSourceTag = "Model_Resource_Dragon";
		Dragon.resBeHaviorMajor = "BTJSON";
		Dragon.resBeHaviorMinor = "ENDERDRAGON";
		Dragon.MonType = MONSTER_TYPE::BOSS;

		auto pDragon = E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon, "02_Dragon", &Dragon);

		if (!pDragon)
		{
			MSG_BOX("Create Dragon Failed in Terrain");
			return E_FAIL;
		}
	
	}
	
	
	{
		CLightPlacementObject::DESC desc{};
		desc.sObjectTag = "TerrainLightPlacement";
		desc.sLightFileName = "Level_Terrain";

		if (!CGameInstance::Get().AddGameObjectToLayer(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoGameObject::
			Prototype_GameObject_LightPlacement,
			"Layer_LightPlacement",
			&desc))
		{
			return E_FAIL;
		}
	}
	return S_OK;
}

void CLevelTerrain::Update(E::_float fTimeDelta)
{
	{
		if (!m_bCreatePlayScreenUI)
		{
			m_bCreatePlayScreenUI = true;
			CGameObject::GAMEOBJECT_DESC Desc{};
			Desc.sObjectTag = "UIController";

			GET_SINGLE(UIManager)->SetUIController(E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_TERRAIN", "Prototype_GameObject_UIController",
				"UIController", &Desc));
		}
	}

	GET_SINGLE(UIManager)->UpdateRootUIHandles();

	Picking();
}

HRESULT CLevelTerrain::Render()
{
	return S_OK;
}

void CLevelTerrain::UpdateGUI()
{
	ImGui::Begin("Terrain");
	if (ImGui::CollapsingHeader("Oil Barrel Pool"))
	{
		auto* pPoolManager =
			CGameInstance::Get().GetGameObjectPoolManager();
		if (pPoolManager)
		{
			ImGui::Text(
				"Total: %zu | Active: %zu | Available: %zu",
				pPoolManager->GetTotalCount("Terrain_OilBarrelPool"),
				pPoolManager->GetActiveCount("Terrain_OilBarrelPool"),
				pPoolManager->GetAvailableCount("Terrain_OilBarrelPool"));
		}
		ImGui::DragFloat3(
			"Acquire Position",
			&m_vOilBarrelPoolSpawnPosition.x,
			0.1f);

		if (ImGui::Button("Acquire Oil Barrel"))
		{
			COilBarrel::POOL_ACQUIRE_DESC tAcquireDesc{};
			tAcquireDesc.vPosition = m_vOilBarrelPoolSpawnPosition;
			if (!pPoolManager ||
				!pPoolManager->Acquire(
					"Terrain_OilBarrelPool",
					&tAcquireDesc))
			{
				DEBUG_LOG("[OilBarrelPool] Acquire failed.\n");
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Release All"))
		{
			if (pPoolManager)
				pPoolManager->ReleaseAll("Terrain_OilBarrelPool");
		}

		ImGui::Separator();
	}
	ImGui::DragFloat(
		"Tomb Boss Bullet Spawn Yaw",
		&m_fTombBossBulletSpawnYawDegrees,
		1.f,
		-180.f,
		180.f,
		"%.1f deg");
	if (ImGui::Button("Spawn Tomb Boss Bullet"))
	{
		if (!CGameInstance::Get().GetGameObjectByHandle(m_hPlayer) ||
			FAILED(InitializeTombBossBulletTest(m_hPlayer)))
		{
			DEBUG_LOG("[Terrain] Failed to spawn TombBossBullet.\n");
		}
	}
	ImGui::Separator();
	//Resources();
	//Objects();
	//BeHaviors();

	//ImGui::Text("Select Resoruce : %s ", m_SelectResourceTag.c_str());
	//ImGui::Text("Select Object : %s ", m_SelectObjecteTag.c_str());
	//ImGui::Text("Select Behavior : %s ", m_SelectFileName.c_str());
	//m_fPos.y = 0.f;
	ImGui::Text("X :%2.f ", m_fPos.x); ImGui::SameLine(); ImGui::Text("Y : %2.f ", m_fPos.y); ImGui::SameLine(); ImGui::Text("Z : %2.f", m_fPos.z);

	//if (ImGui::Button("Activate Med Debris Physics"))
	//{
	//	for (const CHandle& handle : m_MedDebrisHandles)
	//	{
	//		if (auto* debris = CGameInstance::Get()
	//			.GetGameObjectByHandleT<CMedDebris>(handle))
	//		{
	//			debris->RequestActivatePhysics();
	//		}
	//	}
	//}

	//if (!m_SelectResourceTag.empty() && !m_SelectObjecteTag.empty())
	//{
	//	if (ImGui::Button("SPAWN : "))
	//		m_bSpawn = !m_bSpawn;
	//	ImGui::SameLine(); ImGui::Text(m_bSpawn == true ? "TRUE" : "FALSE");

	//	const bool bGuiHovered = ImGui::IsWindowHovered(
	//		ImGuiHoveredFlags_AllowWhenBlockedByPopup);

	//	if (m_bSpawn && !bGuiHovered)
	//	{
	//		if (CGameInstance::Get().MouseDown(MOUSEKEYSTATE::LB))
	//		{

	//			CTestGob::MONSTER_DESC Desc{};
	//			Desc.sObjectTag = "Gobline";
	//			Desc.LevelTag = m_strLevelName;
	//			Desc.ReSourceTag = m_SelectResourceTag;
	//			Desc.BeHaviorTag = m_SelectFilePath;
	//			Desc.vPos = m_fPos;
	//			Desc.vPos.y += 50.f;
	//			auto Gobline = E::CGameInstance::Get().AddGameObjectToLayer(m_strLevelName, m_SelectObjecteTag, "02_Gobline", &Desc);
	//		}
	//	}


	//}

	//if (ImGui::TreeNode("Particle Test Monster"))
	//{

	//	CTestGob::MONSTER_DESC Desc{};

	//	Desc.bDonMove = true;
	//	Desc.sObjectTag = "Gobline";
	//	Desc.LevelTag = m_strLevelName;
	//	XMStoreFloat3(&Desc.vPos, XMVectorSet(0, 0, 0, 1));
	//	XMStoreFloat3(&Desc.vRot, XMVectorSet(0, 1, 0, 1));
	//	Desc.fAngle = 180.f;
	//	if (ImGui::Button("BOSS"))
	//	{
	//		Desc.ReSourceTag = "Model_Resource_TombProtector";
	//		Desc.BeHaviorTag = "./Resources/json/BeHavior/BossDef.json";
	//		XMStoreFloat3(&Desc.vScale, XMVectorSet(5.f, 5.f, 5.f, 1));

	//		auto Gobline = E::CGameInstance::Get().AddGameObjectToLayer(m_strLevelName, "Prototype_GameObject_Gobline", "02_Gobline", &Desc);

	//	}
	//	if (ImGui::Button("NORMAL"))
	//	{
	//		Desc.ReSourceTag = "Model_Resource_TombNormalProtector";
	//		Desc.BeHaviorTag = "./Resources/json/BeHavior/NormalDef.json";
	//		XMStoreFloat3(&Desc.vScale, XMVectorSet(2.f, 2.f, 2.f, 1));

	//		auto Gobline = E::CGameInstance::Get().AddGameObjectToLayer(m_strLevelName, "Prototype_GameObject_Gobline", "02_Gobline", &Desc);

	//	}
	//	ImGui::TreePop();
	//}

	ImGui::End();

}

HRESULT CLevelTerrain::InitializeOilBarrelPool()
{
	auto* pPoolManager =
		CGameInstance::Get().GetGameObjectPoolManager();
	if (!pPoolManager)
		return E_FAIL;

	CGameObjectPoolManager::POOL_DESC tPoolDesc{};
	tPoolDesc.iPrewarmCount = 8;
	tPoolDesc.iGrowCount = 4;
	tPoolDesc.iMaxCount = 32;
	tPoolDesc.eExhaustPolicy =
		CGameObjectPoolManager::EXHAUST_POLICY::GROW;
	tPoolDesc.fnCreate = [iObjectIndex = size_t{ 0 }]() mutable
		-> std::optional<CHandle>
	{
		COilBarrel::DESC tDesc{};
		tDesc.sObjectTag =
			"PooledOilBarrel_" + std::to_string(iObjectIndex++);
		tDesc.vInitialPosition = { 0.f, -1000.f, 0.f };
		tDesc.vConvexScale = { 300.f, 300.f, 300.f };
		tDesc.tFilter = PX_FILTER_DESC{
			.iLayer = ETOUI(COLLISION_LAYER::WORLD_DYNAMIC),
			.iSimulationMask = PX_ALL_LAYERS,
			.iQueryMask = PX_ALL_LAYERS
		};

		return CGameInstance::Get().AddGameObjectToLayer(
			LEVEL::TERRAIN,
			PROTO_GAMEOBJECT::Prototype_GameObject_OilBarrel,
			"03_OilBarrelPool",
			&tDesc);
	};

	return pPoolManager->RegisterPool(
		"Terrain_OilBarrelPool",
		std::move(tPoolDesc)) ? S_OK : E_FAIL;
}

void CLevelTerrain::Picking()
{
	if (auto pObj = CGameInstance::Get().GetActiveCamera())
	{
		const _float2 vMousePosition = CGameInstance::Get().GetMousePos();
		const _float2 vViewportSize = CGameInstance::Get().GetClientScreenSize();
		const auto& [ori, dir] = pObj->GetRayFromScreenPixel(vMousePosition, vViewportSize);
		PX_RAYCAST_RESULT rayResult{};
		if (CGameInstance::Get().GetPhysXManager()
			->RayCast(
				{
					.vOrigin = ori,
					.vDirection = dir,
					.fMaxDistance = 1000.f,
					.tFilter = {.iQueryMask = ETOUI(COLLISION_LAYER::WORLD_STATIC)}
				}
				, rayResult))
		{
			m_fPos = rayResult.vHitpos;
		}
	}
}
void CLevelTerrain::Resources()
{

	ImGui::Text("RESOURCE "); ImGui::SameLine(70.f);
	if (ImGui::BeginCombo("##ReSource", m_SelectResourceTag.c_str()))
	{
		auto Resource = CGameInstance::Get().GetResource(m_strLevelName);
		for (const auto& [key, value] : Resource)
		{
			const _char* pName = key.str;
			_bool isSelected = m_SelectResourceTag == pName;
			ImGui::PushID(pName);
			if (ImGui::Selectable(pName, isSelected))
				m_SelectResourceTag = pName;

			if (isSelected)
				ImGui::SetItemDefaultFocus();
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}
}
void CLevelTerrain::Objects()
{
	ImGui::Text("OBJECT "); ImGui::SameLine(70.f);
	if (ImGui::BeginCombo("##Object", m_SelectObjecteTag.c_str()))
	{
		for (const auto& key : CGameInstance::Get().GetPrototypeTags(m_strLevelName))
		{
			const _char* pName = key.str;
			_bool isSelected = m_SelectObjecteTag == pName;
			ImGui::PushID(pName);
			if (ImGui::Selectable(pName, isSelected))
				m_SelectObjecteTag = pName;

			if (isSelected)
				ImGui::SetItemDefaultFocus();
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}

}
void CLevelTerrain::BeHaviors()
{
	ImGui::Text("BEHAVIOR "); ImGui::SameLine(70.f);
	if (ImGui::BeginCombo("##Behavior", m_SelectFileName.c_str()))
	{
		auto& Prototypes = m_BeHaviorJsonList;
		for (const auto& [key, value] : Prototypes)
		{
			const _char* pName = key.c_str();
			_bool isSelected = m_SelectFileName == pName;
			ImGui::PushID(pName);
			if (ImGui::Selectable(pName, isSelected))
			{
				m_SelectFileName = key;
				m_SelectFilePath = value;
			}

			if (isSelected)
				ImGui::SetItemDefaultFocus();
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}

}

HRESULT CLevelTerrain::SpawnPlayerCamera(std::optional<CHandle> hPlayer)
{
	if (!hPlayer) return E_FAIL;
	CPlayerThirdPersonCamera::DESC Desc{};
	Desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
	Desc.vAt = { 10.f, 50.f, 10.f };
	Desc.vEye = { 10.f, 53.f, 5.f };
	Desc.fAspect = { g_iWinSizeX / (E::_float)g_iWinSizeY };
	Desc.fFovY = 75.f;
	Desc.fNear = 0.1f;
	Desc.fFar = 1000.f;
	Desc.sObjectTag = "PlayerCamera";
	Desc.hTarget = hPlayer.value();

	auto hPlayerCamera = E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::TERRAIN,
		PROTO_GAMEOBJECT::Prototype_GameObject_PlayerThirdPersonCamera,
		"101_CAMERA",
		&Desc);
	if (!hPlayerCamera || FAILED(E::CGameInstance::Get().RegistCamera(
		"PlayerCamera", *hPlayerCamera)))
	{
		return E_FAIL;
	}
	return S_OK;
}
std::optional<CHandle> CLevelTerrain::SpawnPlayer()
{
	CPlayer::DESC PlayerDesc{};
	PlayerDesc.sObjectTag = "Player";
	PlayerDesc.vInitialPosition = { 5.f, 100.f, 5.f };
	PlayerDesc.LevelTag = LEVEL::TERRAIN;
	PlayerDesc.tFilter = PX_FILTER_DESC{
		.iLayer = ETOUI(COLLISION_LAYER::PLAYER_BODY),
		.iSimulationMask = PX_ALL_LAYERS,
		.iQueryMask =
			ETOUI(COLLISION_LAYER::WORLD_STATIC) |
			ETOUI(COLLISION_LAYER::ENEMY_BODY)
	};
	return  E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::TERRAIN,
		PROTO_GAMEOBJECT::Prototype_GameObject_Player,
		"03_Player",
		&PlayerDesc);
}

Engine::UPtr<CLevelTerrain> CLevelTerrain::Create()
{
	auto pInstance = Engine::UPtr<CLevelTerrain>(new CLevelTerrain{});
	pInstance->SetDeferredInitialization();
	return pInstance;
}


void CLevelTerrain::Free()
{
	if (auto* pPoolManager =
		CGameInstance::Get().GetGameObjectPoolManager())
	{
		pPoolManager->UnregisterPool("Terrain_OilBarrelPool");
	}
	CLevel::Free();
}
