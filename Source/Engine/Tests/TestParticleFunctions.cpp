// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Content/Content.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Particles/ParticleEmitterFunction.h"
#include "Engine/Particles/Graph/CPU/ParticleEmitterGraph.CPU.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#if COMPILE_WITH_PARTICLE_GPU_GRAPH && COMPILE_WITH_ASSETS_IMPORTER
#include "Engine/ContentImporters/AssetsImportingManager.h"
#include "Engine/Particles/Graph/GPU/ParticleEmitterGraph.GPU.h"
#endif
#include <ThirdParty/catch2/catch.hpp>

#if COMPILE_WITH_PARTICLE_GPU_GRAPH && COMPILE_WITH_ASSETS_IMPORTER

namespace
{
    // Adds a node with the given float boxes to the graph (boxes are indexed by their ID)
    ParticleEmitterGraphCPUNode& AddNode(ParticleEmitterGraphCPU& graph, uint32 id, uint32 type, std::initializer_list<int32> boxIds)
    {
        auto& node = graph.Nodes.AddOne();
        node.ID = id;
        node.Type = type;
        int32 boxesCount = 0;
        for (const int32 boxId : boxIds)
            boxesCount = Math::Max(boxesCount, boxId + 1);
        node.Boxes.Resize(boxesCount);
        for (const int32 boxId : boxIds)
        {
            node.Boxes[boxId].Parent = &node;
            node.Boxes[boxId].ID = (byte)boxId;
            node.Boxes[boxId].Type = VariantType::Float;
        }
        return node;
    }

    void Connect(ParticleEmitterGraphCPUBox& a, ParticleEmitterGraphCPUBox& b)
    {
        a.Connections.Add(&b);
        b.Connections.Add(&a);
    }

    CreateAssetResult SaveFunction(CreateAssetContext& context, const ParticleEmitterGraphCPU& graph)
    {
        IMPORT_SETUP(ParticleEmitterFunction, 1);
        MemoryWriteStream stream(512);
        if (graph.Save(&stream, true))
            return CreateAssetResult::Error;
        if (context.AllocateChunk(0))
            return CreateAssetResult::CannotAllocateChunk;
        context.Data.Header.Chunks[0]->Data.Copy(ToSpan(stream));
        return CreateAssetResult::Ok;
    }

    // Function: a float constant (node index 0) connected to the function output (node index 1)
    CreateAssetResult CreateOutputFunction(CreateAssetContext& context)
    {
        ParticleEmitterGraphCPU graph;
        graph.Nodes.EnsureCapacity(2);
        auto& constant = AddNode(graph, 1, GRAPH_NODE_MAKE_TYPE(2, 3), { 0 });
        constant.Values.Add(Variant(1234.5f));
        auto& output = AddNode(graph, 2, GRAPH_NODE_MAKE_TYPE(16, 2), { 0 });
        output.Values.Add(Variant(TEXT("System.Single")));
        output.Values.Add(Variant(TEXT("Output")));
        Connect(output.Boxes[0], constant.Boxes[0]);
        return SaveFunction(context, graph);
    }

    // Function: an unused constant (node index 0), the function input (node index 1) connected to the function output (node index 2)
    CreateAssetResult CreateInputFunction(CreateAssetContext& context)
    {
        ParticleEmitterGraphCPU graph;
        graph.Nodes.EnsureCapacity(3);
        auto& constant = AddNode(graph, 1, GRAPH_NODE_MAKE_TYPE(2, 3), { 0 });
        constant.Values.Add(Variant(1.0f));
        auto& input = AddNode(graph, 2, GRAPH_NODE_MAKE_TYPE(16, 1), { 0 });
        input.Values.Add(Variant(TEXT("System.Single")));
        input.Values.Add(Variant(TEXT("Input")));
        auto& output = AddNode(graph, 3, GRAPH_NODE_MAKE_TYPE(16, 2), { 0 });
        output.Values.Add(Variant(TEXT("System.Single")));
        output.Values.Add(Variant(TEXT("Output")));
        Connect(output.Boxes[0], input.Boxes[0]);
        return SaveFunction(context, graph);
    }

    // Generates the GPU simulation code of an emitter, where an Initialize module sets the particle lifetime to the output of the function call. If inputValue is not zero, a constant with this value goes to the function input.
    String GenerateEmitterCode(const Guid& functionId, float inputValue)
    {
        ParticleEmitterGraphCPU emitter;
        emitter.Nodes.EnsureCapacity(4);
        auto& root = AddNode(emitter, 1, GRAPH_NODE_MAKE_TYPE(14, 1), {});
        root.Values.Add(Variant(1024));
        root.Values.Add(Variant(0));
        root.Values.Add(Variant(0));
        root.Values.Add(Variant(0));
        auto& call = AddNode(emitter, 2, GRAPH_NODE_MAKE_TYPE(14, 300), { 0, 16 });
        call.Values.Add(Variant(functionId));
        auto& setLifetime = AddNode(emitter, 3, GRAPH_NODE_MAKE_TYPE(15, 251), { 0 });
        setLifetime.Values.Add(Variant(true));
        setLifetime.Values.Add(Variant(1)); // ModuleType::Initialize
        setLifetime.Values.Add(Variant(0.0f));
        Connect(setLifetime.Boxes[0], call.Boxes[16]);
        if (inputValue != 0.0f)
        {
            auto& constant = AddNode(emitter, 4, GRAPH_NODE_MAKE_TYPE(2, 3), { 0 });
            constant.Values.Add(Variant(inputValue));
            Connect(call.Boxes[0], constant.Boxes[0]);
        }
        MemoryWriteStream emitterData(1024);
        if (emitter.Save(&emitterData, false))
            return String::Empty;

        ParticleEmitterGPUGenerator generator;
        auto graph = New<ParticleEmitterGraphGPU>();
        MemoryReadStream emitterStream(emitterData.GetHandle(), emitterData.GetPosition());
        if (graph->Load(&emitterStream, false))
        {
            Delete(graph);
            return String::Empty;
        }
        generator.AddGraph(graph);
        MemoryWriteStream source(16 * 1024);
        BytesContainer parameters;
        int32 customDataSize = 0;
        if (generator.Generate(source, parameters, customDataSize))
            return String::Empty;
        return String((const char*)source.GetHandle(), (int32)source.GetPosition());
    }
}

TEST_CASE("ParticleEmitterFunction")
{
    SECTION("Test GPU function output")
    {
        const String path = Globals::TemporaryFolder / TEXT("TestParticleFunctionOutput.flax");
        Guid functionId = Guid::New();
        REQUIRE(!AssetsImportingManager::Create(&CreateOutputFunction, path, functionId));
        auto function = Content::Load<ParticleEmitterFunction>(functionId);
        REQUIRE(function);
        REQUIRE(function->Outputs.Count() == 1);
        CHECK(function->Outputs[0] == 1);

        // The function output (node index 1) gives the constant of the function
        const String code = GenerateEmitterCode(functionId, 0.0f);
        CHECK(code.Contains(TEXT("1234.5")));

        Content::DeleteAsset(path);
    }
    SECTION("Test GPU function input")
    {
        const String path = Globals::TemporaryFolder / TEXT("TestParticleFunctionInput.flax");
        Guid functionId = Guid::New();
        REQUIRE(!AssetsImportingManager::Create(&CreateInputFunction, path, functionId));
        auto function = Content::Load<ParticleEmitterFunction>(functionId);
        REQUIRE(function);
        REQUIRE(function->Inputs.Count() == 1);
        CHECK(function->Inputs[0] == 1);

        // The function input (node index 1) gives the value from the calling emitter
        const String code = GenerateEmitterCode(functionId, 6789.25f);
        CHECK(code.Contains(TEXT("6789.25")));

        Content::DeleteAsset(path);
    }
}

#endif
