// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Graph.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Content/Asset.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Content/AssetsContainer.h"
#include "Engine/Animations/Curve.h"

#define VISJECT_GRAPH_NODE_MAX_ASSETS 14

template<class BoxType>
class VisjectGraphNode;

class VisjectGraphBox : public GraphBox
{
public:

    /// <summary>
    /// The cached value.
    /// </summary>
    Variant Cache;

public:

    VisjectGraphBox()
        : GraphBox()
    {
    }

    VisjectGraphBox(void* parent, byte id, const VariantType::Types type)
        : GraphBox(parent, id, type)
    {
    }

    VisjectGraphBox(void* parent, byte id, const VariantType& type)
        : GraphBox(parent, id, type)
    {
    }

    FORCE_INLINE VisjectGraphBox* FirstConnection() const
    {
        return (VisjectGraphBox*)Connections[0];
    }
};

template<class BoxType = VisjectGraphBox>
class VisjectGraphNode : public GraphNode<BoxType>
{
public:

    struct CurveData
    {
        /// <summary>
        /// The curve index.
        /// </summary>
        int32 CurveIndex;
    };

    /// <summary>
    /// Custom cached data per node type. Compact to use as small amount of memory as possible.
    /// </summary>
    struct AdditionalData
    {
        union
        {
            CurveData Curve;

            struct
            {
                void* Method;
                BinaryModule* Module;
                int32 ParamsCount;
                uint32 OutParamsMask;
                bool IsStatic;
            } InvokeMethod;

            struct
            {
                void* Field;
                BinaryModule* Module;
                bool IsStatic;
            } GetSetField;
        };
    };

public:

    VisjectGraphNode()
        : GraphNode<BoxType>()
    {
    }

public:

    /// <summary>
    /// The custom data (depends on node type). Used to cache data for faster usage at runtime.
    /// </summary>
    AdditionalData Data;

    /// <summary>
    /// The asset references. Linked resources such as Animation assets are referenced in graph data as ID. We need to keep valid refs to them at runtime to keep data in memory.
    /// </summary>
    AssetReference<Asset> Assets[VISJECT_GRAPH_NODE_MAX_ASSETS];
};

/// <summary>
/// Visject graph parameter.
/// </summary>
/// <seealso cref="GraphParameter" />
API_CLASS() class VisjectGraphParameter : public GraphParameter
{
DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(VisjectGraphParameter, GraphParameter);
public:

    VisjectGraphParameter(const VisjectGraphParameter& other)
        : VisjectGraphParameter()
    {
#if !BUILD_RELEASE
        CRASH; // Not used
#endif
    }

    VisjectGraphParameter& operator=(const VisjectGraphParameter& other)
    {
#if !BUILD_RELEASE
        CRASH; // Not used
#endif
        return *this;
    }
};

template<class NodeType = VisjectGraphNode<>, class BoxType = VisjectGraphBox, class ParameterType = VisjectGraphParameter>
class VisjectGraph : public Graph<NodeType, BoxType, ParameterType>
{
public:

    typedef Variant Value;
    typedef VariantType::Types ValueType;
    typedef Graph<NodeType, BoxType, ParameterType> Base;

public:

    /// <summary>
    /// The float curves used by the graph.
    /// </summary>
    Array<BezierCurve<float>> FloatCurves;

    /// <summary>
    /// The float curves used by the graph.
    /// </summary>
    Array<BezierCurve<Vector2>> Vector2Curves;

    /// <summary>
    /// The float curves used by the graph.
    /// </summary>
    Array<BezierCurve<Vector3>> Vector3Curves;

    /// <summary>
    /// The float curves used by the graph.
    /// </summary>
    Array<BezierCurve<Vector4>> Vector4Curves;

public:

    // [Graph]
    bool onNodeLoaded(NodeType* n) override
    {
        switch (n->GroupID)
        {
            // Tools
        case 7:
            switch (n->TypeID)
            {
                // Curves
#define SETUP_CURVE(id, curves, access) \
			case id: \
			{ \
				n->Data.Curve.CurveIndex = curves.Count(); \
				auto& curve = curves.AddOne(); \
				const int32 keyframesCount = n->Values[0].AsInt; \
				auto& keyframes = curve.GetKeyframes(); \
				keyframes.Resize(keyframesCount); \
				for (int32 i = 0; i < keyframesCount; i++) \
				{ \
					const int32 idx = i * 4; \
					auto& keyframe = keyframes[i]; \
					keyframe.Time = n->Values[idx + 1].AsFloat; \
					keyframe.Value = n->Values[idx + 2].access; \
					keyframe.TangentIn = n->Values[idx + 3].access; \
					keyframe.TangentOut = n->Values[idx + 4].access; \
				} \
				break; \
			}
            SETUP_CURVE(12, FloatCurves, AsFloat)
            SETUP_CURVE(13, Vector2Curves, AsVector2())
            SETUP_CURVE(14, Vector3Curves, AsVector3())
            SETUP_CURVE(15, Vector4Curves, AsVector4())
#undef SETUP_CURVE
                // Get Gameplay Global
            case 16:
            {
                n->Assets[0] = ::LoadAsset((Guid)n->Values[0], Asset::TypeInitializer);
                break;
            }
            }
        }

        // Base
        return Base::onNodeLoaded(n);
    }
};

/// <summary>
/// Visject Surface graph executor at runtime.
/// </summary>
class VisjectExecutor
{
public:

    typedef VisjectGraph<> Graph;
    typedef VisjectGraph<>::Node Node;
    typedef VisjectGraph<>::Box Box;
    typedef VisjectGraph<>::Parameter Parameter;
    typedef Variant Value;
    typedef VariantType ValueType;
    typedef Delegate<Node*, Box*, const StringView&> ErrorHandler;
    typedef void (VisjectExecutor::*ProcessBoxHandler)(Box*, Node*, Value&);

protected:

    ProcessBoxHandler _perGroupProcessCall[18];

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="VisjectExecutor"/> class.
    /// </summary>
    VisjectExecutor();

    /// <summary>
    /// Finalizes an instance of the <see cref="VisjectExecutor"/> class.
    /// </summary>
    ~VisjectExecutor();

public:

    ErrorHandler Error;

public:

    void OnError(Node* node, Box* box, const StringView& message);

    void ProcessGroupConstants(Box* box, Node* node, Value& value);
    void ProcessGroupMath(Box* box, Node* node, Value& value);
    void ProcessGroupPacking(Box* box, Node* node, Value& value);
    void ProcessGroupTools(Box* box, Node* node, Value& value);
    void ProcessGroupBoolean(Box* box, Node* node, Value& value);
    void ProcessGroupBitwise(Box* box, Node* node, Value& value);
    void ProcessGroupComparisons(Box* box, Node* node, Value& value);
    void ProcessGroupParticles(Box* box, Node* node, Value& value);

protected:

    virtual Value eatBox(Node* caller, Box* box) = 0;
    virtual Graph* GetCurrentGraph() const = 0;
    Value tryGetValue(Box* box, int32 defaultValueBoxIndex, const Value& defaultValue);
    Value tryGetValue(Box* box, const Value& defaultValue);
};
