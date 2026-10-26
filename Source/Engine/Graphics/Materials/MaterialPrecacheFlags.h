// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_PSO_PRECACHING

// Set of material feature flags used by PSO precaching to precompile used permutations of the shaders.
struct MaterialPrecacheFlags
{
    // Different vertex geometry types that affect Vertex Shader permutations.
    enum GeometryFlags : uint32
    {
        Mesh = 1 << 0,
        MeshLightmapped = 1 << 1,
        Skinned = 1 << 2,
        SkinnedWithPrevBones = 1 << 3,
        Sprite = 1 << 4,
        Ribbon = 1 << 5,
        GeometrySurface = Mesh | MeshLightmapped | Skinned | SkinnedWithPrevBones,
        Particle = Mesh | Sprite | Ribbon,
        GeometryBits = 6
    };

    // Based on object scale (World matrix determinant sign) geometry needs to be flipped and so culling mode
    enum WorldDeterminantFlags : uint32
    {
        Positive = 1,
        Negative = 2,
        WorldDeterminantBits = 2
    };

    union
    {
        struct
        {
            uint32 Geometry : GeometryBits;
            uint32 WorldDeterminant : WorldDeterminantBits;
        };
        uint32 Flags;
    };

    friend bool operator!=(const MaterialPrecacheFlags& a, const MaterialPrecacheFlags& b)
    {
        return a.Flags != b.Flags;
    }

    friend MaterialPrecacheFlags& operator|=(MaterialPrecacheFlags& a, const MaterialPrecacheFlags& b)
    {
        a.Flags |= b.Flags;
        return a;
    }

    friend MaterialPrecacheFlags& operator&=(MaterialPrecacheFlags& a, const MaterialPrecacheFlags& b)
    {
        a.Flags &= b.Flags;
        return a;
    }
};

#define INIT_MATERIAL_PRECACHE_FLAGS(flags) \
    MaterialPrecacheFlags flags; \
    flags.Flags = 0; \
    flags.WorldDeterminant = MaterialPrecacheFlags::Positive

#define INIT_MATERIAL_PRECACHE_FLAGS_ACTOR(flags, actor) \
    Matrix world; \
    actor->GetLocalToWorldMatrix(world); \
    MaterialPrecacheFlags flags; \
    flags.Flags = 0; \
    flags.WorldDeterminant = world.RotDeterminant() < 0 ? MaterialPrecacheFlags::Negative : MaterialPrecacheFlags::Positive

#else

struct MaterialPrecacheFlags
{
    uint32 Flags;
};

#endif
