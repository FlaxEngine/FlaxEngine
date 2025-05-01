// Copyright (c) Wojciech Figat. All rights reserved.

#include "AnimGraph.h"
#include "Engine/Debug/DebugLog.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Scripting/ManagedCLR/MException.h"
#include "Engine/Scripting/Internal/InternalCalls.h"
#include "Engine/Content/Assets/SkinnedModel.h"

#if USE_CSHARP

struct InternalInitData
{
    MArray* Values;
    MObject* BaseModel;
};

struct InternalContext
{
    AnimGraph* Graph;
    AnimGraphExecutor* GraphExecutor;
    AnimGraph::Node* Node;
    uint32 NodeId;
    int32 BoxId;
    float DeltaTime;
    uint64 CurrentFrameIndex;
    MObject* BaseModel;
    MObject* Instance;
};

struct InternalImpulse
{
    int32 NodesCount;
    int32 Unused;
    Transform* Nodes;
    Transform RootMotion;
    float Position;
    float Length;
};

static_assert(sizeof(InternalImpulse) == sizeof(AnimGraphImpulse), "Please update managed impulse type for Anim Graph to match the C++ backend data layout.");

DEFINE_INTERNAL_CALL(bool) AnimGraphInternal_HasConnection(InternalContext* context, int32 boxId)
{
    const auto box = context->Node->TryGetBox(boxId);
    if (box == nullptr)
        DebugLog::ThrowArgumentOutOfRange("boxId");
    return box->HasConnection();
}

DEFINE_INTERNAL_CALL(MObject*) AnimGraphInternal_GetInputValue(InternalContext* context, int32 boxId)
{
    const auto box = context->Node->TryGetBox(boxId);
    if (box == nullptr)
        DebugLog::ThrowArgumentOutOfRange("boxId");
    if (!box->HasConnection())
        DebugLog::ThrowArgument("boxId", "This box has no connection. Use HasConnection to check if can get input value.");

    Variant value = Variant::Null;
    context->GraphExecutor->GetInputValue(box, value);

    // Cast value to prevent implicit value conversion issues and handling this on C# side
    if (!(box->Type.Type == VariantType::Void && value.Type.Type == VariantType::Pointer))
        value = Variant::Cast(value, box->Type);
    return MUtils::BoxVariant(value);
}

DEFINE_INTERNAL_CALL(AnimGraphImpulse*) AnimGraphInternal_GetOutputImpulseData(InternalContext* context)
{
    const auto nodes = context->Node->GetNodes(context->GraphExecutor);
    context->GraphExecutor->InitNodes(nodes);
    return nodes;
}

#endif

void AnimGraphExecutor::initRuntime()
{
    ADD_INTERNAL_CALL("FlaxEngine.AnimationGraph::Internal_HasConnection", &AnimGraphInternal_HasConnection);
    ADD_INTERNAL_CALL("FlaxEngine.AnimationGraph::Internal_GetInputValue", &AnimGraphInternal_GetInputValue);
    ADD_INTERNAL_CALL("FlaxEngine.AnimationGraph::Internal_GetOutputImpulseData", &AnimGraphInternal_GetOutputImpulseData);
}

void AnimGraphExecutor::ProcessGroupCustom(Box* boxBase, Node* nodeBase, Value& value)
{
#if USE_CSHARP
    auto& context = *Context.Get();
    if (context.ValueCache.TryGet(boxBase, value))
        return;
    auto box = (AnimGraphBox*)boxBase;
    auto node = (AnimGraphNode*)nodeBase;
    auto& data = node->Data.Custom;
    value = Value::Null;

    // Skip invalid nodes
    if (data.Evaluate == nullptr)
        return;

    // Prepare node context
    InternalContext internalContext;
    internalContext.Graph = &_graph;
    internalContext.GraphExecutor = this;
    internalContext.Node = node;
    internalContext.NodeId = node->ID;
    internalContext.BoxId = box->ID;
    internalContext.DeltaTime = context.DeltaTime;
    internalContext.CurrentFrameIndex = context.CurrentFrameIndex;
    internalContext.BaseModel = _graph.BaseModel->GetOrCreateManagedInstance();
    internalContext.Instance = context.Data->Object ? context.Data->Object->GetOrCreateManagedInstance() : nullptr;

    // Peek managed object
    const auto obj = MCore::GCHandle::GetTarget(data.Handle);
    if (obj == nullptr)
    {
        LOG(Warning, "Custom node instance is null.");
        return;
    }

    // Evaluate node
    void* params[1];
    params[0] = &internalContext;
    MObject* exception = nullptr;
    MObject* result = data.Evaluate->Invoke(obj, params, &exception);
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Warning, TEXT("AnimGraph"));
        return;
    }

    // Extract result
    value = MUtils::UnboxVariant(result);
    context.ValueCache.Add(boxBase, value);
#endif
}

bool AnimGraph::IsReady() const
{
    return BaseModel && BaseModel->IsLoaded();
}

void AnimGraph::ClearCustomNode(Node* node)
{
    // Clear data
    auto& data = node->Data.Custom;
    data.Evaluate = nullptr;
    if (data.Handle)
    {
        MCore::GCHandle::Free(data.Handle);
        data.Handle = 0;
    }
}

bool AnimGraph::InitCustomNode(Node* node)
{
#if USE_CSHARP
    // Fetch the node logic controller type
    if (node->Values.Count() < 2 || node->Values[0].Type.Type != ValueType::String)
    {
        LOG(Warning, "Invalid custom node data values.");
        return false;
    }
    const StringView typeName(node->Values[0]);
    const StringAnsi typeNameStd = typeName.ToStringAnsi();
    MClass* type = Scripting::FindClass(typeNameStd);
    if (type == nullptr)
    {
        LOG(Warning, "Invalid custom node type {0}.", typeName);
        return false;
    }

    // Get methods
    MMethod* load = type->GetMethod("Load", 1);
    MMethod* evaluate = type->GetMethod("Evaluate", 1);
    if (load == nullptr)
    {
        LOG(Warning, "Invalid custom node type {0}. Missing Load method.", typeName);
        return false;
    }
    if (evaluate == nullptr)
    {
        LOG(Warning, "Invalid custom node type {0}. Missing Evaluate method.", typeName);
        return false;
    }

    // Initialization can happen on Content Thread so ensure to have runtime attached
    MCore::Thread::Attach();

    // Allocate managed node object (create GC handle to prevent destruction)
    MObject* obj = type->CreateInstance();
    const MGCHandle handleGC = MCore::GCHandle::New(obj);

    // Initialize node
    InternalInitData initData;
    initData.Values = MUtils::ToArray(node->Values, MCore::TypeCache::Object);
    initData.BaseModel = BaseModel.GetManagedInstance();
    void* params[1];
    params[0] = &initData;
    MObject* exception = nullptr;
    load->Invoke(obj, params, &exception);
    if (exception)
    {
        MCore::GCHandle::Free(handleGC);
        MException ex(exception);
        ex.Log(LogType::Warning, TEXT("AnimGraph"));
        return false;
    }

    // Cache node data
    auto& data = node->Data.Custom;
    data.Evaluate = evaluate;
    data.Handle = handleGC;
#endif
    return false;
}

#if USE_EDITOR

void AnimGraph::OnScriptsReloading()
{
    // Clear all cached custom nodes for nodes from game assemblies (plugins may keep data because they are persistent)
    for (int32 i = 0; i < _customNodes.Count(); i++)
    {
        const auto evaluate = _customNodes[i]->Data.Custom.Evaluate;
        if (evaluate && Scripting::IsTypeFromGameScripts(evaluate->GetParentClass()))
        {
            ClearCustomNode(_customNodes[i]);
        }
    }
}

void AnimGraph::OnScriptsReloaded()
{
    // Cache all custom nodes with no type setup
    for (int32 i = 0; i < _customNodes.Count(); i++)
    {
        if (_customNodes[i]->Data.Custom.Evaluate == nullptr)
        {
            InitCustomNode(_customNodes[i]);
        }
    }
}

#endif

void AnimGraph::OnScriptsLoaded()
{
    // Cache all custom nodes with no type setup
    for (int32 i = 0; i < _customNodes.Count(); i++)
    {
        if (_customNodes[i]->Data.Custom.Evaluate == nullptr)
        {
            InitCustomNode(_customNodes[i]);
        }
    }
}
