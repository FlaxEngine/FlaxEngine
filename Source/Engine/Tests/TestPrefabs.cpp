// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "Engine/Content/Content.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Core/Log.h"
#include "Engine/Level/Actor.h"
#include "Engine/Level/Actors/EmptyActor.h"
#include "Engine/Level/Prefabs/Prefab.h"
#include "Engine/Level/Prefabs/PrefabManager.h"
#include "Engine/Scripting/ScriptingObjectReference.h"

#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("Prefabs")
{
    SECTION("Virtual Prefab")
    {
        auto* prefab = Content::CreateVirtualAsset<Prefab>();
        REQUIRE(prefab);
        Content::DeleteAsset(prefab);
    }
    SECTION("Test Repareting in Nested Prefab")
    {
        // https://github.com/FlaxEngine/FlaxEngine/issues/718

        // Create Prefab B with two children attached to the root
        AssetReference<Prefab> prefabB = Content::CreateVirtualAsset<Prefab>();
        REQUIRE(prefabB);
        Guid id;
        Guid::Parse("665bb01c49a3370f14a023b5395de261", id);
        prefabB->ChangeID(id);
        auto prefabBInit = prefabB->Init(Prefab::TypeName,
                                         "["
                                         "{"
                                         "\"ID\": \"eec6b9644492fbca1a6ab0a7904a557e\","
                                         "\"TypeName\": \"FlaxEngine.EmptyActor\","
                                         "\"Name\": \"Prefab B.Root\""
                                         "},"
                                         "{"
                                         "\"ID\": \"124540354d2d0a1decbb3ebfc279cfe6\","
                                         "\"TypeName\": \"FlaxEngine.EmptyActor\","
                                         "\"ParentID\": \"eec6b9644492fbca1a6ab0a7904a557e\","
                                         "\"Name\": \"Prefab B.Parent\""
                                         "},"
                                         "{"
                                         "\"ID\": \"f701472747356b7c26186db1c4252b53\","
                                         "\"TypeName\": \"FlaxEngine.EmptyActor\","
                                         "\"ParentID\": \"eec6b9644492fbca1a6ab0a7904a557e\","
                                         "\"Name\": \"Prefab B.Child\""
                                         "}"
                                         "]");
        REQUIRE(!prefabBInit);

        // Create Prefab A with nested Prefab B attached to the root
        AssetReference<Prefab> prefabA = Content::CreateVirtualAsset<Prefab>();
        REQUIRE(prefabA);
        Guid::Parse("02524a044184af56b6c664a0f98bd761", id);
        prefabA->ChangeID(id);
        auto prefabAInit = prefabA->Init(Prefab::TypeName,
                                         "["
                                         "{"
                                         "\"ID\": \"5aa124754dcd1cdefed80e828831d45b\","
                                         "\"TypeName\": \"FlaxEngine.EmptyActor\","
                                         "\"Name\": \"Prefab A.Root\""
                                         "},"
                                         "{"
                                         "\"ID\": \"8e51f1094f430733333f8280e78dfcc3\","
                                         "\"PrefabID\": \"665bb01c49a3370f14a023b5395de261\","
                                         "\"PrefabObjectID\": \"eec6b9644492fbca1a6ab0a7904a557e\","
                                         "\"ParentID\": \"5aa124754dcd1cdefed80e828831d45b\""
                                         "},"
                                         "{"
                                         "\"ID\": \"8e1f2bae4aaedeab8725908ce1aec325\","
                                         "\"PrefabID\": \"665bb01c49a3370f14a023b5395de261\","
                                         "\"PrefabObjectID\": \"124540354d2d0a1decbb3ebfc279cfe6\","
                                         "\"ParentID\": \"8e51f1094f430733333f8280e78dfcc3\""
                                         "},"
                                         "{"
                                         "\"ID\": \"4e4f3a1847cf96fe2e8919848b7eca79\","
                                         "\"PrefabID\": \"665bb01c49a3370f14a023b5395de261\","
                                         "\"PrefabObjectID\": \"f701472747356b7c26186db1c4252b53\","
                                         "\"ParentID\": \"8e51f1094f430733333f8280e78dfcc3\""
                                         "}"
                                         "]");
        REQUIRE(!prefabAInit);

        // Spawn test instances of both prefabs
        ScriptingObjectReference<Actor> instanceB = PrefabManager::SpawnPrefab(prefabB);
        ScriptingObjectReference<Actor> instanceA = PrefabManager::SpawnPrefab(prefabA);

        // Verify initial scenario
        REQUIRE(instanceA);
        REQUIRE(instanceB);
        REQUIRE(instanceA->GetName() == TEXT("Prefab A.Root"));
        REQUIRE(instanceA->GetChildrenCount() == 1);
        REQUIRE(instanceA->Children[0]->GetName() == TEXT("Prefab B.Root"));
        REQUIRE(instanceA->Children[0]->GetChildrenCount() == 2);
        REQUIRE(instanceA->Children[0]->Children[0]->GetName() == TEXT("Prefab B.Parent"));
        REQUIRE(instanceA->Children[0]->Children[0]->GetChildrenCount() == 0);
        REQUIRE(instanceA->Children[0]->Children[1]->GetName() == TEXT("Prefab B.Child"));
        REQUIRE(instanceA->Children[0]->Children[1]->GetChildrenCount() == 0);

        // Modify Prefab B instance to reparent the actor
        instanceB->FindActor(TEXT("Prefab B.Child"))->SetParent(instanceB->FindActor(TEXT("Prefab B.Parent")));

        // Apply nested prefab changes
        auto applyResult = PrefabManager::ApplyAll(instanceB);
        REQUIRE(!applyResult);

        // Verify if instance of Prefab B nested in instance of Prefab A was properly updated
        REQUIRE(instanceA);
        REQUIRE(instanceB);
        REQUIRE(instanceA->GetName() == TEXT("Prefab A.Root"));
        REQUIRE(instanceA->GetChildrenCount() == 1);
        REQUIRE(instanceA->Children[0]->GetName() == TEXT("Prefab B.Root"));
        REQUIRE(instanceA->Children[0]->GetChildrenCount() == 1);
        REQUIRE(instanceA->Children[0]->Children[0]->GetName() == TEXT("Prefab B.Parent"));
        REQUIRE(instanceA->Children[0]->Children[0]->GetChildrenCount() == 1);
        REQUIRE(instanceA->Children[0]->Children[0]->Children[0]->GetName() == TEXT("Prefab B.Child"));
        REQUIRE(instanceA->Children[0]->Children[0]->Children[0]->GetChildrenCount() == 0);

        // Cleanup
        instanceA->DeleteObject();
        instanceB->DeleteObject();
        Content::DeleteAsset(prefabA);
        Content::DeleteAsset(prefabB);
    }
}
