// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ObjectsRemovalService.h"
#include "Collections/Dictionary.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Engine/Time.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Log.h"

namespace ObjectsRemovalServiceImpl
{
    bool IsReady = false;
    CriticalSection PoolLocker;
    CriticalSection NewItemsLocker;
    DateTime LastUpdate;
    float LastUpdateGameTime;
    Dictionary<Object*, float> Pool(8192);
    Dictionary<Object*, float> NewItemsPool(2048);
}

using namespace ObjectsRemovalServiceImpl;

class ObjectsRemovalServiceService : public EngineService
{
public:

    ObjectsRemovalServiceService()
        : EngineService(TEXT("Objects Removal Service"), -1000)
    {
    }

    bool Init() override;
    void LateUpdate() override;
    void Dispose() override;
};

ObjectsRemovalServiceService ObjectsRemovalServiceServiceInstance;

bool ObjectsRemovalService::IsInPool(Object* obj)
{
    if (!IsReady)
        return false;

    {
        ScopeLock lock(NewItemsLocker);
        if (NewItemsPool.ContainsKey(obj))
            return true;
    }

    {
        ScopeLock lock(PoolLocker);
        if (Pool.ContainsKey(obj))
            return true;
    }

    return false;
}

bool ObjectsRemovalService::HasNewItemsForFlush()
{
    NewItemsLocker.Lock();
    const bool result = NewItemsPool.HasItems();
    NewItemsLocker.Unlock();

    return result;
}

void ObjectsRemovalService::Dereference(Object* obj)
{
    if (!IsReady)
        return;

    NewItemsLocker.Lock();
    NewItemsPool.Remove(obj);
    NewItemsLocker.Unlock();

    PoolLocker.Lock();
    Pool.Remove(obj);
    PoolLocker.Unlock();
}

void ObjectsRemovalService::Add(Object* obj, float timeToLive, bool useGameTime)
{
    ScopeLock lock(NewItemsLocker);

    obj->Flags |= ObjectFlags::WasMarkedToDelete;
    if (useGameTime)
        obj->Flags |= ObjectFlags::UseGameTimeForDelete;
    else
        obj->Flags &= ~ObjectFlags::UseGameTimeForDelete;
    NewItemsPool[obj] = timeToLive;
}

void ObjectsRemovalService::Flush(float dt, float gameDelta)
{
    // Add new items
    {
        ScopeLock lock(NewItemsLocker);

        for (auto i = NewItemsPool.Begin(); i.IsNotEnd(); ++i)
        {
            Pool[i->Key] = i->Value;
        }
        NewItemsPool.Clear();
    }

    // Update timeouts and delete objects that timed out
    {
        ScopeLock lock(PoolLocker);

        for (auto i = Pool.Begin(); i.IsNotEnd(); ++i)
        {
            auto obj = i->Key;
            const float ttl = i->Value - (obj->Flags & ObjectFlags::UseGameTimeForDelete ? gameDelta : dt);
            if (ttl <= ZeroTolerance)
            {
                Pool.Remove(i);

#if BUILD_DEBUG || BUILD_DEVELOPMENT
                if (NewItemsPool.ContainsKey(obj))
                {
                    const auto asScriptingObj = dynamic_cast<ScriptingObject*>(obj);
                    if (asScriptingObj)
                    {
                        LOG(Warning, "Object {0} was marked to delete after delete timeout", asScriptingObj->GetID());
                    }
                }
#endif
                NewItemsPool.Remove(obj);
                //ASSERT(!NewItemsPool.ContainsKey(obj));

                obj->OnDeleteObject();
            }
            else
            {
                i->Value = ttl;
            }
        }
    }

    // Perform removing in loop
    // Note: objects during OnDeleteObject call can register new objects to remove with timeout=0, for example Actors do that to remove children and scripts
    while (HasNewItemsForFlush())
    {
        // Add new items
        {
            ScopeLock lock(NewItemsLocker);

            for (auto i = NewItemsPool.Begin(); i.IsNotEnd(); ++i)
            {
                Pool[i->Key] = i->Value;
            }
            NewItemsPool.Clear();
        }

        // Delete objects that timed out
        {
            ScopeLock lock(PoolLocker);

            for (auto i = Pool.Begin(); i.IsNotEnd(); ++i)
            {
                if (i->Value <= ZeroTolerance)
                {
                    auto obj = i->Key;
                    Pool.Remove(i);
                    ASSERT(!NewItemsPool.ContainsKey(obj));
                    obj->OnDeleteObject();
                }
            }
        }
    }
}

bool ObjectsRemovalServiceService::Init()
{
    LastUpdate = DateTime::NowUTC();
    LastUpdateGameTime = 0;
    return false;
}

void ObjectsRemovalServiceService::LateUpdate()
{
    PROFILE_CPU();

    // Delete all objects
    const auto now = DateTime::NowUTC();
    const float dt = (now - LastUpdate).GetTotalSeconds();
    float gameDelta = Time::Update.DeltaTime.GetTotalSeconds();
    if (Time::GetGamePaused())
        gameDelta = 0;
    ObjectsRemovalService::Flush(dt, gameDelta);
    LastUpdate = now;
}

void ObjectsRemovalServiceService::Dispose()
{
    // Collect new objects
    ObjectsRemovalService::Flush();

    // Delete all remaining objects
    {
        ScopeLock lock(PoolLocker);
        for (auto i = Pool.Begin(); i.IsNotEnd(); ++i)
        {
            auto obj = i->Key;
            Pool.Remove(i);
            obj->OnDeleteObject();
        }
        Pool.Clear();
    }

    IsReady = false;
}

Object::~Object()
{
#if BUILD_DEBUG
    // Prevent removing object that is still reverenced by the removal service
    ASSERT(!ObjectsRemovalService::IsInPool(this));
#endif
}

void Object::DeleteObjectNow()
{
    ObjectsRemovalService::Dereference(this);

    OnDeleteObject();
}

void Object::DeleteObject(float timeToLive, bool useGameTime)
{
    // Add to deferred remove (or just update timeout but don't remove object here)
    ObjectsRemovalService::Add(this, timeToLive, useGameTime);
}
