// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Core/ObjectsRemovalService.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Async/GPUTask.h"
#include "Engine/Graphics/Async/GPUTasksManager.h"
#include <ThirdParty/catch2/catch.hpp>

namespace
{
    int32 DeletedTasks = 0;

    class TestGPUTask : public GPUTask
    {
    public:
        TestGPUTask()
            : GPUTask(Type::Custom)
        {
        }

        ~TestGPUTask()
        {
            DeletedTasks++;
        }

    protected:
        Result run(GPUTasksContext* context) override
        {
            return Result::Ok;
        }
    };
}

TEST_CASE("GPUTasks")
{
    SECTION("Test task canceled while queued")
    {
        // A task that ends while the tasks manager still lists it must stay alive until the manager drops it
        // (the manager reads its state when it takes it from the queue)
        REQUIRE(GPUDevice::Instance != nullptr);
        DeletedTasks = 0;
        auto task = New<TestGPUTask>();
        task->Start();
        task->Cancel();

        // The time to live of an ended task (30 seconds) runs out while the task is still in the queue
        ObjectsRemovalService::Flush(60.0f, 60.0f);
        REQUIRE(DeletedTasks == 0);

        // The manager drops the canceled task, then it is deleted with the next flush
        GPUTask* work[32];
        const int32 count = GPUDevice::Instance->GetTasksManager()->RequestWork(work, ARRAY_COUNT(work));
        CHECK(count == 0);
        ObjectsRemovalService::Flush(60.0f, 60.0f);
        CHECK(DeletedTasks == 1);
    }
}
