// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "GPUResourcesCollection.h"
#include "GPUResource.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Core/Utilities.h"

uint64 GPUResourcesCollection::GetMemoryUsage() const
{
    uint64 result = 0;
    _locker.Lock();

    for (int32 i = 0; i < _collection.Count(); i++)
        result += _collection[i]->GetMemoryUsage();

    _locker.Unlock();
    return result;
}

void GPUResourcesCollection::OnDeviceDispose()
{
    _locker.Lock();

    for (int32 i = _collection.Count() - 1; i >= 0 && i < _collection.Count(); i--)
    {
        _collection[i]->OnDeviceDispose();
    }
    _collection.Clear();

    _locker.Unlock();
}

void GPUResourcesCollection::DumpToLog() const
{
    StringBuilder sb;
    DumpToLog(sb);
    LOG_STR(Info, sb.ToString());
}

void GPUResourcesCollection::DumpToLog(StringBuilder& output) const
{
    _locker.Lock();

    output.AppendFormat(TEXT("GPU Resources dump. Count: {0}, total GPU memory used: {1}"), _collection.Count(), Utilities::BytesToText(GetMemoryUsage()));
    output.AppendLine();
    output.AppendLine();

    for (int32 typeIndex = 0; typeIndex < GPUResource::ResourceType_Count; typeIndex++)
    {
        const auto type = static_cast<GPUResource::ResourceType>(typeIndex);

        output.AppendFormat(TEXT("Group: {0}s"), GPUResource::ToString(type));
        output.AppendLine();

        int32 count = 0;
        uint64 memUsage = 0;
        for (int32 i = 0; i < _collection.Count(); i++)
        {
            const auto resource = _collection[i];
            if (resource->GetResourceType() == type)
            {
                count++;
                memUsage += resource->GetMemoryUsage();
                auto str = resource->ToString();
                if (str.HasChars())
                {
                    output.Append(TEXT('\t'));
                    output.Append(str);
                    output.AppendLine();
                }
            }
        }

        output.AppendFormat(TEXT("Total count: {0}, memory usage: {1}"), count, Utilities::BytesToText(memUsage));
        output.AppendLine();
        output.AppendLine();
    }

    _locker.Unlock();
}

void GPUResourcesCollection::Add(GPUResource* resource)
{
    _locker.Lock();

    ASSERT(resource && _collection.Contains(resource) == false);
    _collection.Add(resource);

    _locker.Unlock();
}

void GPUResourcesCollection::Remove(GPUResource* resource)
{
    _locker.Lock();

    ASSERT(resource && _collection.Contains(resource) == true);
    _collection.Remove(resource);

    _locker.Unlock();
}
