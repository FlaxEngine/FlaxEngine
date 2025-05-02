// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Content/Assets/Texture.h"
#include "Engine/Content/AssetReference.h"

class MaterialInstance;
class MaterialParams;
class GPUContext;
class GPUTextureView;
class RenderBuffers;

struct MaterialParamsLink
{
    MaterialParams* This;
    MaterialParamsLink* Up;
    MaterialParamsLink* Down;
};

/// <summary>
/// The material parameter types.
/// </summary>
API_ENUM() enum class MaterialParameterType : byte
{
    /// <summary>
    /// The invalid type.
    /// </summary>
    Invalid = 0,

    /// <summary>
    /// The bool.
    /// </summary>
    Bool = 1,

    /// <summary>
    /// The integer.
    /// </summary>
    Integer = 2,

    /// <summary>
    /// The float.
    /// </summary>
    Float = 3,

    /// <summary>
    /// The vector2
    /// </summary>
    Vector2 = 4,

    /// <summary>
    /// The vector3.
    /// </summary>
    Vector3 = 5,

    /// <summary>
    /// The vector4.
    /// </summary>
    Vector4 = 6,

    /// <summary>
    /// The color.
    /// </summary>
    Color = 7,

    /// <summary>
    /// The texture.
    /// </summary>
    Texture = 8,

    /// <summary>
    /// The cube texture.
    /// </summary>
    CubeTexture = 9,

    /// <summary>
    /// The normal map texture.
    /// </summary>
    NormalMap = 10,

    /// <summary>
    /// The scene texture.
    /// </summary>
    SceneTexture = 11,

    /// <summary>
    /// The GPU texture (created from code).
    /// </summary>
    GPUTexture = 12,

    /// <summary>
    /// The matrix.
    /// </summary>
    Matrix = 13,

    /// <summary>
    /// The GPU texture array (created from code).
    /// </summary>
    GPUTextureArray = 14,

    /// <summary>
    /// The GPU volume texture (created from code).
    /// </summary>
    GPUTextureVolume = 15,

    /// <summary>
    /// The GPU cube texture (created from code).
    /// </summary>
    GPUTextureCube = 16,

    /// <summary>
    /// The RGBA channel selection mask.
    /// </summary>
    ChannelMask = 17,

    /// <summary>
    /// The gameplay global.
    /// </summary>
    GameplayGlobal = 18,

    /// <summary>
    /// The texture sampler derived from texture group settings.
    /// </summary>
    TextureGroupSampler = 19,

    /// <summary>
    /// The Global SDF (textures and constants).
    /// </summary>
    GlobalSDF = 20,
};

/// <summary>
/// Structure of serialized material parameter data.
/// </summary>
struct SerializedMaterialParam
{
    MaterialParameterType Type;
    Guid ID;
    bool IsPublic;
    bool Override;
    String Name;
    String ShaderName;

    union
    {
        bool AsBool;
        int32 AsInteger;
        float AsFloat;
        Float2 AsFloat2;
        Float3 AsFloat3;
        Color AsColor;
        Guid AsGuid;
        byte AsData[16 * 4];
    };

    byte RegisterIndex;
    uint16 Offset;

    SerializedMaterialParam()
    {
    }
};

/// <summary>
/// Material variable object. Allows to modify material parameter value at runtime.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API MaterialParameter : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(MaterialParameter, ScriptingObject);
    friend MaterialParams;
    friend MaterialInstance;
private:
    Guid _paramId;
    MaterialParameterType _type = MaterialParameterType::Invalid;
    bool _isPublic;
    bool _override;
    byte _registerIndex;
    uint16 _offset;

    union
    {
        bool _asBool;
        int32 _asInteger;
        float _asFloat;
        Float2 _asVector2;
        Float3 _asVector3;
        Color _asColor;
        byte AsData[16 * 4];
    };

    AssetReference<Asset> _asAsset;
    ScriptingObjectReference<GPUTexture> _asGPUTexture;
    String _name;

public:
    /// <summary>
    /// Gets the parameter ID (not the parameter instance Id but the original parameter ID).
    /// </summary>
    API_PROPERTY() FORCE_INLINE Guid GetParameterID() const
    {
        return _paramId;
    }

    /// <summary>
    /// Gets the parameter type.
    /// </summary>
    API_PROPERTY() FORCE_INLINE MaterialParameterType GetParameterType() const
    {
        return _type;
    }

    /// <summary>
    /// Gets the parameter name.
    /// </summary>
    API_PROPERTY() FORCE_INLINE const String& GetName() const
    {
        return _name;
    }

    /// <summary>
    /// Returns true is parameter is public visible.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsPublic() const
    {
        return _isPublic;
    }

    /// <summary>
    /// Returns true is parameter is overriding the value.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsOverride() const
    {
        return _override;
    }

    /// <summary>
    /// Sets the value override mode.
    /// </summary>
    API_PROPERTY() void SetIsOverride(bool value)
    {
        _override = value;
    }

    /// <summary>
    /// Gets the parameter resource graphics pipeline binding register index.
    /// </summary>
    FORCE_INLINE byte GetRegister() const
    {
        return _registerIndex;
    }

    /// <summary>
    /// Gets the parameter binding offset since the start of the constant buffer.
    /// </summary>
    FORCE_INLINE uint16 GetBindOffset() const
    {
        return _offset;
    }

public:
    /// <summary>
    /// Gets the value of the parameter.
    /// </summary>
    /// <returns>The value.</returns>
    API_PROPERTY() Variant GetValue() const;

    /// <summary>
    /// Sets the value of the parameter.
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetValue(const Variant& value);

public:
    /// <summary>
    /// The material parameter binding metadata.
    /// </summary>
    struct BindMeta
    {
        /// <summary>
        /// The GPU commands context.
        /// </summary>
        GPUContext* Context;

        /// <summary>
        /// The pointer to the constants buffer in the memory.
        /// </summary>
        Span<byte> Constants;

        /// <summary>
        /// The input scene color. It's optional and used in forward/postFx rendering.
        /// </summary>
        GPUTextureView* Input;

        /// <summary>
        /// The scene buffers. It's optional and used in forward/postFx rendering.
        /// </summary>
        const RenderBuffers* Buffers;

        /// <summary>
        /// True if parameters can sample depth buffer.
        /// </summary>
        bool CanSampleDepth;

        /// <summary>
        /// True if parameters can sample GBuffer.
        /// </summary>
        bool CanSampleGBuffer;
    };

    /// <summary>
    /// Binds the parameter to the pipeline.
    /// </summary>
    /// <param name="meta">The bind meta.</param>
    void Bind(BindMeta& meta) const;

    bool HasContentLoaded() const;

private:
    void clone(const MaterialParameter* param);

public:
    bool operator==(const MaterialParameter& other) const;

    // [Object]
    String ToString() const override;
};

/// <summary>
/// The collection of material parameters.
/// </summary>
class FLAXENGINE_API MaterialParams : public Array<MaterialParameter>
{
    friend MaterialInstance;
private:
    int32 _versionHash = 0;

public:
    MaterialParameter* Get(const Guid& id);
    MaterialParameter* Get(const StringView& name);
    int32 Find(const Guid& id);
    int32 Find(const StringView& name);

public:
    /// <summary>
    /// Gets the parameters version hash. Every time the parameters are modified (loaded, edited, etc.) the hash changes. Can be used to sync instanced parameters collection.
    /// </summary>
    int32 GetVersionHash() const;

public:
    /// <summary>
    /// Binds the parameters to the pipeline.
    /// </summary>
    /// <param name="link">The parameters binding link. Used to support per-parameter override.</param>
    /// <param name="meta">The bind meta.</param>
    static void Bind(MaterialParamsLink* link, MaterialParameter::BindMeta& meta);

    /// <summary>
    /// Clones the parameters list.
    /// </summary>
    /// <param name="result">The result container.</param>
    void Clone(MaterialParams& result);

    /// <summary>
    /// Releases the whole data.
    /// </summary>
    void Dispose();

    /// <summary>
    /// Loads material parameters from the stream.
    /// </summary>
    /// <param name="stream">The stream with data.</param>
    /// <returns>True if cannot load parameters for the file.</returns>
    bool Load(ReadStream* stream);

    /// <summary>
    /// Saves material parameters to the stream.
    /// </summary>
    /// <param name="stream">The stream with data.</param>
    void Save(WriteStream* stream);

    /// <summary>
    /// Saves the material parameters to the stream.
    /// </summary>
    /// <param name="stream">The stream with data.</param>
    /// <param name="params">The array of parameters.</param>
    static void Save(WriteStream* stream, const Array<SerializedMaterialParam>* params);

    /// <summary>
    /// Saves the material parameters to the bytes container.
    /// </summary>
    /// <param name="data">The output data.</param>
    /// <param name="params">The array of parameters.</param>
    static void Save(BytesContainer& data, const Array<SerializedMaterialParam>* params);

public:
#if USE_EDITOR
    /// <summary>
    /// Gets the asset references (see Asset.GetReferences for more info).
    /// </summary>
    /// <param name="assets">The output assets.</param>
    void GetReferences(Array<Guid>& assets) const;
#endif

    bool HasContentLoaded() const;

private:
    void UpdateHash();
};
