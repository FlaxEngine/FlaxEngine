// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ManagedEditor.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/WindowsManager.h"
#include "Engine/ContentImporters/AssetsImportingManager.h"
#include "Engine/ContentExporters/AssetsExportingManager.h"
#include "Editor/CustomEditors/CustomEditorsUtil.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/InternalCalls.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/StdTypesContainer.h"
#include "Engine/Graphics/Shaders/Cache/ShaderCacheManager.h"
#include "Engine/ShadowsOfMordor/Builder.h"
#include "Engine/Physics/CollisionData.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/Material.h"
#include "Engine/Content/Assets/VisualScript.h"
#include "Engine/ContentImporters/ImportTexture.h"
#include "Engine/ContentImporters/ImportModel.h"
#include "Engine/ContentImporters/ImportAudio.h"
#include "Engine/ContentImporters/CreateCollisionData.h"
#include "Engine/ContentImporters/CreateJson.h"
#include "Engine/Level/Level.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Core/Cache.h"
#include "Engine/CSG/CSGBuilder.h"
#include "Engine/Debug/DebugLog.h"
#include "Engine/Debug/Exceptions/JsonParseException.h"
#include "Engine/Audio/AudioClip.h"
#include "Engine/Level/Actor.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Utilities/Encryption.h"
#include "Engine/Navigation/Navigation.h"
#include "Engine/Particles/ParticleEmitter.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Input/Keyboard.h"
#include "FlaxEngine.Gen.h"
#include <ThirdParty/mono-2.0/mono/metadata/appdomain.h>

Guid ManagedEditor::ObjectID(0x91970b4e, 0x99634f61, 0x84723632, 0x54c776af);

// Disable warning C4800: 'const byte': forcing value to bool 'true' or 'false' (performance warning)
#if defined(_MSC_VER)
#pragma warning( push )
#pragma warning( disable : 4800)
#endif

struct InternalTextureOptions
{
    TextureFormatType Type;
    byte IsAtlas;
    byte NeverStream;
    byte Compress;
    byte IndependentChannels;
    byte sRGB;
    byte GenerateMipMaps;
    byte FlipY;
    byte Resize;
    byte PreserveAlphaCoverage;
    float PreserveAlphaCoverageReference;
    float Scale;
    int32 MaxSize;
    int32 SizeX;
    int32 SizeY;
    MonoArray* SpriteAreas;
    MonoArray* SpriteNames;

    static void Convert(InternalTextureOptions* from, ImportTexture::Options* to)
    {
        to->Type = from->Type;
        to->IsAtlas = from->IsAtlas;
        to->NeverStream = from->NeverStream;
        to->Compress = from->Compress;
        to->IndependentChannels = from->IndependentChannels;
        to->sRGB = from->sRGB;
        to->GenerateMipMaps = from->GenerateMipMaps;
        to->FlipY = from->FlipY;
        to->Scale = from->Scale;
        to->Resize = from->Resize;
        to->PreserveAlphaCoverage = from->PreserveAlphaCoverage;
        to->PreserveAlphaCoverageReference = from->PreserveAlphaCoverageReference;
        to->MaxSize = from->MaxSize;
        to->SizeX = from->SizeX;
        to->SizeY = from->SizeY;
        to->Sprites.Clear();
        if (from->SpriteNames != nullptr)
        {
            int32 count = (int32)mono_array_length(from->SpriteNames);
            ASSERT(count == (int32)mono_array_length(from->SpriteAreas));
            to->Sprites.EnsureCapacity(count);
            for (int32 i = 0; i < count; i++)
            {
                Sprite sprite;
                sprite.Area = mono_array_get(from->SpriteAreas, Rectangle, i);
                sprite.Name = MUtils::ToString(mono_array_get(from->SpriteNames, MonoString*, i));
                to->Sprites.Add(sprite);
            }
        }
    }

    static void Convert(ImportTexture::Options* from, InternalTextureOptions* to)
    {
        to->Type = from->Type;
        to->IsAtlas = from->IsAtlas;
        to->NeverStream = from->NeverStream;
        to->Compress = from->Compress;
        to->IndependentChannels = from->IndependentChannels;
        to->sRGB = from->sRGB;
        to->GenerateMipMaps = from->GenerateMipMaps;
        to->FlipY = from->FlipY;
        to->Resize = from->Resize;
        to->PreserveAlphaCoverage = from->PreserveAlphaCoverage;
        to->PreserveAlphaCoverageReference = from->PreserveAlphaCoverageReference;
        to->Scale = from->Scale;
        to->MaxSize = from->MaxSize;
        to->SizeX = from->SizeX;
        to->SizeY = from->SizeY;
        if (from->Sprites.HasItems())
        {
            const auto domain = mono_domain_get();
            int32 count = from->Sprites.Count();
            auto rectClass = Scripting::FindClass("FlaxEngine.Rectangle");
            ASSERT(rectClass != nullptr);
            to->SpriteAreas = mono_array_new(domain, rectClass->GetNative(), count);
            to->SpriteNames = mono_array_new(domain, mono_get_string_class(), count);
            for (int32 i = 0; i < count; i++)
            {
                mono_array_set(to->SpriteAreas, Rectangle, i, from->Sprites[i].Area);
                mono_array_setref(to->SpriteNames, i, MUtils::ToString(from->Sprites[i].Name, domain));
            }
        }
        else
        {
            to->SpriteAreas = nullptr;
            to->SpriteNames = nullptr;
        }
    }
};

struct InternalModelOptions
{
    ModelTool::ModelType Type;

    // Geometry
    byte CalculateNormals;
    float SmoothingNormalsAngle;
    byte FlipNormals;
    float SmoothingTangentsAngle;
    byte CalculateTangents;
    byte OptimizeMeshes;
    byte MergeMeshes;
    byte ImportLODs;
    byte ImportVertexColors;
    byte ImportBlendShapes;
    ModelLightmapUVsSource LightmapUVsSource;

    // Transform
    float Scale;
    Quaternion Rotation;
    Vector3 Translation;
    byte CenterGeometry;

    // Animation
    ModelTool::AnimationDuration Duration;
    float FramesRangeStart;
    float FramesRangeEnd;
    float DefaultFrameRate;
    float SamplingRate;
    byte SkipEmptyCurves;
    byte OptimizeKeyframes;
    byte EnableRootMotion;
    MonoString* RootNodeName;
    int32 AnimationIndex;

    // Level Of Detail
    byte GenerateLODs;
    int32 BaseLOD;
    int32 LODCount;
    float TriangleReduction;

    // Misc
    byte ImportMaterials;
    byte ImportTextures;
    byte RestoreMaterialsOnReimport;

    static void Convert(InternalModelOptions* from, ImportModelFile::Options* to)
    {
        to->Type = from->Type;
        to->CalculateNormals = from->CalculateNormals;
        to->SmoothingNormalsAngle = from->SmoothingNormalsAngle;
        to->FlipNormals = from->FlipNormals;
        to->SmoothingTangentsAngle = from->SmoothingTangentsAngle;
        to->CalculateTangents = from->CalculateTangents;
        to->OptimizeMeshes = from->OptimizeMeshes;
        to->MergeMeshes = from->MergeMeshes;
        to->ImportLODs = from->ImportLODs;
        to->ImportVertexColors = from->ImportVertexColors;
        to->ImportBlendShapes = from->ImportBlendShapes;
        to->LightmapUVsSource = from->LightmapUVsSource;
        to->Scale = from->Scale;
        to->Rotation = from->Rotation;
        to->Translation = from->Translation;
        to->CenterGeometry = from->CenterGeometry;
        to->Duration = from->Duration;
        to->FramesRange.X = from->FramesRangeStart;
        to->FramesRange.Y = from->FramesRangeEnd;
        to->DefaultFrameRate = from->DefaultFrameRate;
        to->SamplingRate = from->SamplingRate;
        to->SkipEmptyCurves = from->SkipEmptyCurves;
        to->OptimizeKeyframes = from->OptimizeKeyframes;
        to->EnableRootMotion = from->EnableRootMotion;
        to->RootNodeName = MUtils::ToString(from->RootNodeName);
        to->AnimationIndex = from->AnimationIndex;
        to->GenerateLODs = from->GenerateLODs;
        to->BaseLOD = from->BaseLOD;
        to->LODCount = from->LODCount;
        to->TriangleReduction = from->TriangleReduction;
        to->ImportMaterials = from->ImportMaterials;
        to->ImportTextures = from->ImportTextures;
        to->RestoreMaterialsOnReimport = from->RestoreMaterialsOnReimport;
    }

    static void Convert(ImportModelFile::Options* from, InternalModelOptions* to)
    {
        to->Type = from->Type;
        to->CalculateNormals = from->CalculateNormals;
        to->SmoothingNormalsAngle = from->SmoothingNormalsAngle;
        to->FlipNormals = from->FlipNormals;
        to->SmoothingTangentsAngle = from->SmoothingTangentsAngle;
        to->CalculateTangents = from->CalculateTangents;
        to->OptimizeMeshes = from->OptimizeMeshes;
        to->MergeMeshes = from->MergeMeshes;
        to->ImportLODs = from->ImportLODs;
        to->ImportVertexColors = from->ImportVertexColors;
        to->ImportBlendShapes = from->ImportBlendShapes;
        to->LightmapUVsSource = from->LightmapUVsSource;
        to->Scale = from->Scale;
        to->Rotation = from->Rotation;
        to->Translation = from->Translation;
        to->CenterGeometry = from->CenterGeometry;
        to->Duration = from->Duration;
        to->FramesRangeStart = from->FramesRange.X;
        to->FramesRangeEnd = from->FramesRange.Y;
        to->DefaultFrameRate = from->DefaultFrameRate;
        to->SamplingRate = from->SamplingRate;
        to->SkipEmptyCurves = from->SkipEmptyCurves;
        to->OptimizeKeyframes = from->OptimizeKeyframes;
        to->EnableRootMotion = from->EnableRootMotion;
        to->RootNodeName = MUtils::ToString(from->RootNodeName);
        to->AnimationIndex = from->AnimationIndex;
        to->GenerateLODs = from->GenerateLODs;
        to->BaseLOD = from->BaseLOD;
        to->LODCount = from->LODCount;
        to->TriangleReduction = from->TriangleReduction;
        to->ImportMaterials = from->ImportMaterials;
        to->ImportTextures = from->ImportTextures;
        to->RestoreMaterialsOnReimport = from->RestoreMaterialsOnReimport;
    }
};

struct InternalAudioOptions
{
    AudioFormat Format;
    byte DisableStreaming;
    byte Is3D;
    int32 BitDepth;
    float Quality;

    static void Convert(InternalAudioOptions* from, ImportAudio::Options* to)
    {
        to->Format = from->Format;
        to->DisableStreaming = from->DisableStreaming;
        to->Is3D = from->Is3D;
        to->BitDepth = from->BitDepth;
        to->Quality = from->Quality;
    }

    static void Convert(ImportAudio::Options* from, InternalAudioOptions* to)
    {
        to->Format = from->Format;
        to->DisableStreaming = from->DisableStreaming;
        to->Is3D = from->Is3D;
        to->BitDepth = from->BitDepth;
        to->Quality = from->Quality;
    }
};

#if defined(_MSC_VER)
#pragma warning( pop )
#endif

namespace CustomEditorsUtilInternal
{
    MonoReflectionType* GetCustomEditor(MonoReflectionType* targetType)
    {
        return CustomEditorsUtil::GetCustomEditor(targetType);
    }
}

namespace LayersAndTagsSettingsInternal1
{
    MonoArray* GetCurrentTags()
    {
        return MUtils::ToArray(Level::Tags);
    }

    MonoArray* GetCurrentLayers()
    {
        return MUtils::ToArray(Span<String>(Level::Layers, Math::Max(1, Level::GetNonEmptyLayerNamesCount())));
    }
}

namespace GameSettingsInternal1
{
    void Apply()
    {
        LOG(Info, "Apply game settings");
        GameSettings::Load();
    }
}

// Pack log messages into a single scratch buffer to reduce dynamic memory allocations
CriticalSection CachedLogDataLocker;
Array<byte> CachedLogData;

void OnLogMessage(LogType type, const StringView& msg)
{
    ScopeLock lock(CachedLogDataLocker);

    CachedLogData.EnsureCapacity(4 + 8 + 4 + msg.Length() * 2);

    // Log Type
    int32 buf = (int32)type;
    CachedLogData.Add((byte*)&buf, 4);

    // Time
    auto time = DateTime::Now();
    CachedLogData.Add((byte*)&time.Ticks, 8);

    // Message Length
    buf = msg.Length();
    CachedLogData.Add((byte*)&buf, 4);

    // Message
    CachedLogData.Add((byte*)msg.Get(), msg.Length() * 2);
}

class ManagedEditorInternal
{
public:
    static bool IsDevInstance()
    {
#if COMPILE_WITH_DEV_ENV
        return true;
#else
		return false;
#endif
    }

    static bool IsOfficialBuild()
    {
#if OFFICIAL_BUILD
        return true;
#else
        return false;
#endif
    }

    static bool IsPlayMode()
    {
        return Editor::IsPlayMode;
    }

    static int32 ReadOutputLogs(MonoArray* outMessages, MonoArray* outLogTypes, MonoArray* outLogTimes)
    {
        ScopeLock lock(CachedLogDataLocker);

        if (CachedLogData.IsEmpty())
            return 0;

        int32 count = 0;
        const int32 maxCount = (int32)mono_array_length(outMessages);

        byte* ptr = CachedLogData.Get();
        byte* end = ptr + CachedLogData.Count();
        while (count < maxCount && ptr != end)
        {
            auto type = (byte)*(int32*)ptr;
            ptr += 4;

            auto time = *(int64*)ptr;
            ptr += 8;

            auto length = *(int32*)ptr;
            ptr += 4;

            auto msg = (Char*)ptr;
            ptr += length * 2;

            auto msgObj = MUtils::ToString(StringView(msg, length));

            mono_array_setref(outMessages, count, msgObj);
            mono_array_set(outLogTypes, byte, count, type);
            mono_array_set(outLogTimes, int64, count, time);

            count++;
        }

        const int32 dataLeft = (int32)(end - ptr);
        Platform::MemoryCopy(CachedLogData.Get(), ptr, dataLeft);
        CachedLogData.Resize(dataLeft);

        return count;
    }

    static void SetPlayMode(bool value)
    {
        Editor::IsPlayMode = value;
    }

    static MonoString* GetProjectPath()
    {
        return MUtils::ToString(Editor::Project->ProjectPath);
    }

    static void CloseSplashScreen()
    {
        Editor::CloseSplashScreen();
    }

    static bool CloneAssetFile(MonoString* dstPathObj, MonoString* srcPathObj, Guid* dstId)
    {
        // Get normalized paths
        String dstPath, srcPath;
        MUtils::ToString(dstPathObj, dstPath);
        MUtils::ToString(srcPathObj, srcPath);
        FileSystem::NormalizePath(dstPath);
        FileSystem::NormalizePath(srcPath);

        // Call util function
        return Content::CloneAssetFile(dstPath, srcPath, *dstId);
    }

    enum class NewAssetType
    {
        Material = 0,
        MaterialInstance = 1,
        CollisionData = 2,
        AnimationGraph = 3,
        SkeletonMask = 4,
        ParticleEmitter = 5,
        ParticleSystem = 6,
        SceneAnimation = 7,
        MaterialFunction = 8,
        ParticleEmitterFunction = 9,
        AnimationGraphFunction = 10,
    };

    static bool CreateAsset(NewAssetType type, MonoString* outputPathObj)
    {
        String tag;
        switch (type)
        {
        case NewAssetType::Material:
            tag = AssetsImportingManager::CreateMaterialTag;
            break;
        case NewAssetType::MaterialInstance:
            tag = AssetsImportingManager::CreateMaterialInstanceTag;
            break;
        case NewAssetType::CollisionData:
            tag = AssetsImportingManager::CreateCollisionDataTag;
            break;
        case NewAssetType::AnimationGraph:
            tag = AssetsImportingManager::CreateAnimationGraphTag;
            break;
        case NewAssetType::SkeletonMask:
            tag = AssetsImportingManager::CreateSkeletonMaskTag;
            break;
        case NewAssetType::ParticleEmitter:
            tag = AssetsImportingManager::CreateParticleEmitterTag;
            break;
        case NewAssetType::ParticleSystem:
            tag = AssetsImportingManager::CreateParticleSystemTag;
            break;
        case NewAssetType::SceneAnimation:
            tag = AssetsImportingManager::CreateSceneAnimationTag;
            break;
        case NewAssetType::MaterialFunction:
            tag = AssetsImportingManager::CreateMaterialFunctionTag;
            break;
        case NewAssetType::ParticleEmitterFunction:
            tag = AssetsImportingManager::CreateParticleEmitterFunctionTag;
            break;
        case NewAssetType::AnimationGraphFunction:
            tag = AssetsImportingManager::CreateAnimationGraphFunctionTag;
            break;
        default:
            return true;
        }

        String outputPath;
        MUtils::ToString(outputPathObj, outputPath);
        FileSystem::NormalizePath(outputPath);

        return AssetsImportingManager::Create(tag, outputPath);
    }

    static bool CreateVisualScript(MonoString* outputPathObj, MonoString* baseTypenameObj)
    {
        String outputPath;
        MUtils::ToString(outputPathObj, outputPath);
        FileSystem::NormalizePath(outputPath);
        String baseTypename;
        MUtils::ToString(baseTypenameObj, baseTypename);
        return AssetsImportingManager::Create(AssetsImportingManager::CreateVisualScriptTag, outputPath, &baseTypename);
    }

    static bool CanImport(MonoString* extensionObj)
    {
        String extension;
        MUtils::ToString(extensionObj, extension);
        if (extension.Length() > 0 && extension[0] == '.')
            extension.Remove(0, 1);
        return AssetsImportingManager::GetImporter(extension) != nullptr;
    }

    static bool Import(MonoString* inputPathObj, MonoString* outputPathObj, void* arg)
    {
        String inputPath, outputPath;
        MUtils::ToString(inputPathObj, inputPath);
        MUtils::ToString(outputPathObj, outputPath);
        FileSystem::NormalizePath(inputPath);
        FileSystem::NormalizePath(outputPath);

        return AssetsImportingManager::Import(inputPath, outputPath, arg);
    }

    static bool ImportTexture(MonoString* inputPathObj, MonoString* outputPathObj, InternalTextureOptions* optionsObj)
    {
        ImportTexture::Options options;
        InternalTextureOptions::Convert(optionsObj, &options);

        return Import(inputPathObj, outputPathObj, &options);
    }

    static bool ImportModel(MonoString* inputPathObj, MonoString* outputPathObj, InternalModelOptions* optionsObj)
    {
        ImportModelFile::Options options;
        InternalModelOptions::Convert(optionsObj, &options);

        return Import(inputPathObj, outputPathObj, &options);
    }

    static bool ImportAudio(MonoString* inputPathObj, MonoString* outputPathObj, InternalAudioOptions* optionsObj)
    {
        ImportAudio::Options options;
        InternalAudioOptions::Convert(optionsObj, &options);

        return Import(inputPathObj, outputPathObj, &options);
    }

    static void GetAudioClipMetadata(AudioClip* clip, int32* originalSize, int32* importedSize)
    {
        INTERNAL_CALL_CHECK(clip);
        *originalSize = clip->AudioHeader.OriginalSize;
        *importedSize = clip->AudioHeader.ImportedSize;
    }

    static bool SaveJsonAsset(MonoString* outputPathObj, MonoString* dataObj, MonoString* dataTypeNameObj)
    {
        String outputPath;
        MUtils::ToString(outputPathObj, outputPath);
        FileSystem::NormalizePath(outputPath);

        const auto dataObjPtr = mono_string_to_utf8(dataObj);
        StringAnsiView data(dataObjPtr);

        const auto dataTypeNameObjPtr = mono_string_to_utf8(dataTypeNameObj);
        StringAnsiView dataTypeName(dataTypeNameObjPtr);

        const bool result = CreateJson::Create(outputPath, data, dataTypeName);

        mono_free(dataObjPtr);
        mono_free(dataTypeNameObjPtr);

        return result;
    }

    static bool GetTextureImportOptions(MonoString* pathObj, InternalTextureOptions* result)
    {
        String path;
        MUtils::ToString(pathObj, path);
        FileSystem::NormalizePath(path);

        ImportTexture::Options options;
        if (ImportTexture::TryGetImportOptions(path, options))
        {
            // Convert into managed storage
            InternalTextureOptions::Convert(&options, result);

            return true;
        }

        return false;
    }

    static bool GetModelImportOptions(MonoString* pathObj, InternalModelOptions* result)
    {
        String path;
        MUtils::ToString(pathObj, path);
        FileSystem::NormalizePath(path);

        ImportModelFile::Options options;
        if (ImportModelFile::TryGetImportOptions(path, options))
        {
            // Convert into managed storage
            InternalModelOptions::Convert(&options, result);

            return true;
        }

        return false;
    }

    static bool GetAudioImportOptions(MonoString* pathObj, InternalAudioOptions* result)
    {
        String path;
        MUtils::ToString(pathObj, path);
        FileSystem::NormalizePath(path);

        ImportAudio::Options options;
        if (ImportAudio::TryGetImportOptions(path, options))
        {
            // Convert into managed storage
            InternalAudioOptions::Convert(&options, result);

            return true;
        }

        return false;
    }

    static bool CanExport(MonoString* pathObj)
    {
#if COMPILE_WITH_ASSETS_EXPORTER
        String path;
        MUtils::ToString(pathObj, path);
        FileSystem::NormalizePath(path);

        return AssetsExportingManager::CanExport(path);
#else
		return false;
#endif
    }

    static bool Export(MonoString* inputPathObj, MonoString* outputFolderObj)
    {
#if COMPILE_WITH_ASSETS_EXPORTER
        String inputPath;
        MUtils::ToString(inputPathObj, inputPath);
        FileSystem::NormalizePath(inputPath);

        String outputFolder;
        MUtils::ToString(outputFolderObj, outputFolder);
        FileSystem::NormalizePath(outputFolder);

        return AssetsExportingManager::Export(inputPath, outputFolder);
#else
		return false;
#endif
    }

    static void CopyCache(Guid* dstId, Guid* srcId)
    {
        ShaderCacheManager::CopyCache(*dstId, *srcId);
    }

    static void BakeLightmaps(bool cancel)
    {
        auto builder = ShadowsOfMordor::Builder::Instance();
        if (cancel)
            builder->CancelBuild();
        else
            builder->Build();
    }

    static MonoString* GetShaderAssetSourceCode(BinaryAsset* obj)
    {
        INTERNAL_CALL_CHECK_RETURN(obj, nullptr);
        if (obj->WaitForLoaded())
            DebugLog::ThrowNullReference();

        auto lock = obj->Storage->Lock();

        if (obj->LoadChunk(SHADER_FILE_CHUNK_SOURCE))
            return nullptr;

        BytesContainer data;
        obj->GetChunkData(SHADER_FILE_CHUNK_SOURCE, data);

        Encryption::DecryptBytes((byte*)data.Get(), data.Length());

        const StringAnsiView srcData((const char*)data.Get(), data.Length());
        const String source(srcData);
        const auto str = MUtils::ToString(source);

        Encryption::EncryptBytes((byte*)data.Get(), data.Length());

        return str;
    }

    static bool CookMeshCollision(MonoString* pathObj, CollisionDataType type, Model* modelObj, int32 modelLodIndex, ConvexMeshGenerationFlags convexFlags, int32 convexVertexLimit)
    {
#if COMPILE_WITH_PHYSICS_COOKING
        CollisionCooking::Argument arg;
        String path;
        MUtils::ToString(pathObj, path);
        FileSystem::NormalizePath(path);
        arg.Type = type;
        arg.Model = modelObj;
        arg.ModelLodIndex = modelLodIndex;
        arg.ConvexFlags = convexFlags;
        arg.ConvexVertexLimit = convexVertexLimit;

        return CreateCollisionData::CookMeshCollision(path, arg);
#else
		LOG(Warning, "Collision cooking is disabled.");
		return true;
#endif
    }

    static void GetCollisionWires(CollisionData* collisionData, MonoArray** triangles, MonoArray** indices)
    {
        if (!collisionData || collisionData->WaitForLoaded() || collisionData->GetOptions().Type == CollisionDataType::None)
            return;

        const auto& debugLines = collisionData->GetDebugLines();

        const int32 linesCount = debugLines.Count() / 2;
        *triangles = mono_array_new(mono_domain_get(), StdTypesContainer::Instance()->Vector3Class->GetNative(), debugLines.Count());
        *indices = mono_array_new(mono_domain_get(), mono_get_int32_class(), linesCount * 3);

        // Use one triangle per debug line
        for (int32 i = 0; i < debugLines.Count(); i++)
        {
            mono_array_set(*triangles, Vector3, i, debugLines[i]);
        }
        int32 iI = 0;
        for (int32 i = 0; i < debugLines.Count(); i += 2)
        {
            mono_array_set(*indices, int32, iI++, i);
            mono_array_set(*indices, int32, iI++, i + 1);
            mono_array_set(*indices, int32, iI++, i);
        }
    }

    static void GetEditorBoxWithChildren(Actor* obj, BoundingBox* result)
    {
        INTERNAL_CALL_CHECK(obj);
        *result = obj->GetEditorBoxChildren();
    }

    static void SetOptions(ManagedEditor::InternalOptions* options)
    {
        ManagedEditor::ManagedEditorOptions = *options;
    }

    static void DrawNavMesh()
    {
        Navigation::DrawNavMesh();
    }

    static bool GetIsEveryAssemblyLoaded()
    {
        return Scripting::IsEveryAssemblyLoaded();
    }

    static int32 GetLastProjectOpenedEngineBuild()
    {
        return Editor::LastProjectOpenedEngineBuild;
    }

    static bool GetIsCSGActive()
    {
#if COMPILE_WITH_CSG_BUILDER
        return CSG::Builder::IsActive();
#else
        return false;
#endif
    }

    static void RunVisualScriptBreakpointLoopTick(float deltaTime)
    {
        // Update
        Platform::Tick();
        Engine::HasFocus = (Engine::MainWindow && Engine::MainWindow->IsFocused()) || Platform::GetHasFocus();
        if (Engine::HasFocus)
        {
            InputDevice::EventQueue inputEvents;
            if (Input::Mouse)
            {
                if (Input::Mouse->Update(inputEvents))
                {
                    Input::Mouse->DeleteObject();
                    Input::Mouse = nullptr;
                }
            }
            if (Input::Keyboard)
            {
                if (Input::Keyboard->Update(inputEvents))
                {
                    Input::Keyboard->DeleteObject();
                    Input::Keyboard = nullptr;
                }
            }
            WindowsManager::WindowsLocker.Lock();
            Window* defaultWindow = nullptr;
            for (auto window : WindowsManager::Windows)
            {
                if (window->IsFocused() && window->GetSettings().AllowInput)
                {
                    defaultWindow = window;
                    break;
                }
            }
            for (const auto& e : inputEvents)
            {
                auto window = e.Target ? e.Target : defaultWindow;
                if (!window)
                    continue;
                switch (e.Type)
                {
                    // Keyboard events
                case InputDevice::EventType::Char:
                    window->OnCharInput(e.CharData.Char);
                    break;
                case InputDevice::EventType::KeyDown:
                    window->OnKeyDown(e.KeyData.Key);
                    break;
                case InputDevice::EventType::KeyUp:
                    window->OnKeyUp(e.KeyData.Key);
                    break;
                    // Mouse events
                case InputDevice::EventType::MouseDown:
                    window->OnMouseDown(window->ScreenToClient(e.MouseData.Position), e.MouseData.Button);
                    break;
                case InputDevice::EventType::MouseUp:
                    window->OnMouseUp(window->ScreenToClient(e.MouseData.Position), e.MouseData.Button);
                    break;
                case InputDevice::EventType::MouseDoubleClick:
                    window->OnMouseDoubleClick(window->ScreenToClient(e.MouseData.Position), e.MouseData.Button);
                    break;
                case InputDevice::EventType::MouseWheel:
                    window->OnMouseWheel(window->ScreenToClient(e.MouseWheelData.Position), e.MouseWheelData.WheelDelta);
                    break;
                case InputDevice::EventType::MouseMove:
                    window->OnMouseMove(window->ScreenToClient(e.MouseData.Position));
                    break;
                case InputDevice::EventType::MouseLeave:
                    window->OnMouseLeave();
                    break;
                }
            }
            WindowsManager::WindowsLocker.Unlock();
        }
        WindowsManager::WindowsLocker.Lock();
        for (auto& win : WindowsManager::Windows)
        {
            if (win->IsVisible())
                win->OnUpdate(deltaTime);
        }
        WindowsManager::WindowsLocker.Unlock();

        // Draw
        Engine::OnDraw();
    }

    struct VisualScriptLocalManaged
    {
        MonoString* Value;
        MonoString* ValueTypeName;
        uint32 NodeId;
        int32 BoxId;
    };

    static MonoArray* GetVisualScriptLocals()
    {
        MonoArray* result = nullptr;
        const auto stack = VisualScripting::GetThreadStackTop();
        if (stack && stack->Scope)
        {
            const int32 count = stack->Scope->Parameters.Length() + stack->Scope->ReturnedValues.Count();
            const auto mclass = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly->GetClass("FlaxEditor.Editor+VisualScriptLocal");
            ASSERT(mclass);
            result = mono_array_new(mono_domain_get(), mclass->GetNative(), count);
            VisualScriptLocalManaged local;
            local.NodeId = MAX_uint32;
            if (stack->Scope->Parameters.Length() != 0)
            {
                auto s = stack;
                while (s->PreviousFrame && s->PreviousFrame->Scope == stack->Scope)
                    s = s->PreviousFrame;
                if (s)
                    local.NodeId = s->Node->ID;
            }
            for (int32 i = 0; i < stack->Scope->Parameters.Length(); i++)
            {
                auto& v = stack->Scope->Parameters[i];
                local.BoxId = i + 1;
                local.Value = MUtils::ToString(v.ToString());
                local.ValueTypeName = MUtils::ToString(v.Type.GetTypeName());
                mono_array_set(result, VisualScriptLocalManaged, i, local);
            }
            for (int32 i = 0; i < stack->Scope->ReturnedValues.Count(); i++)
            {
                auto& v = stack->Scope->ReturnedValues[i];
                local.NodeId = v.NodeId;
                local.BoxId = v.BoxId;
                local.Value = MUtils::ToString(v.Value.ToString());
                local.ValueTypeName = MUtils::ToString(v.Value.Type.GetTypeName());
                mono_array_set(result, VisualScriptLocalManaged, stack->Scope->Parameters.Length() + i, local);
            }
        }
        return result;
    }

    struct VisualScriptStackFrameManaged
    {
        MonoObject* Script;
        uint32 NodeId;
        int32 BoxId;
    };

    static MonoArray* GetVisualScriptStackFrames()
    {
        MonoArray* result = nullptr;
        const auto stack = VisualScripting::GetThreadStackTop();
        if (stack)
        {
            int32 count = 0;
            auto s = stack;
            while (s)
            {
                s = s->PreviousFrame;
                count++;
            }
            const auto mclass = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly->GetClass("FlaxEditor.Editor+VisualScriptStackFrame");
            ASSERT(mclass);
            result = mono_array_new(mono_domain_get(), mclass->GetNative(), count);
            s = stack;
            count = 0;
            while (s)
            {
                VisualScriptStackFrameManaged frame;
                frame.Script = s->Script->GetOrCreateManagedInstance();
                frame.NodeId = s->Node->ID;
                frame.BoxId = s->Box ? s->Box->ID : MAX_uint32;
                mono_array_set(result, VisualScriptStackFrameManaged, count, frame);
                s = s->PreviousFrame;
                count++;
            }
        }
        return result;
    }

    static VisualScriptStackFrameManaged GetVisualScriptPreviousScopeFrame()
    {
        VisualScriptStackFrameManaged frame;
        Platform::MemoryClear(&frame, sizeof(frame));
        const auto stack = VisualScripting::GetThreadStackTop();
        if (stack)
        {
            auto s = stack;
            while (s->PreviousFrame && s->PreviousFrame->Scope == stack->Scope)
                s = s->PreviousFrame;
            if (s && s->PreviousFrame)
            {
                s = s->PreviousFrame;
                frame.Script = s->Script->GetOrCreateManagedInstance();
                frame.NodeId = s->Node->ID;
                frame.BoxId = s->Box ? s->Box->ID : MAX_uint32;
            }
        }
        return frame;
    }

    static bool EvaluateVisualScriptLocal(VisualScript* script, VisualScriptLocalManaged* local)
    {
        Variant v;
        if (VisualScripting::Evaluate(script, VisualScripting::GetThreadStackTop()->Instance, local->NodeId, local->BoxId, v))
        {
            local->Value = MUtils::ToString(v.ToString());
            local->ValueTypeName = MUtils::ToString(v.Type.GetTypeName());
            return true;
        }
        return false;
    }

    static void DeserializeSceneObject(SceneObject* sceneObject, MonoString* jsonObj)
    {
        StringAnsi json;
        MUtils::ToString(jsonObj, json);

        rapidjson_flax::Document document;
        document.Parse(json.Get(), json.Length());
        if (document.HasParseError())
        {
            Log::JsonParseException(document.GetParseError(), document.GetErrorOffset());
            DebugLog::ThrowException("Failed to parse Json.");
        }

        auto modifier = Cache::ISerializeModifier.Get();
        modifier->EngineBuild = FLAXENGINE_VERSION_BUILD;
        Scripting::ObjectsLookupIdMapping.Set(&modifier.Value->IdsMapping);

        sceneObject->Deserialize(document, modifier.Value);
    }

    static void InitRuntime()
    {
        ADD_INTERNAL_CALL("FlaxEditor.Editor::IsDevInstance", &IsDevInstance);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::IsOfficialBuild", &IsOfficialBuild);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_IsPlayMode", &IsPlayMode);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_ReadOutputLogs", &ReadOutputLogs);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_SetPlayMode", &SetPlayMode);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_GetProjectPath", &GetProjectPath);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_Import", &Import);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_ImportTexture", &ImportTexture);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_ImportModel", &ImportModel);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_ImportAudio", &ImportAudio);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_GetAudioClipMetadata", &GetAudioClipMetadata);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_SaveJsonAsset", &SaveJsonAsset);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_CopyCache", &CopyCache);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_CloneAssetFile", &CloneAssetFile);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_BakeLightmaps", &BakeLightmaps);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_GetShaderAssetSourceCode", &GetShaderAssetSourceCode);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_CookMeshCollision", &CookMeshCollision);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_GetCollisionWires", &GetCollisionWires);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_GetEditorBoxWithChildren", &GetEditorBoxWithChildren);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_SetOptions", &SetOptions);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_DrawNavMesh", &DrawNavMesh);
        ADD_INTERNAL_CALL("FlaxEditor.CustomEditors.CustomEditorsUtil::Internal_GetCustomEditor", &CustomEditorsUtilInternal::GetCustomEditor);
        ADD_INTERNAL_CALL("FlaxEditor.Content.Import.TextureImportEntry::Internal_GetTextureImportOptions", &GetTextureImportOptions);
        ADD_INTERNAL_CALL("FlaxEditor.Content.Import.ModelImportEntry::Internal_GetModelImportOptions", &GetModelImportOptions);
        ADD_INTERNAL_CALL("FlaxEditor.Content.Import.AudioImportEntry::Internal_GetAudioImportOptions", &GetAudioImportOptions);
        ADD_INTERNAL_CALL("FlaxEditor.Content.Settings.LayersAndTagsSettings::GetCurrentTags", &LayersAndTagsSettingsInternal1::GetCurrentTags);
        ADD_INTERNAL_CALL("FlaxEditor.Content.Settings.LayersAndTagsSettings::GetCurrentLayers", &LayersAndTagsSettingsInternal1::GetCurrentLayers);
        ADD_INTERNAL_CALL("FlaxEditor.Content.Settings.GameSettings::Apply", &GameSettingsInternal1::Apply);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_CloseSplashScreen", &CloseSplashScreen);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_CreateAsset", &CreateAsset);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_CreateVisualScript", &CreateVisualScript);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_CanImport", &CanImport);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_CanExport", &CanExport);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_Export", &Export);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_GetIsEveryAssemblyLoaded", &GetIsEveryAssemblyLoaded);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_GetLastProjectOpenedEngineBuild", &GetLastProjectOpenedEngineBuild);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_GetIsCSGActive", &GetIsCSGActive);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_RunVisualScriptBreakpointLoopTick", &RunVisualScriptBreakpointLoopTick);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_GetVisualScriptLocals", &GetVisualScriptLocals);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_GetVisualScriptStackFrames", &GetVisualScriptStackFrames);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_GetVisualScriptPreviousScopeFrame", &GetVisualScriptPreviousScopeFrame);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_EvaluateVisualScriptLocal", &EvaluateVisualScriptLocal);
        ADD_INTERNAL_CALL("FlaxEditor.Editor::Internal_DeserializeSceneObject", &DeserializeSceneObject);
    }
};

IMPLEMENT_SCRIPTING_TYPE_NO_SPAWN_WITH_BASE(ManagedEditor, ScriptingObject, FlaxEngine, "FlaxEditor.Editor", nullptr, nullptr);
