// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_SHADER_COMPILER

#include "ShadersCompilation.h"
#include "ShaderCompilationContext.h"
#include "ShaderDebugDataExporter.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Parser/ShaderProcessing.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Content/Asset.h"
#if USE_EDITOR
#define COMPILE_WITH_ASSETS_IMPORTER 1 // Hack to use shaders importing in this module
#include "Engine/ContentImporters/AssetsImportingManager.h"
#include "Engine/Platform/FileSystemWatcher.h"
#include "Engine/Platform/FileSystem.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#endif

#if COMPILE_WITH_D3D_SHADER_COMPILER
#include "DirectX/ShaderCompilerD3D.h"
#endif
#if COMPILE_WITH_DX_SHADER_COMPILER
#include "DirectX/ShaderCompilerDX.h"
#endif
#if COMPILE_WITH_VK_SHADER_COMPILER
#include "Vulkan/ShaderCompilerVulkan.h"
#endif
#if COMPILE_WITH_PS4_SHADER_COMPILER
#include "Platforms/PS4/Engine/ShaderCompilerPS4/ShaderCompilerPS4.h"
#endif

namespace ShadersCompilationImpl
{
    CriticalSection Locker;
    Array<ShaderCompiler*> Compilers;
    Array<ShaderCompiler*> ReadyCompilers;
}

using namespace ShadersCompilationImpl;

class ShadersCompilationService : public EngineService
{
public:

    ShadersCompilationService()
        : EngineService(TEXT("Shaders Compilation Service"), -100)
    {
    }

    bool Init() override;
    void Dispose() override;
};

ShadersCompilationService ShadersCompilationServiceInstance;

bool ShadersCompilation::Compile(ShaderCompilationOptions& options)
{
    PROFILE_CPU_NAMED("Shader.Compile");

    // Validate input options
    if (options.TargetName.IsEmpty() || !options.TargetID.IsValid())
    {
        LOG(Warning, "Unknown target object.");
        return true;
    }
    if (options.Output == nullptr)
    {
        LOG(Warning, "Missing output.");
        return true;
    }
    if (options.Profile == ShaderProfile::Unknown)
    {
        LOG(Warning, "Unknown shader profile.");
        return true;
    }
    if (options.Source == nullptr || options.SourceLength < 1)
    {
        LOG(Warning, "Missing source code.");
        return true;
    }

    // Adjust input source length if it ends with null
    while (options.SourceLength > 2 && options.Source[options.SourceLength - 1] == 0)
        options.SourceLength--;

    const DateTime startTime = DateTime::NowUTC();
    const FeatureLevel featureLevel = RenderTools::GetFeatureLevel(options.Profile);

    // Process shader source to collect metadata
    ShaderMeta meta;
    if (ShaderProcessing::Parser::Process(options.TargetName, options.Source, options.SourceLength, options.Macros, featureLevel, &meta))
    {
        LOG(Warning, "Failed to parse source code.");
        return true;
    }
    const int32 shadersCount = meta.GetShadersCount();
    if (shadersCount == 0)
    {
        LOG(Warning, "Shader has no valid functions.");
    }

    // Perform actual compilation
    bool result;
    {
        ShaderCompilationContext context(&options, &meta);

        // Request shaders compiler
        auto compiler = RequestCompiler(options.Profile);
        if (compiler == nullptr)
        {
            LOG(Error, "Shader compiler request failed.");
            return true;
        }
        ASSERT(compiler->GetProfile() == options.Profile);

        // Call compilation process
        result = compiler->Compile(&context);

        // Dismiss compiler
        FreeCompiler(compiler);

#if GPU_USE_SHADERS_DEBUG_LAYER
        // Export debug data
        ShaderDebugDataExporter::Export(&context);
#endif
    }

    // Print info if succeed
    if (result == false)
    {
        const DateTime endTime = DateTime::NowUTC();
        LOG(Info, "Shader compilation '{0}' succeed in {1} ms (profile: {2})", options.TargetName, Math::CeilToInt(static_cast<float>((endTime - startTime).GetTotalMilliseconds())), ::ToString(options.Profile));
    }

    return result;
}

ShaderCompiler* ShadersCompilation::CreateCompiler(ShaderProfile profile)
{
    ShaderCompiler* result = nullptr;

    switch (profile)
    {
#if COMPILE_WITH_D3D_SHADER_COMPILER
        // Direct 3D
    case ShaderProfile::DirectX_SM4:
    case ShaderProfile::DirectX_SM5:
        result = New<ShaderCompilerD3D>(profile);
        break;
#endif
#if COMPILE_WITH_DX_SHADER_COMPILER
    case ShaderProfile::DirectX_SM6:
        result = New<ShaderCompilerDX>(profile);
        break;
#endif
#if COMPILE_WITH_VK_SHADER_COMPILER
        // Vulkan
    case ShaderProfile::Vulkan_SM5:
        result = New<ShaderCompilerVulkan>(profile);
        break;
#endif
#if COMPILE_WITH_PS4_SHADER_COMPILER
        // PS4
    case ShaderProfile::PS4:
        result = New<ShaderCompilerPS4>();
        break;
#endif
    default:
        break;
    }
    ASSERT_LOW_LAYER(result == nullptr || result->GetProfile() == profile);

    return result;
}

ShaderCompiler* ShadersCompilation::RequestCompiler(ShaderProfile profile)
{
    ShaderCompiler* compiler;
    ScopeLock lock(Locker);

    // Try to find ready compiler
    for (int32 i = 0; i < ReadyCompilers.Count(); i++)
    {
        compiler = ReadyCompilers[i];
        if (compiler->GetProfile() == profile)
        {
            // Use it
            ReadyCompilers.RemoveAt(i);
            return compiler;
        }
    }

    // Create new compiler for a target profile
    compiler = CreateCompiler(profile);
    if (compiler == nullptr)
    {
        LOG(Error, "Cannot create Shader Compiler for profile {0}", ::ToString(profile));
        return nullptr;
    }

    // Register new compiler
    Compilers.Add(compiler);

    return compiler;
}

void ShadersCompilation::FreeCompiler(ShaderCompiler* compiler)
{
    ScopeLock lock(Locker);
    ASSERT(compiler && ReadyCompilers.Contains(compiler) == false);

    // Check if service has been disposed (this compiler is not in the compilers list)
    if (Compilers.Contains(compiler) == false)
    {
        // Delete it manually
        Delete(compiler);
    }
    else
    {
        // Make compiler free again
        ReadyCompilers.Add(compiler);
    }
}

namespace
{
    CriticalSection ShaderIncludesMapLocker;
    Dictionary<String, Array<Asset*>> ShaderIncludesMap;
    Dictionary<String, FileSystemWatcher*> ShaderIncludesWatcher;

    void OnShaderIncludesWatcherEvent(const String& path, FileSystemAction action)
    {
        if (action == FileSystemAction::Delete)
            return;

        Array<Asset*> toReload;
        {
            ScopeLock lock(ShaderIncludesMapLocker);

            auto file = ShaderIncludesMap.Find(path);
            if (file == ShaderIncludesMap.End())
                return;
            toReload = file->Value;
        }

        LOG(Info, "Shader include \'{0}\' has been modified.", path);

        // Wait a little so app that was editing the file (e.g. Visual Studio, Notepad++) has enough time to flush whole file change
        Platform::Sleep(100);

        // Reload shaders using this include
        for (Asset* asset : toReload)
        {
            asset->Reload();
        }
    }
}

void ShadersCompilation::RegisterForShaderReloads(Asset* asset, const String& includedPath)
{
    ScopeLock lock(ShaderIncludesMapLocker);

    // Add to collection
    const bool alreadyAdded = ShaderIncludesMap.ContainsKey(includedPath);
    auto& file = ShaderIncludesMap[includedPath];
    ASSERT_LOW_LAYER(!file.Contains(asset));
    file.Add(asset);

    if (!alreadyAdded)
    {
        // Create a directory watcher to track the included file changes
        const String directory = StringUtils::GetDirectoryName(includedPath);
        if (!ShaderIncludesWatcher.ContainsKey(directory))
        {
            auto watcher = New<FileSystemWatcher>(directory, false);
            watcher->OnEvent.Bind<OnShaderIncludesWatcherEvent>();
            ShaderIncludesWatcher.Add(directory, watcher);
        }
    }
}

void ShadersCompilation::UnregisterForShaderReloads(Asset* asset)
{
    ScopeLock lock(ShaderIncludesMapLocker);

    // Remove asset reference
    for (auto& file : ShaderIncludesMap)
    {
        file.Value.Remove(asset);
    }
}

void ShadersCompilation::ExtractShaderIncludes(byte* shaderCache, int32 shaderCacheLength, Array<String>& includes)
{
    MemoryReadStream stream(shaderCache, shaderCacheLength);

    // Read cache format version
    int32 version;
    stream.ReadInt32(&version);
    if (version != GPU_SHADER_CACHE_VERSION)
    {
        return;
    }

    // Read the location of additional data that contains list of included source files
    int32 additionalDataStart;
    stream.ReadInt32(&additionalDataStart);
    stream.SetPosition(additionalDataStart);

    // Read all includes
    int32 includesCount;
    stream.ReadInt32(&includesCount);
    includes.Clear();
    for (int32 i = 0; i < includesCount; i++)
    {
        String& include = includes.AddOne();
        stream.ReadString(&include, 11);
        DateTime lastEditTime;
        stream.Read(&lastEditTime);
    }
}

#if USE_EDITOR

namespace
{
    Array<FileSystemWatcher*> ShadersSourcesWatchers;

    // Tries to generate a stable and unique ID for the given shader name.
    // Used in order to keep the same shader IDs and reduce version control issues with binary diff on ID.
    Guid GetShaderAssetId(const String& name)
    {
        Guid result;
        result.A = name.Length() * 100;
        result.B = GetHash(name);
        result.C = name.HasChars() ? name[0] : 0;
        result.D = name.HasChars() ? name[name.Length() - 1] : 0;
        return result;
    }

    void OnWatcherShadersEvent(const String& path, FileSystemAction action)
    {
        if (action == FileSystemAction::Delete || !path.EndsWith(TEXT(".shader")))
            return;

        LOG(Info, "Shader \'{0}\' has been modified.", path);

        // Wait a little so app that was editing the file (e.g. Visual Studio, Notepad++) has enough time to flush whole file change
        Platform::Sleep(100);

        // Perform hot reload
        const int32 srcSubDirStart = path.FindLast(TEXT("/Source/Shaders"));
        if (srcSubDirStart == -1)
            return;
        String projectFolderPath = path.Substring(0, srcSubDirStart);
        FileSystem::NormalizePath(projectFolderPath);
        const String shadersAssetsPath = projectFolderPath / TEXT("/Content/Shaders");
        const String shadersSourcePath = projectFolderPath / TEXT("/Source/Shaders");
        const String localPath = FileSystem::ConvertAbsolutePathToRelative(shadersSourcePath, path);
        const String name = StringUtils::GetPathWithoutExtension(localPath);
        const String outputPath = shadersAssetsPath / name + ASSET_FILES_EXTENSION_WITH_DOT;
        Guid id = GetShaderAssetId(name);
        AssetsImportingManager::ImportIfEdited(path, outputPath, id);
    }

    void RegisterShaderWatchers(const ProjectInfo* project, HashSet<const ProjectInfo*>& projects)
    {
        if (projects.Contains(project))
            return;
        projects.Add(project);

        // Check if project uses shaders sources
        const String shadersSourcePath = project->ProjectFolderPath / TEXT("/Source/Shaders");
        if (FileSystem::DirectoryExists(shadersSourcePath))
        {
            // Track engine shaders editing
            auto sourceWatcher = New<FileSystemWatcher>(shadersSourcePath, true);
            sourceWatcher->OnEvent.Bind<OnWatcherShadersEvent>();
            ShadersSourcesWatchers.Add(sourceWatcher);

            // Reimport modified or import added shaders
            Array<String> files(64);
            const String shadersAssetsPath = project->ProjectFolderPath / TEXT("/Content/Shaders");
            FileSystem::DirectoryGetFiles(files, shadersSourcePath, TEXT("*.shader"), DirectorySearchOption::AllDirectories);
            for (int32 i = 0; i < files.Count(); i++)
            {
                const String& path = files[i];
                const String localPath = FileSystem::ConvertAbsolutePathToRelative(shadersSourcePath, path);
                const String name = StringUtils::GetPathWithoutExtension(localPath);
                const String outputPath = shadersAssetsPath / name + ASSET_FILES_EXTENSION_WITH_DOT;
                Guid id = GetShaderAssetId(name);
                AssetsImportingManager::ImportIfEdited(path, outputPath, id);
            }
        }

        // Initialize referenced projects
        for (const auto& reference : project->References)
        {
            if (reference.Project)
                RegisterShaderWatchers(reference.Project, projects);
        }
    }
}

#endif

bool ShadersCompilationService::Init()
{
#if USE_EDITOR
    // Initialize automatic shaders importing and reloading for all loaded projects (game, engine, plugins)
    HashSet<const ProjectInfo*> projects;
    RegisterShaderWatchers(Editor::Project, projects);
#endif

    return false;
}

void ShadersCompilationService::Dispose()
{
#if USE_EDITOR
    ShadersSourcesWatchers.ClearDelete();
#endif

    Locker.Lock();

    // Check if any compilation is running
    if (ReadyCompilers.Count() != Compilers.Count())
    {
        LOG(Error, "Cannot dispose Shaders Compilation Service. One or more compilers are still in use.");
    }

    // Cleanup all compilers (delete only those which are not in use)
    ReadyCompilers.ClearDelete();
    Compilers.Clear();

    Locker.Unlock();

    // Cleanup shader includes
    ShaderCompiler::DisposeIncludedFilesCache();

    // Clear includes scanning
    ShaderIncludesMapLocker.Lock();
    ShaderIncludesMap.Clear();
    ShaderIncludesWatcher.ClearDelete();
    ShaderIncludesMapLocker.Unlock();
}

#endif
