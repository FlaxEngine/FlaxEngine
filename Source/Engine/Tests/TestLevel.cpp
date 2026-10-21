// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Level/Level.h"
#include "Engine/Level/LargeWorlds.h"
#include "Engine/Level/Actors/EmptyActor.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Level/Tags.h"
#include "TestScripting.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("LargeWorlds")
{
    SECTION("UpdateOrigin")
    {
        LargeWorlds::Enable = true;
        Vector3 origin = Vector3::Zero;
        LargeWorlds::UpdateOrigin(origin, Vector3::Zero);
        CHECK(origin == Vector3::Zero);
        LargeWorlds::UpdateOrigin(origin, Vector3(LargeWorlds::ChunkSize * 0.5, LargeWorlds::ChunkSize * 1.0001, LargeWorlds::ChunkSize * 1.5));
        CHECK(origin == Vector3(0, 0, LargeWorlds::ChunkSize * 1));
    }
}

TEST_CASE("Tags")
{
    SECTION("Tag")
    {
        auto prevTags = Tags::List;

        Tags::List = Array<String>({ TEXT("A"), TEXT("A.1"), TEXT("B"), TEXT("B.1"), });

        auto a = Tags::Get(TEXT("A"));
        auto a1 = Tags::Get(TEXT("A.1"));
        auto b = Tags::Get(TEXT("B"));
        auto b1 = Tags::Get(TEXT("B.1"));
        auto c = Tags::Get(TEXT("C"));
        CHECK(a.Index == 1);
        CHECK(a1.Index == 2);
        CHECK(b.Index == 3);
        CHECK(b1.Index == 4);
        CHECK(c.Index == 5);

        Tags::List = prevTags;
    }

    SECTION("Tags")
    {
        auto prevTags = Tags::List;

        Tags::List = Array<String>({ TEXT("A"), TEXT("A.1"), TEXT("B"), TEXT("B.1"), });

        auto a = Tags::Get(TEXT("A"));
        auto a1 = Tags::Get(TEXT("A.1"));
        auto b = Tags::Get(TEXT("B"));
        auto b1 = Tags::Get(TEXT("B.1"));
        auto c = Tags::Get(TEXT("C"));

        Array<Tag> list = { a1, b1 };

        CHECK(Tags::HasTag(list, Tag()) == false);
        CHECK(Tags::HasTag(list, a1) == true);
        CHECK(Tags::HasTag(list, a) == true);
        CHECK(Tags::HasTag(list, c) == false);

        CHECK(Tags::HasTagExact(list, a1) == true);
        CHECK(Tags::HasTagExact(list, a) == false);
        CHECK(Tags::HasTagExact(list, c) == false);

        Tags::List = prevTags;
    }
}

TEST_CASE("Actor")
{
    SECTION("Test script added during OnAwake")
    {
        // Scene loading initializes each loaded object (SceneLoader::OnInitialize calls OnAwake of the scripts), then
        // starts the scene (BeginPlay). A script that another script's OnAwake adds in between is not in the loaded
        // objects, but it must also get OnAwake, and so OnDestroy when deleted.
        TestScriptLifecycle::ResetCounts();
        auto actor = ScriptingObject::NewObject<EmptyActor>();
        auto script = ScriptingObject::NewObject<TestScriptAddingScript>();
        script->SetParent(actor);
        actor->Initialize();
        script->Initialize();
        Level::CallBeginPlay(actor);
        CHECK(actor->Scripts.Count() == 2);
        CHECK(TestScriptLifecycle::AwakeCount == 1);
        actor->DeleteObjectNow();
        CHECK(TestScriptLifecycle::DestroyCount == 1);
    }
    SECTION("Test script added to an actor created during OnAwake")
    {
        // A script's OnAwake creates an actor and parents it to the loading scene, which enables the actor (the scene is
        // active), then adds a script to it. The added script must still get OnAwake, OnStart and OnEnable when the scene
        // starts (issue 3619).
        TestScriptLifecycle::ResetCounts();
        auto scene = ScriptingObject::NewObject<Scene>();
        auto script = ScriptingObject::NewObject<TestScriptSpawningActor>();
        script->SetParent(scene);
        ((SceneObject*)scene)->Initialize(); // As SceneLoader::OnInitialize (through SceneObject)
        script->Initialize();
        Level::CallBeginPlay(scene);
        REQUIRE(scene->Children.Count() == 1);
        CHECK(TestScriptLifecycle::AwakeCount == 1);
        CHECK(TestScriptLifecycle::StartCount == 1);
        CHECK(TestScriptLifecycle::EnableCount == 1);
        scene->DeleteObjectNow();
        CHECK(TestScriptLifecycle::DestroyCount == 1);
    }
}
