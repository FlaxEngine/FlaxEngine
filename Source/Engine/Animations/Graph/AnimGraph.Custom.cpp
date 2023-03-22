// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "AnimGraph.h"
#include "Engine/Debug/DebugLog.h"
#include "Engine/Scripting/InternalCalls.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/ManagedCLR/MDomain.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/MException.h"
#include "Engine/Content/Assets/SkinnedModel.h"

#if !COMPILE_WITHOUT_CSHARP
#if USE_MONO
#include <mono/metadata/appdomain.h>
#endif

struct InternalInitData
{
    MonoArray* Values;
    MonoObject* BaseModel;
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
    MonoObject* BaseModel;
    MonoObject* Instance;
};

struct InternalImpulse
{
    int32 NodesCount;
    int32 Unused;
    Transform* Nodes;
    Vector3 RootMotionTranslation;
    Quaternion RootMotionRotation;
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

DEFINE_INTERNAL_CALL(MonoObject*) AnimGraphInternal_GetInputValue(InternalContext* context, int32 boxId)
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
#if USE_MONO
    ADD_INTERNAL_CALL("FlaxEngine.AnimationGraph::Internal_HasConnection", &AnimGraphInternal_HasConnection);
    ADD_INTERNAL_CALL("FlaxEngine.AnimationGraph::Internal_GetInputValue", &AnimGraphInternal_GetInputValue);
    ADD_INTERNAL_CALL("FlaxEngine.AnimationGraph::Internal_GetOutputImpulseData", &AnimGraphInternal_GetOutputImpulseData);
#endif
}

void AnimGraphExecutor::ProcessGroupCustom(Box* boxBase, Node* nodeBase, Value& value)
{
#if USE_MONO
    auto& context = Context.Get();
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
    const auto obj = MUtils::GetGCHandleTarget(data.Handle);
    if (obj == nullptr)
    {
        LOG(Warning, "Custom node instance is null.");
        return;
    }

    // Evaluate node
    void* params[1];
    params[0] = &internalContext;
    MObject* exception = nullptr;
    MonoObject* result = data.Evaluate->Invoke(obj, params, &exception);
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

bool AnimGraph::CanUseWithSkeleton(SkinnedModel* other) const
{
    // All data loaded and nodes count the same
    return IsReady() && other && other->IsLoaded() && other->Skeleton.Nodes.Count() == BaseModel->Skeleton.Nodes.Count();
}

void AnimGraph::ClearCustomNode(Node* node)
{
    // Clear data
    auto& data = node->Data.Custom;
    data.Evaluate = nullptr;
    if (data.Handle)
    {
#if USE_MONO
        MUtils::FreeGCHandle(data.Handle);
#endif
        data.Handle = 0;
    }
}

bool AnimGraph::InitCustomNode(Node* node)
{
#if USE_MONO
    // Fetch the node logic controller type
    if (node->Values.Count() < 2 || node->Values[0].Type.Type != ValueType::String)
    {
        LOG(Warning, "Invalid custom node data values.");
        return false;
    }
    const StringView typeName(node->Values[0]);
    const MString typeNameStd = typeName.ToStringAnsi();
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

    // Create node values managed array
    if (mono_domain_get() == nullptr)
        Scripting::GetScriptsDomain()->Dispatch();
    const auto values = mono_array_new(mono_domain_get(), mono_get_object_class(), node->Values.Count());
    for (int32 i = 0; i < node->Values.Count(); i++)
    {
        const auto v = MUtils::BoxVariant(node->Values[i]);
        mono_array_set(values, MonoObject*, i, v);
    }

    // Allocate managed node object (create GC handle to prevent destruction)
    const auto obj = type->CreateInstance();
    const auto handleGC = MUtils::NewGCHandle(obj, false);

    // Initialize node
    InternalInitData initData;
    initData.Values = values;
    initData.BaseModel = BaseModel.GetManagedInstance();
    void* params[1];
    params[0] = &initData;
    MObject* exception = nullptr;
    load->Invoke(obj, params, &exception);
    if (exception)
    {
        MUtils::FreeGCHandle(handleGC);

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
