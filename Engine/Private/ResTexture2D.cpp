#include "pch.h"
#include "GameInstance.h"
#include "ResTexture2D.h"
#include <directxtk/WICTextureLoader.h>
#include <wincodec.h>
NS_USING(Engine)

CResTexture2D::CResTexture2D(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
    : CResTexture{sPath, pDevice, pContext }
{
}

CResTexture2D::~CResTexture2D()
{
}

HRESULT CResTexture2D::Load(const std::any& arg)
{
    if (m_eState == STATE::LOADED)
    {
        return S_OK;
    }

    m_eState = STATE::LOADING;
    //DirectX::CreateWICTextureFromFileEx(
    //    m_pDevice.Get(),
    //    nullptr,                // 스레드에서는 Context를 nullptr로! (밉맵 생성 미룸)
    //    m_sPath,
    //    0,
    //    D3D11_USAGE_DEFAULT,
    //    D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, // 나중에 밉맵 만들려면 RT 플래그 필수
    //    0,
    //    D3D11_RESOURCE_MISC_GENERATE_MIPS, // 밉맵 생성 가능 옵션
    //    WIC_LOADER_DEFAULT,
    //    0,
    //    m_pTextureSRV.GetAddressOf()
    //);

    //HRESULT hr = CreateWICTextureFromFileEx(
    //    m_pDevice.Get(),                    // ID3D11Device*
    //    StringToWString(m_sPath).c_str(),       // 파일 경로
    //    0,                          // maxsize (0이면 제한 없음)
    //    D3D11_USAGE_DEFAULT,        // usage
    //    D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, // bindFlags
    //    0,                          // cpuAccessFlags
    //    D3D11_RESOURCE_MISC_GENERATE_MIPS, // miscFlags (중요!)
    //    WIC_LOADER_DEFAULT,         // loadFlags
    //    m_pTexture.GetAddressOf(),                  // [OUT]
    //    m_pTextureSRV.GetAddressOf()                       // [OUT]
    //);


//    HRESULT __cdecl CreateWICTextureFromFileEx(
//        _In_ ID3D11Device * d3dDevice,
//        _In_opt_ ID3D11DeviceContext * d3dContext,
//        _In_z_ const wchar_t* szFileName,
//        _In_ size_t maxsize,
//        _In_ D3D11_USAGE usage,
//        _In_ unsigned int bindFlags,
//        _In_ unsigned int cpuAccessFlags,
//        _In_ unsigned int miscFlags,
//        _In_ WIC_LOADER_FLAGS loadFlags,
//        _Outptr_opt_ ID3D11Resource * *texture,
//        _Outptr_opt_ ID3D11ShaderResourceView * *textureView) noexcept;

    //ComPtr<ID3D11Resource> pResource{};
    //HRESULT hr = CreateWICTextureFromFileEx(
    //    m_pDevice.Get(),                     // ID3D11Device*
    //    nullptr,                    // ⭐ ID3D11DeviceContext* (추가!)
    //    StringToWString(m_sPath).c_str(),    // 파일 경로
    //    0,                                   // maxsize
    //    D3D11_USAGE_DEFAULT,
    //    D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
    //    0,
    //    D3D11_RESOURCE_MISC_GENERATE_MIPS,   // mip 생성 가능
    //    DirectX::WIC_LOADER_DEFAULT,
    //    pResource.GetAddressOf(),
    //    m_pSRV.GetAddressOf()
    //);


    //HRESULT hr = CreateWICTextureFromFileEx(
    //    m_pDevice.Get(),                    // ID3D11Device*
    //    StringToWString(m_sPath).c_str(),       // 파일 경로
    //    0,                          // maxsize (0이면 제한 없음)
    //    D3D11_USAGE_DEFAULT,        // usage
    //    D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, // bindFlags
    //    0,                          // cpuAccessFlags
    //    D3D11_RESOURCE_MISC_GENERATE_MIPS, // miscFlags (중요!)
    //    WIC_LOADER_DEFAULT,         // loadFlags
    //    m_pTexture.GetAddressOf(),                  // [OUT]
    //    m_pTextureSRV.GetAddressOf()                       // [OUT]
    //);

   
    ComPtr<ID3D11Resource> pResource{};
    std::wstring path = StringToWString(m_sPath);
    std::wstring ext = std::filesystem::path(path).extension().wstring();
    

    HRESULT hr = E_FAIL;

    if (ext == L".dds")
    {
        hr = DirectX::CreateDDSTextureFromFile(
            m_pDevice.Get(),
            path.c_str(),
            pResource.GetAddressOf(),
            m_pSRV.GetAddressOf()
        );
    }
    else
    {
        hr = DirectX::CreateWICTextureFromFileEx(
            m_pDevice.Get(),
            m_pContext.Get(),
            path.c_str(),
            0,
            D3D11_USAGE_DEFAULT,
            D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
            0,
            D3D11_RESOURCE_MISC_GENERATE_MIPS,
			DirectX::WIC_LOADER_DEFAULT,
            pResource.GetAddressOf(),
            m_pSRV.GetAddressOf()
        );

        //hr = DirectX::CreateWICTextureFromFile(
        //    m_pDevice.Get(),
        //    m_pContext.Get(), // mip 자동 생성
        //    path.c_str(),
        //    pResource.GetAddressOf(),
        //    m_pSRV.GetAddressOf()
        //);
    }
    //HRESULT hr = DirectX::CreateWICTextureFromFile(
    //    m_pDevice.Get(),
    //    m_pContext.Get(), // <--- Context를 넣어주면 내부적으로 밉맵 체인을 전체 생성함
    //    StringToWString(m_sPath).c_str(),
    //    pResource.GetAddressOf(),
    //    m_pSRV.GetAddressOf()
    //);
    if (FAILED(hr))
    {
        m_eState = STATE::LOADFAIL;
        MSG_BOX_STR(_wstring{ L"CreateTextureFromFile Faield Path:" + StringToWString(m_sPath) }.c_str());
        return E_FAIL;
    }



    hr = pResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&m_pTexture);
    if (FAILED(hr))
    {
        m_eState = STATE::LOADFAIL;
        //MSG_BOX("CreateWICTextureFromFile Faield");
        MSG_BOX_STR(_wstring{ L"QueryInterface(__uuidof(ID3D11Texture2D) Faield Path:" + StringToWString(m_sPath) }.c_str());
        return E_FAIL;
    }
    //m_pContext->GenerateMips(m_pSRV.Get());
    // 3. Desc 구조체를 얻어와서 크기 확인
    m_pTexture->GetDesc(&m_Texture2DDesc);

    UINT width = m_Texture2DDesc.Width;   // 원본 가로 크기
    UINT height = m_Texture2DDesc.Height; // 원본 세로 크기

    // MipLevels 값이 1보다 크면 밉맵이 생성된 것임
    UINT mipCount = m_Texture2DDesc.MipLevels;
    // 확인용 출력
    //printf("Texture Size: %u x %u\n", width, height);
    int x = 0;


    m_eState = STATE::LOADED;


    return S_OK;
}

HRESULT CResTexture2D::Unload(const std::any& arg)
{
    return S_OK;
}

SPtr<CResTexture2D> CResTexture2D::Create(const _string& sPath)
{
    return ToSPtr(new CResTexture2D{ sPath, CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext() });
}


struct ImageData
{
    std::vector<uint8_t> pixels;
    UINT width = 0;
    UINT height = 0;
    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
};

static bool LoadImageWIC(const std::wstring& path, ImageData& out)
{
    ComPtr<IWICImagingFactory> factory;
    CoInitialize(nullptr);

    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory, nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory)
    );
    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapDecoder> decoder;
    hr = factory->CreateDecoderFromFilename(
        path.c_str(), nullptr, GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &decoder
    );
    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapFrameDecode> frame;
    decoder->GetFrame(0, &frame);

    frame->GetSize(&out.width, &out.height);

    // RGBA로 변환
    ComPtr<IWICFormatConverter> converter;
    factory->CreateFormatConverter(&converter);

    converter->Initialize(
        frame.Get(),
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr, 0.f,
        WICBitmapPaletteTypeCustom
    );

    out.pixels.resize(out.width * out.height * 4);

    converter->CopyPixels(
        nullptr,
        out.width * 4,
        static_cast<UINT>(out.pixels.size()),
        out.pixels.data()
    );

    return true;
}

static bool CreateTextureFromImage(
    ID3D11Device* device,
    ID3D11DeviceContext* context,
    const ImageData& img,
    ID3D11Texture2D** outTex,
    ID3D11ShaderResourceView** outSRV)
{
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = img.width;
    desc.Height = img.height;
    desc.MipLevels = 0; // ⭐ 중요 (전체 mip 생성)
    desc.ArraySize = 1;
    desc.Format = img.format;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    HRESULT hr = device->CreateTexture2D(&desc, nullptr, outTex);
    if (FAILED(hr)) return false;

    // level 0 업로드
    context->UpdateSubresource(
        *outTex,
        0,
        nullptr,
        img.pixels.data(),
        img.width * 4,
        0
    );

    // SRV 생성 (mip 전체)
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = -1; // ⭐ 중요

    hr = device->CreateShaderResourceView(*outTex, &srvDesc, outSRV);
    if (FAILED(hr)) return false;

    // mip 생성
    context->GenerateMips(*outSRV);

    return true;
}
