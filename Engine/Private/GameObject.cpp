#include "pch.h"

#include "GameObject.h"
#include "Component.h"
#include "GameInstance.h"

#include "ComTransform.h"

NS_USING(Engine)

CGameObject::CGameObject()
    : CPrototype{ }
{
}

CGameObject::CGameObject(const CGameObject& Prototype)
    : CPrototype{ Prototype }
{
}

CGameObject::~CGameObject()
{
}

HRESULT CGameObject::Initialize(void* pArg)
{
    auto pDesc = static_cast<GAMEOBJECT_DESC*>(pArg);
    m_sObjectTag = pDesc->sObjectTag;
    //m_ObjectHandle = CGameInstance::Get().GetFreeHandle().value();

    m_ObjectHandle = pDesc->__handle;

    {
        if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_Transform", "Com_Transform", nullptr, &m_pComTransform)))
        {
            return E_FAIL;
        }


		CComCollider::DESC Desc{};
		Desc.eCollType = CollType::Box;
		Desc.vCenter	= { 0.f, 0.f, 0.f };
		Desc.vExtents	= { 1.f, 1.f, 1.f };
		if (FAILED(AddComponentFromProto("COLLIDER", "Prototype_Component_Collider", "Com_Collider", &Desc, &m_pComCollider)))
		{
			return E_FAIL;
		}
        //CComponent::DESC componentDesc{};
        //componentDesc.pGameObject = this;
        //auto pProto = CGameInstance::Get().ClonePrototype("PERMANENT", "Prototype_Component_Transform", &componentDesc);
        //if (pProto == nullptr)
        //{
        //    return E_FAIL;
        //}
        //m_pComTransform = AddComponent("Com_Transform", 
        // <CComTransform>(std::move(pProto)));
    }

    return S_OK;
}

void CGameObject::FixedUpdate(_float fTimeDelta)
{

}

void CGameObject::PriorityUpdate(_float fTimeDelta)
{
}

void CGameObject::Update(_float fTimeDelta)
{
}

void CGameObject::LateUpdate(_float fTimeDelta)
{
}

HRESULT CGameObject::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
    return S_OK;
}


UPtr<CPrototype> CGameObject::CloneComponentProtoType(const StringID& svGroupTag, const StringID& svPrototypetag, void* pArg) const
{
    CComponent::DESC componentDesc{};
    if (!pArg)
    {
        componentDesc.pGameObject = const_cast<CGameObject*>(this);
        pArg = &componentDesc;
    }
    else
    {
        auto pDesc = static_cast<CComponent::DESC*>(pArg);
        auto myPtr = const_cast<CGameObject*>(this);;
        static_cast<CComponent::DESC*>(pArg)->pGameObject = const_cast<CGameObject*>(this);
        int x = 0;
    }

    auto pProto = CGameInstance::Get().ClonePrototype(svGroupTag, svPrototypetag,  pArg);
    if (pProto == nullptr)
    {
        return nullptr;
    }
    return pProto;
}


void CGameObject::UpdateGUI()
{
    if (ImGui::Button("DestroyCascade"))
    {
        SetPendingDestroyCascade();
    }
    ImGui::SameLine();
    if (ImGui::Button("Destroy"))
    {
        SetPendingDestroy();
    }

	_bool bManagedUpdateEnabled = IsManagedUpdateEnabled();
	if (ImGui::Checkbox("Managed Update", &bManagedUpdateEnabled))
	{
		SetManagedUpdateEnabledCascade(bManagedUpdateEnabled);
	}

    //ImGui::Text("dest: %s", m_bPendingDestroy ? "true" : "false");
    if (ImGui::TreeNode("Components"))
    {
        for (const auto& [comID, pCom] : m_Components)
        {
            if (ImGui::TreeNode(comID.GetDbgStr()))
            {
                pCom->UpdateGUI();

                ImGui::TreePop();
            }
        }

        ImGui::TreePop();
    }
}


HRESULT CGameObject::DelComponent(const StringID& tagComponent)
{
    auto iter = m_ComponentsLookup.find(tagComponent);
    if (iter == m_ComponentsLookup.end())
    {
        return E_FAIL;
    }

    const size_t idx = iter->second;
    const size_t last = m_Components.size() - 1;

    if (idx != last)
    {
        std::swap(m_Components[idx], m_Components[last]);

        // 이동된 요소의 키 인덱스 업데이트
        m_ComponentsLookup[m_Components[idx].first] = idx;
    }

    m_ComponentsLookup.erase(iter);
    m_Components.pop_back();
    return S_OK;
}

void CGameObject::Free()
{
    CPrototype::Free();
}

void CGameObject::SetPendingDestroy(_bool b)
{
    m_bPendingDestroy = b;
}

void CGameObject::SetPendingDestroyCascade(_bool b)
{
    MyTreeDFS(this, [&](auto pObj) {pObj->SetPendingDestroy(b); });
}

void CGameObject::SetManagedUpdateEnabled(_bool bEnabled)
{
	if (m_bManagedUpdateEnabled == bEnabled)
		return;

	m_bManagedUpdateEnabled = bEnabled;

	if (m_bManagedUpdateEnabled)
		OnManagedUpdateEnabled();
	else
		OnManagedUpdateDisabled();
}

void CGameObject::SetManagedUpdateEnabledCascade(_bool bEnabled)
{
	MyTreeDFS(this, [bEnabled](CGameObject* pObject)
	{
		pObject->SetManagedUpdateEnabled(bEnabled);
	});
}

_bool CGameObject::AcquireFromPool(void* pArg)
{
	// [LSY] 이전 사용에서 남은 PhysX 동기화 결과를 재사용하지 않는다.
	m_PhysXSyncData = {};
	m_bPhysXSynced = false;

	if (m_bPendingDestroy || !OnAcquireFromPool(pArg))
		return false;

	SetManagedUpdateEnabledCascade(true);
	return true;
}

void CGameObject::ReleaseToPool()
{
	SetManagedUpdateEnabledCascade(false);
	// [LSY] 비활성 객체에 이전 PhysX 결과가 남지 않도록 정리한다.
	m_PhysXSyncData = {};
	m_bPhysXSynced = false;
	OnReleaseToPool();
}

void CGameObject::SyncActivePhysXData(const PX_SYNC_DATA& syncData)
{
    m_PhysXSyncData = syncData;
    m_bPhysXSynced = true;
}

void CGameObject::UpdatePhysicData()
{
	if (m_bPhysXSynced)
	{
		GetTransform().SetPosition(XMVectorSet(m_PhysXSyncData.vPos.x, m_PhysXSyncData.vPos.y, m_PhysXSyncData.vPos.z, 1.f));
		GetTransform().SetQuaternion(XMVectorSet(m_PhysXSyncData.vQuat.x, m_PhysXSyncData.vQuat.y, m_PhysXSyncData.vQuat.z, m_PhysXSyncData.vQuat.w));
	}
}
