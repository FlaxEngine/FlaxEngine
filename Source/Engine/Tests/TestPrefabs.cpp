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
    SECTION("Test Adding Object in Nested Prefab")
    {
        // https://github.com/FlaxEngine/FlaxEngine/issues/690

        // Create Prefab B with just root object
        AssetReference<Prefab> prefabB = Content::CreateVirtualAsset<Prefab>();
        REQUIRE(prefabB);
        Guid id;
        Guid::Parse("25dbe4b0416be0777a6ce59e8788b10f", id);
        prefabB->ChangeID(id);
        auto prefabBInit = prefabB->Init(Prefab::TypeName,
                                         "["
                                         "{"
                                         "\"ID\": \"aac6b9644492fbca1a6ab0a7904a557e\","
                                         "\"TypeName\": \"FlaxEngine.EmptyActor\","
                                         "\"Name\": \"Prefab B.Root\""
                                         "}"
                                         "]");
        REQUIRE(!prefabBInit);

        // Create Prefab A with two nested Prefab B attached to the root
        AssetReference<Prefab> prefabA = Content::CreateVirtualAsset<Prefab>();
        REQUIRE(prefabA);
        Guid::Parse("4cb744714f746e31855f41815612d14b", id);
        prefabA->ChangeID(id);
        auto prefabAInit = prefabA->Init(Prefab::TypeName,
                                         "["
                                         "{"
                                         "\"ID\": \"244274a04cc60d56a2f024bfeef5772d\","
                                         "\"TypeName\": \"FlaxEngine.EmptyActor\","
                                         "\"Name\": \"Prefab A.Root\""
                                         "},"
                                         "{"
                                         "\"ID\": \"1e51f1094f430733333f8280e78dfcc3\","
                                         "\"PrefabID\": \"25dbe4b0416be0777a6ce59e8788b10f\","
                                         "\"PrefabObjectID\": \"aac6b9644492fbca1a6ab0a7904a557e\","
                                         "\"ParentID\": \"244274a04cc60d56a2f024bfeef5772d\""
                                         "},"
                                         "{"
                                         "\"ID\": \"2e1f2bae4aaedeab8725908ce1aec325\","
                                         "\"PrefabID\": \"25dbe4b0416be0777a6ce59e8788b10f\","
                                         "\"PrefabObjectID\": \"aac6b9644492fbca1a6ab0a7904a557e\","
                                         "\"ParentID\": \"244274a04cc60d56a2f024bfeef5772d\""
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
        REQUIRE(instanceA->GetChildrenCount() == 2);
        REQUIRE(instanceA->Children[0]->GetName() == TEXT("Prefab B.Root"));
        REQUIRE(instanceA->Children[0]->GetChildrenCount() == 0);
        REQUIRE(instanceA->Children[1]->GetName() == TEXT("Prefab B.Root"));
        REQUIRE(instanceA->Children[1]->GetChildrenCount() == 0);

        // Modify Prefab B instance to add a new actor as a child so at appears in both nested instances in Prefab A instance
        Guid newChildId;
        Guid::Parse(TEXT("123456a04cc60d56a2f024bfeef57723"), newChildId);
        auto newChild = EmptyActor::Spawn(ScriptingObject::SpawnParams(newChildId, EmptyActor::TypeInitializer));
        newChild->SetName(TEXT("Prefab B.Child"));
        newChild->SetParent(instanceB);

        // Apply nested prefab changes
        auto applyResult = PrefabManager::ApplyAll(instanceB);
        REQUIRE(!applyResult);

        // Verify if instance of Prefab B nested instances in instance of Prefab A was properly updated
        REQUIRE(instanceA);
        REQUIRE(instanceB);
        REQUIRE(instanceA->GetName() == TEXT("Prefab A.Root"));
        REQUIRE(instanceA->GetChildrenCount() == 2);
        REQUIRE(instanceA->Children[0]->GetName() == TEXT("Prefab B.Root"));
        REQUIRE(instanceA->Children[0]->GetChildrenCount() == 1);
        REQUIRE(instanceA->Children[0]->Children[0]->GetName() == TEXT("Prefab B.Child"));
        REQUIRE(instanceA->Children[0]->Children[0]->GetChildrenCount() == 0);
        REQUIRE(instanceA->Children[1]->GetName() == TEXT("Prefab B.Root"));
        REQUIRE(instanceA->Children[1]->GetChildrenCount() == 1);
        REQUIRE(instanceA->Children[1]->Children[0]->GetName() == TEXT("Prefab B.Child"));
        REQUIRE(instanceA->Children[1]->Children[0]->GetChildrenCount() == 0);

        // Add another child 
        Guid::Parse(TEXT("678906a04cc60d56a2f024bfeef57723"), newChildId);
        newChild = EmptyActor::Spawn(ScriptingObject::SpawnParams(newChildId, EmptyActor::TypeInitializer));
        newChild->SetName(TEXT("Prefab B.Child 2"));
        newChild->SetParent(instanceB);

        // Apply nested prefab changes
        applyResult = PrefabManager::ApplyAll(instanceB);
        REQUIRE(!applyResult);

        // Reparent another child into the first child
        newChild->SetParent(instanceB->Children[0]);

        // Apply nested prefab changes
        applyResult = PrefabManager::ApplyAll(instanceB);
        REQUIRE(!applyResult);

        // Verify if instance of Prefab B nested instances in instance of Prefab A was properly updated
        REQUIRE(instanceA);
        REQUIRE(instanceB);
        REQUIRE(instanceA->GetChildrenCount() == 2);
        REQUIRE(instanceA->Children[0]->GetChildrenCount() == 1);
        REQUIRE(instanceA->Children[0]->Children[0]->GetChildrenCount() == 1);
        REQUIRE(instanceA->Children[0]->Children[0]->Children[0]->GetChildrenCount() == 0);
        REQUIRE(instanceA->Children[1]->GetChildrenCount() == 1);
        REQUIRE(instanceA->Children[1]->Children[0]->GetChildrenCount() == 1);
        REQUIRE(instanceA->Children[1]->Children[0]->Children[0]->GetChildrenCount() == 0);

        // Cleanup
        instanceA->DeleteObject();
        instanceB->DeleteObject();
        Content::DeleteAsset(prefabA);
        Content::DeleteAsset(prefabB);
    }
}
