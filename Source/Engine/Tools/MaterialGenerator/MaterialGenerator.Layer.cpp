// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MATERIAL_GRAPH

#include "MaterialGenerator.h"
#include "Engine/Content/Assets/MaterialInstance.h"
#include "Engine/Content/Assets/Material.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Engine/Engine.h"

MaterialGenerator::MaterialGraphBoxesMapping MaterialGenerator::MaterialGraphBoxesMappings[] =
{
    { 0, nullptr, MaterialTreeType::PixelShader, MaterialValue::Zero },
    { 1, TEXT("Color"), MaterialTreeType::PixelShader, MaterialValue::InitForZero(VariantType::Vector3) },
    { 2, TEXT("Mask"), MaterialTreeType::PixelShader, MaterialValue::InitForOne(VariantType::Float) },
    { 3, TEXT("Emissive"), MaterialTreeType::PixelShader, MaterialValue::InitForZero(VariantType::Vector3) },
    { 4, TEXT("Metalness"), MaterialTreeType::PixelShader, MaterialValue::InitForZero(VariantType::Float) },
    { 5, TEXT("Specular"), MaterialTreeType::PixelShader, MaterialValue::InitForHalf(VariantType::Float) },
    { 6, TEXT("Roughness"), MaterialTreeType::PixelShader, MaterialValue::InitForHalf(VariantType::Float) },
    { 7, TEXT("AO"), MaterialTreeType::PixelShader, MaterialValue::InitForOne(VariantType::Float) },
    { 8, TEXT("TangentNormal"), MaterialTreeType::PixelShader, MaterialValue(VariantType::Vector3, TEXT("float3(0, 0, 1.0)")) },
    { 9, TEXT("Opacity"), MaterialTreeType::PixelShader, MaterialValue::InitForOne(VariantType::Float) },
    { 10, TEXT("Refraction"), MaterialTreeType::PixelShader, MaterialValue::InitForOne(VariantType::Float) },
    { 11, TEXT("PositionOffset"), MaterialTreeType::VertexShader, MaterialValue::InitForZero(VariantType::Vector3) },
    { 12, TEXT("TessellationMultiplier"), MaterialTreeType::VertexShader, MaterialValue(VariantType::Float, TEXT("4.0f")) },
    { 13, TEXT("WorldDisplacement"), MaterialTreeType::DomainShader, MaterialValue::InitForZero(VariantType::Vector3) },
    { 14, TEXT("SubsurfaceColor"), MaterialTreeType::PixelShader, MaterialValue::InitForZero(VariantType::Vector3) },
};

const MaterialGenerator::MaterialGraphBoxesMapping& MaterialGenerator::GetMaterialRootNodeBox(MaterialGraphBoxes box)
{
    return MaterialGraphBoxesMappings[static_cast<int32>(box)];
}

void MaterialGenerator::AddLayer(MaterialLayer* layer)
{
    _layers.Add(layer);
}

MaterialLayer* MaterialGenerator::GetLayer(const Guid& id, Node* caller)
{
    // Find layer first
    for (int32 i = 0; i < _layers.Count(); i++)
    {
        if (_layers[i]->ID == id)
        {
            // Found
            return _layers[i];
        }
    }

    // Load asset
    Asset* asset = Assets.LoadAsync<MaterialBase>(id);
    if (asset == nullptr || asset->WaitForLoaded(30000))
    {
        OnError(caller, nullptr, TEXT("Failed to load material asset."));
        return nullptr;
    }

    // Special case for engine exit event
    if (Engine::ShouldExit())
    {
        // End
        return nullptr;
    }

    // Check if load failed
    if (!asset->IsLoaded())
    {
        OnError(caller, nullptr, TEXT("Failed to load material asset."));
        return nullptr;
    }

    // Check if it's a material instance
    Material* material = nullptr;
    Asset* iterator = asset;
    while (material == nullptr)
    {
        // Wait for material to be loaded
        if (iterator->WaitForLoaded())
        {
            OnError(caller, nullptr, TEXT("Material asset load failed."));
            return nullptr;
        }

        if (iterator->GetTypeName() == MaterialInstance::TypeName)
        {
            auto instance = ((MaterialInstance*)iterator);

            // Check if base material has been assigned
            if (instance->GetBaseMaterial() == nullptr)
            {
                OnError(caller, nullptr, TEXT("Material instance has missing base material."));
                return nullptr;
            }

            iterator = instance->GetBaseMaterial();
        }
        else
        {
            material = ((Material*)iterator);

            // Check if instanced material is not instancing current material
            ASSERT(GetRootLayer());
            if (material->GetID() == GetRootLayer()->ID)
            {
                OnError(caller, nullptr, TEXT("Cannot use instance of the current material as layer."));
                return nullptr;
            }
        }
    }
    ASSERT(material);

    // Get surface data
    BytesContainer surfaceData = material->LoadSurface(true);
    if (surfaceData.IsInvalid())
    {
        OnError(caller, nullptr, TEXT("Cannot load surface data."));
        return nullptr;
    }
    MemoryReadStream surfaceDataStream(surfaceData.Get(), surfaceData.Length());

    // Load layer
    auto layer = MaterialLayer::Load(id, &surfaceDataStream, material->GetInfo(), material->ToString());

    // Validate layer info
    if (layer == nullptr)
    {
        OnError(caller, nullptr, TEXT("Cannot load layer."));
        return nullptr;
    }
#if 0 // allow mixing material domains because material generator uses the base layer for features usage checks and sub layers produce only Material structure for blending or extracting
	if (layer->Domain != GetRootLayer()->Domain) // TODO: maybe allow solid on transparent?
	{
		OnError(caller, nullptr, TEXT("Cannot mix materials with different Domains."));
		return nullptr;
	}
#endif

    // Override parameters values if using material instance
    if (asset->GetTypeName() == MaterialInstance::TypeName)
    {
        auto instance = ((MaterialInstance*)asset);
        auto instanceParams = &instance->Params;

        // Clone overriden parameters values
        for (auto& param : layer->Graph.Parameters)
        {
            const auto instanceParam = instanceParams->Get(param.Identifier);
            if (instanceParam && instanceParam->IsOverride())
            {
                param.Value = instanceParam->GetValue();

                // Fold object references to their ids
                if (param.Value.Type == VariantType::Object)
                    param.Value = param.Value.AsObject ? param.Value.AsObject->GetID() : Guid::Empty;
                else if (param.Value.Type == VariantType::Asset)
                    param.Value = param.Value.AsObject ? param.Value.AsObject->GetID() : Guid::Empty;
            }
        }
    }

    // Prepare layer
    layer->Prepare();
    const bool allowPublicParameters = true;
    prepareLayer(layer, allowPublicParameters);

    AddLayer(layer);
    return layer;
}

MaterialLayer* MaterialGenerator::GetRootLayer() const
{
    return _layers.Count() > 0 ? _layers[0] : nullptr;
}

void MaterialGenerator::prepareLayer(MaterialLayer* layer, bool allowVisibleParams)
{
    LayerParamMapping m;

    // Ensure that layer hasn't been used
    ASSERT(layer->HasAnyVariableName() == false);

    // Add all parameters to be saved in the result material parameters collection (perform merge)
    bool isRooLayer = GetRootLayer() == layer;
    for (int32 j = 0; j < layer->Graph.Parameters.Count(); j++)
    {
        const auto param = &layer->Graph.Parameters[j];

        // For all not root layers (sub-layers) we won't to change theirs ID in order to prevent duplicated ID)
        m.SrcId = param->Identifier;
        if (isRooLayer)
        {
            // Use the same ID (so we can edit it)
            m.DstId = param->Identifier;
        }
        else
        {
            // Generate new ID
            m.DstId = param->Identifier;
            m.DstId.A += _parameters.Count() * 17 + 13;
        }
        layer->ParamIdsMappings.Add(m);

        SerializedMaterialParam& mp = _parameters.AddOne();
        mp.ID = m.DstId;
        mp.IsPublic = param->IsPublic && allowVisibleParams;
        mp.Override = true;
        mp.Name = param->Name;
        mp.ShaderName = getParamName(_parameters.Count());
        mp.Type = MaterialParameterType::Bool;
        mp.AsBool = false;
        switch (param->Type.Type)
        {
        case VariantType::Bool:
            mp.Type = MaterialParameterType::Bool;
            mp.AsBool = param->Value.AsBool;
            break;
        case VariantType::Int:
            mp.Type = MaterialParameterType::Integer;
            mp.AsInteger = param->Value.AsInt;
            break;
        case VariantType::Int64:
            mp.Type = MaterialParameterType::Integer;
            mp.AsInteger = (int32)param->Value.AsInt64;
            break;
        case VariantType::Uint:
            mp.Type = MaterialParameterType::Integer;
            mp.AsInteger = (int32)param->Value.AsUint;
            break;
        case VariantType::Uint64:
            mp.Type = MaterialParameterType::Integer;
            mp.AsInteger = (int32)param->Value.AsUint64;
            break;
        case VariantType::Float:
            mp.Type = MaterialParameterType::Float;
            mp.AsFloat = param->Value.AsFloat;
            break;
        case VariantType::Double:
            mp.Type = MaterialParameterType::Float;
            mp.AsFloat = (float)param->Value.AsDouble;
            break;
        case VariantType::Vector2:
            mp.Type = MaterialParameterType::Vector2;
            mp.AsVector2 = param->Value.AsVector2();
            break;
        case VariantType::Vector3:
            mp.Type = MaterialParameterType::Vector3;
            mp.AsVector3 = param->Value.AsVector3();
            break;
        case VariantType::Vector4:
            mp.Type = MaterialParameterType::Vector4;
            mp.AsVector4 = param->Value.AsVector4();
            break;
        case VariantType::Color:
            mp.Type = MaterialParameterType::Color;
            mp.AsColor = param->Value.AsColor();
            break;
        case VariantType::Matrix:
            mp.Type = MaterialParameterType::Matrix;
            mp.AsMatrix = *(Matrix*)param->Value.AsBlob.Data;
            break;
        case VariantType::Asset:
            if (!param->Type.TypeName)
            {
                OnError(nullptr, nullptr, String::Format(TEXT("Invalid or unsupported material parameter type {0}."), param->Type));
                break;
            }
            if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.Texture") == 0)
            {
                mp.Type = MaterialParameterType::Texture;

                // Special case for Normal Maps
                auto asset = (Texture*)::LoadAsset((Guid)param->Value, Texture::TypeInitializer);
                if (asset && !asset->WaitForLoaded() && asset->IsNormalMap())
                    mp.Type = MaterialParameterType::NormalMap;
            }
            else if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.CubeTexture") == 0)
                mp.Type = MaterialParameterType::CubeTexture;
            else
                OnError(nullptr, nullptr, String::Format(TEXT("Invalid or unsupported material parameter type {0}."), param->Type));
            mp.AsGuid = (Guid)param->Value;
            break;
        case VariantType::Object:
            if (!param->Type.TypeName)
            {
                OnError(nullptr, nullptr, String::Format(TEXT("Invalid or unsupported material parameter type {0}."), param->Type));
                break;
            }
            if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.GPUTexture") == 0)
                mp.Type = MaterialParameterType::GPUTexture;
            else
                OnError(nullptr, nullptr, String::Format(TEXT("Invalid or unsupported material parameter type {0}."), param->Type));
            mp.AsGuid = (Guid)param->Value;
            break;
        case VariantType::Enum:
            if (!param->Type.TypeName)
            {
                OnError(nullptr, nullptr, String::Format(TEXT("Invalid or unsupported material parameter type {0}."), param->Type));
                break;
            }
            if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.MaterialSceneTextures") == 0)
                mp.Type = MaterialParameterType::SceneTexture;
            else if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.ChannelMask") == 0)
                mp.Type = MaterialParameterType::ChannelMask;
            else
                OnError(nullptr, nullptr, String::Format(TEXT("Invalid or unsupported material parameter type {0}."), param->Type));
            mp.AsInteger = (int32)param->Value.AsUint64;
            break;
        default:
            OnError(nullptr, nullptr, String::Format(TEXT("Invalid or unsupported material parameter type {0}."), param->Type));
            break;
        }
    }
}

#endif
