// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "Engine/Graphics/Models/Types.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Core/Math/Triangle.h"

class ModelData;

namespace CSG
{
    class Brush;

    /// <summary>
    /// Represents raw CSG mesh data after triangulation. Can be used to export it to model vertex/index buffers. Separates triangles by materials.
    /// </summary>
    class RawData
    {
    public:
        struct Surface
        {
            float ScaleInLightmap;
            Rectangle LightmapUVsBox;
            Float2 Size;
            Rectangle UVsArea;
            Array<MeshVertex> Vertices;
        };

        struct SurfaceTriangle
        {
            Float3 V[3];
        };

        struct SurfaceData
        {
            Array<SurfaceTriangle> Triangles;
        };

        struct BrushData
        {
            Array<SurfaceData> Surfaces;
        };

        class Slot
        {
        public:
            Guid Material;
            Array<Surface> Surfaces;

        public:
            /// <summary>
            /// Initializes a new instance of the <see cref="Slot"/> class.
            /// </summary>
            /// <param name="material">The material.</param>
            Slot(const Guid& material)
                : Material(material)
            {
            }

        public:
            bool IsEmpty() const
            {
                return Surfaces.IsEmpty();
            }

            void AddSurface(float scaleInLightmap, const Rectangle& lightmapUVsBox, const MeshVertex* firstVertex, int32 vertexCount);
        };

    public:
        /// <summary>
        /// The slots.
        /// </summary>
        Array<Slot*> Slots;

        /// <summary>
        /// The brushes.
        /// </summary>
        Dictionary<Guid, BrushData> Brushes;

    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="RawData"/> class.
        /// </summary>
        RawData()
        {
        }

        /// <summary>
        /// Finalizes an instance of the <see cref="RawData"/> class.
        /// </summary>
        ~RawData()
        {
            Slots.ClearDelete();
        }

    public:
        /// <summary>
        /// Gets or adds the slot for the given material.
        /// </summary>
        /// <param name="material">The material.</param>
        /// <returns>The geometry slot.</returns>
        Slot* GetOrAddSlot(const Guid& material)
        {
            for (int32 i = 0; i < Slots.Count(); i++)
            {
                if (Slots[i]->Material == material)
                    return Slots[i];
            }

            auto slot = New<Slot>(material);
            Slots.Add(slot);
            return slot;
        }

    public:
        void AddSurface(Brush* brush, int32 brushSurfaceIndex, const Guid& surfaceMaterial, float scaleInLightmap, const Rectangle& lightmapUVsBox, const MeshVertex* firstVertex, int32 vertexCount);

        /// <summary>
        /// Removes the empty slots.
        /// </summary>
        void RemoveEmptySlots()
        {
            for (int32 i = 0; i < Slots.Count() && Slots.HasItems(); i++)
            {
                if (Slots[i]->IsEmpty())
                {
                    Delete(Slots[i]);
                    Slots.RemoveAt(i);
                    i--;
                }
            }
        }

        /// <summary>
        /// Outputs mesh data to the ModelData storage container.
        /// </summary>
        /// <param name="modelData">The result model data.</param>
        void ToModelData(ModelData& modelData) const;
    };
}
