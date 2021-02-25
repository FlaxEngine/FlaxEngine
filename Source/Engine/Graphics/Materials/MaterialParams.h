// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Content/Assets/Texture.h"
#include "Engine/Content/AssetReference.h"

class MaterialInstance;
class MaterialParams;
class GPUContext;
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
enum class MaterialParameterType : byte
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
};

const Char* ToString(MaterialParameterType value);

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
        Vector2 AsVector2;
        Vector3 AsVector3;
        Vector4 AsVector4;
        Color AsColor;
        Guid AsGuid;
        Matrix AsMatrix;
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
API_CLASS(NoSpawn) class FLAXENGINE_API MaterialParameter : public PersistentScriptingObject
{
DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(MaterialParameter, PersistentScriptingObject);
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
        Vector2 _asVector2;
        Vector3 _asVector3;
        Vector4 _asVector4;
        Color _asColor;
        Matrix _asMatrix;
    };

    AssetReference<Asset> _asAsset;
    ScriptingObjectReference<GPUTexture> _asGPUTexture;
    String _name;

public:

    MaterialParameter(const MaterialParameter& other)
        : MaterialParameter()
    {
#if !BUILD_RELEASE
        CRASH; // Not used
#endif
    }

    MaterialParameter& operator=(const MaterialParameter& other)
    {
#if !BUILD_RELEASE
        CRASH; // Not used
#endif
        return *this;
    }

public:

    /// <summary>
    /// Gets the parameter ID (not the parameter instance Id but the original parameter ID).
    /// </summary>
    /// <returns>The ID.</returns>
    API_PROPERTY() FORCE_INLINE Guid GetParameterID() const
    {
        return _paramId;
    }

    /// <summary>
    /// Gets the parameter type.
    /// </summary>
    /// <returns>The type.</returns>
    API_PROPERTY() FORCE_INLINE MaterialParameterType GetParameterType() const
    {
        return _type;
    }

    /// <summary>
    /// Gets the parameter name.
    /// </summary>
    /// <returns>The name.</returns>
    API_PROPERTY() FORCE_INLINE const String& GetName() const
    {
        return _name;
    }

    /// <summary>
    /// Returns true is parameter is public visible.
    /// </summary>
    /// <returns>True if parameter has public access, otherwise false.</returns>
    API_PROPERTY() FORCE_INLINE bool IsPublic() const
    {
        return _isPublic;
    }

    /// <summary>
    /// Returns true is parameter is overriding the value.
    /// </summary>
    /// <returns>True if parameter is overriding the value, otherwise false.</returns>
    API_PROPERTY() FORCE_INLINE bool IsOverride() const
    {
        return _override;
    }

    /// <summary>
    /// Sets the value override mode.
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() void SetIsOverride(bool value)
    {
        _override = value;
    }

    /// <summary>
    /// Gets the parameter resource graphics pipeline binding register index.
    /// </summary>
    /// <returns>The binding register.</returns>
    FORCE_INLINE byte GetRegister() const
    {
        return _registerIndex;
    }

    /// <summary>
    /// Gets the parameter binding offset since the start of the constant buffer.
    /// </summary>
    /// <returns>The binding data offset (in bytes).</returns>
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
        /// The pointer to the first constants buffer in memory.
        /// </summary>
        byte* Buffer0;

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
    /// <param name="output">The output.</param>
    void GetReferences(Array<Guid>& output) const;

#endif

    bool HasContentLoaded() const;

private:

    void UpdateHash();
};
