#pragma once

#include "Handle.h"

namespace Engine
{

	typedef struct tagEngineDesc
	{
		HWND hWnd;
		HINSTANCE hInstance;
		WINMODE eWinMode;
		uint32_t iWinSizeX, iWinSizeY;
		uint32_t		iNumLevels;
		NVCLOTH_BACKEND eNvClothBackend{ NVCLOTH_BACKEND::DX11 };
	} ENGINE_DESC;


	typedef struct tagRenderContext
	{
		RENDERPASS pass;
		_vector eye{};
		_matrix matView{};
		_matrix matProj{};
		_matrix matViewProj{};

		int32_t	PointShadowFaceIndex = -1;
	} RENDER_CTX;

	typedef struct tagWorkerTask
	{
		_string sTaskName;
		_Func func;
	} WORKER_TASK;

	typedef struct tagMaterial
	{
		_float4 ambient{};
		_float4 diffuse{};
		_float4 specular{};
		_float4 reflect{};
	} MATERIAL;

	typedef struct tagDirectionalLight
	{
		_float4 ambient{};
		_float4 diffuse{};
		_float4 specular{};
		_float3 direction{};
		_float _pad{};
	} DIRECTIONAL_LIGHT;

	typedef struct tagPointLight
	{
		_float4 ambient{};
		_float4 diffuse{};
		_float4 specular{};
		_float3 pos{};
		_float range{};
		_float3 att{};//감쇠
		_float _pad{};
	} POINT_LIGHT;

	typedef struct tagSpotLight
	{
		_float4 ambient{};
		_float4 diffuse{};
		_float4 specular{};
		_float3 pos{};
		_float range{};
		_float3 direction{};
		_float spot{};
		_float3 att{};//감쇠
		_float _pad{};
	} SPOT_LIGHT;

	typedef struct tagDynamicLight {
		XMFLOAT4X4	g_LightViewProj[POINT_SHADOW_MAPCOUNT];

		_float3		LightDirection;
		_float		LightIntensity;
		_float3		LightColor;
		_float		LightRange;

		_float3		Position;
		uint32_t	LightType;

		_float		InnerAttanuation;
		_float		OuterAttanuation;

		int32_t		ShadowSlot;
		_float		LightPadding;
	} DYNAMIC_LIGHT;

	typedef struct tagEffectLight {
		_float3		Position;
		_float		LightIntensity;

		_float3		LightColor;

		_float		InnerAttanuation;
		_float		OuterAttanuation;

		_float3		LightPadding;
	} EFFECT_LIGHT;

	typedef struct tagTexture3D
	{
		ComPtr<ID3D11Texture3D>           pTexture;
		ComPtr<ID3D11ShaderResourceView>  pSRV;
		ComPtr<ID3D11UnorderedAccessView> pUAV;

		void Reset()
		{
			pTexture.Reset();
			pSRV.Reset();
			pUAV.Reset();
		}
	} TEXTURE3D;

	typedef struct tagPostProcess
	{
		_float BloomIntensity;		 // 블룸 강도

		_float DistortionIntensity;  // 왜곡 강도
		_float ChromaticIntensity;   // 색수차 강도
		_float VignetteIntensity;    // 비네팅 강도
		_float VignetteSmoothness;   // 비네팅

		_float3 PostProcessPadding;
	} POSTPROCESS;
	typedef struct tagUiDesc
	{
		_string name;
		_float2 Pos;
		_float2 Scale;
	} UI_DESC;
	typedef struct tagKeyFrame
	{
		XMFLOAT3	vScale;
		XMFLOAT4	vRotation;
		XMFLOAT3	vTranslation;
		float		fTrackPosition;
	}KEYFRAME;

	///////BeHavior//////
	typedef struct tagactionvalue
	{
		tagactionvalue() = default;
		tagactionvalue(int32_t iAnim) { iAnim = iAnimIndex; }
		int32_t  iAnimIndex{ -1 };
		_float   fSpeed{}, fTime{ 1.f }, fTick{}; 

	}ACTION_VALUE;
	typedef struct tagdestnode
	{
		tagdestnode() = default;
		tagdestnode(_string Name, BEHAVIOR eBType, int32_t iNode) : DestName(Name)
		{
			eType = eBType; iDestNode = iNode;
		}
		_string  DestName{};
		int32_t iDestNode{ -1 };
		BEHAVIOR eType{ BEHAVIOR::END };
		void Reset() { DestName = "";  eType = BEHAVIOR::END; iDestNode = -1; }

	}DEST_NODE;
	typedef struct tagimguinode
	{
		uint32_t	iID{};
		_string		Name{};
		XMFLOAT2	vPos{}, vSize{};
		float		fValue{};
		XMFLOAT4	vColor{};
		_bool		bAbort{ false };
		BEHAVIOR    eMyType{};
		tagimguinode() = default;
		tagimguinode(BEHAVIOR eType, int32_t id, const _char* name, XMFLOAT2 pos, float value, XMFLOAT4 color):Name(name){ eMyType = eType; iID = id; vPos = pos; fValue = value; vColor = color; }
		XMFLOAT2 GetStartSlotPos() { return XMFLOAT2(vPos.x + vSize.x * 0.5f, vPos.y); }
		XMFLOAT2 GetEndSlotPos(int slot_no, int32_t iMaxCnt) const {
			return XMFLOAT2(vPos.x + vSize.x * ((float)slot_no + 1) / ((float)iMaxCnt), vPos.y + vSize.y);
		}
		DEST_NODE Get_DestInfo() {
			DEST_NODE Dst{};
			Dst.DestName = Name;
			Dst.eType = eMyType;
			Dst.iDestNode = iID;
			return Dst;
		}

	}GUINODE;

	typedef struct tagimguinodelink
	{
		int32_t					iStartIdx{ -1 };
		DEST_NODE				ParentNode;
		std::vector<DEST_NODE>  SlotEnd{};

		tagimguinodelink() = default;
		tagimguinodelink(int32_t iEnd)
		{
			SlotEnd.resize(iEnd);
		}
	}GUINODE_LINK;
	typedef struct tagimguiCurrentNode
	{
		tagimguiCurrentNode() = default;
		tagimguiCurrentNode(GUINODE* pNode, GUINODE_LINK* pLink, int32_t iSlot)
		{
			pCurrentNode = pNode; pCurrentLink = pLink;  iSelectedSlot = iSlot;
		}
		GUINODE* pCurrentNode{ nullptr };
		GUINODE_LINK* pCurrentLink{ nullptr };
		int32_t  iSelectedSlot{ -1 }, iD{ -1 };
		_float2  vSlotPos;
		_bool	 bSelected = false;

		NODETYPE eType = NODETYPE::END;
	}GUICURRENT_NODE;

	typedef struct tagParticleSpawnData
	{
		_float3  position;
		_float   pad0;
		_float3  velocity;
		_float   life;
		_float3   fSize;
		_float3   fEndSize;
		_float4  rotation;
		_float4  color;
		_float4  originalEmissive;
		_float4  emissive;
		_float4  endEmissive;
		_float   spawnDelay;
		uint32_t ownerID = 0;
		uint32_t iBehaviorType = 0;
		_bool    loop;
		_float3  originalPosition{};
		_float3 originalVelocity{}; // 원래 스폰 속도+ 방향
		_float fStopSizeTime = 0.f;
		_float3 pad1;
		_float3 rotationAxis {};
		_float fRotationSpeed{};
	} PARTICLE_SPAWN_DATA;
	static_assert(sizeof(PARTICLE_SPAWN_DATA) % 16 == 0);

	constexpr uint32_t PREVIEW_OWNER_ID = 0xFFFFFFFF; //미리보기 전용 





	typedef struct tagParticleEmitRequest
	{
		uint32_t count;
		_bool    bLoop;
		_float   fSpawnInterval;
	} PARTICLE_EMIT_REQUEST;

	typedef struct tagBeamVertex
	{
		_float3 vPosition;
		_float2 vUV;
		_float4 vColor;
		_float4 vEmissive;
		_float4 vEndEmissive;
	}BEAM_VERTEX;

	typedef struct ChunkHeader
	{
		uint32_t type;
		uint32_t size;
	}CHUCKHEADER;

	typedef struct MODEL_FILE_HEADER
	{
		bool bHasBone;
		bool bHasAnimation;

		uint32_t MeshCount;
		uint32_t MaterialCount;
		uint32_t AnimationCount;
		uint32_t BoneCount;

	}MODEL_FILE_HEADER;



	// 여러 청크를 관리할 때 key로 사용할 ChunkCoord
	typedef struct tagMapChunkCoord
	{
		int64_t x = 0;
		int64_t y = 0;
		int64_t z = 0;

		bool operator==(const tagMapChunkCoord& rhs) const
		{
			return x == rhs.x && y == rhs.y && z == rhs.z;
		}
	}MAPCHUNK_COORD;

	//----------------------------MapMeshObject 인스턴싱------------------------
	struct WIND_DESC
	{
		EWindType type = EWindType::None;
		_float strength = 0.f;
		_float speed = 1.f;
		_float frequency = 1.f;
		_float bendExponent = 2.f;
		_float heightStart = 0.f;
		_float heightEnd = 1.f;
	};

	typedef struct tagMapMeshInstanceData
	{
		_float4x4 world;
		_float4 windParams; // strength, speed, frequency, bendExponent
		_float2 windHeightParams; // local influence start Y, inverse influence height
		uint32_t windType;
		_float padding;
	} MAPMESH_INSTANCE_DATA;
	static_assert(sizeof(MAPMESH_INSTANCE_DATA) % 16 == 0);

	typedef struct tagMapMeshOcclusionData
	{
		_float3		worldCenter;
		_float		padding0;
		_float3		worldExtents;
		uint32_t	instanceIndex;
	} MAPMESH_OCCLUSION_DATA;

	typedef struct tagMapMeshBatchKey
	{
		std::string modelGroup;
		std::string modelTag;

		bool operator==(const tagMapMeshBatchKey& rhs) const
		{
			return modelGroup == rhs.modelGroup && modelTag == rhs.modelTag;
		}
	} MAPMESH_BATCH_KEY;

	struct tagMapMeshBatchKeyHash
	{
		size_t operator()(const MAPMESH_BATCH_KEY& key) const noexcept
		{
			const size_t h1 = std::hash<std::string>{}(key.modelGroup);
			const size_t h2 = std::hash<std::string>{}(key.modelTag);
			return h1 ^ (h2 << 1);
		}
	};

	struct MAPMESH_INSTANCE_BATCH
	{
		std::vector<MAPMESH_INSTANCE_DATA> instances;
		std::vector<MAPMESH_OCCLUSION_DATA> occlusionData;
	};

	struct MAPMESH_CULL_META
	{
		uint32_t outputOffset = 0;
		uint32_t batchIndex = 0;
	};

	struct INSTANCING_STATS
	{
		_bool bEnabled = false;
		uint32_t iObjects = 0;
		uint32_t iInstances = 0;
		uint32_t iBatches = 0;
		uint32_t iDrawCalls = 0;
		uint32_t iVisibleInstances = 0;
		uint32_t iCulledInstances = 0;
	};
	//----------------------------MapMeshObject ?몄뒪??�떛------------------------


	//----------------------------AnimationObject------------------------------------


	typedef struct GPU_BONE_DESC
	{
		_float4x4 BindLocalMatrix;
		// Bind pose도 애니메이션 키와 같은 SRT 형태로 보관한다.
		// GPU 블렌딩 시 행렬 원소를 직접 보간하지 않기 위해 사용한다.
		_float3 BindScale{ 1.f, 1.f, 1.f };
		_float4 BindRotation{ 0.f, 0.f, 0.f, 1.f };
		_float3 BindTranslation{ 0.f, 0.f, 0.f };
		float   fBindPadding = 0.f;

		int32_t  iParentBoneIndex = -1;
		uint32_t iDepth = 0;
		uint32_t iPadding0 = 0;
		uint32_t iPadding1 = 0;
	}GPU_BONE_DESC;

	typedef struct GPU_ANIM_DESC
	{
		_float4x4 PreTransformMatrix;

		uint32_t iChannelOffset = 0;
		uint32_t iChannelCount = 0;

		uint32_t iBoneChannelMapOffset = 0;
		uint32_t iBoneCount = 0;



		float fDuration = 0.f;
		float fPadding0 = 0.f;
		float fPadding1 = 0.f;
		float fPadding2 = 0.f;
	}GPU_ANIM_DESC;

	typedef struct GPU_CHANNEL_DESC
	{
		uint32_t iBoneIndex = 0;
		uint32_t iKeyFrameOffset = 0;
		uint32_t iKeyFrameCount = 0;
		uint32_t iPadding = 0;
	}GPU_CHANNEL_DESC;

	typedef struct GPU_KEYFRAME_DESC
	{
		_float3 vScale{ 1.f, 1.f, 1.f };
		float fTrackPosition = 0.f;

		_float4 vRotation{ 0.f, 0.f, 0.f, 1.f };

		_float3 vTranslation{ 0.f, 0.f, 0.f };
		float fPadding = 0.f;
	}GPU_KEYFRAME_DESC;

	typedef struct GPU_SKIN_BONE_DESC
	{
		_float4x4 OffsetMatrix;

		uint32_t iSkeletonBoneIndex = 0;
		uint32_t iPadding0 = 0;
		uint32_t iPadding1 = 0;
		uint32_t iPadding2 = 0;
	}GPU_SKIN_BONE_DESC;

	typedef struct GPU_MESH_SKIN_RANGE
	{
		uint32_t iSkinBoneOffset = 0;
		uint32_t iSkinBoneCount = 0;
	}GPU_MESH_SKIN_RANGE;

	typedef struct GPU_SKIN_MESH_CONSTANTS
	{
		uint32_t iSkinBoneOffset = 0;
		uint32_t iVertexCount = 0;
		uint32_t iSkinBoneCount = 0;
		/*----------- 광윤 추가 -----------*/
		uint32_t iBonePaletteStride = 0;
		/*---------------------------------*/
	}GPU_SKIN_MESH_CONSTANTS;


	//struct GPU_ANIM_INSTANCE_DATA { 
	// float4x4 WorldMatrix; 
	// uint iAnimIndex; 
	// uint iFlags; 
	// float fTrackPosition; 
	// uint RootBoneIndex; 
	// uint iPrevAnimIndex; 
	// float fPrevTrackPosition; 
	// float fBlendWeight;
	// uint bBlending; };

	typedef struct GPU_ANIM_INSTANCE_DATA
	{
		_float4x4 WorldMatrix{};

		uint32_t iAnimIndex = 0;
		uint32_t iFlags = 0;

		_float fTrackPosition = 0.f;
		
		uint32_t iRootBoneIndex = 0;

		uint32_t iPrevAnimIndex = 0;
		_float fPrevTrackPosition = 0.f;
		_float fBlendWeight = 1.f;
		uint32_t bBlending = 0;
	}GPU_ANIM_INSTANCE_DATA;

	constexpr uint32_t INVALID_ANIM_INDEX = UINT32_MAX;

	typedef struct GPU_PART_INSTANCE_DATA
	{
		_float4x4 WorldMatrix{};
		uint32_t iParentInstanceIndex = 0;
		uint32_t iParentBoneIndex = 0;
		_bool    bAttach;
		_float   pad;
	} GPU_PART_INSTANCE_DATA;

	typedef struct MODEL_INSTANCE_KEY
	{
		StringID modelGroup{};
		StringID modelTag{};
		_bool bStaticModel = false;
		uint32_t iEvaluationMode = 0;

		_bool operator==(const MODEL_INSTANCE_KEY& rhs) const
		{
			return
				modelGroup == rhs.modelGroup &&
				modelTag == rhs.modelTag &&
				bStaticModel == rhs.bStaticModel &&
				iEvaluationMode == rhs.iEvaluationMode;
		}
	}MODEL_INSTANCE_KEY;
	typedef struct MODEL_INSTANCE_KEY_HASH
	{
		size_t operator()(const MODEL_INSTANCE_KEY& Key) const
		{
			size_t Seed = 0;

			auto HashCombine =
				[&Seed](size_t Value)
				{
					Seed ^= Value
						+ 0x9e3779b9
						+ (Seed << 6)
						+ (Seed >> 2);
				};

			HashCombine(
				std::hash<StringID>{}(
					Key.modelGroup));

			HashCombine(
				std::hash<StringID>{}(
					Key.modelTag));

			HashCombine(std::hash<_bool>{}(Key.bStaticModel));
			HashCombine(std::hash<uint32_t>{}(Key.iEvaluationMode));

			return Seed;
		}
	}MODEL_INSTANCE_KEY_HASH;

	typedef struct MODEL_INSTANCE_BATCH
	{
		MODEL_INSTANCE_KEY Key{};
		
		CHandle		ObjectHandle;
		std::vector<GPU_ANIM_INSTANCE_DATA>Instances;
		std::vector<GPU_PART_INSTANCE_DATA> PartInstances;
		std::vector<std::vector<_float4x4>> CombinedBoneMatrices;
		
		_bool bModelStatic = false;
		_bool bGPUSkinned = false;

		_bool bActiveThisFrame = false;

		/*----------- 광윤 추가 -----------*/
		std::vector<std::optional<BoundingBox>>	ShadowBounds;
		/*---------------------------------*/

	}MODEL_INSTANCE_BATCH;

	struct CSM_DATA
	{
		std::vector<ComPtr<ID3D11DepthStencilView>>		m_pShadowDSVList{};
		ComPtr<ID3D11ShaderResourceView>				m_pShadowSRV{};
		ComPtr<ID3D11Texture2D>							m_pTextureArray{};

		std::optional<CHandle>							m_pLightHandle{};
	};
	

	//----------------------------AnimationObject------------------------------------
}
