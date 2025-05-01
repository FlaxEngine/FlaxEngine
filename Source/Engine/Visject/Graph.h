// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"
#include "VisjectMeta.h"
#include "GraphNode.h"
#include "GraphParameter.h"
#include "Engine/Content/Deprecated.h"
#include "Engine/Serialization/ReadStream.h"
#include "Engine/Serialization/WriteStream.h"

// [Deprecated on 31.07.2020, expires on 31.07.2022]
extern FLAXENGINE_API void ReadOldGraphParamValue_Deprecated(byte graphParamType, ReadStream* stream, GraphParameter* param);
extern FLAXENGINE_API void ReadOldGraphNodeValue_Deprecated(ReadStream* stream, Variant& result);
extern FLAXENGINE_API void ReadOldGraphBoxType_Deprecated(uint32 connectionType, VariantType& type);
extern FLAXENGINE_API StringView GetGraphFunctionTypeName_Deprecated(const Variant& v);

/// <summary>
/// Visject graph
/// </summary>
template<class NodeType, class BoxType, class ParameterType>
class Graph
{
public:
    typedef Graph<NodeType, BoxType, ParameterType> GraphType;
    typedef NodeType Node;
    typedef BoxType Box;
    typedef ParameterType Parameter;

private:
    struct TmpConnectionHint
    {
        Node* Node;
        byte BoxID;
    };

public:
    NON_COPYABLE(Graph);

    /// <summary>
    /// All graph nodes
    /// </summary>
    Array<Node> Nodes;

    /// <summary>
    /// All graph parameters
    /// </summary>
    Array<Parameter> Parameters;

    /// <summary>
    /// Metadata for whole graph
    /// </summary>
    VisjectMeta Meta;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="Graph"/> class.
    /// </summary>
    Graph()
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="Graph"/> class.
    /// </summary>
    virtual ~Graph()
    {
    }

public:
    /// <summary>
    /// Save graph to the stream
    /// </summary>
    /// <param name="stream">Output stream</param>
    /// <param name="saveMeta">True if save all loaded metadata</param>
    /// <returns>True if cannot save data, otherwise false</returns>
    virtual bool Save(WriteStream* stream, bool saveMeta) const
    {
        // Magic Code
        stream->WriteInt32(1963542358);

        // Version
        stream->WriteInt32(7000);

        // Nodes count
        stream->WriteInt32(Nodes.Count());

        // Parameters count
        stream->WriteInt32(Parameters.Count());

        // For each node
        for (int32 i = 0; i < Nodes.Count(); i++)
        {
            auto node = &Nodes[i];
            stream->WriteUint32(node->ID);
            stream->WriteUint32(node->Type);
        }

        // For each param
        for (int32 i = 0; i < Parameters.Count(); i++)
        {
            const Parameter* param = &Parameters[i];
            stream->Write(param->Type);
            stream->Write(param->Identifier);
            stream->Write(param->Name, 97);
            stream->WriteBool(param->IsPublic);
            stream->Write(param->Value);
            if (param->Meta.Save(stream, saveMeta))
                return true;
        }

        // For each node
        Array<const Box*, InlinedAllocation<32>> boxes;
        for (int32 i = 0; i < Nodes.Count(); i++)
        {
            const Node* node = &Nodes[i];

            // Values
            stream->WriteInt32(node->Values.Count());
            for (int32 j = 0; j < node->Values.Count(); j++)
                stream->Write(node->Values[j]);

            // Boxes
            node->GetBoxes(boxes);
            stream->WriteUint16(boxes.Count());
            for (int32 j = 0; j < boxes.Count(); j++)
            {
                const Box* box = boxes[j];
                stream->WriteByte(box->ID);
                stream->Write(box->Type);
                stream->WriteUint16(box->Connections.Count());
                for (int32 k = 0; k < box->Connections.Count(); k++)
                {
                    auto targetBox = box->Connections[k];
                    if (targetBox == nullptr)
                        return true;
                    stream->WriteUint32(targetBox->template GetParent<Node>()->ID);
                    stream->WriteByte(targetBox->ID);
                }
            }

            // Meta
            if (node->Meta.Save(stream, saveMeta))
                return true;
        }

        // Meta
        if (Meta.Save(stream, saveMeta))
            return true;

        // Ending char
        stream->WriteByte('\t');

        return false;
    }

    /// <summary>
    /// Load graph from the stream
    /// </summary>
    /// <param name="stream">Input stream</param>
    /// <param name="loadMeta">True if load all saved metadata</param>
    /// <returns>True if cannot load data, otherwise false</returns>
    virtual bool Load(ReadStream* stream, bool loadMeta)
    {
        // Clear previous data
        Clear();

        // Magic Code
        int32 tmp;
        stream->ReadInt32(&tmp);
        if (tmp != 1963542358)
        {
            LOG(Warning, "Invalid data.");
            return true;
        }

        // Version
        uint32 version;
        stream->ReadUint32(&version);

        Array<TmpConnectionHint> tmpHints;
        if (version < 7000)
        {
            // [Deprecated on 31.07.2020, expires on 31.07.2022]
            MARK_CONTENT_DEPRECATED();

            // Time saved
            int64 timeSaved;
            stream->ReadInt64(&timeSaved);

            // Nodes count
            int32 nodesCount;
            stream->ReadInt32(&nodesCount);
            Nodes.Resize(nodesCount, false);

            // Parameters count
            int32 parametersCount;
            stream->ReadInt32(&parametersCount);
            Parameters.Resize(parametersCount, false);

            // For each node
            for (int32 i = 0; i < nodesCount; i++)
            {
                auto node = &Nodes[i];

                // ID
                stream->ReadUint32(&node->ID);

                // Type
                stream->ReadUint32(&node->Type);

                if (onNodeCreated(node))
                    return true;
            }

            // For each param
            for (int32 i = 0; i < parametersCount; i++)
            {
                // Create param
                auto param = &Parameters[i];

                // Properties
                auto type = stream->ReadByte();
                stream->Read(param->Identifier);
                stream->Read(param->Name, 97);
                param->IsPublic = stream->ReadBool();
                bool isStatic = stream->ReadBool();
                bool isUIVisible = stream->ReadBool();
                bool isUIEditable = stream->ReadBool();

                // References [Deprecated]
                int32 refsCount;
                stream->ReadInt32(&refsCount);
                for (int32 j = 0; j < refsCount; j++)
                {
                    uint32 refID;
                    stream->ReadUint32(&refID);
                }

                // Value
                ReadOldGraphParamValue_Deprecated(type, stream, param);

                // Meta
                if (param->Meta.Load(stream, loadMeta))
                    return true;

                if (onParamCreated(param))
                    return true;
            }

            // For each node
            for (int32 i = 0; i < nodesCount; i++)
            {
                auto node = &Nodes[i];

                // Values
                int32 valuesCnt;
                stream->ReadInt32(&valuesCnt);
                node->Values.Resize(valuesCnt);
                for (int32 j = 0; j < valuesCnt; j++)
                    ReadOldGraphNodeValue_Deprecated(stream, node->Values[j]);

                // Boxes
                uint16 boxesCount;
                stream->ReadUint16(&boxesCount);
                node->Boxes.Clear();
                for (int32 j = 0; j < boxesCount; j++)
                {
                    byte boxID = stream->ReadByte();
                    node->Boxes.Resize(node->Boxes.Count() > boxID + 1 ? node->Boxes.Count() : boxID + 1);
                    Box* box = &node->Boxes[boxID];

                    box->Parent = node;
                    box->ID = boxID;
                    uint32 connectionType;
                    stream->ReadUint32((uint32*)&connectionType);
                    ReadOldGraphBoxType_Deprecated(connectionType, box->Type);

                    uint16 connectionsCount;
                    stream->ReadUint16(&connectionsCount);
                    box->Connections.Resize(connectionsCount);
                    for (int32 k = 0; k < connectionsCount; k++)
                    {
                        uint32 targetNodeID;
                        stream->ReadUint32(&targetNodeID);
                        byte targetBoxID = stream->ReadByte();
                        TmpConnectionHint hint;
                        hint.Node = GetNode(targetNodeID);
                        if (hint.Node == nullptr)
                            return true;
                        hint.BoxID = targetBoxID;
                        box->Connections[k] = (Box*)(intptr)tmpHints.Count();
                        tmpHints.Add(hint);
                    }
                }

                // Meta
                if (node->Meta.Load(stream, loadMeta))
                    return true;

                if (onNodeLoaded(node))
                    return true;
            }

            // Visject Meta
            if (Meta.Load(stream, loadMeta))
                return true;

            // Setup connections
            for (int32 i = 0; i < Nodes.Count(); i++)
            {
                auto node = &Nodes[i];
                for (int32 j = 0; j < node->Boxes.Count(); j++)
                {
                    Box* box = &node->Boxes[j];
                    if (box->Parent == nullptr)
                        continue;
                    for (int32 k = 0; k < box->Connections.Count(); k++)
                    {
                        int32 hintIndex = (int32)(intptr)box->Connections[k];
                        TmpConnectionHint hint = tmpHints[hintIndex];
                        box->Connections[k] = hint.Node->GetBox(hint.BoxID);
                    }
                }
            }

            // Ending char
            byte end;
            stream->ReadByte(&end);
            if (end != '\t')
            {
                return true;
            }
        }
        else if (version == 7000)
        {
            // Nodes count
            int32 nodesCount;
            stream->ReadInt32(&nodesCount);
            Nodes.Resize(nodesCount, false);

            // Parameters count
            int32 parametersCount;
            stream->ReadInt32(&parametersCount);
            Parameters.Resize(parametersCount, false);

            // For each node
            for (int32 i = 0; i < nodesCount; i++)
            {
                auto node = &Nodes[i];
                stream->ReadUint32(&node->ID);
                stream->ReadUint32(&node->Type);
                if (onNodeCreated(node))
                    return true;
            }

            // For each param
            for (int32 i = 0; i < parametersCount; i++)
            {
                auto param = &Parameters[i];
                stream->Read(param->Type);
                stream->Read(param->Identifier);
                stream->Read(param->Name, 97);
                param->IsPublic = stream->ReadBool();
                stream->Read(param->Value);
                if (param->Meta.Load(stream, loadMeta))
                    return true;
                if (onParamCreated(param))
                    return true;
            }

            // For each node
            for (int32 i = 0; i < nodesCount; i++)
            {
                auto node = &Nodes[i];

                // Values
                int32 valuesCnt;
                stream->ReadInt32(&valuesCnt);
                node->Values.Resize(valuesCnt);
                for (int32 j = 0; j < valuesCnt; j++)
                    stream->Read(node->Values[j]);

                // Boxes
                uint16 boxesCount;
                stream->ReadUint16(&boxesCount);
                node->Boxes.Clear();
                for (int32 j = 0; j < boxesCount; j++)
                {
                    byte boxID = stream->ReadByte();
                    node->Boxes.Resize(node->Boxes.Count() > boxID + 1 ? node->Boxes.Count() : boxID + 1);
                    Box* box = &node->Boxes[boxID];
                    box->Parent = node;
                    box->ID = boxID;
                    stream->Read(box->Type);
                    uint16 connectionsCount;
                    stream->ReadUint16(&connectionsCount);
                    box->Connections.Resize(connectionsCount);
                    for (int32 k = 0; k < connectionsCount; k++)
                    {
                        uint32 targetNodeID;
                        stream->ReadUint32(&targetNodeID);
                        byte targetBoxID = stream->ReadByte();
                        TmpConnectionHint hint;
                        hint.Node = GetNode(targetNodeID);
                        if (hint.Node == nullptr)
                            return true;
                        hint.BoxID = targetBoxID;
                        box->Connections[k] = (Box*)(intptr)tmpHints.Count();
                        tmpHints.Add(hint);
                    }
                }

                // Meta
                if (node->Meta.Load(stream, loadMeta))
                    return true;

                if (onNodeLoaded(node))
                    return true;
            }

            // Visject Meta
            if (Meta.Load(stream, loadMeta))
                return true;

            // Setup connections
            for (int32 i = 0; i < Nodes.Count(); i++)
            {
                auto node = &Nodes[i];
                for (int32 j = 0; j < node->Boxes.Count(); j++)
                {
                    Box* box = &node->Boxes[j];
                    if (box->Parent == nullptr)
                        continue;
                    for (int32 k = 0; k < box->Connections.Count(); k++)
                    {
                        int32 hintIndex = (int32)(intptr)box->Connections[k];
                        TmpConnectionHint hint = tmpHints[hintIndex];
                        box->Connections[k] = hint.Node->GetBox(hint.BoxID);
                    }
                }
            }

            // Ending char
            byte end;
            stream->ReadByte(&end);
            if (end != '\t')
                return true;
        }

        return false;
    }

public:
    /// <summary>
    /// Find node by ID
    /// </summary>
    /// <param name="id">Node ID</param>
    /// <returns>Node or null if cannot find it</returns>
    Node* GetNode(uint32 id)
    {
        Node* result = nullptr;
        for (int32 i = 0; i < Nodes.Count(); i++)
        {
            if (Nodes[i].ID == id)
            {
                result = &Nodes[i];
                break;
            }
        }
        return result;
    }

    /// <summary>
    /// Find parameter by ID
    /// </summary>
    /// <param name="id">Parameter ID</param>
    /// <returns>Parameter or null if cannot find it</returns>
    Parameter* GetParameter(const Guid& id)
    {
        Parameter* result = nullptr;
        for (int32 i = 0; i < Parameters.Count(); i++)
        {
            if (Parameters[i].Identifier == id)
            {
                result = &Parameters[i];
                break;
            }
        }
        return result;
    }

    /// <summary>
    /// Find parameter by ID
    /// </summary>
    /// <param name="id">Parameter ID</param>
    /// <param name="outIndex">Parameter index</param>
    /// <returns>Parameter or null if cannot find it</returns>
    Parameter* GetParameter(const Guid& id, int32& outIndex)
    {
        Parameter* result = nullptr;
        outIndex = INVALID_INDEX;
        for (int32 i = 0; i < Parameters.Count(); i++)
        {
            if (Parameters[i].Identifier == id)
            {
                result = &Parameters[i];
                outIndex = i;
                break;
            }
        }
        return result;
    }

    /// <summary>
    /// Clear whole graph data
    /// </summary>
    virtual void Clear()
    {
        Nodes.Resize(0);
        Parameters.Resize(0);
        Meta.Release();
    }

public:
#if USE_EDITOR

    /// <summary>
    /// Gets the asset references.
    /// </summary>
    /// </remarks>
    /// <param name="assets">The output collection of the asset ids referenced by this object.</param>
    virtual void GetReferences(Array<Guid>& assets) const
    {
        for (int32 i = 0; i < Parameters.Count(); i++)
        {
            const auto& p = Parameters[i];
            const Guid id = (Guid)p.Value;
            if (id.IsValid())
                assets.Add(id);
        }

        for (int32 i = 0; i < Nodes.Count(); i++)
        {
            const auto& n = Nodes[i];
            for (int32 j = 0; j < n.Values.Count(); j++)
            {
                const Guid id = (Guid)n.Values[j];
                if (id.IsValid())
                    assets.Add(id);
            }
        }
    }

#endif

protected:
    virtual bool onNodeCreated(Node* n)
    {
        return false;
    }

    virtual bool onNodeLoaded(Node* n)
    {
        return false;
    }

    virtual bool onParamCreated(Parameter* p)
    {
        return false;
    }

    uint32 getFreeNodeID() const
    {
        uint32 result = 1;
        while (true)
        {
            bool valid = true;
            for (int32 i = 0; i < Nodes.Count(); i++)
            {
                if (Nodes[i].Identifier == result)
                {
                    result++;
                    valid = false;
                    break;
                }
            }
            if (valid)
                break;
        }
        return result;
    }
};
