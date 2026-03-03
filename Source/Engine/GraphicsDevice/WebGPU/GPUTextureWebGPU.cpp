// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_WEBGPU

#include "GPUTextureWebGPU.h"
#include "RenderToolsWebGPU.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/PixelFormatExtensions.h"

bool HasStencil(WGPUTextureFormat format)
{
    switch (format)
    {
    case WGPUTextureFormat_Depth24PlusStencil8:
    case WGPUTextureFormat_Depth32FloatStencil8:
        return true;
    default:
        return false;
    }
}

WGPUTextureFormat DropStencil(WGPUTextureFormat format)
{
    switch (format)
    {
    case WGPUTextureFormat_Depth24PlusStencil8:
        return WGPUTextureFormat_Depth24Plus;
    case WGPUTextureFormat_Depth32FloatStencil8:
        return WGPUTextureFormat_Depth32Float;
    default:
        return format;
    }
}

void SetWebGPUTextureViewSampler(GPUTextureView* view, uint32 samplerType)
{
    ((GPUTextureViewWebGPU*)view)->SampleType = (WGPUTextureSampleType)samplerType;
}

void GPUTextureViewWebGPU::Create(WGPUTexture texture, const WGPUTextureViewDescriptor& desc)
{
    Ptr.Version++;
    if (View)
        wgpuTextureViewRelease(View);
    Texture = texture;

    auto viewDesc = desc;
    auto renderDesc = desc;
    auto separateViews = false;

    // Render views cannot have more than 1 mip levels count
    if (desc.usage & WGPUTextureUsage_RenderAttachment && renderDesc.mipLevelCount > 1)
    {
        renderDesc.mipLevelCount = 1;
        separateViews = true;
    }

    // Depth-stencil textures expose depth-only when binding to shaders (unless via custom _handleStencil view) so make separate ViewRender for rendering with all components
    if (desc.aspect == WGPUTextureAspect_All && ::HasStencil(desc.format))
    {
        viewDesc.aspect = WGPUTextureAspect_DepthOnly;
        viewDesc.format = DropStencil(viewDesc.format);
        separateViews = true;
    }

    // Create views
    View = wgpuTextureCreateView(texture, &viewDesc);
    if (!View)
    {
#if GPU_ENABLE_RESOURCE_NAMING
        LOG(Error, "Failed to create a view for texture '{}'", GetParent() ? GetParent()->GetName() : StringView::Empty);
#else
        LOG(Error, "Failed to create a view for texture");
#endif
    }
    if (separateViews)
        ViewRender = wgpuTextureCreateView(texture, &renderDesc);
    else
        ViewRender = View;

    // Cache metadata
    Format = desc.format;
    switch (Format)
    {
    case WGPUTextureFormat_Depth16Unorm:
    case WGPUTextureFormat_Depth24Plus:
    case WGPUTextureFormat_Depth24PlusStencil8:
    case WGPUTextureFormat_Depth32Float:
    case WGPUTextureFormat_Depth32FloatStencil8:
        // https://www.w3.org/TR/webgpu/#depth-formats
        SampleType = WGPUTextureSampleType_UnfilterableFloat;
        break;
    case WGPUTextureFormat_Stencil8:
        // https://www.w3.org/TR/webgpu/#depth-formats
        SampleType = WGPUTextureSampleType_Uint;
        break;
    default:
        SampleType = WGPUTextureSampleType_Undefined;
        break;
    }
    RenderSize.Width = Math::Max<uint16>(wgpuTextureGetWidth(Texture) >> renderDesc.baseMipLevel, 1);
    RenderSize.Height = Math::Max<uint16>(wgpuTextureGetHeight(Texture) >> renderDesc.baseMipLevel, 1);
}

void GPUTextureViewWebGPU::Release()
{
    if (View != ViewRender)
    {
        wgpuTextureViewRelease(ViewRender);
        ViewRender = nullptr;
    }
    if (View)
    {
        wgpuTextureViewRelease(View);
        View = nullptr;
    }
    Texture = nullptr;
}

bool GPUTextureWebGPU::OnInit()
{
    // Create texture
    WGPUTextureDescriptor textureDesc = WGPU_TEXTURE_DESCRIPTOR_INIT;
#if GPU_ENABLE_RESOURCE_NAMING
    _name.Set(_namePtr, _nameSize);
    textureDesc.label = { _name.Get(), (size_t)_name.Length() };
#endif
    if (!IsDepthStencil())
        textureDesc.usage |= WGPUTextureUsage_CopyDst;
    if (IsStaging())
        textureDesc.usage |= WGPUTextureUsage_CopySrc | WGPUTextureUsage_CopyDst;
    if (IsShaderResource())
        textureDesc.usage |= WGPUTextureUsage_TextureBinding;
    if (IsUnorderedAccess())
        textureDesc.usage |= WGPUTextureUsage_StorageBinding;
    if (IsRenderTarget() || IsDepthStencil())
        textureDesc.usage |= WGPUTextureUsage_RenderAttachment;
    textureDesc.size.width = _desc.Width;
    textureDesc.size.height = _desc.Height;
    switch (_desc.Dimensions)
    {
    case TextureDimensions::Texture:
        _viewDimension = IsArray() ? WGPUTextureViewDimension_2DArray : WGPUTextureViewDimension_2D;
        textureDesc.dimension = WGPUTextureDimension_2D;
        textureDesc.size.depthOrArrayLayers = _desc.ArraySize;
        break;
    case TextureDimensions::VolumeTexture:
        _viewDimension = WGPUTextureViewDimension_3D;
        textureDesc.dimension = WGPUTextureDimension_3D;
        textureDesc.size.depthOrArrayLayers = _desc.Depth;
        break;
    case TextureDimensions::CubeTexture:
        _viewDimension = _desc.ArraySize > 6 ? WGPUTextureViewDimension_CubeArray : WGPUTextureViewDimension_Cube;
        textureDesc.dimension = WGPUTextureDimension_2D;
        textureDesc.size.depthOrArrayLayers = _desc.ArraySize;
        break;
    }
    textureDesc.format = RenderToolsWebGPU::ToTextureFormat(Format());
    textureDesc.mipLevelCount = _desc.MipLevels;
    textureDesc.sampleCount = (uint32_t)_desc.MultiSampleLevel;
    textureDesc.viewFormats = &textureDesc.format;
    textureDesc.viewFormatCount = 1;
    _format = textureDesc.format;
    Usage = textureDesc.usage;
    Texture = wgpuDeviceCreateTexture(_device->Device, &textureDesc);
    if (!Texture)
        return true;

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
        InitHandles();
    }

    // Setup metadata for the views
    bool hasStencil = PixelFormatExtensions::HasStencil(Format());
    if (hasStencil)
    {
        _handleArray.HasStencil = hasStencil;
        _handleVolume.HasStencil = hasStencil;
        _handleReadOnlyDepth.HasStencil = hasStencil;
        _handleStencil.HasStencil = hasStencil;
        for (auto& e : _handlesPerSlice)
            e.HasStencil = hasStencil;
        for (auto& q : _handlesPerMip)
            for (auto& e : q)
                e.HasStencil = hasStencil;
    }

    return false;
}

void GPUTextureWebGPU::OnResidentMipsChanged()
{
    // Update the view to handle base mip level as highest resident mip
    WGPUTextureViewDescriptor viewDesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
    viewDesc.format = _format;
    viewDesc.usage = Usage;
    viewDesc.dimension = _viewDimension;
    viewDesc.baseMipLevel = MipLevels() - ResidentMipLevels();
    viewDesc.mipLevelCount = ResidentMipLevels();
    viewDesc.arrayLayerCount = ArraySize();
    viewDesc.aspect = WGPUTextureAspect_All;
    GPUTextureViewWebGPU& view = IsVolume() ? _handleVolume : _handlesPerSlice[0];
    if (view.GetParent() == nullptr)
        view.Init(this, _desc.Format, _desc.MultiSampleLevel);
    view.Create(Texture, viewDesc);
}

void GPUTextureWebGPU::OnReleaseGPU()
{
    _handlesPerMip.Resize(0, false);
    _handlesPerSlice.Resize(0, false);
    _handleArray.Release();
    _handleVolume.Release();
    _handleReadOnlyDepth.Release();
    _handleStencil.Release();
    if (Texture)
    {
        wgpuTextureDestroy(Texture);
        wgpuTextureRelease(Texture);
        Texture = nullptr;
    }
#if GPU_ENABLE_RESOURCE_NAMING
    _name.Clear();
#endif

    // Base
    GPUTexture::OnReleaseGPU();
}

void GPUTextureWebGPU::InitHandles()
{
    WGPUTextureViewDescriptor viewDesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
#if GPU_ENABLE_RESOURCE_NAMING
    viewDesc.label = { _name.Get(), (size_t)_name.Length() };
#endif
    viewDesc.format = _format;
    viewDesc.usage = Usage;
    viewDesc.dimension = _viewDimension;
    viewDesc.mipLevelCount = MipLevels();
    viewDesc.arrayLayerCount = ArraySize();
    viewDesc.aspect = WGPUTextureAspect_All;

    auto format = Format();
    auto msaa = MultiSampleLevel();

    if (IsVolume())
    {
        // Create handle for whole 3d texture
        {
            auto& view = _handleVolume;
            view.Init(this, format, msaa);
            view.Create(Texture, viewDesc);
        }

        // Init per slice views
        _handlesPerSlice.Resize(Depth(), false);
        if (_desc.HasPerSliceViews() && IsRenderTarget())
        {
            for (int32 sliceIndex = 0; sliceIndex < Depth(); sliceIndex++)
            {
                auto& view = _handlesPerSlice[sliceIndex];
                view.Init(this, format, msaa);
                view.Create(Texture, viewDesc);
                view.DepthSlice = sliceIndex;
            }
        }
    }
    else if (IsArray())
    {
        // Create whole array handle
        {
            auto& view = _handleArray;
            view.Init(this, format, msaa);
            view.Create(Texture, viewDesc);
        }

        // Create per array slice handles
        _handlesPerSlice.Resize(ArraySize(), false);
        viewDesc.dimension = WGPUTextureViewDimension_2D;
        for (int32 arrayIndex = 0; arrayIndex < _handlesPerSlice.Count(); arrayIndex++)
        {
            viewDesc.baseArrayLayer = arrayIndex;
            viewDesc.arrayLayerCount = 1;
            auto& view = _handlesPerSlice[arrayIndex];
            view.Init(this, format, msaa);
            view.Create(Texture, viewDesc);
        }
        viewDesc.baseArrayLayer = 0;
        viewDesc.arrayLayerCount = MipLevels();
        viewDesc.dimension = _viewDimension;
    }
    else
    {
        // Create single handle for the whole texture
        _handlesPerSlice.Resize(1, false);
        auto& view = _handlesPerSlice[0];
        view.Init(this, format, msaa);
        view.Create(Texture, viewDesc);
    }

    // Init per mip map handles
    if (HasPerMipViews())
    {
        // Init handles
        _handlesPerMip.Resize(ArraySize(), false);
        viewDesc.dimension = WGPUTextureViewDimension_2D;
        for (int32 arrayIndex = 0; arrayIndex < _handlesPerMip.Count(); arrayIndex++)
        {
            auto& slice = _handlesPerMip[arrayIndex];
            slice.Resize(MipLevels(), false);
            viewDesc.baseArrayLayer = arrayIndex;
            viewDesc.arrayLayerCount = 1;
            viewDesc.mipLevelCount = 1;
            for (int32 mipIndex = 0; mipIndex < slice.Count(); mipIndex++)
            {
                auto& view = slice[mipIndex];
                viewDesc.baseMipLevel = mipIndex;
                view.Init(this, format, msaa);
                view.Create(Texture, viewDesc);
            }
        }
        viewDesc.dimension = _viewDimension;
    }

    // Read-only depth-stencil
    if (EnumHasAnyFlags(_desc.Flags, GPUTextureFlags::ReadOnlyDepthView))
    {
        auto& view = _handleReadOnlyDepth;
        view.Init(this, format, msaa);
        view.Create(Texture, viewDesc);
        view.ReadOnly = true;
    }

    // Stencil view
    if (IsDepthStencil() && IsShaderResource() && PixelFormatExtensions::HasStencil(format))
    {
        PixelFormat stencilFormat = format;
        switch (format)
        {
        case PixelFormat::D24_UNorm_S8_UInt:
            stencilFormat = PixelFormat::X24_Typeless_G8_UInt;
            break;
        case PixelFormat::D32_Float_S8X24_UInt:
            stencilFormat = PixelFormat::X32_Typeless_G8X24_UInt;
            break;
        }
        viewDesc.aspect = WGPUTextureAspect_StencilOnly;
        viewDesc.format = WGPUTextureFormat_Stencil8;
        _handleStencil.Init(this, stencilFormat, msaa);
        _handleStencil.Create(Texture, viewDesc);
    }
}

bool GPUTextureWebGPU::GetData(int32 arrayIndex, int32 mipMapIndex, TextureMipData& data, uint32 mipRowPitch)
{
    // TODO: not implemented
    return true;
}

#endif
