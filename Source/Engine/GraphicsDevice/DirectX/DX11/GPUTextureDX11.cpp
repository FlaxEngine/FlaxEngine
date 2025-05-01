// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX11

#include "GPUTextureDX11.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"

bool GPUTextureDX11::OnInit()
{
    // Cache formats
    const PixelFormat format = Format();
    const PixelFormat typelessFormat = PixelFormatExtensions::MakeTypeless(format);
    const DXGI_FORMAT dxgiFormat = RenderToolsDX::ToDxgiFormat(typelessFormat);
    _dxgiFormatDSV = RenderToolsDX::ToDxgiFormat(PixelFormatExtensions::FindDepthStencilFormat(format));
    _dxgiFormatSRV = RenderToolsDX::ToDxgiFormat(PixelFormatExtensions::FindShaderResourceFormat(format, _sRGB));
    _dxgiFormatRTV = _dxgiFormatSRV;
    _dxgiFormatUAV = RenderToolsDX::ToDxgiFormat(PixelFormatExtensions::FindUnorderedAccessFormat(format));

    // Cache properties
    auto device = _device->GetDevice();
    bool useSRV = IsShaderResource();
    bool useDSV = IsDepthStencil();
    bool useRTV = IsRenderTarget();
    bool useUAV = IsUnorderedAccess();

    // Create resource
    HRESULT result;
    if (IsVolume())
    {
        // Create texture description
        D3D11_TEXTURE3D_DESC textureDesc;
        textureDesc.MipLevels = MipLevels();
        textureDesc.Format = dxgiFormat;
        textureDesc.Width = Width();
        textureDesc.Height = Height();
        textureDesc.Depth = Depth();
        textureDesc.BindFlags = 0;
        textureDesc.CPUAccessFlags = RenderToolsDX::GetDX11CpuAccessFlagsFromUsage(_desc.Usage);
        textureDesc.MiscFlags = 0;
        textureDesc.Usage = RenderToolsDX::ToD3D11Usage(_desc.Usage);
        if (useSRV)
            textureDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        if (useRTV)
            textureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        if (useDSV)
            textureDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
        if (useUAV)
            textureDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

        // Create texture
        ID3D11Texture3D* texture;
        result = device->CreateTexture3D(&textureDesc, nullptr, &texture);
        _resource = texture;
    }
    else
    {
        // Create texture description
        D3D11_TEXTURE2D_DESC textureDesc;
        textureDesc.MipLevels = MipLevels();
        textureDesc.Format = dxgiFormat;
        textureDesc.Width = Width();
        textureDesc.Height = Height();
        textureDesc.Usage = RenderToolsDX::ToD3D11Usage(_desc.Usage);
        textureDesc.BindFlags = 0;
        textureDesc.CPUAccessFlags = RenderToolsDX::GetDX11CpuAccessFlagsFromUsage(_desc.Usage);
        textureDesc.ArraySize = ArraySize();
        textureDesc.SampleDesc.Count = static_cast<UINT>(_desc.MultiSampleLevel);
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.MiscFlags = 0;
        if (useSRV)
            textureDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        if (useRTV)
            textureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        if (useDSV)
            textureDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
        if (useUAV)
            textureDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        if (_device->GetFeatureLevel() >= FeatureLevel::SM5 && IsMultiSample())
            textureDesc.SampleDesc.Quality = D3D11_STANDARD_MULTISAMPLE_PATTERN;
        if (IsCubeMap())
            textureDesc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

        // Create texture
        ID3D11Texture2D* texture;
        result = device->CreateTexture2D(&textureDesc, nullptr, &texture);
        _resource = texture;
    }
    LOG_DIRECTX_RESULT_WITH_RETURN(result, true);
    ASSERT(_resource != nullptr);
    DX_SET_DEBUG_NAME(_resource, GetName());

    // Update memory usage
    _memoryUsage = calculateMemoryUsage();

    // Initialize handles to the resource
    if (IsRegularTexture())
    {
        // 'Regular' texture is using only one handle (texture/cubemap)
        _handlesPerSlice.Resize(1, false);
    }
    else
    {
        // Create all handles
        initHandles();
    }

    return false;
}

void GPUTextureDX11::OnResidentMipsChanged()
{
    const int32 firstMipIndex = MipLevels() - ResidentMipLevels();
    const int32 mipLevels = ResidentMipLevels();
    D3D11_SHADER_RESOURCE_VIEW_DESC srDesc;
    srDesc.Format = _dxgiFormatSRV;
    if (IsCubeMap())
    {
        srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srDesc.TextureCube.MostDetailedMip = firstMipIndex;
        srDesc.TextureCube.MipLevels = mipLevels;
    }
    else if (IsVolume())
    {
        srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
        srDesc.Texture3D.MostDetailedMip = firstMipIndex;
        srDesc.Texture3D.MipLevels = mipLevels;
    }
    else
    {
        srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srDesc.Texture2D.MostDetailedMip = firstMipIndex;
        srDesc.Texture2D.MipLevels = mipLevels;
    }
    ID3D11ShaderResourceView* srView = nullptr;
    if (mipLevels != 0)
        VALIDATE_DIRECTX_CALL(_device->GetDevice()->CreateShaderResourceView(_resource, &srDesc, &srView));
    GPUTextureViewDX11& view = IsVolume() ? _handleVolume : _handlesPerSlice[0];
    if (view.GetParent() == nullptr)
       view.Init(this, nullptr, srView, nullptr, nullptr, Format(), MultiSampleLevel());
    else
        view.SetSRV(srView);
}

void GPUTextureDX11::OnReleaseGPU()
{
    _handlesPerMip.Resize(0, false);
    _handlesPerSlice.Resize(0, false);
    _handleArray.Release();
    _handleVolume.Release();
    _handleReadOnlyDepth.Release();
    DX_SAFE_RELEASE_CHECK(_resource, 0);

    // Base
    GPUTexture::OnReleaseGPU();
}

#define CLEAR_VIEWS() rtView = nullptr; srView = nullptr; dsView = nullptr; uaView = nullptr

void GPUTextureDX11::initHandles()
{
    ID3D11RenderTargetView* rtView;
    ID3D11ShaderResourceView* srView;
    ID3D11DepthStencilView* dsView;
    ID3D11UnorderedAccessView* uaView;
    D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
    D3D11_SHADER_RESOURCE_VIEW_DESC srDesc;
    D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc;
    D3D11_UNORDERED_ACCESS_VIEW_DESC uaDesc;

    CLEAR_VIEWS();

    rtDesc.Format = _dxgiFormatRTV;
    srDesc.Format = _dxgiFormatSRV;
    dsDesc.Format = _dxgiFormatDSV;
    dsDesc.Flags = 0;
    uaDesc.Format = _dxgiFormatUAV;

    // Cache properties
    auto device = _device->GetDevice();
    bool useSRV = IsShaderResource();
    bool useDSV = IsDepthStencil();
    bool useRTV = IsRenderTarget();
    bool useUAV = IsUnorderedAccess();
    int32 arraySize = ArraySize();
    int32 mipLevels = MipLevels();
    bool isArray = arraySize > 1;
    bool isCubeMap = IsCubeMap();
    bool isMsaa = IsMultiSample();
    bool isVolume = IsVolume();
    auto format = Format();
    auto msaa = MultiSampleLevel();

    // Create resource views
    if (isVolume)
    {
        // Create handle for whole 3d texture
        if (useSRV)
        {
            srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
            srDesc.Texture3D.MostDetailedMip = 0;
            srDesc.Texture3D.MipLevels = mipLevels;
            VALIDATE_DIRECTX_CALL(device->CreateShaderResourceView(_resource, &srDesc, &srView));
        }
        if (useRTV)
        {
            rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
            rtDesc.Texture3D.MipSlice = 0;
            rtDesc.Texture3D.FirstWSlice = 0;
            rtDesc.Texture3D.WSize = Depth();
            VALIDATE_DIRECTX_CALL(device->CreateRenderTargetView(_resource, &rtDesc, &rtView));
        }
        if (useUAV)
        {
            uaDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
            uaDesc.Texture3D.MipSlice = 0;
            uaDesc.Texture3D.WSize = Depth();
            uaDesc.Texture3D.FirstWSlice = 0;
            VALIDATE_DIRECTX_CALL(device->CreateUnorderedAccessView(_resource, &uaDesc, &uaView));
        }
        _handleVolume.Init(this, rtView, srView, nullptr, uaView, format, msaa);

        // Init per slice views
        _handlesPerSlice.Resize(Depth(), false);
        if (_desc.HasPerSliceViews() && useRTV)
        {
            rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
            rtDesc.Texture3D.MipSlice = 0;
            rtDesc.Texture3D.WSize = 1;

            for (int32 sliceIndex = 0; sliceIndex < Depth(); sliceIndex++)
            {
                rtDesc.Texture3D.FirstWSlice = sliceIndex;
                VALIDATE_DIRECTX_CALL(device->CreateRenderTargetView(_resource, &rtDesc, &rtView));
                _handlesPerSlice[sliceIndex].Init(this, rtView, nullptr, nullptr, nullptr, format, msaa);
            }
        }
    }
    else if (isArray)
    {
        // Resize handles
        _handlesPerSlice.Resize(ArraySize(), false);

        // Create per array slice handles
        for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
        {
            CLEAR_VIEWS();

            if (useDSV)
            {
                /*if (isCubeMap)
                {
                    dsDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    dsDesc.Texture2DArray.ArraySize = 6;
                    dsDesc.Texture2DArray.FirstArraySlice = arrayIndex * 6;
                    dsDesc.Texture2DArray.MipSlice = 0;
                }
                else*/
                {
                    dsDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    dsDesc.Texture2DArray.ArraySize = 1;
                    dsDesc.Texture2DArray.FirstArraySlice = arrayIndex;
                    dsDesc.Texture2DArray.MipSlice = 0;
                }
                VALIDATE_DIRECTX_CALL(device->CreateDepthStencilView(_resource, &dsDesc, &dsView));
            }
            if (useRTV)
            {
                /*if (isCubeMap)
                {
                    rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    rtDesc.Texture2DArray.ArraySize = 6;
                    rtDesc.Texture2DArray.FirstArraySlice = arrayIndex * 6;
                    rtDesc.Texture2DArray.MipSlice = 0;
                }
                else*/
                {
                    rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    rtDesc.Texture2DArray.ArraySize = 1;
                    rtDesc.Texture2DArray.FirstArraySlice = arrayIndex;
                    rtDesc.Texture2DArray.MipSlice = 0;
                }
                VALIDATE_DIRECTX_CALL(device->CreateRenderTargetView(_resource, &rtDesc, &rtView));
            }
            if (useSRV)
            {
                // When GetFeatureLevel returns D3D_FEATURE_LEVEL_10_0 or less, Resources created with D3D11_RESOURCE_MISC_TEXTURECUBE may only be treated as cubemap ShaderResourceViews.
                // (ViewDimension must be D3D11_SRV_DIMENSION_TEXTURECUBE) [ STATE_CREATION ERROR #126: CREATESHADERRESOURCEVIEW_INVALIDDESC]
                if (isCubeMap && _device->GetRendererType() != RendererType::DirectX10)
                {
                    /*if (isCubeMap)
                    {
                        srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
                        srDesc.TextureCubeArray.First2DArrayFace = arrayIndex * 6;
                        srDesc.TextureCubeArray.NumCubes = 1;
                        srDesc.TextureCubeArray.MipLevels = mipLevels;
                        srDesc.TextureCubeArray.MostDetailedMip = 0;
                    }
                    else*/
                    {
                        srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                        srDesc.Texture2DArray.ArraySize = 1;
                        srDesc.Texture2DArray.FirstArraySlice = arrayIndex;
                        srDesc.Texture2DArray.MipLevels = mipLevels;
                        srDesc.Texture2DArray.MostDetailedMip = 0;
                    }
                    VALIDATE_DIRECTX_CALL(device->CreateShaderResourceView(_resource, &srDesc, &srView));
                }
            }

            _handlesPerSlice[arrayIndex].Init(this, rtView, srView, dsView, nullptr, format, msaa);
        }

        // Create whole array handle
        {
            CLEAR_VIEWS();

            if (useDSV)
            {
                dsDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                dsDesc.Texture2DArray.ArraySize = arraySize;
                dsDesc.Texture2DArray.FirstArraySlice = 0;
                dsDesc.Texture2DArray.MipSlice = 0;
                VALIDATE_DIRECTX_CALL(device->CreateDepthStencilView(_resource, &dsDesc, &dsView));
            }
            if (useRTV)
            {
                rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                rtDesc.Texture2DArray.ArraySize = arraySize;
                rtDesc.Texture2DArray.FirstArraySlice = 0;
                rtDesc.Texture2DArray.MipSlice = 0;
                VALIDATE_DIRECTX_CALL(device->CreateRenderTargetView(_resource, &rtDesc, &rtView));
            }
            if (useSRV)
            {
                if (isCubeMap)
                {
                    srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                    srDesc.TextureCube.MostDetailedMip = 0;
                    srDesc.TextureCube.MipLevels = mipLevels;
                }
                else
                {
                    srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                    srDesc.Texture2DArray.ArraySize = arraySize;
                    srDesc.Texture2DArray.FirstArraySlice = 0;
                    srDesc.Texture2DArray.MipLevels = mipLevels;
                    srDesc.Texture2DArray.MostDetailedMip = 0;
                }
                VALIDATE_DIRECTX_CALL(device->CreateShaderResourceView(_resource, &srDesc, &srView));
            }
            if (useUAV)
            {
                uaDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                uaDesc.Texture2DArray.MipSlice = 0;
                uaDesc.Texture2DArray.ArraySize = arraySize;
                uaDesc.Texture2DArray.FirstArraySlice = 0;
                VALIDATE_DIRECTX_CALL(device->CreateUnorderedAccessView(_resource, &uaDesc, &uaView));
            }
            _handleArray.Init(this, rtView, srView, dsView, uaView, format, msaa);
        }
    }
    else
    {
        // Resize handles
        _handlesPerSlice.Resize(1, false);
        CLEAR_VIEWS();

        // Create single handle for the whole texture
        if (useDSV)
        {
            if (isCubeMap)
            {
                dsDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                dsDesc.Texture2DArray.MipSlice = 0;
                dsDesc.Texture2DArray.FirstArraySlice = 0;
                dsDesc.Texture2DArray.ArraySize = arraySize * 6;
            }
            else if (isMsaa)
            {
                dsDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                dsDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                dsDesc.Texture2D.MipSlice = 0;
            }
            VALIDATE_DIRECTX_CALL(device->CreateDepthStencilView(_resource, &dsDesc, &dsView));
        }
        if (useRTV)
        {
            if (isCubeMap)
            {
                rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                rtDesc.Texture2DArray.MipSlice = 0;
                rtDesc.Texture2DArray.FirstArraySlice = 0;
                rtDesc.Texture2DArray.ArraySize = arraySize * 6;
            }
            else if (isMsaa)
            {
                rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                rtDesc.Texture2D.MipSlice = 0;
            }
            VALIDATE_DIRECTX_CALL(device->CreateRenderTargetView(_resource, &rtDesc, &rtView));
        }
        if (useSRV)
        {
            if (isCubeMap)
            {
                srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                srDesc.TextureCube.MostDetailedMip = 0;
                srDesc.TextureCube.MipLevels = mipLevels;
            }
            else if (isMsaa)
            {
                srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srDesc.Texture2D.MostDetailedMip = 0;
                srDesc.Texture2D.MipLevels = mipLevels;
            }
            VALIDATE_DIRECTX_CALL(device->CreateShaderResourceView(_resource, &srDesc, &srView));
        }
        if (useUAV)
        {
            uaDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            uaDesc.Texture2D.MipSlice = 0;
            VALIDATE_DIRECTX_CALL(device->CreateUnorderedAccessView(_resource, &uaDesc, &uaView));
        }
        _handlesPerSlice[0].Init(this, rtView, srView, dsView, uaView, format, msaa);
    }

    // Init per mip map handles
    if (HasPerMipViews())
    {
        // Init handles
        _handlesPerMip.Resize(arraySize, false);
        for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
        {
            auto& slice = _handlesPerMip[arrayIndex];
            slice.Resize(mipLevels, false);

            for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
            {
                dsView = nullptr;
                rtView = nullptr;
                srView = nullptr;

                // DSV
                if (useDSV)
                {
                    dsDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    dsDesc.Texture2DArray.ArraySize = 1;
                    dsDesc.Texture2DArray.FirstArraySlice = arrayIndex;
                    dsDesc.Texture2DArray.MipSlice = mipIndex;
                    _device->GetDevice()->CreateDepthStencilView(_resource, &dsDesc, &dsView);
                }

                // RTV
                if (useRTV)
                {
                    rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    rtDesc.Texture2DArray.ArraySize = 1;
                    rtDesc.Texture2DArray.FirstArraySlice = arrayIndex;
                    rtDesc.Texture2DArray.MipSlice = mipIndex;
                    _device->GetDevice()->CreateRenderTargetView(_resource, &rtDesc, &rtView);
                }

                // SRV
                if (useSRV)
                {
                    // When GetFeatureLevel returns D3D_FEATURE_LEVEL_10_0 or less, Resources created with D3D11_RESOURCE_MISC_TEXTURECUBE may only be treated as cubemap ShaderResourceViews.
                    // (ViewDimension must be D3D11_SRV_DIMENSION_TEXTURECUBE) [ STATE_CREATION ERROR #126: CREATESHADERRESOURCEVIEW_INVALIDDESC]
                    if ((isCubeMap && _device->GetRendererType() == RendererType::DirectX10) == false)
                    {
                        srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                        srDesc.Texture2DArray.ArraySize = 1;
                        srDesc.Texture2DArray.FirstArraySlice = arrayIndex;
                        srDesc.Texture2DArray.MipLevels = 1;
                        srDesc.Texture2DArray.MostDetailedMip = mipIndex;
                        _device->GetDevice()->CreateShaderResourceView(_resource, &srDesc, &srView);
                    }
                }

                slice[mipIndex].Init(this, rtView, srView, dsView, nullptr, format, msaa);
            }
        }
    }

    // Read-only depth-stencil
    if (EnumHasAnyFlags(_desc.Flags, GPUTextureFlags::ReadOnlyDepthView))
    {
        CLEAR_VIEWS();

        // Create single handle for the whole texture
        if (useDSV)
        {
            if (isCubeMap)
            {
                dsDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                dsDesc.Texture2DArray.MipSlice = 0;
                dsDesc.Texture2DArray.FirstArraySlice = 0;
                dsDesc.Texture2DArray.ArraySize = arraySize * 6;
            }
            else if (isMsaa)
            {
                dsDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                dsDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                dsDesc.Texture2D.MipSlice = 0;
            }
            dsDesc.Flags = D3D11_DSV_READ_ONLY_DEPTH;
            if (PixelFormatExtensions::HasStencil(format))
                dsDesc.Flags |= D3D11_DSV_READ_ONLY_STENCIL;
            VALIDATE_DIRECTX_CALL(device->CreateDepthStencilView(_resource, &dsDesc, &dsView));
        }
        ASSERT(!useRTV);
        rtView = nullptr;
        if (useSRV)
        {
            if (isCubeMap)
            {
                srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                srDesc.TextureCube.MostDetailedMip = 0;
                srDesc.TextureCube.MipLevels = mipLevels;
            }
            else if (isMsaa)
            {
                srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srDesc.Texture2D.MostDetailedMip = 0;
                srDesc.Texture2D.MipLevels = mipLevels;
            }
            VALIDATE_DIRECTX_CALL(device->CreateShaderResourceView(_resource, &srDesc, &srView));
        }
        _handleReadOnlyDepth.Init(this, rtView, srView, dsView, nullptr, format, msaa);
    }
}

bool GPUTextureDX11::GetData(int32 arrayIndex, int32 mipMapIndex, TextureMipData& data, uint32 mipRowPitch)
{
    if (!IsStaging())
    {
        LOG(Warning, "Texture::GetData is valid only for staging resources.");
        return true;
    }
    GPUDeviceLock lock(_device);

    // Map the staging resource mip map for reading
    const uint32 subresource = RenderToolsDX::CalcSubresourceIndex(mipMapIndex, arrayIndex, MipLevels());
    D3D11_MAPPED_SUBRESOURCE mapped;
    const HRESULT mapResult = _device->GetIM()->Map(_resource, subresource, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(mapResult))
    {
        LOG_DIRECTX_RESULT(mapResult);
        return true;
    }

    data.Copy(mapped.pData, mapped.RowPitch, mapped.DepthPitch, Depth(), mipRowPitch);

    // Unmap texture
    _device->GetIM()->Unmap(_resource, subresource);

    return false;
}

#endif
