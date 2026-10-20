// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Core/Collections/Array.h"
#include "Engine/Level/SceneAction.h"
#include <ThirdParty/catch2/catch.hpp>

namespace
{
    // Test scene action: waits for the given number of runs, then succeeds. Logs its runs and its deletion.
    class TestSceneAction : public SceneAction
    {
    public:
        int32 Id;
        int32 WaitRuns;
        bool Preloading;
        Array<int32>* Runs;
        Array<int32>* Deleted;

        TestSceneAction(int32 id, int32 waitRuns, bool preloading, Array<int32>* runs, Array<int32>* deleted)
            : Id(id)
            , WaitRuns(waitRuns)
            , Preloading(preloading)
            , Runs(runs)
            , Deleted(deleted)
        {
        }

        ~TestSceneAction() override
        {
            Deleted->Add(Id);
        }

        SceneResult Do(Context& context) override
        {
            Runs->Add(Id);
            return WaitRuns-- > 0 ? SceneResult::Wait : SceneResult::Success;
        }

        bool IsPreloading(const Guid* sceneId) override
        {
            return Preloading;
        }
    };
}

TEST_CASE("SceneAction")
{
    SECTION("Test RunActions")
    {
        Array<int32> runs, deleted;
        Array<SceneAction*> actions;
        actions.Add(New<TestSceneAction>(1, 1, false, &runs, &deleted));
        actions.Add(New<TestSceneAction>(2, 0, false, &runs, &deleted));

        // Frame 1: action 1 waits, so action 2 waits too (the actions run in order)
        SceneAction::Context context;
        SceneAction::RunActions(actions, context);
        CHECK(runs == Array<int32>({ 1 }));
        CHECK(deleted.Count() == 0);
        REQUIRE(actions.Count() == 2); // Before the fix: action 2 removed without a run, action 1 deleted but kept in the queue

        // Frame 2: action 1 is done, then action 2 runs and is done
        context = SceneAction::Context();
        SceneAction::RunActions(actions, context);
        CHECK(runs == Array<int32>({ 1, 1, 2 }));
        CHECK(deleted == Array<int32>({ 1, 2 }));
        CHECK(actions.Count() == 0);
        actions.ClearDelete();
    }
    SECTION("Test RemovePreloadingActions")
    {
        // Two preloading actions next to each other, then another action
        Array<int32> runs, deleted;
        Array<SceneAction*> actions;
        actions.Add(New<TestSceneAction>(1, 0, true, &runs, &deleted));
        actions.Add(New<TestSceneAction>(2, 0, true, &runs, &deleted));
        actions.Add(New<TestSceneAction>(3, 0, false, &runs, &deleted));
        SceneAction::RemovePreloadingActions(actions);
        CHECK(actions.Count() == 1);
        CHECK(deleted.Count() == 2);
        actions.ClearDelete();
    }
}
