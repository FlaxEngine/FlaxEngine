// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "MaterialParams.h"
#include "MaterialInfo.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Engine/GameplayGlobals.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPULimits.h"

const Char* ToString(MaterialParameterType value)
{
    const Char* result;
    switch (value)
    {
    case MaterialParameterType::Invalid:
        result = TEXT("Invalid");
        break;
    case MaterialParameterType::Bool:
        result = TEXT("Bool");
        break;
    case MaterialParameterType::Integer:
        result = TEXT("Integer");
        break;
    case MaterialParameterType::Float:
        result = TEXT("Float");
        break;
    case MaterialParameterType::Vector2:
        result = TEXT("Vector2");
        break;
    case MaterialParameterType::Vector3:
        result = TEXT("Vector3");
        break;
    case MaterialParameterType::Vector4:
        result = TEXT("Vector4");
        break;
    case MaterialParameterType::Color:
        result = TEXT("Color");
        break;
    case MaterialParameterType::Texture:
        result = TEXT("Texture");
        break;
    case MaterialParameterType::CubeTexture:
        result = TEXT("CubeTexture");
        break;
    case MaterialParameterType::NormalMap:
        result = TEXT("NormalMap");
        break;
    case MaterialParameterType::SceneTexture:
        result = TEXT("SceneTexture");
        break;
    case MaterialParameterType::GPUTexture:
        result = TEXT("GPUTexture");
        break;
    case MaterialParameterType::Matrix:
        result = TEXT("Matrix");
        break;
    case MaterialParameterType::GPUTextureArray:
        result = TEXT("GPUTextureArray");
        break;
    case MaterialParameterType::GPUTextureVolume:
        result = TEXT("GPUTextureVolume");
        break;
    case MaterialParameterType::GPUTextureCube:
        result = TEXT("GPUTextureCube");
        break;
    case MaterialParameterType::ChannelMask:
        result = TEXT("ChannelMask");
        break;
    case MaterialParameterType::GameplayGlobal:
        result = TEXT("GameplayGlobal");
        break;
    default:
        result = TEXT("");
        break;
    }
    return result;
}

Variant MaterialParameter::GetValue() const
{
    switch (_type)
    {
    case MaterialParameterType::Bool:
        return _asBool;
    case MaterialParameterType::Integer:
    case MaterialParameterType::SceneTexture:
    case MaterialParameterType::ChannelMask:
        return _asInteger;
    case MaterialParameterType::Float:
        return _asFloat;
    case MaterialParameterType::Vector2:
        return _asVector2;
    case MaterialParameterType::Vector3:
        return _asVector3;
    case MaterialParameterType::Vector4:
        return _asVector4;
    case MaterialParameterType::Color:
        return _asColor;
    case MaterialParameterType::Matrix:
        return Variant(_asMatrix);
    case MaterialParameterType::NormalMap:
    case MaterialParameterType::Texture:
    case MaterialParameterType::CubeTexture:
    case MaterialParameterType::GameplayGlobal:
        return _asAsset.Get();
    case MaterialParameterType::GPUTextureVolume:
    case MaterialParameterType::GPUTextureArray:
    case MaterialParameterType::GPUTextureCube:
    case MaterialParameterType::GPUTexture:
        return _asGPUTexture.Get();
    default:
    CRASH;
        return Variant::Zero;
    }
}

void MaterialParameter::SetValue(const Variant& value)
{
    bool invalidType = false;
    switch (_type)
    {
    case MaterialParameterType::Bool:
        _asBool = (bool)value;
        break;
    case MaterialParameterType::Integer:
    case MaterialParameterType::SceneTexture:
    case MaterialParameterType::ChannelMask:
        _asInteger = (int32)value;
        break;
    case MaterialParameterType::Float:
        _asFloat = (float)value;
        break;
    case MaterialParameterType::Vector2:
        _asVector2 = (Vector2)value;
        break;
    case MaterialParameterType::Vector3:
        _asVector3 = (Vector3)value;
        break;
    case MaterialParameterType::Vector4:
        _asVector4 = (Vector4)value;
        break;
    case MaterialParameterType::Color:
        _asColor = (Color)value;
        break;
    case MaterialParameterType::Matrix:
        _asMatrix = (Matrix)value;
        break;
    case MaterialParameterType::NormalMap:
    case MaterialParameterType::Texture:
    case MaterialParameterType::CubeTexture:
        switch (value.Type.Type)
        {
        case VariantType::Null:
            _asAsset = nullptr;
            break;
        case VariantType::Guid:
            _asAsset = Content::LoadAsync<TextureBase>(*(Guid*)value.AsData);
            break;
        case VariantType::Pointer:
            _asAsset = (TextureBase*)value.AsPointer;
            break;
        case VariantType::Object:
            _asAsset = Cast<TextureBase>(value.AsObject);
            break;
        case VariantType::Asset:
            _asAsset = Cast<TextureBase>(value.AsAsset);
            break;
        default:
            invalidType = true;
        }
        break;
    case MaterialParameterType::GPUTextureVolume:
    case MaterialParameterType::GPUTextureCube:
    case MaterialParameterType::GPUTextureArray:
    case MaterialParameterType::GPUTexture:
        switch (value.Type.Type)
        {
        case VariantType::Null:
            _asGPUTexture = nullptr;
            break;
        case VariantType::Guid:
            _asGPUTexture = *(Guid*)value.AsData;
            break;
        case VariantType::Pointer:
            _asGPUTexture = (GPUTexture*)value.AsPointer;
            break;
        case VariantType::Object:
            _asGPUTexture = Cast<GPUTexture>(value.AsObject);
            break;
        default:
            invalidType = true;
        }
        break;
    case MaterialParameterType::GameplayGlobal:
        switch (value.Type.Type)
        {
        case VariantType::Null:
            _asAsset = nullptr;
            break;
        case VariantType::Guid:
            _asAsset = Content::LoadAsync<GameplayGlobals>(*(Guid*)value.AsData);
            break;
        case VariantType::Pointer:
            _asAsset = (GameplayGlobals*)value.AsPointer;
            break;
        case VariantType::Object:
            _asAsset = Cast<GameplayGlobals>(value.AsObject);
            break;
        case VariantType::Asset:
            _asAsset = Cast<GameplayGlobals>(value.AsAsset);
            break;
        default:
            invalidType = true;
        }
        break;
    default:
        invalidType = true;
    }
    if (invalidType)
    {
        LOG(Error, "Invalid material parameter value type {0} to set (param type: {1})", value.Type, ::ToString(_type));
    }
}

void MaterialParameter::Bind(BindMeta& meta) const
{
    switch (_type)
    {
    case MaterialParameterType::Bool:
        if (meta.Buffer0)
            *((int32*)(meta.Buffer0 + _offset)) = _asBool;
        break;
    case MaterialParameterType::Integer:
        if (meta.Buffer0)
            *((int32*)(meta.Buffer0 + _offset)) = _asInteger;
        break;
    case MaterialParameterType::Float:
        if (meta.Buffer0)
            *((float*)(meta.Buffer0 + _offset)) = _asFloat;
        break;
    case MaterialParameterType::Vector2:
        if (meta.Buffer0)
            *((Vector2*)(meta.Buffer0 + _offset)) = _asVector2;
        break;
    case MaterialParameterType::Vector3:
        if (meta.Buffer0)
            *((Vector3*)(meta.Buffer0 + _offset)) = _asVector3;
        break;
    case MaterialParameterType::Vector4:
        if (meta.Buffer0)
            *((Vector4*)(meta.Buffer0 + _offset)) = _asVector4;
        break;
    case MaterialParameterType::Color:
        if (meta.Buffer0)
            *((Color*)(meta.Buffer0 + _offset)) = _asColor;
        break;
    case MaterialParameterType::Matrix:
        if (meta.Buffer0)
            Matrix::Transpose(_asMatrix, *(Matrix*)(meta.Buffer0 + _offset));
        break;
    case MaterialParameterType::NormalMap:
    {
        // If normal map texture is set but not loaded yet, use default engine normal map (reduces loading artifacts)
        auto texture = _asAsset ? ((TextureBase*)_asAsset.Get())->GetTexture() : nullptr;
        if (texture && texture->ResidentMipLevels() == 0)
            texture = GPUDevice::Instance->GetDefaultNormalMap();
        const auto view = GET_TEXTURE_VIEW_SAFE(texture);
        meta.Context->BindSR(_registerIndex, view);
        break;
    }
    case MaterialParameterType::Texture:
    case MaterialParameterType::CubeTexture:
    {
        const auto texture = _asAsset ? ((TextureBase*)_asAsset.Get())->GetTexture() : nullptr;
        const auto view = GET_TEXTURE_VIEW_SAFE(texture);
        meta.Context->BindSR(_registerIndex, view);
        break;
    }
    case MaterialParameterType::GPUTexture:
    {
        const auto texture = _asGPUTexture.Get();
        const auto view = GET_TEXTURE_VIEW_SAFE(texture);
        meta.Context->BindSR(_registerIndex, view);
        break;
    }
    case MaterialParameterType::GPUTextureArray:
    case MaterialParameterType::GPUTextureCube:
    {
        const auto view = _asGPUTexture ? _asGPUTexture->ViewArray() : nullptr;
        meta.Context->BindSR(_registerIndex, view);
        break;
    }
    case MaterialParameterType::GPUTextureVolume:
    {
        const auto view = _asGPUTexture ? _asGPUTexture->ViewVolume() : nullptr;
        meta.Context->BindSR(_registerIndex, view);
        break;
    }
    case MaterialParameterType::SceneTexture:
    {
        GPUTextureView* view = nullptr;
        const auto type = (MaterialSceneTextures)_asInteger;
        if (type == MaterialSceneTextures::SceneColor)
        {
            view = meta.Input;
        }
        else if (meta.Buffers)
        {
            switch (type)
            {
            case MaterialSceneTextures::SceneDepth:
                view = meta.CanSampleDepth
                           ? (GPUDevice::Instance->Limits.HasReadOnlyDepth ? meta.Buffers->DepthBuffer->ViewReadOnlyDepth() : meta.Buffers->DepthBuffer->View())
                           : GPUDevice::Instance->GetDefaultWhiteTexture()->View();
                break;
            case MaterialSceneTextures::AmbientOcclusion:
            case MaterialSceneTextures::BaseColor:
                view = meta.CanSampleGBuffer ? meta.Buffers->GBuffer0->View() : nullptr;
                break;
            case MaterialSceneTextures::WorldNormal:
            case MaterialSceneTextures::ShadingModel:
                view = meta.CanSampleGBuffer ? meta.Buffers->GBuffer1->View() : nullptr;
                break;
            case MaterialSceneTextures::Roughness:
            case MaterialSceneTextures::Metalness:
            case MaterialSceneTextures::Specular:
                view = meta.CanSampleGBuffer ? meta.Buffers->GBuffer2->View() : nullptr;
                break;
            default: ;
            }
        }
        meta.Context->BindSR(_registerIndex, view);
        break;
    }
    case MaterialParameterType::ChannelMask:
        if (meta.Buffer0)
            *((Vector4*)(meta.Buffer0 + _offset)) = Vector4(_asInteger == 0, _asInteger == 1, _asInteger == 2, _asInteger == 3);
        break;
    case MaterialParameterType::GameplayGlobal:
        if (meta.Buffer0 && _asAsset)
        {
            const auto e = _asAsset.As<GameplayGlobals>()->Variables.TryGet(_name);
            if (e)
            {
                switch (e->Value.Type.Type)
                {
                case VariantType::Bool:
                    *((bool*)(meta.Buffer0 + _offset)) = e->Value.AsBool;
                    break;
                case VariantType::Int:
                    *((int32*)(meta.Buffer0 + _offset)) = e->Value.AsInt;
                    break;
                case VariantType::Uint:
                    *((uint32*)(meta.Buffer0 + _offset)) = e->Value.AsUint;
                    break;
                case VariantType::Float:
                    *((float*)(meta.Buffer0 + _offset)) = e->Value.AsFloat;
                    break;
                case VariantType::Vector2:
                    *((Vector2*)(meta.Buffer0 + _offset)) = e->Value.AsVector2();
                    break;
                case VariantType::Vector3:
                    *((Vector3*)(meta.Buffer0 + _offset)) = e->Value.AsVector3();
                    break;
                case VariantType::Vector4:
                case VariantType::Color:
                    *((Vector4*)(meta.Buffer0 + _offset)) = e->Value.AsVector4();
                    break;
                default: ;
                }
            }
        }
        break;
    default:
        break;
    }
}

bool MaterialParameter::HasContentLoaded() const
{
    return _asAsset == nullptr || _asAsset->IsLoaded();
}

void MaterialParameter::clone(const MaterialParameter* param)
{
    // Clone data
    _type = param->_type;
    _isPublic = param->_isPublic;
    _override = param->_override;
    _registerIndex = param->_registerIndex;
    _offset = param->_offset;
    _name = param->_name;
    _paramId = param->_paramId;

    // Clone value
    switch (_type)
    {
    case MaterialParameterType::Bool:
        _asBool = param->_asBool;
        break;
    case MaterialParameterType::Integer:
    case MaterialParameterType::SceneTexture:
        _asInteger = param->_asInteger;
        break;
    case MaterialParameterType::Float:
        _asFloat = param->_asFloat;
        break;
    case MaterialParameterType::Vector2:
        _asVector2 = param->_asVector2;
        break;
    case MaterialParameterType::Vector3:
        _asVector3 = param->_asVector3;
        break;
    case MaterialParameterType::Vector4:
        _asVector4 = param->_asVector4;
        break;
    case MaterialParameterType::Color:
        _asColor = param->_asColor;
        break;
    case MaterialParameterType::Matrix:
        _asMatrix = param->_asMatrix;
        break;
    default:
        break;
    }
    _asAsset = param->_asAsset;
    _asGPUTexture = param->_asGPUTexture;
}

bool MaterialParameter::operator==(const MaterialParameter& other) const
{
    return _paramId == other._paramId;
}

String MaterialParameter::ToString() const
{
    return String::Format(TEXT("\'{0}\' ({1}:{2}:{3})"), _name, ::ToString(_type), _paramId, _isPublic);
}

MaterialParameter* MaterialParams::Get(const Guid& id)
{
    MaterialParameter* result = nullptr;
    for (int32 i = 0; i < Count(); i++)
    {
        if (At(i).GetParameterID() == id)
        {
            result = &At(i);
            break;
        }
    }
    return result;
}

MaterialParameter* MaterialParams::Get(const StringView& name)
{
    MaterialParameter* result = nullptr;
    for (int32 i = 0; i < Count(); i++)
    {
        if (At(i).GetName() == name)
        {
            result = &At(i);
            break;
        }
    }
    return result;
}

int32 MaterialParams::Find(const Guid& id)
{
    int32 result = -1;
    for (int32 i = 0; i < Count(); i++)
    {
        if (At(i).GetParameterID() == id)
        {
            result = i;
            break;
        }
    }
    return result;
}

int32 MaterialParams::Find(const StringView& name)
{
    int32 result = -1;
    for (int32 i = 0; i < Count(); i++)
    {
        if (At(i).GetName() == name)
        {
            result = i;
            break;
        }
    }
    return result;
}

int32 MaterialParams::GetVersionHash() const
{
    return _versionHash;
}

void MaterialParams::Bind(MaterialParamsLink* link, MaterialParameter::BindMeta& meta)
{
    ASSERT(link && link->This);
    for (int32 i = 0; i < link->This->Count(); i++)
    {
        MaterialParamsLink* l = link;
        while (l->Down && !l->This->At(i).IsOverride())
        {
            l = l->Down;
        }

        l->This->At(i).Bind(meta);
    }
}

void MaterialParams::Clone(MaterialParams& result)
{
    ASSERT(this != &result);

    // Clone all parameters
    result.Resize(Count(), false);
    for (int32 i = 0; i < Count(); i++)
    {
        result.At(i).clone(&At(i));
    }

    result._versionHash = _versionHash;
}

void MaterialParams::Dispose()
{
    Resize(0);
    _versionHash = 0;
}

bool MaterialParams::Load(ReadStream* stream)
{
    bool result = false;

    // Release
    Resize(0);

    // Check for not empty params
    if (stream != nullptr && stream->CanRead())
    {
        // Version
        uint16 version;
        stream->ReadUint16(&version);
        switch (version)
        {
        case 1: // [Deprecated on 15.11.2019, expires on 15.11.2021]
        {
            // Size of the collection
            uint16 paramsCount;
            stream->ReadUint16(&paramsCount);
            Resize(paramsCount, false);

            // Read all parameters
            Guid id;
            for (int32 i = 0; i < paramsCount; i++)
            {
                auto param = &At(i);

                // Read properties
                param->_paramId = Guid::New();
                param->_type = static_cast<MaterialParameterType>(stream->ReadByte());
                param->_isPublic = stream->ReadBool();
                param->_override = param->_isPublic;
                stream->ReadString(&param->_name, 10421);
                param->_registerIndex = stream->ReadByte();
                stream->ReadUint16(&param->_offset);

                // Read value
                switch (param->_type)
                {
                case MaterialParameterType::Bool:
                    param->_asBool = stream->ReadBool();
                    break;
                case MaterialParameterType::Integer:
                case MaterialParameterType::SceneTexture:
                case MaterialParameterType::ChannelMask:
                    stream->ReadInt32(&param->_asInteger);
                    break;
                case MaterialParameterType::Float:
                    stream->ReadFloat(&param->_asFloat);
                    break;
                case MaterialParameterType::Vector2:
                    stream->Read(&param->_asVector2);
                    break;
                case MaterialParameterType::Vector3:
                    stream->Read(&param->_asVector3);
                    break;
                case MaterialParameterType::Vector4:
                    stream->Read(&param->_asVector4);
                    break;
                case MaterialParameterType::Color:
                    stream->Read(&param->_asColor);
                    break;
                case MaterialParameterType::Matrix:
                    stream->Read(&param->_asMatrix);
                    break;
                case MaterialParameterType::NormalMap:
                case MaterialParameterType::Texture:
                case MaterialParameterType::CubeTexture:
                    stream->Read(&id);
                    param->_asAsset = Content::LoadAsync<TextureBase>(id);
                    break;
                case MaterialParameterType::GPUTextureVolume:
                case MaterialParameterType::GPUTextureCube:
                case MaterialParameterType::GPUTextureArray:
                case MaterialParameterType::GPUTexture:
                    stream->Read(&id);
                    param->_asGPUTexture = id;
                    break;
                case MaterialParameterType::GameplayGlobal:
                    stream->Read(&id);
                    param->_asAsset = Content::LoadAsync<GameplayGlobals>(id);
                    break;
                default:
                    break;
                }
            }
        }
        break;
        case 2: // [Deprecated on 15.11.2019, expires on 15.11.2021]
        {
            // Size of the collection
            uint16 paramsCount;
            stream->ReadUint16(&paramsCount);
            Resize(paramsCount, false);

            // Read all parameters
            Guid id;
            for (int32 i = 0; i < paramsCount; i++)
            {
                auto param = &At(i);

                // Read properties
                param->_type = static_cast<MaterialParameterType>(stream->ReadByte());
                stream->Read(&param->_paramId);
                param->_isPublic = stream->ReadBool();
                param->_override = param->_isPublic;
                stream->ReadString(&param->_name, 10421);
                param->_registerIndex = stream->ReadByte();
                stream->ReadUint16(&param->_offset);

                // Read value
                switch (param->_type)
                {
                case MaterialParameterType::Bool:
                    param->_asBool = stream->ReadBool();
                    break;
                case MaterialParameterType::Integer:
                case MaterialParameterType::SceneTexture:
                    stream->ReadInt32(&param->_asInteger);
                    break;
                case MaterialParameterType::Float:
                    stream->ReadFloat(&param->_asFloat);
                    break;
                case MaterialParameterType::Vector2:
                    stream->Read(&param->_asVector2);
                    break;
                case MaterialParameterType::Vector3:
                    stream->Read(&param->_asVector3);
                    break;
                case MaterialParameterType::Vector4:
                    stream->Read(&param->_asVector4);
                    break;
                case MaterialParameterType::Color:
                    stream->Read(&param->_asColor);
                    break;
                case MaterialParameterType::Matrix:
                    stream->Read(&param->_asMatrix);
                    break;
                case MaterialParameterType::NormalMap:
                case MaterialParameterType::Texture:
                case MaterialParameterType::CubeTexture:
                    stream->Read(&id);
                    param->_asAsset = Content::LoadAsync<TextureBase>(id);
                    break;
                case MaterialParameterType::GPUTextureVolume:
                case MaterialParameterType::GPUTextureCube:
                case MaterialParameterType::GPUTextureArray:
                case MaterialParameterType::GPUTexture:
                    stream->Read(&id);
                    param->_asGPUTexture = id;
                    break;
                case MaterialParameterType::GameplayGlobal:
                    stream->Read(&id);
                    param->_asAsset = Content::LoadAsync<GameplayGlobals>(id);
                    break;
                default:
                    break;
                }
            }
        }
        break;
        case 3:
        {
            // Size of the collection
            uint16 paramsCount;
            stream->ReadUint16(&paramsCount);
            Resize(paramsCount, false);

            // Read all parameters
            Guid id;
            for (int32 i = 0; i < paramsCount; i++)
            {
                auto param = &At(i);

                // Read properties
                param->_type = static_cast<MaterialParameterType>(stream->ReadByte());
                stream->Read(&param->_paramId);
                param->_isPublic = stream->ReadBool();
                param->_override = stream->ReadBool();
                stream->ReadString(&param->_name, 10421);
                param->_registerIndex = stream->ReadByte();
                stream->ReadUint16(&param->_offset);

                // Read value
                switch (param->_type)
                {
                case MaterialParameterType::Bool:
                    param->_asBool = stream->ReadBool();
                    break;
                case MaterialParameterType::Integer:
                case MaterialParameterType::SceneTexture:
                case MaterialParameterType::ChannelMask:
                    stream->ReadInt32(&param->_asInteger);
                    break;
                case MaterialParameterType::Float:
                    stream->ReadFloat(&param->_asFloat);
                    break;
                case MaterialParameterType::Vector2:
                    stream->Read(&param->_asVector2);
                    break;
                case MaterialParameterType::Vector3:
                    stream->Read(&param->_asVector3);
                    break;
                case MaterialParameterType::Vector4:
                    stream->Read(&param->_asVector4);
                    break;
                case MaterialParameterType::Color:
                    stream->Read(&param->_asColor);
                    break;
                case MaterialParameterType::Matrix:
                    stream->Read(&param->_asMatrix);
                    break;
                case MaterialParameterType::NormalMap:
                case MaterialParameterType::Texture:
                case MaterialParameterType::CubeTexture:
                    stream->Read(&id);
                    param->_asAsset = Content::LoadAsync<TextureBase>(id);
                    break;
                case MaterialParameterType::GPUTextureVolume:
                case MaterialParameterType::GPUTextureCube:
                case MaterialParameterType::GPUTextureArray:
                case MaterialParameterType::GPUTexture:
                    stream->Read(&id);
                    param->_asGPUTexture = id;
                    break;
                case MaterialParameterType::GameplayGlobal:
                    stream->Read(&id);
                    param->_asAsset = Content::LoadAsync<GameplayGlobals>(id);
                    break;
                default:
                    break;
                }
            }
        }
        break;

        default:
            result = true;
            break;
        }
    }

    UpdateHash();

    return result;
}

void MaterialParams::Save(WriteStream* stream)
{
    // Skip serialization if no parameters to save
    if (IsEmpty())
        return;

    // Version
    stream->WriteUint16(3);

    // Size of the collection
    stream->WriteUint16(Count());

    // Write all parameters
    for (int32 i = 0; i < Count(); i++)
    {
        // Cache
        auto param = &At(i);

        // Write properties
        stream->WriteByte(static_cast<byte>(param->_type));
        stream->Write(&param->_paramId);
        stream->WriteBool(param->_isPublic);
        stream->WriteBool(param->_override);
        stream->WriteString(param->_name, 10421);
        stream->WriteByte(param->_registerIndex);
        stream->WriteUint16(param->_offset);

        // Write value
        Guid id;
        switch (param->_type)
        {
        case MaterialParameterType::Bool:
            stream->WriteBool(param->_asBool);
            break;
        case MaterialParameterType::Integer:
        case MaterialParameterType::SceneTexture:
        case MaterialParameterType::ChannelMask:
            stream->WriteInt32(param->_asInteger);
            break;
        case MaterialParameterType::Float:
            stream->WriteFloat(param->_asFloat);
            break;
        case MaterialParameterType::Vector2:
            stream->Write(&param->_asVector2);
            break;
        case MaterialParameterType::Vector3:
            stream->Write(&param->_asVector3);
            break;
        case MaterialParameterType::Vector4:
            stream->Write(&param->_asVector4);
            break;
        case MaterialParameterType::Color:
            stream->Write(&param->_asColor);
            break;
        case MaterialParameterType::Matrix:
            stream->Write(&param->_asMatrix);
            break;
        case MaterialParameterType::NormalMap:
        case MaterialParameterType::Texture:
        case MaterialParameterType::CubeTexture:
        case MaterialParameterType::GameplayGlobal:
            id = param->_asAsset.GetID();
            stream->Write(&id);
            break;
        case MaterialParameterType::GPUTextureVolume:
        case MaterialParameterType::GPUTextureArray:
        case MaterialParameterType::GPUTextureCube:
        case MaterialParameterType::GPUTexture:
            id = param->_asGPUTexture.GetID();
            stream->Write(&id);
            break;
        default:
            break;
        }
    }
}

void MaterialParams::Save(WriteStream* stream, const Array<SerializedMaterialParam>* params)
{
    // Version
    stream->WriteUint16(3);

    // Size of the collection
    stream->WriteUint16(params ? params->Count() : 0);

    // Write all parameters
    if (params)
    {
        for (int32 i = 0; i < params->Count(); i++)
        {
            // Cache
            const SerializedMaterialParam& param = params->At(i);

            // Write properties
            stream->WriteByte(static_cast<byte>(param.Type));
            stream->Write(&param.ID);
            stream->WriteBool(param.IsPublic);
            stream->WriteBool(param.Override);
            stream->WriteString(param.Name, 10421);
            stream->WriteByte(param.RegisterIndex);
            stream->WriteUint16(param.Offset);

            // Write value
            switch (param.Type)
            {
            case MaterialParameterType::Bool:
                stream->WriteBool(param.AsBool);
                break;
            case MaterialParameterType::SceneTexture:
            case MaterialParameterType::ChannelMask:
            case MaterialParameterType::Integer:
                stream->WriteInt32(param.AsInteger);
                break;
            case MaterialParameterType::Float:
                stream->WriteFloat(param.AsFloat);
                break;
            case MaterialParameterType::Vector2:
                stream->Write(&param.AsVector2);
                break;
            case MaterialParameterType::Vector3:
                stream->Write(&param.AsVector3);
                break;
            case MaterialParameterType::Vector4:
                stream->Write(&param.AsVector4);
                break;
            case MaterialParameterType::Color:
                stream->Write(&param.AsColor);
                break;
            case MaterialParameterType::Matrix:
                stream->Write(&param.AsMatrix);
                break;
            case MaterialParameterType::NormalMap:
            case MaterialParameterType::Texture:
            case MaterialParameterType::CubeTexture:
            case MaterialParameterType::GameplayGlobal:
            case MaterialParameterType::GPUTextureVolume:
            case MaterialParameterType::GPUTextureCube:
            case MaterialParameterType::GPUTextureArray:
            case MaterialParameterType::GPUTexture:
                stream->Write(&param.AsGuid);
                break;
            default:
                break;
            }
        }
    }
}

void MaterialParams::Save(BytesContainer& data, const Array<SerializedMaterialParam>* params)
{
    MemoryWriteStream stream(1024);
    Save(&stream, params);
    if (stream.GetPosition() > 0)
        data.Copy(stream.GetHandle(), stream.GetPosition());
    else
        data.Release();
}

#if USE_EDITOR

void MaterialParams::GetReferences(Array<Guid>& output) const
{
    for (int32 i = 0; i < Count(); i++)
    {
        if (At(i)._asAsset)
            output.Add(At(i)._asAsset->GetID());
    }
}

#endif

bool MaterialParams::HasContentLoaded() const
{
    bool result = true;

    for (int32 i = 0; i < Count(); i++)
    {
        if (!At(i).HasContentLoaded())
        {
            result = false;
            break;
        }
    }

    return result;
}

void MaterialParams::UpdateHash()
{
    _versionHash = rand();
}
