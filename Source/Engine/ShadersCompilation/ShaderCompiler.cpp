// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_SHADER_COMPILER

#include "ShaderCompiler.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Utilities/StringConverter.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#endif

namespace IncludedFiles
{
    struct File
    {
        String Path;
        DateTime LastEditTime;
        Array<byte> Source;
    };

    CriticalSection Locker;
    Dictionary<String, File*> Files;

#if USE_EDITOR
    bool FindProject(const ProjectInfo* project, HashSet<const ProjectInfo*>& projects, const StringView& projectName, String& path)
    {
        if (!project || projects.Contains(project))
            return false;
        projects.Add(project);

        // Check the project name
        if (project->Name == projectName)
        {
            path = project->ProjectFolderPath;
            return true;
        }

        // Initialize referenced projects
        for (const auto& reference : project->References)
        {
            if (reference.Project && FindProject(reference.Project, projects, projectName, path))
                return true;
        }

        return false;
    }
#endif
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
    output->WriteInt32(7);

    // [Output] Additional data start
    const int32 additionalDataStartPos = output->GetPosition();
    output->WriteInt32(-1);

    // [Output] Amount of shaders
    output->WriteInt32(shadersCount);

    // Compile all shaders
    if (CompileShaders())
        return true;

    // [Output] Constant Buffers
    {
        const int32 cbsCount = _constantBuffers.Count();
        ASSERT(cbsCount == meta->CB.Count());

        // Find maximum used slot index
        byte maxCbSlot = 0;
        for (int32 i = 0; i < cbsCount; i++)
        {
            maxCbSlot = Math::Max(maxCbSlot, _constantBuffers[i].Slot);
        }

        output->WriteByte(static_cast<byte>(cbsCount));
        output->WriteByte(maxCbSlot);
        // TODO: do we still need to serialize max cb slot?

        for (int32 i = 0; i < cbsCount; i++)
        {
            output->WriteByte(_constantBuffers[i].Slot);
            output->WriteUint32(_constantBuffers[i].Size);
        }
    }

    // Additional Data Start
    *(int32*)(output->GetHandle() + additionalDataStartPos) = output->GetPosition();

    // [Output] Includes
    output->WriteInt32(context->Includes.Count());
    for (auto& include : context->Includes)
    {
        output->WriteString(include.Item, 11);
        const auto date = FileSystem::GetFileLastEditTime(include.Item);
        output->Write(&date);
    }

    return false;
}

bool ShaderCompiler::GetIncludedFileSource(ShaderCompilationContext* context, const char* sourceFile, const char* includedFile, const char*& source, int32& sourceLength)
{
    PROFILE_CPU();
    source = nullptr;
    sourceLength = 0;

    // Skip to the last root start './' but preserve the leading one
    const int32 includedFileLength = StringUtils::Length(includedFile);
    for (int32 i = includedFileLength - 2; i >= 2; i--)
    {
        if (StringUtils::Compare(includedFile + i, "./", 2) == 0)
        {
            includedFile = includedFile + i;
            break;
        }
    }

    ScopeLock lock(IncludedFiles::Locker);

    // Find the included file path
    String path;
#if USE_EDITOR
    if (StringUtils::Compare(includedFile, "./", 2) == 0)
    {
        int32 projectNameEnd = -1;
        for (int32 i = 2; i < includedFileLength; i++)
        {
            if (includedFile[i] == '/')
            {
                projectNameEnd = i;
                break;
            }
        }
        if (projectNameEnd == -1)
        {
            LOG(Error, "Unknown shader source file '{0}' included in '{1}'.{2}", String(includedFile), String(sourceFile), TEXT("Missing project name after root path."));
            return true;
        }
        const StringAsUTF16<120> projectName(includedFile + 2, projectNameEnd - 2);
        if (StringUtils::Compare(projectName.Get(), TEXT("FlaxPlatforms")) == 0)
        {
            // Hard-coded redirect to platform-specific includes
            path /= Globals::StartupFolder / TEXT("Source/Platforms");
        }
        else
        {
            HashSet<const ProjectInfo*> projects;
            if (!IncludedFiles::FindProject(Editor::Project, projects, StringView(projectName.Get(), projectNameEnd - 2), path))
            {
                LOG(Error, "Unknown shader source file '{0}' included in '{1}'.{2}", String(includedFile), String(sourceFile), TEXT("Failed to find the project of the given name."));
                return true;
            }
            path /= TEXT("Source/Shaders/");
        }
        const StringAsUTF16<250> localPath(includedFile + projectNameEnd + 1, includedFileLength - projectNameEnd - 1);
        path /= localPath.Get();
    }
#else
    if (StringUtils::Compare(includedFile, "./Flax/", 7) == 0)
    {
        // Engine project relative shader path
        const auto includedFileStr = String(includedFile + 6);
        path = Globals::StartupFolder / TEXT("Source/Shaders") / includedFileStr;
    }
#endif
    else if (FileSystem::FileExists(path = String(includedFile)))
    {
        // Absolute shader path
    }
    else
    {
        LOG(Error, "Unknown shader source file '{0}' included in '{1}'.{2}", String(includedFile), String(sourceFile), String::Empty);
        return true;
    }

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
        if (File::ReadAllBytes(result->Path, result->Source))
        {
            LOG(Error, "Failed to load shader source file '{0}' included in '{1}' (path: '{2}')", String(includedFile), String(sourceFile), path);
            Delete(result);
            return true;
        }
        IncludedFiles::Files.Add(path, result);
    }

    context->Includes.Add(path);

    // Copy to output
    source = (const char*)result->Source.Get();
    sourceLength = result->Source.Count() - 1;
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

    // Generate vertex shaders cache
    for (int32 i = 0; i < meta->VS.Count(); i++)
    {
        auto& shader = meta->VS[i];
        ASSERT(shader.GetStage() == ShaderStage::Vertex && (shader.Flags & ShaderFlags::Hidden) == 0);
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
        ASSERT(shader.GetStage() == ShaderStage::Hull && (shader.Flags & ShaderFlags::Hidden) == 0);
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
        ASSERT(shader.GetStage() == ShaderStage::Domain && (shader.Flags & ShaderFlags::Hidden) == 0);
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
        ASSERT(shader.GetStage() == ShaderStage::Geometry && (shader.Flags & ShaderFlags::Hidden) == 0);
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
        ASSERT(shader.GetStage() == ShaderStage::Pixel && (shader.Flags & ShaderFlags::Hidden) == 0);
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
        ASSERT(shader.GetStage() == ShaderStage::Compute && (shader.Flags & ShaderFlags::Hidden) == 0);
        if (CompileShader(shader))
        {
            LOG(Error, "Failed to compile \'{0}\'", String(shader.Name));
            return true;
        }
    }

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

    // [Output] Type
    output->WriteByte(static_cast<byte>(meta.GetStage()));

    // [Output] Permutations count
    output->WriteByte(meta.Permutations.Count());

    // [Output] Shader function name
    output->WriteStringAnsi(meta.Name, 11);

    // [Output] Shader flags
    output->WriteUint32((uint32)meta.Flags);

    return false;
}

bool ShaderCompiler::WriteShaderFunctionPermutation(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const ShaderBindings& bindings, const void* cache, int32 cacheSize)
{
    auto output = context->Output;

    // [Output] Write compiled shader cache
    output->WriteUint32(cacheSize);
    output->WriteBytes(cache, cacheSize);

    // [Output] Shader bindings meta
    output->Write(&bindings);

    return false;
}

bool ShaderCompiler::WriteShaderFunctionEnd(ShaderCompilationContext* context, ShaderFunctionMeta& meta)
{
    return false;
}

bool ShaderCompiler::WriteCustomDataVS(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const Array<ShaderMacro>& macros)
{
    auto output = context->Output;
    auto& metaVS = *(VertexShaderMeta*)&meta;
    auto& layout = metaVS.InputLayout;

    // Get visible entries (based on `visible` flag switch)
    int32 layoutSize = 0;
    bool layoutVisible[VERTEX_SHADER_MAX_INPUT_ELEMENTS];
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
            LOG(Error, "Invalid vertex shader layout element \'visible\' option value.");
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

        // TODO: serialize whole struct?

        output->WriteByte(static_cast<byte>(element.Type));
        output->WriteByte(element.Index);
        output->WriteByte(static_cast<byte>(element.Format));
        output->WriteByte(element.InputSlot);
        output->WriteUint32(element.AlignedByteOffset);
        output->WriteByte(element.InputSlotClass);
        output->WriteUint32(element.InstanceDataStepRate);
    }

    return false;
}

bool ShaderCompiler::WriteCustomDataHS(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const Array<ShaderMacro>& macros)
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
    const int32 functionNameLength = static_cast<int32>(functionName.Length());
    _funcNameDefineBuffer.Clear();
    _funcNameDefineBuffer.EnsureCapacity(functionNameLength + 2);
    _funcNameDefineBuffer.Add('_');
    _funcNameDefineBuffer.Add(functionName.Get(), functionNameLength);
    _funcNameDefineBuffer.Add('\0');
    macros.Add({ _funcNameDefineBuffer.Get(), "1" });
}

#endif
