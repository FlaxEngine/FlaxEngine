// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_SHADER_COMPILER

#include "ShaderCompiler.h"
#include "ShadersCompilation.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/Shaders/VertexElement.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "FlaxEngine.Gen.h"

namespace IncludedFiles
{
    struct File
    {
        String Path;
        DateTime LastEditTime;
        StringAnsi Source;
    };

    CriticalSection Locker;
    Dictionary<String, File*> Files;
}

bool ShaderCompiler::Compile(ShaderCompilationContext* context)
{
    // Clear cache
    _globalMacros.Clear();
    _macros.Clear();
    _constantBuffers.Clear();
    _globalMacros.EnsureCapacity(32);
    _macros.EnsureCapacity(32);
    _context = context;

    // Prepare
    auto output = context->Output;
    auto meta = context->Meta;
    const int32 shadersCount = meta->GetShadersCount();
    if (OnCompileBegin())
        return true;
    _globalMacros.Add({ nullptr, nullptr });

    // Setup constant buffers cache
    _constantBuffers.EnsureCapacity(meta->CB.Count(), false);
    for (int32 i = 0; i < meta->CB.Count(); i++)
        _constantBuffers.Add({ meta->CB[i].Slot, false, 0 });

    // [Output] Version number
    output->WriteInt32(GPU_SHADER_CACHE_VERSION);

    // [Output] Additional data start
    const int32 additionalDataStartPos = output->GetPosition();
    output->WriteInt32(-1);

    // [Output] Amount of shaders
    output->WriteInt32(shadersCount);

    // Compile all shaders
    if (CompileShaders())
        return true;

    // [Output] Constant Buffers
    output->Write((byte)_constantBuffers.Count());
    for (const ShaderResourceBuffer& cb : _constantBuffers)
    {
        output->Write((byte)cb.Slot);
        output->Write((uint32)cb.Size);
    }

    // Additional Data Start
    *(int32*)(output->GetHandle() + additionalDataStartPos) = output->GetPosition();

    // [Output] Includes
    output->WriteInt32(context->Includes.Count());
    for (auto& include : context->Includes)
    {
        String compactPath = ShadersCompilation::CompactShaderPath(include.Item);
        output->Write(compactPath, 11);
        const auto date = FileSystem::GetFileLastEditTime(include.Item);
        output->Write(date);
    }

    return false;
}

bool ShaderCompiler::GetIncludedFileSource(ShaderCompilationContext* context, const char* sourceFile, const char* includedFile, const char*& source, int32& sourceLength)
{
    PROFILE_CPU();
    source = nullptr;
    sourceLength = 0;

    // Get actual file path
    const String includedFileName(includedFile);
    String path = ShadersCompilation::ResolveShaderPath(includedFileName);
    if (!FileSystem::FileExists(path))
    {
        LOG(Error, "Unknown shader source file '{0}' included in '{1}'.{2}", includedFileName, String(sourceFile), String::Empty);
        return true;
    }

    ScopeLock lock(IncludedFiles::Locker);

    // Try to reuse file
    IncludedFiles::File* result = nullptr;
    if (!IncludedFiles::Files.TryGet(path, result) || FileSystem::GetFileLastEditTime(path) > result->LastEditTime)
    {
        // Remove old one
        if (result)
        {
            Delete(result);
            IncludedFiles::Files.Remove(path);
        }

        // Load file
        result = New<IncludedFiles::File>();
        result->Path = path;
        result->LastEditTime = FileSystem::GetFileLastEditTime(path);
        if (File::ReadAllText(result->Path, result->Source))
        {
            LOG(Error, "Failed to load shader source file '{0}' included in '{1}' (path: '{2}')", String(includedFile), String(sourceFile), path);
            Delete(result);
            return true;
        }
        IncludedFiles::Files.Add(path, result);
    }

    context->Includes.Add(path);

    // Copy to output
    source = result->Source.Get();
    sourceLength = result->Source.Length();
    return false;
}

void ShaderCompiler::DisposeIncludedFilesCache()
{
    ScopeLock lock(IncludedFiles::Locker);

    IncludedFiles::Files.ClearDelete();
}

bool ShaderCompiler::CompileShaders()
{
    auto meta = _context->Meta;
#if BUILD_DEBUG
#define PROFILE_COMPILE_SHADER(s) ZoneTransientN(___tracy_scoped_zone, s.Name.Get(), true);
#else
#define PROFILE_COMPILE_SHADER(s)
#endif

    // Generate vertex shaders cache
    for (int32 i = 0; i < meta->VS.Count(); i++)
    {
        auto& shader = meta->VS[i];
        ASSERT(shader.GetStage() == ShaderStage::Vertex && (shader.Flags & ShaderFlags::Hidden) == (ShaderFlags)0);
        PROFILE_COMPILE_SHADER(shader);
        if (CompileShader(shader, &WriteCustomDataVS))
        {
            LOG(Error, "Failed to compile \'{0}\'", String(shader.Name));
            return true;
        }
    }

    // Generate hull shaders cache
    for (int32 i = 0; i < meta->HS.Count(); i++)
    {
        auto& shader = meta->HS[i];
        ASSERT(shader.GetStage() == ShaderStage::Hull && (shader.Flags & ShaderFlags::Hidden) == (ShaderFlags)0);
        PROFILE_COMPILE_SHADER(shader);
        if (CompileShader(shader, &WriteCustomDataHS))
        {
            LOG(Error, "Failed to compile \'{0}\'", String(shader.Name));
            return true;
        }
    }

    // Generate domain shaders cache
    for (int32 i = 0; i < meta->DS.Count(); i++)
    {
        auto& shader = meta->DS[i];
        ASSERT(shader.GetStage() == ShaderStage::Domain && (shader.Flags & ShaderFlags::Hidden) == (ShaderFlags)0);
        PROFILE_COMPILE_SHADER(shader);
        if (CompileShader(shader))
        {
            LOG(Error, "Failed to compile \'{0}\'", String(shader.Name));
            return true;
        }
    }

    // Generate geometry shaders cache
    for (int32 i = 0; i < meta->GS.Count(); i++)
    {
        auto& shader = meta->GS[i];
        ASSERT(shader.GetStage() == ShaderStage::Geometry && (shader.Flags & ShaderFlags::Hidden) == (ShaderFlags)0);
        PROFILE_COMPILE_SHADER(shader);
        if (CompileShader(shader))
        {
            LOG(Error, "Failed to compile \'{0}\'", String(shader.Name));
            return true;
        }
    }

    // Generate pixel shaders cache
    for (int32 i = 0; i < meta->PS.Count(); i++)
    {
        auto& shader = meta->PS[i];
        ASSERT(shader.GetStage() == ShaderStage::Pixel && (shader.Flags & ShaderFlags::Hidden) == (ShaderFlags)0);
        PROFILE_COMPILE_SHADER(shader);
        if (CompileShader(shader))
        {
            LOG(Error, "Failed to compile \'{0}\'", String(shader.Name));
            return true;
        }
    }

    // Generate compute shaders cache
    for (int32 i = 0; i < meta->CS.Count(); i++)
    {
        auto& shader = meta->CS[i];
        ASSERT(shader.GetStage() == ShaderStage::Compute && (shader.Flags & ShaderFlags::Hidden) == (ShaderFlags)0);
        PROFILE_COMPILE_SHADER(shader);
        if (CompileShader(shader))
        {
            LOG(Error, "Failed to compile \'{0}\'", String(shader.Name));
            return true;
        }
    }

#undef PROFILE_COMPILE_SHADER
    return false;
}

bool ShaderCompiler::OnCompileBegin()
{
    // Setup global macros
    static const char* Numbers[] =
    {
        // @formatter:off
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10",
        // @formatter:on
    };
    const auto profile = GetProfile();
    const auto featureLevel = RenderTools::GetFeatureLevel(profile);
    _globalMacros.Add({ "FEATURE_LEVEL", Numbers[(int32)featureLevel] });

    return false;
}

bool ShaderCompiler::OnCompileEnd()
{
    return false;
}

bool ShaderCompiler::WriteShaderFunctionBegin(ShaderCompilationContext* context, ShaderFunctionMeta& meta)
{
    auto output = context->Output;
    output->Write((byte)meta.GetStage());
    output->Write((byte)meta.Permutations.Count());
    output->Write(meta.Name, 11);
    output->Write((uint32)meta.Flags);
    return false;
}

bool ShaderCompiler::WriteShaderFunctionPermutation(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const ShaderBindings& bindings, const void* header, int32 headerSize, const void* cache, int32 cacheSize)
{
    auto output = context->Output;
    output->Write((uint32)(cacheSize + headerSize));
    output->WriteBytes(header, headerSize);
    output->WriteBytes(cache, cacheSize);
    output->Write(bindings);
    return false;
}

bool ShaderCompiler::WriteShaderFunctionPermutation(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const ShaderBindings& bindings, const void* cache, int32 cacheSize)
{
    auto output = context->Output;
    output->Write((uint32)cacheSize);
    output->WriteBytes(cache, cacheSize);
    output->Write(bindings);
    return false;
}

bool ShaderCompiler::WriteShaderFunctionEnd(ShaderCompilationContext* context, ShaderFunctionMeta& meta)
{
    return false;
}

bool ShaderCompiler::WriteCustomDataVS(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const Array<ShaderMacro>& macros, void* additionalData)
{
    auto output = context->Output;

    // Write vertex shader inputs (based on compiled shader reflection) to bind any missing vertex buffer streaming at runtime (during drawing - see usage of GPUVertexLayout::Merge)
    if (auto* additionalDataVS = (AdditionalDataVS*)additionalData)
        output->Write(additionalDataVS->Inputs);
    else
        output->WriteInt32(0);

    // [Deprecated in v1.10]
    auto& metaVS = *(VertexShaderMeta*)&meta;
    auto& layout = metaVS.InputLayout;
#if FLAXENGINE_VERSION_MAJOR > 2 || (FLAXENGINE_VERSION_MAJOR == 2 && FLAXENGINE_VERSION_MINOR >= 1)
    if (layout.HasItems())
        LOG(Warning, "Vertex Shader '{}' (asset '{}') uses explicit vertex layout via 'META_VS_IN_ELEMENT' macros which has been deprecated. Remove this code and migrate to GPUVertexLayout with VertexElement array in code (assigned to vertex buffer).", String(meta.Name), context->Options->TargetName);
#endif

    // Get visible entries (based on `visible` flag switch)
    int32 layoutSize = 0;
    bool layoutVisible[GPU_MAX_VS_ELEMENTS];
    for (int32 i = 0; i < layout.Count(); i++)
    {
        auto& element = layout[i];
        layoutVisible[i] = false;

        // Parse using all input macros
        StringAnsi value = element.VisibleFlag;
        for (int32 j = 0; j < macros.Count() - 1; j++)
        {
            if (macros[j].Name == value)
            {
                value = macros[j].Definition;
                break;
            }
        }

        if (value == "true" || value == "1")
        {
            // Visible
            layoutSize++;
            layoutVisible[i] = true;
        }
        else if (value == "false" || value == "0")
        {
            // Hidden
        }
        else
        {
            LOG(Error, "Invalid option value \'{1}\' for  layout element \'visible\' flag on vertex shader \'{0}\'.", String(metaVS.Name), String(value));
            return true;
        }
    }

    // [Output] Input Layout
    output->WriteByte(static_cast<byte>(layoutSize));
    for (int32 a = 0; a < layout.Count(); a++)
    {
        auto& element = layout[a];
        if (!layoutVisible[a])
            continue;
        VertexElement data;
        switch (element.Type)
        {
        case VertexShaderMeta::InputType::POSITION:
            data.Type = VertexElement::Types::Position;
            break;
        case VertexShaderMeta::InputType::COLOR:
            data.Type = VertexElement::Types::Color;
            break;
        case VertexShaderMeta::InputType::TEXCOORD:
            switch (element.Index)
            {
            case 0:
                data.Type = VertexElement::Types::TexCoord0;
                break;
            case 1:
                data.Type = VertexElement::Types::TexCoord1;
                break;
            case 2:
                data.Type = VertexElement::Types::TexCoord2;
                break;
            case 3:
                data.Type = VertexElement::Types::TexCoord3;
                break;
            case 4:
                data.Type = VertexElement::Types::TexCoord4;
                break;
            case 5:
                data.Type = VertexElement::Types::TexCoord5;
                break;
            case 6:
                data.Type = VertexElement::Types::TexCoord6;
                break;
            case 7:
                data.Type = VertexElement::Types::TexCoord7;
                break;
            default:
                LOG(Error, "Vertex Shader '{}' (asset '{}') uses deprecated texcoord attribute index. Valid range is 0-7.", String(meta.Name), context->Options->TargetName);
                data.Type = VertexElement::Types::TexCoord;
                break;
            }
            break;
        case VertexShaderMeta::InputType::NORMAL:
            data.Type = VertexElement::Types::Normal;
            break;
        case VertexShaderMeta::InputType::TANGENT:
            data.Type = VertexElement::Types::Tangent;
            break;
        case VertexShaderMeta::InputType::BITANGENT:
            LOG(Error, "Vertex Shader '{}' (asset '{}') uses deprecated attribute 'BITANGENT'. Remapping it to `ATTRIBUTE`.", String(meta.Name), context->Options->TargetName);
            data.Type = VertexElement::Types::Attribute;
            break;
        case VertexShaderMeta::InputType::ATTRIBUTE:
            switch (element.Index)
            {
            case 0:
                data.Type = VertexElement::Types::Attribute0;
                break;
            case 1:
                data.Type = VertexElement::Types::Attribute1;
                break;
            case 2:
                data.Type = VertexElement::Types::Attribute2;
                break;
            case 3:
                data.Type = VertexElement::Types::Attribute3;
                break;
            default:
                LOG(Error, "Vertex Shader '{}' (asset '{}') uses deprecated attribute index. Valid range is 0-3.", String(meta.Name), context->Options->TargetName);
                data.Type = VertexElement::Types::Attribute;
                break;
            }
            break;
        case VertexShaderMeta::InputType::BLENDINDICES:
            data.Type = VertexElement::Types::BlendIndices;
            break;
        case VertexShaderMeta::InputType::BLENDWEIGHT:
            data.Type = VertexElement::Types::BlendWeights;
            break;
        default:
            data.Type = VertexElement::Types::Unknown;
            break;
        }
        data.Slot = element.InputSlot;
        data.Offset = element.AlignedByteOffset != INPUT_LAYOUT_ELEMENT_ALIGN && element.AlignedByteOffset <= MAX_uint8 ? element.AlignedByteOffset : 0;
        data.PerInstance = element.InputSlotClass == INPUT_LAYOUT_ELEMENT_PER_INSTANCE_DATA;
        data.Format = element.Format;
        output->Write(data);
    }

    return false;
}

bool ShaderCompiler::WriteCustomDataHS(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const Array<ShaderMacro>& macros, void* additionalData)
{
    auto output = context->Output;
    auto& metaHS = *(HullShaderMeta*)&meta;

    // [Output] Control Points Count
    output->WriteInt32(metaHS.ControlPointsCount);

    return false;
}

void ShaderCompiler::GetDefineForFunction(ShaderFunctionMeta& meta, Array<ShaderMacro>& macros)
{
    auto& functionName = meta.Name;
    const int32 functionNameLength = functionName.Length();
    _funcNameDefineBuffer.Clear();
    _funcNameDefineBuffer.EnsureCapacity(functionNameLength + 2);
    _funcNameDefineBuffer.Add('_');
    _funcNameDefineBuffer.Add(functionName.Get(), functionNameLength);
    _funcNameDefineBuffer.Add('\0');
    macros.Add({ _funcNameDefineBuffer.Get(), "1" });
}

VertexElement::Types ShaderCompiler::ParseVertexElementType(StringAnsiView semantic, uint32 index)
{
    if (semantic.HasChars() && StringUtils::IsDigit(semantic[semantic.Length() - 1]))
    {
        // Get index from end of the name
        index = semantic[semantic.Length() - 1] - '0';
        semantic = StringAnsiView(semantic.Get(), semantic.Length() - 1);
    }

    if (semantic == "POSITION")
        return VertexElement::Types::Position;
    if (semantic == "COLOR")
        return VertexElement::Types::Color;
    if (semantic == "NORMAL")
        return VertexElement::Types::Normal;
    if (semantic == "TANGENT")
        return VertexElement::Types::Tangent;
    if (semantic == "BLENDINDICES")
        return VertexElement::Types::BlendIndices;
    if (semantic == "LIGHTMAP")
        return VertexElement::Types::Lightmap;
    if (semantic == "BLENDWEIGHTS" ||
        semantic == "BLENDWEIGHT") // [Deprecated in v1.10]
        return VertexElement::Types::BlendWeights;
    if (semantic == "TEXCOORD" && index < 8)
        return (VertexElement::Types)((int32)VertexElement::Types::TexCoord0 + index);
    if (semantic == "ATTRIBUTE" && index < 4)
        return (VertexElement::Types)((int32)VertexElement::Types::Attribute0 + index);

    LOG(Warning, "Unsupported vertex shader input element semantic {}{}", semantic.ToString(), index);
    return VertexElement::Types::Unknown;;
}

#endif
