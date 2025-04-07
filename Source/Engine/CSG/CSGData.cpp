// Copyright (c) Wojciech Figat. All rights reserved.

#include "CSGData.h"
#include "Brush.h"
#include "Engine/Graphics/Models/ModelData.h"
#include "Engine/Utilities/RectPack.h"

using namespace CSG;

namespace CSG
{
    class LightmapUVsPacker
    {
        typedef RawData::Surface* ChartType;

    public:

        struct Node : RectPackNode<float>
        {
            Node(Size x, Size y, Size width, Size height)
                : RectPackNode(x, y, width, height)
            {
            }

            void OnInsert(ChartType chart, float atlasSize)
            {
                const float invSize = 1.0f / atlasSize;
                chart->UVsArea = Rectangle(X * invSize, Y * invSize, chart->Size.X * invSize, chart->Size.Y * invSize);
            }
        };

    private:

        RectPackAtlas<Node> _root;
        const float _atlasSize;
        const float _chartsPadding;

    public:

        LightmapUVsPacker(float atlasSize, float chartsPadding)
            : _atlasSize(atlasSize)
            , _chartsPadding(chartsPadding)
        {
            _root.Init(atlasSize, atlasSize, chartsPadding);
        }

        ~LightmapUVsPacker()
        {
        }

        Node* Insert(ChartType chart)
        {
            return _root.Insert(chart->Size.X, chart->Size.Y, chart, _atlasSize);
        }
    };
}

void RawData::Slot::AddSurface(float scaleInLightmap, const Rectangle& lightmapUVsBox, const MeshVertex* firstVertex, int32 vertexCount)
{
    auto& surface = Surfaces.AddOne();
    surface.ScaleInLightmap = scaleInLightmap;
    surface.LightmapUVsBox = lightmapUVsBox;
    surface.Size = lightmapUVsBox.Size * scaleInLightmap;
    surface.UVsArea = Rectangle::Empty;
    surface.Vertices.Add(firstVertex, vertexCount);
}

void RawData::AddSurface(Brush* brush, int32 brushSurfaceIndex, const Guid& surfaceMaterial, float scaleInLightmap, const Rectangle& lightmapUVsBox, const MeshVertex* firstVertex, int32 vertexCount)
{
    // Add surface to slot
    auto slot = GetOrAddSlot(surfaceMaterial);
    slot->AddSurface(scaleInLightmap, lightmapUVsBox, firstVertex, vertexCount);

    // Add surface to brush
    auto& brushData = Brushes[brush->GetBrushID()];
    if (brushData.Surfaces.Count() != brush->GetSurfacesCount())
        brushData.Surfaces.Resize(brush->GetSurfacesCount());
    auto& surfaceData = brushData.Surfaces[brushSurfaceIndex];
    auto& triangles = surfaceData.Triangles;
    triangles.Resize(vertexCount / 3);

    // Copy triangles
    for (int32 i = 0; i < vertexCount;)
    {
        auto& triangle = triangles[i / 3];
        triangle.V[0] = firstVertex[i++].Position;
        triangle.V[1] = firstVertex[i++].Position;
        triangle.V[2] = firstVertex[i++].Position;
    }
}

void RawData::ToModelData(ModelData& modelData) const
{
    // Generate lightmap UVs (single chart for the whole mesh)
    {
        // Every surface has custom lightmap scale so we have to try to pack all triangles into
        // single rectangle using fast rectangle pack algorithm. Rectangle size can be calculated from
        // all triangles surfaces area with margins.

        // Sum the area for the atlas surfaces
        float areaSum = 0;
        for (int32 slotIndex = 0; slotIndex < Slots.Count(); slotIndex++)
        {
            auto& slot = Slots[slotIndex];
            for (int32 i = 0; i < slot->Surfaces.Count(); i++)
            {
                auto& surface = slot->Surfaces[i];
                areaSum += surface.Size.MulValues();
            }
        }

        // Insert only non-empty surfaces
        if (areaSum > ZeroTolerance)
        {
            // Pack all surfaces into atlas
            float atlasSize = Math::Sqrt(areaSum) * 1.02f;
            int32 triesLeft = 10;
            while (triesLeft--)
            {
                bool failed = false;
                const float chartsPadding = (8.0f / 1024.0f) * atlasSize;
                LightmapUVsPacker packer(atlasSize, chartsPadding);
                for (int32 slotIndex = 0; slotIndex < Slots.Count(); slotIndex++)
                {
                    auto& slot = Slots[slotIndex];
                    for (int32 i = 0; i < slot->Surfaces.Count(); i++)
                    {
                        auto& surface = slot->Surfaces[i];
                        if (packer.Insert(&surface) == nullptr)
                        {
                            // Failed to insert surface, increase atlas size and try again
                            atlasSize *= 1.5f;
                            failed = true;
                            break;
                        }
                    }
                }

                if (!failed)
                    break;
            }
        }
    }

    // Transfer data (use 1-1 mesh-material slot linkage)
    modelData.MinScreenSize = 0.0f;
    modelData.LODs.Resize(1);
    modelData.LODs[0].Meshes.Resize(Slots.Count());
    modelData.Materials.Resize(Slots.Count());
    for (int32 slotIndex = 0; slotIndex < Slots.Count(); slotIndex++)
    {
        auto& slot = Slots[slotIndex];
        auto& mesh = modelData.LODs[0].Meshes[slotIndex];
        auto& materialSlot = modelData.Materials[slotIndex];
        ASSERT(slot->Surfaces.HasItems());

        // Setup mesh and material slot
        mesh = New<MeshData>();
        mesh->Name = materialSlot.Name = String(TEXT("Mesh ")) + StringUtils::ToString(slotIndex);
        materialSlot.AssetID = slot->Material;
        mesh->MaterialSlotIndex = slotIndex;

        // Generate vertex and index buffers from the surfaces (don't use vertex colors)
        int32 vertexCount = 0;
        for (int32 i = 0; i < slot->Surfaces.Count(); i++)
        {
            auto& surface = slot->Surfaces[i];
            vertexCount += surface.Vertices.Count();
        }
        mesh->EnsureCapacity(vertexCount, vertexCount, false, false, false, 2);

        // Write surfaces into vertex and index buffers
        int32 index = 0;
        for (int32 i = 0; i < slot->Surfaces.Count(); i++)
        {
            auto& surface = slot->Surfaces.Get()[i];
            for (int32 vIndex = 0; vIndex < surface.Vertices.Count(); vIndex++)
            {
                auto& v = surface.Vertices.Get()[vIndex];

                mesh->Positions.Add(v.Position);
                mesh->UVs.Get()[0].Add(v.TexCoord);
                mesh->UVs.Get()[1].Add(v.LightmapUVs * surface.UVsArea.Size + surface.UVsArea.Location);
                mesh->Normals.Add(v.Normal);
                mesh->Tangents.Add(v.Tangent);

                mesh->Indices.Add(index++);
            }
        }
    }
}
