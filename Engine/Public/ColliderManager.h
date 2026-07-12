#pragma once
#include "Engine_Defines.h"
#include "IRenderable.h"

NS_BEGIN(Engine)
class CCollider;
class CResDynamicVIBuffer;
class CResVertexShader;
class CResPixelShader;
class CResCBuffer;
class CColliderManager final: public CEngineBase, public IRenderable
{
private:
	typedef struct tagConstantBufferPerFrame
	{
		_matrix viewProjMatrix;
	} CB_COLL_PER_FRAME;
private:
	explicit CColliderManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CColliderManager() override;

public:
	void UpdateGUI();
	void Update();

	void FrameStart();
	void FrameEnd();

public:
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override ;
	bool HasRenderPass(RENDERPASS ePass) const override { return ePass == RENDERPASS::DEFAULT; };

public:
	void AddColliderGroup(const StringID& groupTag, const CCollider*);
	const std::vector<const CCollider*>* GetColliderGroup(const StringID& groupTag) const;
	const std::unordered_map<StringID, std::vector<const CCollider*>>* GetColliders() const { return &m_Colliders; }
	const CCollider* GetColliderGroupFirst(const StringID& groupTag) const;
	_bool IntersectColl(const CCollider* pColl1, const CCollider* pColl2);

private:
	void ClearColliderGroup();

private:
	HRESULT Initialize();

private:
	std::unordered_map<StringID, std::vector<const CCollider*>> m_Colliders{};
	std::unordered_map<const CCollider*, _float4> m_DbgColor{};

	_bool m_bRender{ true };
	std::unordered_map<StringID, _bool> m_DbgRenders{};
	//_bool m_bDbgBufferInitialize{ false };

private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};

	const uint32_t m_iVertexCnt{ 100 };

	SPtr<CResDynamicVIBuffer> m_pDbgBuffer{};
	SPtr<CResVertexShader> m_pDbgVShader{};
	SPtr<CResPixelShader> m_pDbgPShader{};
	SPtr<CResCBuffer> m_pPerFrame{};
	std::vector<VTX_COL> m_Vertices{};

public:
	static UPtr<CColliderManager> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
};

NS_END
