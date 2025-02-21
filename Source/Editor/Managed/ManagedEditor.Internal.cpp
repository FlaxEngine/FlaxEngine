// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "ManagedEditor.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/WindowsManager.h"
#include "Engine/ContentImporters/AssetsImportingManager.h"
#include "Engine/ContentExporters/AssetsExportingManager.h"
#include "Editor/CustomEditors/CustomEditorsUtil.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/Internal/InternalCalls.h"
#include "Engine/Scripting/Internal/StdTypesContainer.h"
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
#include "Engine/Level/Actor.h"
#include "Engine/Level/Prefabs/Prefab.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Core/Config/GraphicsSettings.h"
#include "Engine/Core/Cache.h"
#include "Engine/CSG/CSGBuilder.h"
#include "Engine/Debug/DebugLog.h"
#include "Engine/Debug/Exceptions/JsonParseException.h"
#include "Engine/Audio/AudioClip.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Utilities/Encryption.h"
#include "Engine/Navigation/Navigation.h"
#include "Engine/Particles/ParticleEmitter.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Input/Keyboard.h"
#include "Engine/Threading/Threading.h"
#include "FlaxEngine.Gen.h"
#include "Engine/Level/Actors/AnimatedModel.h"
#include "Engine/Serialization/JsonTools.h"

Guid ManagedEditor::ObjectID(0x91970b4e, 0x99634f61, 0x84723632, 0x54c776af);

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

DEFINE_INTERNAL_CALL(bool) EditorInternal_IsDevInstance()
{
#if COMPILE_WITH_DEV_ENV
    return true;
#else
	return false;
#endif
}

DEFINE_INTERNAL_CALL(bool) EditorInternal_IsOfficialBuild()
{
#if OFFICIAL_BUILD
    return true;
#else
    return false;
#endif
}

DEFINE_INTERNAL_CALL(bool) EditorInternal_IsPlayMode()
{
    return Editor::IsPlayMode;
}

DEFINE_INTERNAL_CALL(int32) EditorInternal_ReadOutputLogs(MArray** outMessages, MArray** outLogTypes, MArray** outLogTimes, int outArraySize)
{
    ScopeLock lock(CachedLogDataLocker);
    if (CachedLogData.IsEmpty() || CachedLogData.Get() == nullptr)
        return 0;

    int32 count = 0;
    const int32 maxCount = outArraySize;

    byte* ptr = CachedLogData.Get();
    byte* end = ptr + CachedLogData.Count();
    byte* outLogTypesPtr = MCore::Array::GetAddress<byte>(*outLogTypes);
    int64* outLogTimesPtr = MCore::Array::GetAddress<int64>(*outLogTimes);
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

        MCore::GC::WriteArrayRef(*outMessages, (MObject*)msgObj, count);
        outLogTypesPtr[count] = type;
        outLogTimesPtr[count] = time;

        count++;
    }

    const int32 dataLeft = (int32)(end - ptr);
    Platform::MemoryCopy(CachedLogData.Get(), ptr, dataLeft);
    CachedLogData.Resize(dataLeft);

    return count;
}

DEFINE_INTERNAL_CALL(void) EditorInternal_SetPlayMode(bool value)
{
    Editor::IsPlayMode = value;
}

DEFINE_INTERNAL_CALL(MString*) EditorInternal_GetProjectPath()
{
    return MUtils::ToString(Editor::Project->ProjectPath);
}

DEFINE_INTERNAL_CALL(void) EditorInternal_CloseSplashScreen()
{
    Editor::CloseSplashScreen();
}

DEFINE_INTERNAL_CALL(bool) EditorInternal_CloneAssetFile(MString* dstPathObj, MString* srcPathObj, Guid* dstId)
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

DEFINE_INTERNAL_CALL(bool) EditorInternal_CreateVisualScript(MString* outputPathObj, MString* baseTypenameObj)
{
    String outputPath;
    MUtils::ToString(outputPathObj, outputPath);
    FileSystem::NormalizePath(outputPath);
    String baseTypename;
    MUtils::ToString(baseTypenameObj, baseTypename);
    return AssetsImportingManager::Create(AssetsImportingManager::CreateVisualScriptTag, outputPath, &baseTypename);
}

DEFINE_INTERNAL_CALL(MString*) EditorInternal_CanImport(MString* extensionObj)
{
    String extension;
    MUtils::ToString(extensionObj, extension);
    if (extension.Length() > 0 && extension[0] == '.')
        extension.Remove(0, 1);
    const AssetImporter* importer = AssetsImportingManager::GetImporter(extension);
    return importer ? MUtils::ToString(importer->ResultExtension) : nullptr;
}

DEFINE_INTERNAL_CALL(void) EditorInternal_GetAudioClipMetadata(AudioClip* clip, int32* originalSize, int32* importedSize)
{
    INTERNAL_CALL_CHECK(clip);
    *originalSize = clip->AudioHeader.OriginalSize;
    *importedSize = clip->AudioHeader.ImportedSize;
}

DEFINE_INTERNAL_CALL(bool) EditorInternal_SaveJsonAsset(MString* outputPathObj, MString* dataObj, MString* dataTypeNameObj)
{
    String outputPath;
    MUtils::ToString(outputPathObj, outputPath);
    FileSystem::NormalizePath(outputPath);

    const StringView dataObjChars = MCore::String::GetChars(dataObj);
    const StringAsUTF8<> data(dataObjChars.Get(), dataObjChars.Length());
    const StringAnsiView dataAnsi(data.Get(), data.Length());

    const StringView dataTypeNameObjChars = MCore::String::GetChars(dataTypeNameObj);
    const StringAsANSI<> dataTypeName(dataTypeNameObjChars.Get(), dataTypeNameObjChars.Length());
    const StringAnsiView dataTypeNameAnsi(dataTypeName.Get(), dataTypeName.Length());

    return CreateJson::Create(outputPath, dataAnsi, dataTypeNameAnsi);
}

DEFINE_INTERNAL_CALL(bool) EditorInternal_CanExport(MString* pathObj)
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

DEFINE_INTERNAL_CALL(bool) EditorInternal_Export(MString* inputPathObj, MString* outputFolderObj)
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

DEFINE_INTERNAL_CALL(void) EditorInternal_CopyCache(Guid* dstId, Guid* srcId)
{
    ShaderCacheManager::CopyCache(*dstId, *srcId);
}

DEFINE_INTERNAL_CALL(void) EditorInternal_BakeLightmaps(bool cancel)
{
    auto builder = ShadowsOfMordor::Builder::Instance();
    if (cancel)
        builder->CancelBuild();
    else
        builder->Build();
}

DEFINE_INTERNAL_CALL(MString*) EditorInternal_GetShaderAssetSourceCode(BinaryAsset* obj)
{
    INTERNAL_CALL_CHECK_RETURN(obj, nullptr);
    if (obj->WaitForLoaded())
        DebugLog::ThrowNullReference();
    auto lock = obj->Storage->Lock();
    if (obj->LoadChunk(SHADER_FILE_CHUNK_SOURCE))
        return nullptr;

    // Decrypt source code
    BytesContainer data;
    obj->GetChunkData(SHADER_FILE_CHUNK_SOURCE, data);
    auto source = data.Get<char>();
    auto sourceLength = data.Length();
    if (sourceLength <= 0)
        return MCore::String::GetEmpty();
    Encryption::DecryptBytes(data.Get(), data.Length());
    source[sourceLength - 1] = 0;

    // Get source and encrypt it back
    const StringAnsiView srcData(source, sourceLength);
    const auto str = MUtils::ToString(srcData);
    Encryption::EncryptBytes(data.Get(), data.Length());

    return str;
}

DEFINE_INTERNAL_CALL(bool) EditorInternal_CookMeshCollision(MString* pathObj, CollisionDataType type, ModelBase* modelObj, int32 modelLodIndex, uint32 materialSlotsMask, ConvexMeshGenerationFlags convexFlags, int32 convexVertexLimit)
{
#if COMPILE_WITH_PHYSICS_COOKING
    CollisionCooking::Argument arg;
    String path;
    MUtils::ToString(pathObj, path);
    FileSystem::NormalizePath(path);
    arg.Type = type;
    arg.Model = modelObj;
    arg.ModelLodIndex = modelLodIndex;
    arg.MaterialSlotsMask = materialSlotsMask;
    arg.ConvexFlags = convexFlags;
    arg.ConvexVertexLimit = convexVertexLimit;
    return CreateCollisionData::CookMeshCollision(path, arg);
#else
	LOG(Warning, "Collision cooking is disabled.");
	return true;
#endif
}

DEFINE_INTERNAL_CALL(void) EditorInternal_GetCollisionWires(CollisionData* collisionData, MArray** triangles, MArray** indices, int* trianglesCount, int* indicesCount)
{
    if (!collisionData || collisionData->WaitForLoaded() || collisionData->GetOptions().Type == CollisionDataType::None)
        return;

    const auto& debugLines = collisionData->GetDebugLines();

    const int32 linesCount = debugLines.Count() / 2;
    MCore::GC::WriteRef(triangles, (MObject*)MCore::Array::New(Float3::TypeInitializer.GetClass(), debugLines.Count()));
    MCore::GC::WriteRef(indices, (MObject*)MCore::Array::New(MCore::TypeCache::Int32, linesCount * 3));

    // Use one triangle per debug line
    Platform::MemoryCopy(MCore::Array::GetAddress<Float3>(*triangles), debugLines.Get(), debugLines.Count() * sizeof(Float3));
    int32 iI = 0;
    int32* indicesPtr = MCore::Array::GetAddress<int32>(*indices);
    for (int32 i = 0; i < debugLines.Count(); i += 2)
    {
        indicesPtr[iI++] = i;
        indicesPtr[iI++] = i + 1;
        indicesPtr[iI++] = i;
    }
    *trianglesCount = debugLines.Count();
    *indicesCount = linesCount * 3;
}

DEFINE_INTERNAL_CALL(void) EditorInternal_GetEditorBoxWithChildren(Actor* obj, BoundingBox* result)
{
    INTERNAL_CALL_CHECK(obj);
    *result = obj->GetEditorBoxChildren();
}

DEFINE_INTERNAL_CALL(void) EditorInternal_SetOptions(ManagedEditor::InternalOptions* options)
{
    ManagedEditor::ManagedEditorOptions = *options;

    // Apply options
    AssetsImportingManager::UseImportPathRelative = ManagedEditor::ManagedEditorOptions.UseAssetImportPathRelative != 0;
}

DEFINE_INTERNAL_CALL(void) EditorInternal_DrawNavMesh()
{
    Navigation::DrawNavMesh();
}

DEFINE_INTERNAL_CALL(bool) EditorInternal_GetIsEveryAssemblyLoaded()
{
    return Scripting::IsEveryAssemblyLoaded();
}

DEFINE_INTERNAL_CALL(int32) EditorInternal_GetLastProjectOpenedEngineBuild()
{
    return Editor::LastProjectOpenedEngineBuild;
}

DEFINE_INTERNAL_CALL(bool) EditorInternal_GetIsCSGActive()
{
#if COMPILE_WITH_CSG_BUILDER
    return CSG::Builder::IsActive();
#else
    return false;
#endif
}

DEFINE_INTERNAL_CALL(void) EditorInternal_RunVisualScriptBreakpointLoopTick(float deltaTime)
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
    Array<Window*, InlinedAllocation<32>> windows;
    windows.Add(WindowsManager::Windows);
    for (Window* win : windows)
    {
        if (win->IsVisible())
            win->OnUpdate(deltaTime);
    }
    WindowsManager::WindowsLocker.Unlock();

    // Draw
    Engine::OnDraw();
}

DEFINE_INTERNAL_CALL(void) EditorInternal_DeserializeSceneObject(SceneObject* sceneObject, MString* jsonObj)
{
    PROFILE_CPU_NAMED("DeserializeSceneObject");

    StringAnsi json;
    MUtils::ToString(jsonObj, json);

    rapidjson_flax::Document document;
    {
        PROFILE_CPU_NAMED("Json.Parse");
        document.Parse(json.Get(), json.Length());
    }
    if (document.HasParseError())
    {
        Log::JsonParseException(document.GetParseError(), document.GetErrorOffset());
        DebugLog::ThrowException("Failed to parse Json.");
    }

    auto modifier = Cache::ISerializeModifier.Get();
    modifier->EngineBuild = FLAXENGINE_VERSION_BUILD;
    Scripting::ObjectsLookupIdMapping.Set(&modifier.Value->IdsMapping);

    {
        PROFILE_CPU_NAMED("Deserialize");
        sceneObject->Deserialize(document, modifier.Value);
    }
}

DEFINE_INTERNAL_CALL(void) EditorInternal_LoadAsset(Guid* id)
{
    Content::LoadAsync<Asset>(*id);
}

DEFINE_INTERNAL_CALL(bool) EditorInternal_CanSetToRoot(Prefab* prefab, Actor* targetActor)
{
    // Reference: Prefab::ApplyAll(Actor* targetActor)
    if (targetActor->GetPrefabID() != prefab->GetID())
        return false;
    if (targetActor->GetPrefabObjectID() != prefab->GetRootObjectId())
    {
        const ISerializable::DeserializeStream** newRootDataPtr = prefab->ObjectsDataCache.TryGet(targetActor->GetPrefabObjectID());
        if (!newRootDataPtr || !*newRootDataPtr)
            return false;
        const ISerializable::DeserializeStream& newRootData = **newRootDataPtr;
        Guid prefabId, prefabObjectID;
        if (JsonTools::GetGuidIfValid(prefabId, newRootData, "PrefabID") && 
            JsonTools::GetGuidIfValid(prefabObjectID, newRootData, "PrefabObjectID"))
        {
            const auto nestedPrefab = Content::Load<Prefab>(prefabId);
            if (nestedPrefab && nestedPrefab->GetRootObjectId() != prefabObjectID)
                return false;
        }
    }
    return true;
}

DEFINE_INTERNAL_CALL(float) EditorInternal_GetAnimationTime(AnimatedModel* animatedModel)
{
    return animatedModel && animatedModel->GraphInstance.State.Count() == 1 ? animatedModel->GraphInstance.State[0].Animation.TimePosition : 0.0f;
}

DEFINE_INTERNAL_CALL(void) EditorInternal_SetAnimationTime(AnimatedModel* animatedModel, float time)
{
    if (animatedModel && animatedModel->GraphInstance.State.Count() == 1)
        animatedModel->GraphInstance.State[0].Animation.TimePosition = time;
}

DEFINE_INTERNAL_CALL(MTypeObject*) CustomEditorsUtilInternal_GetCustomEditor(MTypeObject* targetType)
{
    return CustomEditorsUtil::GetCustomEditor(targetType);
}

DEFINE_INTERNAL_CALL(MArray*) LayersAndTagsSettingsInternal_GetCurrentLayers(int* layersCount)
{
    *layersCount = Math::Max(1, Level::GetNonEmptyLayerNamesCount());
    return MUtils::ToArray(Span<String>(Level::Layers, *layersCount));
}

DEFINE_INTERNAL_CALL(void) GameSettingsInternal_Apply()
{
    LOG(Info, "Apply game settings");
    GameSettings::Load();
}

bool ManagedEditor::Import(String inputPath, String outputPath, void* arg)
{
    FileSystem::NormalizePath(inputPath);
    FileSystem::NormalizePath(outputPath);
    return AssetsImportingManager::Import(inputPath, outputPath, arg);
}

bool ManagedEditor::Import(const String& inputPath, const String& outputPath, const TextureTool::Options& options)
{
    return Import(inputPath, outputPath, (void*)&options);
}

bool ManagedEditor::TryRestoreImportOptions(TextureTool::Options& options, String assetPath)
{
    FileSystem::NormalizePath(assetPath);
    return ImportTexture::TryGetImportOptions(assetPath, options);
}

bool ManagedEditor::Import(const String& inputPath, const String& outputPath, const ModelTool::Options& options)
{
    return Import(inputPath, outputPath, (void*)&options);
}

bool ManagedEditor::TryRestoreImportOptions(ModelTool::Options& options, String assetPath)
{
    // Initialize defaults
    if (const auto* graphicsSettings = GraphicsSettings::Get())
    {
        options.GenerateSDF = graphicsSettings->GenerateSDFOnModelImport;
    }
    FileSystem::NormalizePath(assetPath);
    return ImportModel::TryGetImportOptions(assetPath, options);
}

bool ManagedEditor::Import(const String& inputPath, const String& outputPath, const AudioTool::Options& options)
{
    return Import(inputPath, outputPath, (void*)&options);
}

bool ManagedEditor::TryRestoreImportOptions(AudioTool::Options& options, String assetPath)
{
    FileSystem::NormalizePath(assetPath);
    return ImportAudio::TryGetImportOptions(assetPath, options);
}

bool ManagedEditor::CreateAsset(const String& tag, String outputPath)
{
    FileSystem::NormalizePath(outputPath);
    return AssetsImportingManager::Create(tag, outputPath);
}

Array<Guid> ManagedEditor::GetAssetReferences(const Guid& assetId)
{
    Array<Guid> result;
    if (auto* asset = Content::Load<Asset>(assetId))
    {
        Array<String> files;
        asset->GetReferences(result, files);
    }
    return result;
}
