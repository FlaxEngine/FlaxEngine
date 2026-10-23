// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Graphics/GPUResource.h"
#include "Engine/Graphics/GPUResourceProperty.h"
#include "Engine/Graphics/Async/Tasks/GPUCopyResourceTask.h"
#include <ThirdParty/catch2/catch.hpp>

namespace
{
    // Test GPU resource that has memory (so ReleaseGPU releases it and raises Releasing) without a GPU device.
    class TestGPUResource : public GPUResource
    {
    public:
        TestGPUResource()
            : GPUResource(SpawnParams(Guid::New(), GPUResource::TypeInitializer))
        {
            _memoryUsage = 64;
        }

        GPUResourceType GetResourceType() const override
        {
            return GPUResourceType::Buffer;
        }
    };

    int32 ReleasedCount = 0;

    void OnReleased()
    {
        ReleasedCount++;
    }
}

TEST_CASE("GPUResourceProperty")
{
    SECTION("Test Released")
    {
        // The property clears its reference and tells its owner when the resource is released
        auto resource = New<TestGPUResource>();
        GPUResourceProperty<GPUResource> property;
        ReleasedCount = 0;
        property.Released.Bind<&OnReleased>();
        property = resource;
        resource->ReleaseGPU();
        CHECK(property.Get() == nullptr);
        CHECK(ReleasedCount == 1);
        resource->DeleteObjectNow();
    }
    SECTION("Test GPU task cancel")
    {
        // A GPU task cancels itself when its resource is released (else it stays queued and fails with MissingResources)
        auto source = New<TestGPUResource>();
        auto destination = New<TestGPUResource>();
        auto task = New<GPUCopyResourceTask>(source, destination);
        source->ReleaseGPU();
        CHECK(task->IsCanceled());
        task->Cancel(); // Ends and deletes the task (later) if it is still there
        source->DeleteObjectNow();
        destination->DeleteObjectNow();
    }
}
