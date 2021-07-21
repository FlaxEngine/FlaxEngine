// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Enums.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Config.h"
#include "Types.h"

#define DECLARE_ASSET_HEADER(type) \
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(type); \
	public: \
	static const String TypeName; \
	const String& GetTypeName() const override { return type::TypeName; } \
	public: \
	explicit type(const SpawnParams& params, const AssetInfo* info)

/// <summary>
/// Asset objects base class.
/// </summary>
API_CLASS(Abstract, NoSpawn) class FLAXENGINE_API Asset : public ManagedScriptingObject
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(Asset);
    friend Content;
    friend LoadAssetTask;
    friend class ContentService;
public:

    /// <summary>
    /// The asset loading result.
    /// </summary>
    DECLARE_ENUM_7(LoadResult, Ok, Failed, MissingDataChunk, CannotLoadData, CannotLoadStorage, CannotLoadInitData, InvalidData);

protected:

    volatile int64 _refCount;
    ContentLoadTask* _loadingTask;

    int8 _isLoaded : 1; // Indicates that asset is loaded
    int8 _loadFailed : 1; // Indicates that last asset loading has failed
    int8 _deleteFileOnUnload : 1; // Indicates that asset source file should be removed on asset unload
    int8 _isVirtual : 1; // Indicates that asset is pure virtual (generated or temporary, has no storage so won't be saved)

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="Asset"/> class.
    /// </summary>
    /// <param name="params">The object initialization parameters.</param>
    /// <param name="info">The asset object information.</param>
    explicit Asset(const SpawnParams& params, const AssetInfo* info);

public:

    typedef Delegate<Asset*> EventType;

    /// <summary>
    /// Action called when asset gets loaded
    /// </summary>
    EventType OnLoaded;

    /// <summary>
    /// Action called when asset start reloading (e.g. after reimport). Always called from the main thread.
    /// </summary>
    EventType OnReloading;

    /// <summary>
    /// Action called when asset gets unloaded
    /// </summary>
    EventType OnUnloaded;

    /// <summary>
    /// General purpose mutex for an asset object. Should guard most of asset functionalities to be secure.
    /// </summary>
    CriticalSection Locker;

public:

    /// <summary>
    /// Gets asset's reference count. Asset will be automatically unloaded when this reaches zero.
    /// </summary>
    API_PROPERTY() int32 GetReferencesCount() const;

    /// <summary>
    /// Adds reference to that asset.
    /// </summary>
    FORCE_INLINE void AddReference()
    {
        Platform::InterlockedIncrement(&_refCount);
    }

    /// <summary>
    /// Removes reference from that asset.
    /// </summary>
    FORCE_INLINE void RemoveReference()
    {
        Platform::InterlockedDecrement(&_refCount);
    }

public:

    /// <summary>
    /// Gets the path to the asset storage file. In Editor it reflects the actual file, in cooked Game, it fakes the Editor path to be informative for developers.
    /// </summary>
    API_PROPERTY() virtual const String& GetPath() const = 0;

    /// <summary>
    /// Gets the asset type name.
    /// </summary>
    virtual const String& GetTypeName() const = 0;

    /// <summary>
    /// Returns true if asset is loaded, otherwise false.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsLoaded() const
    {
        return _isLoaded != 0;
    }

    /// <summary>
    /// Returns true if last asset loading failed, otherwise false.
    /// </summary>
    API_PROPERTY() bool LastLoadFailed() const;

    /// <summary>
    /// Determines whether this asset is virtual (generated or temporary, has no storage so it won't be saved).
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsVirtual() const
    {
        return _isVirtual != 0;
    }

#if USE_EDITOR

    /// <summary>
    /// Determines whether this asset was marked to be deleted on unload.
    /// </summary>
    API_PROPERTY() bool ShouldDeleteFileOnUnload() const;

#endif

public:

    /// <summary>
    /// Reloads the asset.
    /// </summary>
    API_FUNCTION() void Reload();

    /// <summary>
    /// Stops the current thread execution and waits until asset will be loaded (loading will fail, success or be cancelled).
    /// </summary>
    /// <param name="timeoutInMilliseconds">Custom timeout value in milliseconds.</param>
    /// <returns>True if cannot load that asset (failed or has been cancelled), otherwise false.</returns>
    API_FUNCTION() bool WaitForLoaded(double timeoutInMilliseconds = 30000.0);

    /// <summary>
    /// Initializes asset data as virtual asset.
    /// </summary>
    virtual void InitAsVirtual();

#if USE_EDITOR

    /// <summary>
    /// Gets the asset references. Supported only in Editor.
    /// </summary>
    /// <remarks>
    /// For some asset types (e.g. scene or prefab) it may contain invalid asset ids due to not perfect gather method,
    /// which is optimized to perform scan very quickly. Before using those ids perform simple validation via Content cache API.
    /// The result collection contains only 1-level-deep references (only direct ones) and is invalid if asset is not loaded.
    /// Also the output data may have duplicated asset ids or even invalid ids (Guid::Empty).
    /// </remarks>
    /// <param name="output">The output collection of the asset ids referenced by this asset.</param>
    virtual void GetReferences(Array<Guid, HeapAllocation>& output) const;

    /// <summary>
    /// Gets the asset references. Supported only in Editor.
    /// </summary>
    /// <remarks>
    /// For some asset types (e.g. scene or prefab) it may contain invalid asset ids due to not perfect gather method,
    /// which is optimized to perform scan very quickly. Before using those ids perform simple validation via Content cache API.
    /// The result collection contains only 1-level-deep references (only direct ones) and is invalid if asset is not loaded.
    /// Also the output data may have duplicated asset ids or even invalid ids (Guid::Empty).
    /// </remarks>
    /// <returns>The collection of the asset ids referenced by this asset.</returns>
    API_FUNCTION() Array<Guid, HeapAllocation> GetReferences() const;

#endif

    /// <summary>
    /// Deletes the managed object.
    /// </summary>
    void DeleteManaged();

protected:

    /// <summary>
    /// Creates the loading tasks sequence (allows to inject custom tasks to asset loading logic).
    /// </summary>
    /// <returns>First task to call on start loading</returns>
    virtual ContentLoadTask* createLoadingTask();

    /// <summary>
    /// Starts the asset loading.
    /// </summary>
    virtual void startLoading();

    /// <summary>
    /// Releases the storage file/container handle to prevent issues when renaming or moving the asset.
    /// </summary>
    virtual void releaseStorage();

    /// <summary>
    /// Loads asset
    /// </summary>
    /// <returns>Loading result</returns>
    virtual LoadResult loadAsset() = 0;

    /// <summary>
    /// Unloads asset data
    /// </summary>
    /// <param name="isReloading">True if asset is reloading data, otherwise false.</param>
    virtual void unload(bool isReloading) = 0;

protected:

    virtual bool IsInternalType() const;

    bool onLoad(LoadAssetTask* task);
    void onLoaded();
    void onLoaded_MainThread();
    virtual void onUnload_MainThread();
#if USE_EDITOR
    virtual void onRename(const StringView& newPath) = 0;
#endif

public:

    // [ManagedScriptingObject]
    String ToString() const override;
    void OnDeleteObject() override;
    void CreateManaged() override;
    void DestroyManaged() override;
    void OnManagedInstanceDeleted() override;
    void OnScriptingDispose() override;
    void ChangeID(const Guid& newId) override;
};

// Don't include Content.h but just Load method
extern FLAXENGINE_API Asset* LoadAsset(const Guid& id, const ScriptingTypeHandle& type);
