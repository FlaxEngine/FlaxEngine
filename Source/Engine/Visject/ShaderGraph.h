// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "Graph.h"
#include "ShaderGraphValue.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Collections/HashSet.h"
#include "Engine/Utilities/TextWriter.h"
#include "Engine/Graphics/Materials/MaterialParams.h"
#include "Engine/Content/AssetsContainer.h"
#include "Engine/Animations/Curve.h"

#define SHADER_GRAPH_MAX_CALL_STACK 100

enum class MaterialSceneTextures;
template<class BoxType>
class ShaderGraphNode;

class ShaderGraphBox : public GraphBox
{
public:

    /// <summary>
    /// The cached value.
    /// </summary>
    ShaderGraphValue Cache;

public:

    ShaderGraphBox()
        : GraphBox()
    {
    }

    ShaderGraphBox(void* parent, byte id, const VariantType::Types type)
        : GraphBox(parent, id, type)
    {
    }

    ShaderGraphBox(void* parent, byte id, const VariantType& type)
        : GraphBox(parent, id, type)
    {
    }

public:

    ShaderGraphBox* FirstConnection() const
    {
        return (ShaderGraphBox*)Connections[0];
    }
};

template<class BoxType = ShaderGraphBox>
class ShaderGraphNode : public GraphNode<BoxType>
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
        };
    };

public:

    ShaderGraphNode()
        : GraphNode<ShaderGraphBox>()
    {
    }

public:

    /// <summary>
    /// The custom data (depends on node type). Used to cache data for faster usage at runtime.
    /// </summary>
    AdditionalData Data;
};

class ShaderGraphParameter : public GraphParameter
{
public:

    ShaderGraphParameter()
        : GraphParameter(SpawnParams(Guid::New(), TypeInitializer))
    {
    }

    ShaderGraphParameter(const ShaderGraphParameter& other)
        : ShaderGraphParameter()
    {
#if !BUILD_RELEASE
        CRASH; // Not used
#endif
    }

    ShaderGraphParameter& operator=(const ShaderGraphParameter& other)
    {
#if !BUILD_RELEASE
        CRASH; // Not used
#endif
        return *this;
    }
};

template<class NodeType = ShaderGraphNode<>, class BoxType = ShaderGraphBox, class ParameterType = ShaderGraphParameter>
class ShaderGraph : public Graph<NodeType, BoxType, ParameterType>
{
public:

    typedef ShaderGraphValue Value;
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
        // Check if this node needs a state or data cache
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
            }
            break;
        }

        // Base
        return Base::onNodeLoaded(n);
    }
};

/// <summary>
/// Shaders generator from graphs.
/// </summary>
class ShaderGenerator
{
public:

    typedef ShaderGraph<> Graph;
    typedef ShaderGraph<>::Node Node;
    typedef ShaderGraph<>::Box Box;
    typedef ShaderGraph<>::Parameter Parameter;
    typedef ShaderGraphValue Value;
    typedef VariantType::Types ValueType;
    typedef Delegate<Node*, Box*, const StringView&> ErrorHandler;
    typedef Function<void(Box*, Node*, Value&)> ProcessBoxHandler;

protected:

    int32 _localIndex;
    Dictionary<Node*, Graph*> _functions;
    Array<SerializedMaterialParam> _parameters;
    TextWriterUnicode _writer;
    HashSet<String> _includes;
    Array<ProcessBoxHandler, FixedAllocation<17>> _perGroupProcessCall;
    Array<Node*, FixedAllocation<SHADER_GRAPH_MAX_CALL_STACK>> _callStack;
    Array<Graph*, FixedAllocation<32>> _graphStack;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="ShaderGenerator"/> class.
    /// </summary>
    ShaderGenerator();

    /// <summary>
    /// Finalizes an instance of the <see cref="ShaderGenerator"/> class.
    /// </summary>
    ~ShaderGenerator();

public:

    ErrorHandler Error;

    /// <summary>
    /// The assets container for graph generation. Holds references to used assets. Can be used to gather assets referenced by graph (eg. nested graph functions).
    /// </summary>
    AssetsContainer Assets;

public:

    void OnError(Node* node, Box* box, const StringView& message);

    void ProcessGroupConstants(Box* box, Node* node, Value& value);
    void ProcessGroupMath(Box* box, Node* node, Value& value);
    void ProcessGroupPacking(Box* box, Node* node, Value& value);
    void ProcessGroupTools(Box* box, Node* node, Value& value);
    void ProcessGroupBoolean(Box* box, Node* node, Value& value);
    void ProcessGroupBitwise(Box* box, Node* node, Value& value);
    void ProcessGroupComparisons(Box* box, Node* node, Value& value);

protected:

    static const Char* _mathFunctions[];
    static const Char* _subs[];

protected:

    Value eatBox(Node* caller, Box* box);
    Value tryGetValue(Box* box, int32 defaultValueBoxIndex, const Value& defaultValue);
    Value tryGetValue(Box* box, const Value& defaultValue);
    Value tryGetValue(Box* box, const Variant& defaultValue);
    Value writeLocal(ValueType type, Node* caller);
    Value writeLocal(ValueType type, Node* caller, const String& name);
    Value writeLocal(ValueType type, const Value& value, Node* caller);
    Value writeLocal(const Value& value, Node* caller);
    Value writeLocal(ValueType type, const String& value, Node* caller);
    Value writeLocal(ValueType type, const String& value, Node* caller, const String& name);
    Value writeOperation2(Node* caller, const Value& valueA, const Value& valueB, Char op1);
    Value writeFunction1(Node* caller, const Value& valueA, const String& function);
    Value writeFunction2(Node* caller, const Value& valueA, const Value& valueB, const String& function);
    Value writeFunction2(Node* caller, const Value& valueA, const Value& valueB, const String& function, ValueType resultType);
    Value writeFunction3(Node* caller, const Value& valueA, const Value& valueB, const Value& valueC, const String& function, ValueType resultType);

    SerializedMaterialParam* findParam(const String& shaderName);
    SerializedMaterialParam* findParam(const Guid& id);

    SerializedMaterialParam findOrAddTexture(const Guid& id);
    SerializedMaterialParam findOrAddNormalMap(const Guid& id);
    SerializedMaterialParam findOrAddCubeTexture(const Guid& id);
    SerializedMaterialParam findOrAddSceneTexture(MaterialSceneTextures type);

    static String getLocalName(int32 index);
    static String getParamName(int32 index);
};

#endif
