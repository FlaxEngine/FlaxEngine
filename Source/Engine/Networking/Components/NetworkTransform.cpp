// Copyright (c) Wojciech Figat. All rights reserved.

// Interpolation and prediction logic based on https://www.gabrielgambetta.com/client-server-game-architecture.html

#include "NetworkTransform.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Engine/Time.h"
#include "Engine/Level/Actor.h"
#include "Engine/Networking/NetworkManager.h"
#include "Engine/Networking/NetworkPeer.h"
#include "Engine/Networking/NetworkReplicator.h"
#include "Engine/Networking/NetworkStream.h"
#include "Engine/Networking/NetworkStats.h"
#include "Engine/Networking/INetworkDriver.h"
#include "Engine/Networking/NetworkRpc.h"

PACK_STRUCT(struct Data
    {
    uint8 LocalSpace : 1;
    uint8 HasSequenceIndex : 1;
    NetworkTransform::ReplicationComponents Components : 9;
    });

static_assert((int32)NetworkTransform::ReplicationComponents::All + 1 == 512, "Invalid ReplicationComponents bit count for Data.");

namespace
{
    // Percentage of local error that is acceptable (eg. 4 frames error)
    constexpr float Precision = 8.0f;

    template<typename T>
    FORCE_INLINE bool IsWithinPrecision(const Vector3Base<T>& currentDelta, const Vector3Base<T>& targetDelta)
    {
        const T targetDeltaMax = targetDelta.GetAbsolute().MaxValue();
        return targetDeltaMax > (T)ZeroTolerance && currentDelta.GetAbsolute().MaxValue() < targetDeltaMax * (T)Precision;
    }
}

NetworkTransform::NetworkTransform(const SpawnParams& params)
    : Script(params)
{
    // TODO: don't tick when using Default mode or with OwnedAuthoritative role to optimize cpu perf OR introduce TaskGraphSystem to batch NetworkTransform updates over Job System
    _tickUpdate = 1;
}

void NetworkTransform::SetSequenceIndex(uint16 value)
{
    NETWORK_RPC_IMPL(NetworkTransform, SetSequenceIndex, value);
    _currentSequenceIndex = value;
}

void NetworkTransform::OnEnable()
{
    // Initialize state
    _bufferHasDeltas = false;
    _currentSequenceIndex = 0;
    _lastFrameTransform = GetActor() ? GetActor()->GetTransform() : Transform::Identity;
    _buffer.Clear();

    // Register for replication
    NetworkReplicator::AddObject(this);
}

void NetworkTransform::OnDisable()
{
    // Unregister from replication
    NetworkReplicator::RemoveObject(this);

    _buffer.Resize(0);
}

void NetworkTransform::OnUpdate()
{
    // TODO: cache role in Deserialize to improve cpu perf
    const NetworkObjectRole role = NetworkReplicator::GetObjectRole(this);
    if (role == NetworkObjectRole::OwnedAuthoritative)
        return; // Ignore itself
    if (Mode == ReplicationModes::Default)
    {
        // Transform replicated in Deserialize
    }
    else if (role == NetworkObjectRole::ReplicatedSimulated && Mode == ReplicationModes::Prediction)
    {
        // Compute delta of the actor transformation simulated locally
        const Transform thisFrameTransform = GetActor() ? GetActor()->GetTransform() : Transform::Identity;
        Transform delta = thisFrameTransform - _lastFrameTransform;

        if (!delta.IsIdentity())
        {
            // Move to the next input sequence number
            _currentSequenceIndex++;

            // Add delta to buffer to re-apply after receiving authoritative transform value
            if (!_bufferHasDeltas)
            {
                _buffer.Clear();
                _bufferHasDeltas = true;
            }
            delta.Orientation = thisFrameTransform.Orientation; // Store absolute orientation value to prevent jittering when blending rotation deltas
            _buffer.Add({ 0.0f, _currentSequenceIndex, delta, });

            // Inform server about sequence number change (add offset to lead before server data)
            SetSequenceIndex(_currentSequenceIndex - 1);
        }
        _lastFrameTransform = thisFrameTransform;
    }
    else
    {
        float lag = 0.0f;
        // TODO: use lag from last used NetworkStream context
        if (NetworkManager::Peer && NetworkManager::Peer->NetworkDriver)
        {
            // Use lag from the RTT between server and the client
            const auto stats = NetworkManager::Peer->NetworkDriver->GetStats();
            lag = stats.RTT / 2000.0f;
        }
        else
        {
            // Default lag is based on the network manager update rate
            const float fps = NetworkManager::NetworkFPS;
            lag = 1.0f / fps;
        }

        // Find the two authoritative positions surrounding the rendering timestamp
        const float now = Time::Update.UnscaledTime.GetTotalSeconds();
        const float gameTime = now - lag;

        // Drop older positions
        while (_buffer.Count() >= 2 && _buffer[1].Timestamp <= gameTime)
            _buffer.RemoveAtKeepOrder(0);

        // Interpolate between the two surrounding authoritative positions
        if (_buffer.Count() >= 2 && _buffer[0].Timestamp <= gameTime && gameTime <= _buffer[1].Timestamp)
        {
            const auto& b0 = _buffer[0];
            const auto& b1 = _buffer[1];
            Transform transform;
            const float alpha = (gameTime - b0.Timestamp) / (b1.Timestamp - b0.Timestamp);
            Transform::Lerp(b0.Value, b1.Value, alpha, transform);
            Set(transform);
        }
        else if (_buffer.Count() == 1 && _buffer[0].Timestamp <= gameTime)
        {
            Set(_buffer[0].Value);
        }
    }
}

void NetworkTransform::Serialize(NetworkStream* stream)
{
    // Get transform
    Transform transform;
    if (const auto* parent = GetParent())
        transform = LocalSpace ? parent->GetLocalTransform() : parent->GetTransform();
    else
        transform = Transform::Identity;

    // Encode data
    Data data;
    Platform::MemoryClear(&data, sizeof(data));
    data.LocalSpace = LocalSpace;
    data.HasSequenceIndex = Mode == ReplicationModes::Prediction;
    data.Components = Components;
    stream->Write(data);
    if (EnumHasAllFlags(data.Components, ReplicationComponents::All))
    {
        stream->Write(transform);
    }
    else
    {
        if (EnumHasAllFlags(data.Components, ReplicationComponents::Position))
        {
            stream->Write(transform.Translation);
        }
        else if (EnumHasAnyFlags(data.Components, ReplicationComponents::Position))
        {
            if (EnumHasAnyFlags(data.Components, ReplicationComponents::PositionX))
                stream->Write(transform.Translation.X);
            if (EnumHasAnyFlags(data.Components, ReplicationComponents::PositionY))
                stream->Write(transform.Translation.Y);
            if (EnumHasAnyFlags(data.Components, ReplicationComponents::PositionZ))
                stream->Write(transform.Translation.Z);
        }
        if (EnumHasAllFlags(data.Components, ReplicationComponents::Scale))
        {
            stream->Write(transform.Scale);
        }
        else if (EnumHasAnyFlags(data.Components, ReplicationComponents::Scale))
        {
            if (EnumHasAnyFlags(data.Components, ReplicationComponents::ScaleX))
                stream->Write(transform.Scale.X);
            if (EnumHasAnyFlags(data.Components, ReplicationComponents::ScaleY))
                stream->Write(transform.Scale.Y);
            if (EnumHasAnyFlags(data.Components, ReplicationComponents::ScaleZ))
                stream->Write(transform.Scale.Z);
        }
        if (EnumHasAllFlags(data.Components, ReplicationComponents::Rotation))
        {
            stream->Write(transform.Orientation);
        }
        else if (EnumHasAnyFlags(data.Components, ReplicationComponents::Rotation))
        {
            const Float3 rotation = transform.Orientation.GetEuler();
            if (EnumHasAnyFlags(data.Components, ReplicationComponents::RotationX))
                stream->Write(rotation.X);
            if (EnumHasAnyFlags(data.Components, ReplicationComponents::RotationY))
                stream->Write(rotation.Y);
            if (EnumHasAnyFlags(data.Components, ReplicationComponents::RotationZ))
                stream->Write(rotation.Z);
        }
    }
    if (data.HasSequenceIndex)
        stream->Write(_currentSequenceIndex);
}

void NetworkTransform::Deserialize(NetworkStream* stream)
{
    // Get transform
    Transform transform;
    if (const auto* parent = GetParent())
        transform = LocalSpace ? parent->GetLocalTransform() : parent->GetTransform();
    else
        transform = Transform::Identity;
    Transform transformLocal = transform;

    // Decode data
    Data data;
    stream->Read(data);
    if (EnumHasAllFlags(data.Components, ReplicationComponents::All))
    {
        stream->Read(transform);
    }
    else
    {
        if (EnumHasAllFlags(data.Components, ReplicationComponents::Position))
        {
            stream->Read(transform.Translation);
        }
        else if (EnumHasAnyFlags(data.Components, ReplicationComponents::Position))
        {
            if (EnumHasAnyFlags(data.Components, ReplicationComponents::PositionX))
                stream->Read(transform.Translation.X);
            if (EnumHasAnyFlags(data.Components, ReplicationComponents::PositionY))
                stream->Read(transform.Translation.Y);
            if (EnumHasAnyFlags(data.Components, ReplicationComponents::PositionZ))
                stream->Read(transform.Translation.Z);
        }
        if (EnumHasAllFlags(data.Components, ReplicationComponents::Scale))
        {
            stream->Read(transform.Scale);
        }
        else if (EnumHasAnyFlags(data.Components, ReplicationComponents::Scale))
        {
            if (EnumHasAnyFlags(data.Components, ReplicationComponents::ScaleX))
                stream->Read(transform.Scale.X);
            if (EnumHasAnyFlags(data.Components, ReplicationComponents::ScaleY))
                stream->Read(transform.Scale.Y);
            if (EnumHasAnyFlags(data.Components, ReplicationComponents::ScaleZ))
                stream->Read(transform.Scale.Z);
        }
        if (EnumHasAllFlags(data.Components, ReplicationComponents::Rotation))
        {
            stream->Read(transform.Orientation);
        }
        else if (EnumHasAnyFlags(data.Components, ReplicationComponents::Rotation))
        {
            Float3 rotation = transform.Orientation.GetEuler();
            if (EnumHasAnyFlags(data.Components, ReplicationComponents::RotationX))
                stream->Read(rotation.X);
            if (EnumHasAnyFlags(data.Components, ReplicationComponents::RotationY))
                stream->Read(rotation.Y);
            if (EnumHasAnyFlags(data.Components, ReplicationComponents::RotationZ))
                stream->Read(rotation.Z);
            transform.Orientation = Quaternion::Euler(rotation);
        }
    }
    uint16 sequenceIndex = 0;
    if (data.HasSequenceIndex)
        stream->Read(sequenceIndex);
    if (data.LocalSpace != LocalSpace)
        return; // TODO: convert transform space if server-client have different values set

    const NetworkObjectRole role = NetworkReplicator::GetObjectRole(this);
    if (role == NetworkObjectRole::OwnedAuthoritative)
        return; // Ignore itself
    if (Mode == ReplicationModes::Default)
    {
        // Immediate set
        Set(transform);
    }
    else if (role == NetworkObjectRole::ReplicatedSimulated && Mode == ReplicationModes::Prediction)
    {
        const Transform transformAuthoritative = transform;
        const Transform transformDeltaBefore = transformAuthoritative - transformLocal;

        // Remove any transform deltas from local simulation that happened before the incoming authoritative transform data
        if (!_bufferHasDeltas)
        {
            _buffer.Clear();
            _bufferHasDeltas = true;
        }
        while (_buffer.Count() != 0 && _buffer[0].SequenceIndex < sequenceIndex)
            _buffer.RemoveAtKeepOrder(0);

        // Use received authoritative actor transformation but re-apply all deltas not yet processed by the server due to lag (reconciliation)
        for (auto& e : _buffer)
        {
            transform.Translation = transform.Translation + e.Value.Translation;
            transform.Scale = transform.Scale * e.Value.Scale;
        }
        // TODO: use euler angles or similar to cache/reapply rotation deltas (Quaternion jitters)
        transform.Orientation = transformLocal.Orientation;

        // If local simulation is very close to the authoritative server value then ignore slight error (based relative delta threshold)
        const Transform transformDeltaAfter = transformAuthoritative - transform;
        if (IsWithinPrecision(transformDeltaBefore.Translation, transformDeltaAfter.Translation) &&
            IsWithinPrecision(transformDeltaBefore.Scale, transformDeltaAfter.Scale)
        )
        {
            return;
        }

        // Set to the incoming value with applied local deltas
        Set(transform);
        _lastFrameTransform = transform;
    }
    else
    {
        // Add to the interpolation buffer
        const float now = Time::Update.UnscaledTime.GetTotalSeconds();
        _buffer.Add({ now, 0, transform });
        if (_bufferHasDeltas)
        {
            _buffer.Clear();
            _bufferHasDeltas = false;
        }
    }
}

void NetworkTransform::Set(const Transform& transform)
{
    if (auto* parent = GetParent())
    {
        if (LocalSpace)
            parent->SetLocalTransform(transform);
        else
            parent->SetTransform(transform);
    }
}
