#include "pch.h"
#include "LuaManager.h"

#include "CameraObject.h"
#include "DbgLineRender.h"
#include "GameInstance.h"
#include "GameObject.h"

NS_USING(Engine)

CGameObject* CLuaManager::ResolveObject(const CHandle& hObject) const
{
	CGameObject* pObject = CGameInstance::Get().GetGameObjectByHandle(hObject);
	if (!pObject || pObject->GetPendingDestroy())
		return nullptr;

	return pObject;
}

HRESULT CLuaManager::InitializeEngineBindings()
{
	if (FAILED(BindInputAPI()))
		return E_FAIL;
	if (FAILED(BindObjectAPI()))
		return E_FAIL;
	if (FAILED(BindComponentAPI()))
		return E_FAIL;
	if (FAILED(BindTransformAPI()))
		return E_FAIL;
	if (FAILED(BindCameraAPI()))
		return E_FAIL;
	if (FAILED(BindUtilityAPI()))
		return E_FAIL;

	return S_OK;
}

HRESULT CLuaManager::BindComponentAPI()
{
	sol::table Component = m_Lua.create_named_table("Component");

	Component.set_function("Has",
		[this](const CHandle& hObject, const std::string& sComponentTag)
		{
			CGameObject* pObject = ResolveObject(hObject);
			if (!pObject || sComponentTag.empty())
				return false;

			return pObject->GetComponent<CComponent>(StringID{ sComponentTag }) != nullptr;
		});
	Component.set_function("CanAttach",
		[this](const std::string& sAttachType)
		{
			return m_ComponentAttachFactories.contains(sAttachType);
		});

	auto AttachComponent = [this](
		const CHandle& hObject,
		const std::string& sAttachType,
		const std::string& sComponentTag,
		const sol::table& Parameters)
	{
		CGameObject* pObject = ResolveObject(hObject);
		const auto FactoryIter = m_ComponentAttachFactories.find(sAttachType);
		if (!pObject || FactoryIter == m_ComponentAttachFactories.end() ||
			sComponentTag.empty())
		{
			return false;
		}

		if (pObject->GetComponent<CComponent>(StringID{ sComponentTag }))
			return false;

		if (!FactoryIter->second(*pObject, sComponentTag, Parameters))
			return false;

		return pObject->GetComponent<CComponent>(StringID{ sComponentTag }) != nullptr;
	};

	Component.set_function("Attach",
		sol::overload(
			[this, AttachComponent](
				const CHandle& hObject,
				const std::string& sAttachType,
				const std::string& sComponentTag)
			{
				return AttachComponent(
					hObject,
					sAttachType,
					sComponentTag,
					m_Lua.create_table());
			},
			[AttachComponent](
				const CHandle& hObject,
				const std::string& sAttachType,
				const std::string& sComponentTag,
				const sol::table& Parameters)
			{
				return AttachComponent(
					hObject,
					sAttachType,
					sComponentTag,
					Parameters);
			}));

	return S_OK;
}

HRESULT CLuaManager::BindInputAPI()
{
	sol::table Input = m_Lua.create_named_table("Input");

	Input.set_function("IsKeyHeld",
		[](int32_t iKey)
		{
			if (iKey < 0 || iKey > 255)
				return false;

			return CGameInstance::Get().KeyPressing(static_cast<_ubyte>(iKey));
		});
	Input.set_function("WasKeyPressed",
		[](int32_t iKey)
		{
			if (iKey < 0 || iKey > 255)
				return false;

			return CGameInstance::Get().KeyDown(static_cast<_ubyte>(iKey));
		});
	Input.set_function("WasKeyReleased",
		[](int32_t iKey)
		{
			if (iKey < 0 || iKey > 255)
				return false;

			return CGameInstance::Get().KeyUp(static_cast<_ubyte>(iKey));
		});
	Input.set_function("MouseDelta",
		[](int32_t iAxis)
		{
			if (iAxis < 0 || iAxis >= ETOI(MOUSEMOVESTATE::END))
				return int32_t{};

			return CGameInstance::Get().MouseMove(static_cast<MOUSEMOVESTATE>(iAxis));
		});
	Input.set_function("IsMouseHeld",
		[](int32_t iButton)
		{
			if (iButton < 0 || iButton >= ETOI(MOUSEKEYSTATE::END))
				return false;

			return CGameInstance::Get().MousePressing(static_cast<MOUSEKEYSTATE>(iButton));
		});
	Input.set_function("WasMousePressed",
		[](int32_t iButton)
		{
			if (iButton < 0 || iButton >= ETOI(MOUSEKEYSTATE::END))
				return false;

			return CGameInstance::Get().MouseDown(static_cast<MOUSEKEYSTATE>(iButton));
		});
	Input.set_function("WasMouseReleased",
		[](int32_t iButton)
		{
			if (iButton < 0 || iButton >= ETOI(MOUSEKEYSTATE::END))
				return false;

			return CGameInstance::Get().MouseUp(static_cast<MOUSEKEYSTATE>(iButton));
		});

	return S_OK;
}

HRESULT CLuaManager::BindObjectAPI()
{
	sol::table Object = m_Lua.create_named_table("Object");

	Object.set_function("IsValid",
		[this](const CHandle& hObject)
		{
			return ResolveObject(hObject) != nullptr;
		});
	Object.set_function("GetFirst",
		[this](const std::string& sLayer)
		{
			const auto* pHandles = CGameInstance::Get().GetGameObjectLayer(sLayer);
			if (!pHandles)
				return CHandle{};

			for (const CHandle& hObject : *pHandles)
			{
				if (ResolveObject(hObject))
					return hObject;
			}

			return CHandle{};
		});
	Object.set_function("GetLayer",
		[this](const std::string& sLayer)
		{
			sol::table Handles = m_Lua.create_table();
			const auto* pLayerHandles = CGameInstance::Get().GetGameObjectLayer(sLayer);
			if (!pLayerHandles)
				return Handles;

			int32_t iLuaIndex = 1;
			for (const CHandle& hObject : *pLayerHandles)
			{
				if (ResolveObject(hObject))
					Handles[iLuaIndex++] = hObject;
			}

			return Handles;
		});
	auto SpawnObject = [this](
		const std::string& sSpawnType,
		const std::string& sLayer,
		const sol::table& Parameters)
	{
		const auto FactoryIter = m_ObjectSpawnFactories.find(sSpawnType);
		if (FactoryIter == m_ObjectSpawnFactories.end() || sLayer.empty())
			return CHandle{};

		const auto hObject = FactoryIter->second(sLayer, Parameters);
		if (!hObject || !ResolveObject(*hObject))
			return CHandle{};

		return *hObject;
	};

	Object.set_function("CanSpawn",
		[this](const std::string& sSpawnType)
		{
			return m_ObjectSpawnFactories.contains(sSpawnType);
		});
	Object.set_function("Spawn",
		sol::overload(
			[this, SpawnObject](
				const std::string& sSpawnType,
				const std::string& sLayer)
			{
				return SpawnObject(sSpawnType, sLayer, m_Lua.create_table());
			},
			[SpawnObject](
				const std::string& sSpawnType,
				const std::string& sLayer,
				const sol::table& Parameters)
			{
				return SpawnObject(sSpawnType, sLayer, Parameters);
			}));
	Object.set_function("Destroy",
		[this](const CHandle& hObject, sol::optional<_bool> bCascade)
		{
			CGameObject* pObject = ResolveObject(hObject);
			if (!pObject)
				return false;

			if (bCascade.value_or(true))
				pObject->SetPendingDestroyCascade();
			else
				pObject->SetPendingDestroy();

			return true;
		});
	Object.set_function("GetTag",
		[this](const CHandle& hObject) -> std::optional<std::string>
		{
			const CGameObject* pObject = ResolveObject(hObject);
			if (!pObject)
				return std::nullopt;

			return std::string{ pObject->GetObjectTag() };
		});
	Object.set_function("SetTag",
		[this](const CHandle& hObject, const std::string& sTag)
		{
			CGameObject* pObject = ResolveObject(hObject);
			if (!pObject)
				return false;

			pObject->SetObjectTag(sTag);
			return true;
		});
	Object.set_function("GetType",
		[this](const CHandle& hObject) -> std::optional<std::string>
		{
			const CGameObject* pObject = ResolveObject(hObject);
			if (!pObject)
				return std::nullopt;

			return std::string{ pObject->GetTypeString() };
		});
	Object.set_function("IsPendingDestroy",
		[](const CHandle& hObject)
		{
			const CGameObject* pObject = CGameInstance::Get().GetGameObjectByHandle(hObject);
			return !pObject || pObject->GetPendingDestroy();
		});
	Object.set_function("SetUpdateEnabled",
		[this](const CHandle& hObject, _bool bEnabled, sol::optional<_bool> bCascade)
		{
			CGameObject* pObject = ResolveObject(hObject);
			if (!pObject)
				return false;

			if (bCascade.value_or(false))
				pObject->SetManagedUpdateEnabledCascade(bEnabled);
			else
				pObject->SetManagedUpdateEnabled(bEnabled);

			return true;
		});
	Object.set_function("IsUpdateEnabled",
		[this](const CHandle& hObject)
		{
			const CGameObject* pObject = ResolveObject(hObject);
			return pObject && pObject->IsManagedUpdateEnabled();
		});

	return S_OK;
}

HRESULT CLuaManager::BindTransformAPI()
{
	sol::table Transform = m_Lua.create_named_table("Transform");

	Transform.set_function("GetPosition",
		[this](const CHandle& hObject) -> std::optional<_float3>
		{
			const CGameObject* pObject = ResolveObject(hObject);
			if (!pObject)
				return std::nullopt;

			return pObject->GetTransform().GetPosition();
		});
	Transform.set_function("SetPosition",
		[this](const CHandle& hObject, const _float3& vPosition)
		{
			CGameObject* pObject = ResolveObject(hObject);
			if (!pObject)
				return false;

			pObject->GetTransform().SetPosition(vPosition);
			return true;
		});
	Transform.set_function("AddPosition",
		[this](const CHandle& hObject, const _float3& vOffset)
		{
			CGameObject* pObject = ResolveObject(hObject);
			if (!pObject)
				return false;

			pObject->GetTransform().AddPosition(vOffset);
			return true;
		});
	Transform.set_function("GetRotationEuler",
		[this](const CHandle& hObject) -> std::optional<_float3>
		{
			const CGameObject* pObject = ResolveObject(hObject);
			if (!pObject)
				return std::nullopt;

			return pObject->GetTransform().GetRotationEuler();
		});
	Transform.set_function("SetRotationEuler",
		[this](const CHandle& hObject, const _float3& vRotation)
		{
			CGameObject* pObject = ResolveObject(hObject);
			if (!pObject)
				return false;

			pObject->GetTransform().SetRotationEuler(vRotation);
			return true;
		});
	Transform.set_function("AddRotationEuler",
		[this](const CHandle& hObject, const _float3& vRotation)
		{
			CGameObject* pObject = ResolveObject(hObject);
			if (!pObject)
				return false;

			pObject->GetTransform().AddRotationEuler(vRotation);
			return true;
		});
	Transform.set_function("GetQuaternion",
		[this](const CHandle& hObject) -> std::optional<_float4>
		{
			const CGameObject* pObject = ResolveObject(hObject);
			if (!pObject)
				return std::nullopt;

			return pObject->GetTransform().GetQuaternion();
		});
	Transform.set_function("SetQuaternion",
		[this](const CHandle& hObject, const _float4& vQuaternion)
		{
			CGameObject* pObject = ResolveObject(hObject);
			if (!pObject)
				return false;

			const _vector Quaternion = XMLoadFloat4(&vQuaternion);
			if (XMVectorGetX(XMVector4LengthSq(Quaternion)) <= 1e-12f)
				return false;

			pObject->GetTransform().SetQuaternion(vQuaternion);
			return true;
		});
	Transform.set_function("GetScale",
		[this](const CHandle& hObject) -> std::optional<_float3>
		{
			const CGameObject* pObject = ResolveObject(hObject);
			if (!pObject)
				return std::nullopt;

			return pObject->GetTransform().GetScale();
		});
	Transform.set_function("SetScale",
		[this](const CHandle& hObject, const _float3& vScale)
		{
			CGameObject* pObject = ResolveObject(hObject);
			if (!pObject)
				return false;

			pObject->GetTransform().SetScale(vScale);
			return true;
		});

	auto BindDirectionGetter = [this, &Transform](const char* pName, STATE eState)
	{
		Transform.set_function(pName,
			[this, eState](const CHandle& hObject) -> std::optional<_float3>
			{
				const CGameObject* pObject = ResolveObject(hObject);
				if (!pObject)
					return std::nullopt;

				_float3 Direction{};
				XMStoreFloat3(&Direction, pObject->GetTransform().GetState(eState));
				return Direction;
			});
	};

	BindDirectionGetter("GetRight", STATE::RIGHT);
	BindDirectionGetter("GetUp", STATE::UP);
	BindDirectionGetter("GetForward", STATE::LOOK);

	Transform.set_function("GetWorldMatrix",
		[this](const CHandle& hObject) -> std::optional<_float4x4>
		{
			const CGameObject* pObject = ResolveObject(hObject);
			if (!pObject)
				return std::nullopt;

			return *pObject->GetTransform().GetWorldMatrix();
		});
	Transform.set_function("LookAt",
		[this](const CHandle& hObject, const _float3& vTarget)
		{
			CGameObject* pObject = ResolveObject(hObject);
			if (!pObject)
				return false;

			pObject->GetTransform().LookAt(XMLoadFloat3(&vTarget));
			return true;
		});

	return S_OK;
}

HRESULT CLuaManager::BindCameraAPI()
{
	sol::table Camera = m_Lua.create_named_table("Camera");

	Camera.set_function("GetActive",
		sol::overload(
			[]()
			{
				const CCameraObject* pCamera = CGameInstance::Get().GetActiveCamera();
				return pCamera ? pCamera->GetHandle() : CHandle{};
			},
			[](const std::string& sCameraID)
			{
				const CCameraObject* pCamera = CGameInstance::Get().GetActiveCamera(StringID{ sCameraID });
				return pCamera ? pCamera->GetHandle() : CHandle{};
			}));
	Camera.set_function("Get",
		[](const std::string& sCameraID)
		{
			const CCameraObject* pCamera = CGameInstance::Get().GetCamera(StringID{ sCameraID });
			return pCamera ? pCamera->GetHandle() : CHandle{};
		});
	Camera.set_function("SetActive",
		[](const std::string& sCameraID)
		{
			return SUCCEEDED(CGameInstance::Get().SetActiveCamera(StringID{ sCameraID }));
		});
	Camera.set_function("GetRay",
		[this](const CHandle& hCamera)
		{
			const CGameObject* pObject = ResolveObject(hCamera);
			if (!pObject || !pObject->IsA(CCameraObject::StaticType))
				return std::make_tuple(false, _float3{}, _float3{});

			const auto* pCamera = static_cast<const CCameraObject*>(pObject);
			const auto [vOrigin, vDirection] = pCamera->GetRay();
			return std::make_tuple(true, vOrigin, vDirection);
		});

	return S_OK;
}

HRESULT CLuaManager::BindUtilityAPI()
{
	sol::table Random = m_Lua.create_named_table("Random");
	Random.set_function("Float", [](_float fMin, _float fMax) { return Randf(fMin, fMax); });
	Random.set_function("Int", [](int32_t iMin, int32_t iMax) { return RandInt(iMin, iMax); });

	sol::table Level = m_Lua.create_named_table("Level");
	Level.set_function("Change",
		[](const std::string& sLevelID)
		{
			return SUCCEEDED(CGameInstance::Get().ChangeLevel(sLevelID));
		});

	sol::table DebugDraw = m_Lua.create_named_table("DebugDraw");
	DebugDraw.set_function("SetColor",
		[](const _float4& vColor)
		{
			if (auto* pRenderer = CGameInstance::Get().GetDbgLineRender())
				pRenderer->SetColor(vColor);
		});
	DebugDraw.set_function("GetColor",
		[]()
		{
			if (const auto* pRenderer = CGameInstance::Get().GetDbgLineRender())
				return pRenderer->GetColor();

			return _float4{};
		});
	DebugDraw.set_function("SetDepthTest",
		[](_bool bEnable)
		{
			if (auto* pRenderer = CGameInstance::Get().GetDbgLineRender())
				pRenderer->SetDepthTest(bEnable);
		});
	DebugDraw.set_function("Line",
		sol::overload(
			[](const _float3& vStart, const _float3& vEnd)
			{
				if (auto* pRenderer = CGameInstance::Get().GetDbgLineRender())
					pRenderer->AddLine(vStart, vEnd);
			},
			[](const _float3& vStart, const _float3& vEnd, const _float4& vColor)
			{
				if (auto* pRenderer = CGameInstance::Get().GetDbgLineRender())
					pRenderer->AddLine(vStart, vEnd, vColor);
			}));
	DebugDraw.set_function("Box",
		[](const _float3& vHalfExtent, const _float4x4& World)
		{
			if (auto* pRenderer = CGameInstance::Get().GetDbgLineRender())
				pRenderer->AddBox(vHalfExtent, XMLoadFloat4x4(&World));
		});
	DebugDraw.set_function("Sphere",
		[](_float fRadius, const _float4x4& World)
		{
			if (auto* pRenderer = CGameInstance::Get().GetDbgLineRender())
				pRenderer->AddSphere(fRadius, XMLoadFloat4x4(&World));
		});
	DebugDraw.set_function("Capsule",
		[](_float fRadius, _float fHalfHeight, const _float4x4& World)
		{
			if (auto* pRenderer = CGameInstance::Get().GetDbgLineRender())
				pRenderer->AddCapsule(fRadius, fHalfHeight, XMLoadFloat4x4(&World));
		});
	DebugDraw.set_function("Cylinder",
		[](_float fRadius, _float fHalfHeight, const _float4x4& World)
		{
			if (auto* pRenderer = CGameInstance::Get().GetDbgLineRender())
				pRenderer->AddCylinder(fRadius, fHalfHeight, XMLoadFloat4x4(&World));
		});
	DebugDraw.set_function("Cone",
		[](_float fRadius, _float fHeight, const _float4x4& World)
		{
			if (auto* pRenderer = CGameInstance::Get().GetDbgLineRender())
				pRenderer->AddCone(fRadius, fHeight, XMLoadFloat4x4(&World));
		});
	DebugDraw.set_function("Ray",
		[](const _float3& vOrigin, const _float3& vDirection, _float fLength)
		{
			if (auto* pRenderer = CGameInstance::Get().GetDbgLineRender())
				pRenderer->AddRay(vOrigin, vDirection, fLength);
		});
	DebugDraw.set_function("Arrow",
		[](const _float3& vOrigin, const _float3& vDirection, _float fLength)
		{
			if (auto* pRenderer = CGameInstance::Get().GetDbgLineRender())
				pRenderer->AddArrow(vOrigin, vDirection, fLength);
		});
	DebugDraw.set_function("Grid",
		[](uint32_t iHalfCount, _float fCellSize, const _float4x4& World)
		{
			if (auto* pRenderer = CGameInstance::Get().GetDbgLineRender())
				pRenderer->AddGrid(iHalfCount, fCellSize, XMLoadFloat4x4(&World));
		});
	DebugDraw.set_function("Quad",
		[](_float fWidth, _float fHeight, const _float4x4& World)
		{
			if (auto* pRenderer = CGameInstance::Get().GetDbgLineRender())
				pRenderer->AddQuad(fWidth, fHeight, XMLoadFloat4x4(&World));
		});
	DebugDraw.set_function("Triangle",
		[](const _float3& vA, const _float3& vB, const _float3& vC)
		{
			if (auto* pRenderer = CGameInstance::Get().GetDbgLineRender())
				pRenderer->AddTriangle(vA, vB, vC);
		});
	DebugDraw.set_function("Axis",
		[](_float fLength, const _float4x4& World)
		{
			if (auto* pRenderer = CGameInstance::Get().GetDbgLineRender())
				pRenderer->AddAxis(fLength, XMLoadFloat4x4(&World));
		});
	DebugDraw.set_function("Circle",
		[](_float fRadius, const _float4x4& World, sol::optional<uint32_t> iSlices)
		{
			if (auto* pRenderer = CGameInstance::Get().GetDbgLineRender())
				pRenderer->AddCircle(fRadius, XMLoadFloat4x4(&World), iSlices.value_or(32));
		});
	DebugDraw.set_function("Cross",
		[](const _float3& vPosition, sol::optional<_float> fSize)
		{
			if (auto* pRenderer = CGameInstance::Get().GetDbgLineRender())
				pRenderer->AddCross(vPosition, fSize.value_or(0.1f));
		});

	return S_OK;
}
