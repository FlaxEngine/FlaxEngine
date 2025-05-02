// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/Script.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Networking/INetworkSerializable.h"

/// <summary>
/// Actor script component that synchronizes the Transform over the network.
/// </summary>
/// <remarks>Interpolation and prediction logic based on https://www.gabrielgambetta.com/client-server-game-architecture.html.</remarks>
API_CLASS(Namespace="FlaxEngine.Networking", Attributes="Category(\"Flax Engine\")") class FLAXENGINE_API NetworkTransform : public Script, public INetworkSerializable
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE(NetworkTransform);

    /// <summary>
    /// Actor transform replication components (flags).
    /// </summary>
    API_ENUM(Attributes="Flags") enum class ReplicationComponents
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

    /// <summary>
    /// Actor transform replication modes.
    /// </summary>
    API_ENUM() enum class ReplicationModes
    {
        // The transform replicated from the owner (raw replication data messages that might result in sudden object jumps when moving).
        Default,
        // The transform replicated from the owner with local interpolation between received data to provide smoother movement.
        Interpolation,
        // The transform replicated from the owner but with local prediction (eg. player character that has local simulation but is validated against authoritative server).
        Prediction,
    };

private:
    struct BufferedItem
    {
        float Timestamp;
        uint16 SequenceIndex;
        Transform Value;
    };

    bool _bufferHasDeltas;
    uint16 _currentSequenceIndex = 0;
    Transform _lastFrameTransform;
    Array<BufferedItem> _buffer;

public:
    /// <summary>
    /// If checked, actor transform will be synchronized in local space of the parent actor (otherwise in world space).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10)")
    bool LocalSpace = false;
    
    /// <summary>
    /// Actor transform replication components (flags).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20)")
    ReplicationComponents Components = ReplicationComponents::All;

    /// <summary>
    /// Actor transform replication mode.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30)")
    ReplicationModes Mode = ReplicationModes::Default;

private:
    API_FUNCTION(Hidden, NetworkRpc=Server) void SetSequenceIndex(uint16 value);
    
public:
    // [Script]
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;

    // [INetworkSerializable]
    void Serialize(NetworkStream* stream) override;
    void Deserialize(NetworkStream* stream) override;

private:
    void Set(const Transform& transform);
};

DECLARE_ENUM_OPERATORS(NetworkTransform::ReplicationComponents);
