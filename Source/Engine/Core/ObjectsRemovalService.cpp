// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "ObjectsRemovalService.h"
#include "Collections/Dictionary.h"
#include "Engine/Engine/Time.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/ScriptingObject.h"

namespace ObjectsRemovalServiceImpl
{
    CriticalSection PoolLocker;
    DateTime LastUpdate;
    float LastUpdateGameTime;
    Dictionary<Object*, float> Pool(8192);
}

using namespace ObjectsRemovalServiceImpl;

class ObjectsRemoval : public EngineService
{
public:
    ObjectsRemoval()
        : EngineService(TEXT("Objects Removal Service"), -1000)
    {
    }

    bool Init() override;
    void LateUpdate() override;
    void Dispose() override;
};

ObjectsRemoval ObjectsRemovalInstance;

bool ObjectsRemovalService::IsInPool(Object* obj)
{
    PoolLocker.Lock();
    const bool result = Pool.ContainsKey(obj);
    PoolLocker.Unlock();
    return result;
}

void ObjectsRemovalService::Dereference(Object* obj)
{
    PoolLocker.Lock();
    Pool.Remove(obj);
    PoolLocker.Unlock();
}

void ObjectsRemovalService::Add(Object* obj, float timeToLive, bool useGameTime)
{
    obj->Flags |= ObjectFlags::WasMarkedToDelete;
    if (useGameTime)
        obj->Flags |= ObjectFlags::UseGameTimeForDelete;
    else
        obj->Flags &= ~ObjectFlags::UseGameTimeForDelete;

    PoolLocker.Lock();
    Pool[obj] = timeToLive;
    PoolLocker.Unlock();
}

void ObjectsRemovalService::Flush(float dt, float gameDelta)
{
    PROFILE_CPU();

    PoolLocker.Lock();

    int32 itemsLeft;
    do
    {
        // Update timeouts and delete objects that timed out
        itemsLeft = Pool.Count();
        for (auto i = Pool.Begin(); i.IsNotEnd(); ++i)
        {
            Object* obj = i->Key;
            const float ttl = i->Value - ((obj->Flags & ObjectFlags::UseGameTimeForDelete) != ObjectFlags::None ? gameDelta : dt);
            if (ttl <= 0.0f)
            {
                Pool.Remove(i);
                obj->OnDeleteObject();
                itemsLeft--;
            }
            else
            {
                i->Value = ttl;
            }
        }
    }
    while (itemsLeft != Pool.Count()); // Continue removing if any new item was added during removing (eg. sub-object delete with 0 timeout)

    PoolLocker.Unlock();
}

bool ObjectsRemoval::Init()
{
    LastUpdate = DateTime::NowUTC();
    LastUpdateGameTime = 0;
    return false;
}

void ObjectsRemoval::LateUpdate()
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

void ObjectsRemoval::Dispose()
{
    // Collect new objects
    ObjectsRemovalService::Flush();

    // Delete all remaining objects
    {
        ScopeLock lock(PoolLocker);
        for (auto i = Pool.Begin(); i.IsNotEnd(); ++i)
        {
            Object* obj = i->Key;
            Pool.Remove(i);
            obj->OnDeleteObject();
        }
        Pool.Clear();
    }
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
