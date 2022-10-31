// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/Script.h"
#include "Engine/Networking/INetworkSerializable.h"

/// <summary>
/// Actor script component that synchronizes the Transform over the network.
/// </summary>
API_CLASS(Namespace="FlaxEngine.Networking") class FLAXENGINE_API NetworkTransform : public Script, public INetworkSerializable
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE(NetworkTransform);

    /// <summary>
    /// Actor transform synchronization modes (flags).
    /// </summary>
    API_ENUM(Attributes="Flags") enum class SyncModes
    {
        // No sync.
        None = 0,

        // Position X component.
        PositionX = 1 << 0,
        // Position Y component.
        PositionY = 1 << 1,
        // Position Z component.
        PositionZ = 1 << 2,
        // Position XYZ components (full).
        Position = PositionX | PositionY | PositionZ,

        // Scale X component.
        ScaleX = 1 << 3,
        // Scale Y component.
        ScaleY = 1 << 4,
        // Scale Z component.
        ScaleZ = 1 << 5,
        // Scale XYZ components (full).
        Scale = ScaleX | ScaleY | ScaleZ,

        // Position X component.
        RotationX = 1 << 6,
        // Position Y component.
        RotationY = 1 << 7,
        // Position Z component.
        RotationZ = 1 << 8,
        // Rotation XYZ components (full).
        Rotation = RotationX | RotationY | RotationZ,

        // All components fully synchronized.
        All = Position | Scale | Rotation,
    };

public:
    /// <summary>
    /// If checked, actor transform will be synchronized in local space of the parent actor (otherwise in world space).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10)")
    bool LocalSpace = false;

    /// <summary>
    /// Actor transform synchronization mode (flags).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20)")
    SyncModes SyncMode = SyncModes::All;

public:
    // [Script]
    void OnEnable() override;
    void OnDisable() override;

    // [INetworkSerializable]
    void Serialize(NetworkStream* stream) override;
    void Deserialize(NetworkStream* stream) override;
};

DECLARE_ENUM_OPERATORS(NetworkTransform::SyncModes);
