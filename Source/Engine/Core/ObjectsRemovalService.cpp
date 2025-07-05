// Copyright (c) Wojciech Figat. All rights reserved.

#include "ObjectsRemovalService.h"
#include "Utilities.h"
#include "Collections/Dictionary.h"
#include "Engine/Engine/Time.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/ScriptingObject.h"

const Char* BytesSizesData[] = { TEXT("b"), TEXT("Kb"), TEXT("Mb"), TEXT("Gb"), TEXT("Tb"), TEXT("Pb"), TEXT("Eb"), TEXT("Zb"), TEXT("Yb") };
const Char* HertzSizesData[] = { TEXT("Hz"), TEXT("KHz"), TEXT("MHz"), TEXT("GHz"), TEXT("THz"), TEXT("PHz"), TEXT("EHz"), TEXT("ZHz"), TEXT("YHz") };
Span<const Char*> Utilities::Private::BytesSizes(BytesSizesData, ARRAY_COUNT(BytesSizesData));
Span<const Char*> Utilities::Private::HertzSizes(HertzSizesData, ARRAY_COUNT(HertzSizesData));

namespace
{
    CriticalSection PoolLocker;
    double LastUpdate;
    float LastUpdateGameTime;
    Dictionary<Object*, float> Pool(8192);
    uint64 PoolCounter = 0;
}

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
    PoolCounter++;
    PoolLocker.Unlock();
}

void ObjectsRemovalService::Flush(float dt, float gameDelta)
{
    PROFILE_CPU();

    PoolLocker.Lock();
    PoolCounter = 0;

    // Update timeouts and delete objects that timed out
    for (auto i = Pool.Begin(); i.IsNotEnd(); ++i)
    {
        auto& bucket = *i;
        Object* obj = bucket.Key;
        const float ttl = bucket.Value - ((obj->Flags & ObjectFlags::UseGameTimeForDelete) != ObjectFlags::None ? gameDelta : dt);
        if (ttl <= 0.0f)
        {
            Pool.Remove(i);
            obj->OnDeleteObject();
        }
        else
        {
            bucket.Value = ttl;
        }
    }

    // If any object was added to the pool while removing objects (by this thread) then retry removing any nested objects (but without delta time)
    if (PoolCounter != 0)
    {
    RETRY:
        PoolCounter = 0;
        for (auto i = Pool.Begin(); i.IsNotEnd(); ++i)
        {
            if (i->Value <= 0.0f)
            {
                Object* obj = i->Key;
                Pool.Remove(i);
                obj->OnDeleteObject();
            }
        }
        if (PoolCounter != 0)
            goto RETRY;
    }

    PoolLocker.Unlock();
}

bool ObjectsRemoval::Init()
{
    LastUpdate = Platform::GetTimeSeconds();
    LastUpdateGameTime = 0;
    return false;
}

void ObjectsRemoval::LateUpdate()
{
    PROFILE_CPU();

    // Delete all objects
    const double now = Platform::GetTimeSeconds();
    const float dt = (float)(now - LastUpdate);
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
        PoolLocker.Lock();
        for (auto i = Pool.Begin(); i.IsNotEnd(); ++i)
        {
            Object* obj = i->Key;
            Pool.Remove(i);
            obj->OnDeleteObject();
        }
        Pool.Clear();
        PoolLocker.Unlock();
    }
}

Object::~Object()
{
#if BUILD_DEBUG && 0
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
