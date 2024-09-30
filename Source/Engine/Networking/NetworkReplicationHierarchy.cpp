// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "NetworkReplicationHierarchy.h"
#include "NetworkManager.h"
#include "Engine/Level/Actor.h"
#include "Engine/Level/SceneObject.h"

uint16 NetworkReplicationNodeObjectCounter = 0;
NetworkClientsMask NetworkClientsMask::All = { MAX_uint64, MAX_uint64 };

Actor* NetworkReplicationHierarchyObject::GetActor() const
{
    auto* actor = ScriptingObject::Cast<Actor>(Object);
    if (!actor)
    {
        if (const auto* sceneObject = ScriptingObject::Cast<SceneObject>(Object))
            actor = sceneObject->GetParent();
    }
    return actor;
}

void NetworkReplicationHierarchyUpdateResult::Init()
{
    _clientsHaveLocation = false;
    _clients.Resize(NetworkManager::Clients.Count());
    _clientsMask = NetworkManager::Mode == NetworkManagerMode::Client ? NetworkClientsMask::All : NetworkClientsMask();
    for (int32 i = 0; i < _clients.Count(); i++)
        _clientsMask.SetBit(i);
    _entries.Clear();
    ReplicationScale = 1.0f;
}

void NetworkReplicationHierarchyUpdateResult::SetClientLocation(int32 clientIndex, const Vector3& location)
{
    CHECK(clientIndex >= 0 && clientIndex < _clients.Count());
    _clientsHaveLocation = true;
    Client& client = _clients[clientIndex];
    client.HasLocation = true;
    client.Location = location;
}

bool NetworkReplicationHierarchyUpdateResult::GetClientLocation(int32 clientIndex, Vector3& location) const
{
    CHECK_RETURN(clientIndex >= 0 && clientIndex < _clients.Count(), false);
    const Client& client = _clients[clientIndex];
    location = client.Location;
    return client.HasLocation;
}

void NetworkReplicationNode::AddObject(NetworkReplicationHierarchyObject obj)
{
    if (obj.ReplicationFPS > ZeroTolerance) // > 0
    {
        // Randomize initial replication update to spread rep rates more evenly for large scenes that register all objects within the same frame
        obj.ReplicationUpdatesLeft = NetworkReplicationNodeObjectCounter++ % Math::Clamp(Math::RoundToInt(NetworkManager::NetworkFPS / obj.ReplicationFPS), 1, 60);
    }

    Objects.Add(obj);
}

bool NetworkReplicationNode::RemoveObject(ScriptingObject* obj)
{
    return !Objects.Remove(obj);
}

bool NetworkReplicationNode::GetObject(ScriptingObject* obj, NetworkReplicationHierarchyObject& result)
{
    const int32 index = Objects.Find(obj);
    if (index != -1)
    {
        result = Objects[index];
        return true;
    }
    return false;
}

bool NetworkReplicationNode::SetObject(const NetworkReplicationHierarchyObject& value)
{
    const int32 index = Objects.Find(value.Object.Get());
    if (index != -1)
    {
        Objects[index] = value;
        return true;
    }
    return false;
}

bool NetworkReplicationNode::DirtyObject(ScriptingObject* obj)
{
    const int32 index = Objects.Find(obj);
    if (index != -1)
    {
        NetworkReplicationHierarchyObject& e = Objects[index];
        if (e.ReplicationFPS < -ZeroTolerance) // < 0
        {
            // Indicate for manual sync (see logic in Update)
            e.ReplicationUpdatesLeft = 1;
        }
        else
        {
            // Replicate it next frame
            e.ReplicationUpdatesLeft = 0;
        }
    }
    return index != -1;
}

void NetworkReplicationNode::Update(NetworkReplicationHierarchyUpdateResult* result)
{
    CHECK(result);
    const float networkFPS = NetworkManager::NetworkFPS / result->ReplicationScale;
    for (NetworkReplicationHierarchyObject& obj : Objects)
    {
        if (obj.ReplicationFPS < -ZeroTolerance) // < 0
        {
            if (obj.ReplicationUpdatesLeft)
            {
                // Marked as dirty to sync manually
                obj.ReplicationUpdatesLeft = 0;
                result->AddObject(obj.Object);
            }
            continue;
        }
        else if (obj.ReplicationFPS < ZeroTolerance) // == 0
        {
            // Always relevant
            result->AddObject(obj.Object);
        }
        else if (obj.ReplicationUpdatesLeft > 0)
        {
            // Move to the next frame
            obj.ReplicationUpdatesLeft--;
        }
        else
        {
            NetworkClientsMask targetClients = result->GetClientsMask();
            if (result->_clientsHaveLocation && obj.CullDistance > 0.0f)
            {
                // Cull object against viewers locations
                if (const Actor* actor = obj.GetActor())
                {
                    const Vector3 objPosition = actor->GetPosition();
                    const Real cullDistanceSq = Math::Square(obj.CullDistance);
                    for (int32 clientIndex = 0; clientIndex < result->_clients.Count(); clientIndex++)
                    {
                        const auto& client = result->_clients[clientIndex];
                        if (client.HasLocation)
                        {
                            const Real distanceSq = Vector3::DistanceSquared(objPosition, client.Location);
                            // TODO: scale down replication FPS when object is far away from all clients (eg. by 10-50%)
                            if (distanceSq >= cullDistanceSq)
                            {
                                // Object is too far from this viewer so don't send data to him
                                targetClients.UnsetBit(clientIndex);
                            }
                        }
                    }
                }
            }
            if (targetClients && obj.Object)
            {
                // Replicate this frame
                result->AddObject(obj.Object, targetClients);
            }

            // Calculate frames until next replication
            obj.ReplicationUpdatesLeft = (uint16)Math::Clamp<int32>(Math::RoundToInt(networkFPS / obj.ReplicationFPS) - 1, 0, MAX_uint16);
        }
    }
}

NetworkReplicationGridNode::~NetworkReplicationGridNode()
{
    for (const auto& e : _children)
        Delete(e.Value.Node);
}

void NetworkReplicationGridNode::AddObject(NetworkReplicationHierarchyObject obj)
{
    // Chunk actors locations into a grid coordinates
    Int3 coord = Int3::Zero;
    if (const Actor* actor = obj.GetActor())
    {
        coord = actor->GetPosition() / CellSize;
    }

    Cell* cell = _children.TryGet(coord);
    if (!cell)
    {
        // Allocate new cell
        cell = &_children[coord];
        cell->Node = New<NetworkReplicationNode>();
        cell->MinCullDistance = obj.CullDistance;
    }
    cell->Node->AddObject(obj);
    _objectToCell[obj.Object.Get()] = coord;

    // Cache minimum culling distance for a whole cell to skip it at once
    cell->MinCullDistance = Math::Min(cell->MinCullDistance, obj.CullDistance);
}

bool NetworkReplicationGridNode::RemoveObject(ScriptingObject* obj)
{
    Int3 coord;
    if (!_objectToCell.TryGet(obj, coord))
    {
        return false;
    }
    if (_children[coord].Node->RemoveObject(obj))
    {
        _objectToCell.Remove(obj);
        // TODO: remove empty cells?
        // TODO: update MinCullDistance for cell?
        return true;
    }
    return false;
}

bool NetworkReplicationGridNode::GetObject(ScriptingObject* obj, NetworkReplicationHierarchyObject& result)
{
    Int3 coord;
    if (!_objectToCell.TryGet(obj, coord))
    {
        return false;
    }
    return _children[coord].Node->GetObject(obj, result);
}

bool NetworkReplicationGridNode::SetObject(const NetworkReplicationHierarchyObject& value)
{
    Int3 coord;
    if (!_objectToCell.TryGet(value.Object.Get(), coord))
    {
        return false;
    }
    return _children[coord].Node->SetObject(value);
}

bool NetworkReplicationGridNode::DirtyObject(ScriptingObject* obj)
{
    Int3 coord;
    if (_objectToCell.TryGet(obj, coord))
    {
        return _children[coord].Node->DirtyObject(obj);
    }
    return NetworkReplicationNode::DirtyObject(obj);
}

void NetworkReplicationGridNode::Update(NetworkReplicationHierarchyUpdateResult* result)
{
    CHECK(result);
    if (result->_clientsHaveLocation)
    {
        // Update only cells within a range
        const Real cellRadiusSq = Math::Square(CellSize * 1.414f);
        for (const auto& e : _children)
        {
            const Vector3 cellPosition = (e.Key * CellSize) + (CellSize * 0.5f);
            Real distanceSq = MAX_Real;
            for (auto& client : result->_clients)
            {
                if (client.HasLocation)
                    distanceSq = Math::Min(distanceSq, Vector3::DistanceSquared(cellPosition, client.Location));
            }
            const Real minCullDistanceSq = Math::Square(e.Value.MinCullDistance);
            if (distanceSq < minCullDistanceSq + cellRadiusSq)
            {
                e.Value.Node->Update(result);
            }
        }
    }
    else
    {
        // Brute-force over all cells
        for (const auto& e : _children)
        {
            e.Value.Node->Update(result);
        }
    }
}
