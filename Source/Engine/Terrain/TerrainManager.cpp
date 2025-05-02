// Copyright (c) Wojciech Figat. All rights reserved.

#include "TerrainManager.h"
#include "Terrain.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Collections/ChunkedArray.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Content/Content.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Content/Assets/MaterialBase.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/Shaders/GPUVertexLayout.h"
#include "Engine/Renderer/DrawCall.h"

// Must match structure defined in Terrain.shader
struct TerrainVertex
{
    Float2 TexCoord;
    Color32 Morph;
};

class GeometryData
{
public:
    GPUBuffer* VertexBuffer;
    GPUBuffer* IndexBuffer;
    uint32 IndicesCount;

    void GetChunkGeometry(DrawCall& drawCall) const
    {
        drawCall.Geometry.IndexBuffer = IndexBuffer;
        drawCall.Geometry.VertexBuffers[0] = VertexBuffer;
        drawCall.Geometry.VertexBuffers[1] = nullptr;
        drawCall.Geometry.VertexBuffers[2] = nullptr;
        drawCall.Geometry.VertexBuffersOffsets[0] = 0;
        drawCall.Geometry.VertexBuffersOffsets[1] = 0;
        drawCall.Geometry.VertexBuffersOffsets[2] = 0;
        drawCall.Draw.StartIndex = 0;
        drawCall.Draw.IndicesCount = IndicesCount;
    }
};

CriticalSection GeometryLocker;
ChunkedArray<GeometryData, 1024> Pool;
Dictionary<uint32, GeometryData*> Lookup;
Array<byte> TempData;
AssetReference<MaterialBase> DefaultTerrainMaterial;
GPUVertexLayout* TerrainVertexLayout = nullptr;

class TerrainManagerService : public EngineService
{
public:
    TerrainManagerService()
        : EngineService(TEXT("Terrain Manager"), 40)
    {
    }

    bool Init() override;
    void BeforeExit() override;
};

TerrainManagerService TerrainManagerServiceInstance;

MaterialBase* TerrainManager::GetDefaultTerrainMaterial()
{
    return DefaultTerrainMaterial.Get();
}

bool TerrainManager::GetChunkGeometry(DrawCall& drawCall, int32 chunkSize, int32 lodIndex)
{
    const int32 chunkSizeLOD0 = chunkSize;

    // Try fast lookup
    GeometryData* data;
    const uint32 key = (uint32)chunkSizeLOD0 | ((uint32)lodIndex << 20);
    if (Lookup.TryGet(key, data))
    {
        data->GetChunkGeometry(drawCall);
        return false;
    }

    ASSERT(chunkSize >= 3 && chunkSize < MAX_uint16);
    ASSERT(lodIndex >= 0 && lodIndex <= TERRAIN_MAX_LODS);

    ScopeLock lock(GeometryLocker);

    // Check if someone just created buffer
    if (Lookup.TryGet(key, data))
    {
        data->GetChunkGeometry(drawCall);
        return false;
    }

    // Prepare
    const int32 vertexCount = (chunkSize + 1) >> lodIndex;
    chunkSize = vertexCount - 1;
    const bool indexUse16bits = chunkSize * chunkSize * vertexCount < MAX_uint16;
    const int32 indexSize = indexUse16bits ? sizeof(uint16) : sizeof(uint32);
    const int32 indexCount = chunkSize * chunkSize * 2 * 3;
    const int32 vertexCount2 = vertexCount * vertexCount;
    const int32 vbSize = sizeof(TerrainVertex) * vertexCount2;
    const int32 ibSize = indexSize * indexCount;
    TempData.Clear();
    TempData.EnsureCapacity(Math::Max(vbSize, ibSize));

    // Create vertex buffer
    auto vb = GPUDevice::Instance->CreateBuffer(TEXT("TerrainChunk.VB"));
    auto vertex = (TerrainVertex*)TempData.Get();
    const float vertexTexelSnapTexCoord = 1.0f / chunkSize;
    for (int32 z = 0; z < vertexCount; z++)
    {
        for (int32 x = 0; x < vertexCount; x++)
        {
            vertex->TexCoord.X = x * vertexTexelSnapTexCoord;
            vertex->TexCoord.Y = z * vertexTexelSnapTexCoord;

            // Smooth LODs morphing based on Barycentric coordinates to morph to the lower LOD near chunk edges
            Float4 coord(vertex->TexCoord.Y, vertex->TexCoord.X, 1.0f - vertex->TexCoord.X, 1.0f - vertex->TexCoord.Y);

            // Apply some contrast
            const float AdjustPower = 0.3f;
            coord.X = Math::Pow(coord.X, AdjustPower);
            coord.Y = Math::Pow(coord.Y, AdjustPower);
            coord.Z = Math::Pow(coord.Z, AdjustPower);
            coord.W = Math::Pow(coord.W, AdjustPower);

            vertex->Morph = Color32(coord);
            vertex++;
        }
    }
    if (!TerrainVertexLayout)
    {
        TerrainVertexLayout = GPUVertexLayout::Get({
            { VertexElement::Types::TexCoord0, 0, 0, 0, PixelFormat::R32G32_Float },
            { VertexElement::Types::TexCoord1, 0, 0, 0, PixelFormat::R8G8B8A8_UNorm },
        });
    }
    auto desc = GPUBufferDescription::Vertex(TerrainVertexLayout, sizeof(TerrainVertex), vertexCount2, TempData.Get());
    if (vb->Init(desc))
    {
        Delete(vb);
        LOG(Warning, "Failed to create terrain chunk buffer.");
        return true;
    }

    // Create index buffer
    auto ib = GPUDevice::Instance->CreateBuffer(TEXT("TerrainChunk.IB"));
    if (indexUse16bits)
    {
        auto index = (uint16*)TempData.Get();
        for (int32 z = 0; z < chunkSize; z++)
        {
            for (int32 x = 0; x < chunkSize; x++)
            {
                const uint16 i00 = (x + 0) + (z + 0) * vertexCount;
                const uint16 i10 = (x + 1) + (z + 0) * vertexCount;
                const uint16 i11 = (x + 1) + (z + 1) * vertexCount;
                const uint16 i01 = (x + 0) + (z + 1) * vertexCount;

                *index++ = i00;
                *index++ = i11;
                *index++ = i10;

                *index++ = i00;
                *index++ = i01;
                *index++ = i11;
            }
        }
    }
    else
    {
        auto index = (uint32*)TempData.Get();
        for (int32 z = 0; z < chunkSize; z++)
        {
            for (int32 x = 0; x < chunkSize; x++)
            {
                const uint32 i00 = (x + 0) + (z + 0) * vertexCount;
                const uint32 i10 = (x + 1) + (z + 0) * vertexCount;
                const uint32 i11 = (x + 1) + (z + 1) * vertexCount;
                const uint32 i01 = (x + 0) + (z + 1) * vertexCount;

                *index++ = i00;
                *index++ = i11;
                *index++ = i10;

                *index++ = i00;
                *index++ = i01;
                *index++ = i11;
            }
        }
    }
    desc = GPUBufferDescription::Index(indexSize, indexCount, TempData.Get());
    if (ib->Init(desc))
    {
        vb->ReleaseGPU();
        Delete(vb);
        Delete(ib);
        LOG(Warning, "Failed to create terrain chunk buffer.");
        return true;
    }

    // TODO: use MeshData::ImproveCacheLocality() or similar to improve perf

    // Cache data
    GeometryData d;
    d.VertexBuffer = vb;
    d.IndexBuffer = ib;
    d.IndicesCount = indexCount;
    data = Pool.Add(d);
    Lookup.Add(key, data);

    data->GetChunkGeometry(drawCall);
    return false;
}

bool TerrainManagerService::Init()
{
    // Load default terrain material as fallback
    DefaultTerrainMaterial = Content::LoadAsyncInternal<MaterialBase>(TEXT("Engine/DefaultTerrainMaterial"));
    if (DefaultTerrainMaterial == nullptr)
    {
        LOG(Warning, "Default terrain material is missing.");
    }

    return false;
}

void TerrainManagerService::BeforeExit()
{
    // Cleanup data
    for (auto i = Lookup.Begin(); i.IsNotEnd(); ++i)
    {
        auto v = i->Value;
        v->VertexBuffer->ReleaseGPU();
        Delete(v->VertexBuffer);
        v->IndexBuffer->ReleaseGPU();
        Delete(v->IndexBuffer);
    }
    Pool.Clear();
    Lookup.Clear();
    TempData.Resize(0);
}
