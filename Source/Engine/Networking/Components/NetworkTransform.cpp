// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "NetworkTransform.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Level/Actor.h"
#include "Engine/Networking/NetworkReplicator.h"
#include "Engine/Networking/NetworkStream.h"

PACK_STRUCT(struct Data
    {
    uint8 LocalSpace : 1;
    NetworkTransform::SyncModes SyncMode : 9;
    });

static_assert((int32)NetworkTransform::SyncModes::All + 1 == 512, "Invalid SyncModes bit count for Data.");

NetworkTransform::NetworkTransform(const SpawnParams& params)
    : Script(params)
{
}

void NetworkTransform::OnEnable()
{
    // Register for replication
    NetworkReplicator::AddObject(this);
}

void NetworkTransform::OnDisable()
{
    // Unregister from replication
    NetworkReplicator::RemoveObject(this);
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
    data.LocalSpace = LocalSpace;
    data.SyncMode = SyncMode;
    stream->Write(data);
    if ((data.SyncMode & SyncModes::All) == (int)SyncModes::All)
    {
        stream->Write(transform);
    }
    else
    {
        if ((data.SyncMode & SyncModes::Position) == (int)SyncModes::Position)
        {
            stream->Write(transform.Translation);
        }
        else if (data.SyncMode & SyncModes::Position)
        {
            if (data.SyncMode & SyncModes::PositionX)
                stream->Write(transform.Translation.X);
            if (data.SyncMode & SyncModes::PositionY)
                stream->Write(transform.Translation.X);
            if (data.SyncMode & SyncModes::PositionZ)
                stream->Write(transform.Translation.X);
        }
        if ((data.SyncMode & SyncModes::Scale) == (int)SyncModes::Scale)
        {
            stream->Write(transform.Scale);
        }
        else if (data.SyncMode & SyncModes::Scale)
        {
            if (data.SyncMode & SyncModes::ScaleX)
                stream->Write(transform.Scale.X);
            if (data.SyncMode & SyncModes::ScaleY)
                stream->Write(transform.Scale.X);
            if (data.SyncMode & SyncModes::ScaleZ)
                stream->Write(transform.Scale.X);
        }
        if ((data.SyncMode & SyncModes::Rotation) == (int)SyncModes::Rotation)
        {
            const Float3 rotation = transform.Orientation.GetEuler();
            stream->Write(rotation);
        }
        else if (data.SyncMode & SyncModes::Rotation)
        {
            const Float3 rotation = transform.Orientation.GetEuler();
            if (data.SyncMode & SyncModes::RotationX)
                stream->Write(rotation.X);
            if (data.SyncMode & SyncModes::RotationY)
                stream->Write(rotation.Y);
            if (data.SyncMode & SyncModes::RotationZ)
                stream->Write(rotation.Z);
        }
    }
}

void NetworkTransform::Deserialize(NetworkStream* stream)
{
    // Get transform
    Transform transform;
    if (const auto* parent = GetParent())
        transform = LocalSpace ? parent->GetLocalTransform() : parent->GetTransform();
    else
        transform = Transform::Identity;

    // Decode data
    Data data;
    stream->Read(data);
    if ((data.SyncMode & SyncModes::All) == (int)SyncModes::All)
    {
        stream->Read(transform);
    }
    else
    {
        if ((data.SyncMode & SyncModes::Position) == (int)SyncModes::Position)
        {
            stream->Read(transform.Translation);
        }
        else if (data.SyncMode & SyncModes::Position)
        {
            if (data.SyncMode & SyncModes::PositionX)
                stream->Read(transform.Translation.X);
            if (data.SyncMode & SyncModes::PositionY)
                stream->Read(transform.Translation.X);
            if (data.SyncMode & SyncModes::PositionZ)
                stream->Read(transform.Translation.X);
        }
        if ((data.SyncMode & SyncModes::Scale) == (int)SyncModes::Scale)
        {
            stream->Read(transform.Scale);
        }
        else if (data.SyncMode & SyncModes::Scale)
        {
            if (data.SyncMode & SyncModes::ScaleX)
                stream->Read(transform.Scale.X);
            if (data.SyncMode & SyncModes::ScaleY)
                stream->Read(transform.Scale.X);
            if (data.SyncMode & SyncModes::ScaleZ)
                stream->Read(transform.Scale.X);
        }
        if ((data.SyncMode & SyncModes::Rotation) == (int)SyncModes::Rotation)
        {
            Float3 rotation;
            stream->Read(rotation);
            transform.Orientation = Quaternion::Euler(rotation);
        }
        else if (data.SyncMode & SyncModes::Rotation)
        {
            Float3 rotation = transform.Orientation.GetEuler();
            if (data.SyncMode & SyncModes::RotationX)
                stream->Read(rotation.X);
            if (data.SyncMode & SyncModes::RotationY)
                stream->Read(rotation.Y);
            if (data.SyncMode & SyncModes::RotationZ)
                stream->Read(rotation.Z);
            transform.Orientation = Quaternion::Euler(rotation);
        }
    }

    // Set transform
    if (auto* parent = GetParent())
    {
        if (data.LocalSpace)
            parent->SetLocalTransform(transform);
        else
            parent->SetTransform(transform);
    }
}
