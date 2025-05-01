// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/ISerializable.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Math/Triangle.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Physics/CollisionData.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Content/Assets/RawDataAsset.h"
#include "Engine/Content/Assets/Model.h"

class Scene;

namespace CSG
{
    /// <summary>
    /// CSG geometry data container (used per scene).
    /// </summary>
    class SceneCSGData : public ISerializable
    {
    private:
        Scene* _scene;

    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="SceneCSGData"/> class.
        /// </summary>
        /// <param name="scene">The parent scene.</param>
        SceneCSGData(Scene* scene);

    public:
        /// <summary>
        /// CSG mesh building action time (registered by CSG::Builder, in UTC format). Invalid if not build by active engine instance.
        /// </summary>
        DateTime BuildTime;

        /// <summary>
        /// The model mesh (used for rendering).
        /// </summary>
        AssetReference<Model> Model;

        /// <summary>
        /// The CSG mesh raw data.
        /// </summary>
        AssetReference<RawDataAsset> Data;

        /// <summary>
        /// The CSG mesh collision data.
        /// </summary>
        AssetReference<CollisionData> CollisionData;

        /// <summary>
        /// The brush data locations lookup for faster searching though Data container.
        /// </summary>
        Dictionary<Guid, int32> DataBrushLocations;

        /// <summary>
        /// The post CSG build action. Called by the CSGBuilder after CSG mesh building end.
        /// </summary>
        Action PostCSGBuild;

    public:
        /// <summary>
        /// Build CSG geometry for the given scene.
        /// </summary>
        /// <param name="timeoutMs">The timeout to wait before building CSG (in milliseconds).</param>
        void BuildCSG(float timeoutMs = 50) const;

        /// <summary>
        /// Determines whether this container has CSG data linked.
        /// </summary>
        bool HasData() const;

    public:
        struct SurfaceData
        {
            Array<Triangle> Triangles;

            bool Intersects(const Ray& ray, Real& distance, Vector3& normal);
        };

        /// <summary>
        /// Tries the get the brush surface data (may fail if data is missing or given brush surface doesn't result with any triangle).
        /// </summary>
        /// <param name="brushId">The brush identifier.</param>
        /// <param name="brushSurfaceIndex">Index of the brush surface.</param>
        /// <param name="outData">The output data.</param>
        /// <returns>True if found data, otherwise false.</returns>
        bool TryGetSurfaceData(const Guid& brushId, int32 brushSurfaceIndex, SurfaceData& outData);

    private:
        void OnDataChanged();

    public:
        // [ISerializable]
        void Serialize(SerializeStream& stream, const void* otherObj) override;
        void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    };
};
