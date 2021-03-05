// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MATERIAL_GRAPH

#include "MaterialGenerator.h"
#include "Engine/Content/Assets/MaterialFunction.h"

void MaterialGenerator::ProcessGroupMaterial(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
        // World Position
    case 2:
        value = Value(VariantType::Vector3, TEXT("input.WorldPosition.xyz"));
        break;
        // View
    case 3:
    {
        switch (box->ID)
        {
            // Position
        case 0:
            value = Value(VariantType::Vector3, TEXT("ViewPos"));
            break;
            // Direction
        case 1:
            value = Value(VariantType::Vector3, TEXT("ViewDir"));
            break;
            // Far Plane
        case 2:
            value = Value(VariantType::Float, TEXT("ViewFar"));
            break;
        default: CRASH;
        }
        break;
    }
        // Normal
    case 4:
        value = getNormal;
        break;
        // Camera Vector
    case 5:
        value = getCameraVector(node);
        break;
        // Screen Position
    case 6:
    {
        // Position
        if (box->ID == 0)
            value = Value(VariantType::Vector2, TEXT("input.SvPosition.xy"));
            // Texcoord
        else if (box->ID == 1)
            value = writeLocal(VariantType::Vector2, TEXT("input.SvPosition.xy * ScreenSize.zw"), node);

        break;
    }
        // Screen Size
    case 7:
    {
        value = Value(VariantType::Vector2, box->ID == 0 ? TEXT("ScreenSize.xy") : TEXT("ScreenSize.zw"));
        break;
    }
        // Custom code
    case 8:
    {
        // Skip if has no code
        if (((StringView)node->Values[0]).IsEmpty())
        {
            value = Value::Zero;
            break;
        }

        const int32 InputsMax = 8;
        const int32 OutputsMax = 4;
        const int32 Input0BoxID = 0;
        const int32 Output0BoxID = 8;

        // Create output variables
        Value values[OutputsMax];
        for (int32 i = 0; i < OutputsMax; i++)
        {
            const auto outputBox = node->GetBox(Output0BoxID + i);
            if (outputBox && outputBox->HasConnection())
            {
                values[i] = writeLocal(VariantType::Vector4, node);
            }
        }

        // Process custom code (inject inputs and outputs)
        String code;
        code = (StringView)node->Values[0];
        for (int32 i = 0; i < InputsMax; i++)
        {
            auto inputName = TEXT("Input") + StringUtils::ToString(i);
            const auto inputBox = node->GetBox(Input0BoxID + i);
            if (inputBox && inputBox->HasConnection())
            {
                auto inputValue = tryGetValue(inputBox, Value::Zero);
                if (inputValue.Type != VariantType::Vector4)
                    inputValue = inputValue.Cast(VariantType::Vector4);
                code.Replace(*inputName, *inputValue.Value, StringSearchCase::CaseSensitive);
            }
        }
        for (int32 i = 0; i < OutputsMax; i++)
        {
            auto outputName = TEXT("Output") + StringUtils::ToString(i);
            const auto outputBox = node->GetBox(Output0BoxID + i);
            if (outputBox && outputBox->HasConnection())
            {
                code.Replace(*outputName, *values[i].Value, StringSearchCase::CaseSensitive);
            }
        }

        // Write code
        _writer.Write(TEXT("{\n"));
        _writer.Write(*code);
        _writer.Write(TEXT("}\n"));

        // Link output values to boxes
        for (int32 i = 0; i < OutputsMax; i++)
        {
            const auto outputBox = node->GetBox(Output0BoxID + i);
            if (outputBox && outputBox->HasConnection())
            {
                outputBox->Cache = values[i];
            }
        }

        value = box->Cache;
        break;
    }
        // Object Position
    case 9:
        value = Value(VariantType::Vector3, TEXT("GetObjectPosition(input)"));
        break;
        // Two Sided Sign
    case 10:
        value = Value(VariantType::Float, TEXT("input.TwoSidedSign"));
        break;
        // Camera Depth Fade
    case 11:
    {
        auto faeLength = tryGetValue(node->GetBox(0), node->Values[0]).AsFloat();
        auto fadeOffset = tryGetValue(node->GetBox(1), node->Values[1]).AsFloat();

        // TODO: for pixel shader it could calc PixelDepth = mul(float4(WorldPos.xyz, 1), ViewProjMatrix).w and use it

        auto x1 = writeLocal(VariantType::Vector3, TEXT("ViewPos - input.WorldPosition"), node);
        auto x2 = writeLocal(VariantType::Vector3, TEXT("TransformViewVectorToWorld(input, float3(0, 0, -1))"), node);
        auto x3 = writeLocal(VariantType::Float, String::Format(TEXT("dot(normalize({0}), {1}) * length({0})"), x1.Value, x2.Value), node);
        auto x4 = writeLocal(VariantType::Float, String::Format(TEXT("{0} - {1}"), x3.Value, fadeOffset.Value), node);
        auto x5 = writeLocal(VariantType::Float, String::Format(TEXT("saturate({0} / {1})"), x4.Value, faeLength.Value), node);

        value = x5;
        break;
    }
        // Vertex Color
    case 12:
        value = getVertexColor;
        _treeLayer->UsageFlags |= MaterialUsageFlags::UseVertexColor;
        break;
        // Pre-skinned Local Position
    case 13:
        value = _treeType == MaterialTreeType::VertexShader ? Value(VariantType::Vector3, TEXT("input.PreSkinnedPosition")) : Value::Zero;
        break;
        // Pre-skinned Local Normal
    case 14:
        value = _treeType == MaterialTreeType::VertexShader ? Value(VariantType::Vector3, TEXT("input.PreSkinnedNormal")) : Value::Zero;
        break;
        // Depth
    case 15:
        value = writeLocal(VariantType::Float, TEXT("distance(ViewPos, input.WorldPosition)"), node);
        break;
        // Tangent
    case 16:
        value = Value(VariantType::Vector3, TEXT("input.TBN[0]"));
        break;
        // Bitangent
    case 17:
        value = Value(VariantType::Vector3, TEXT("input.TBN[1]"));
        break;
        // Camera Position
    case 18:
        value = Value(VariantType::Vector3, TEXT("ViewPos"));
        break;
        // Per Instance Random
    case 19:
        value = Value(VariantType::Float, TEXT("GetPerInstanceRandom(input)"));
        break;
        // Interpolate VS To PS
    case 20:
    {
        const auto input = node->GetBox(0);

        // If used in VS then pass the value from the input box
        if (_treeType == MaterialTreeType::VertexShader)
        {
            value = tryGetValue(input, Value::Zero).AsVector4();
            break;
        }

        // Check if can use more interpolants
        if (_vsToPsInterpolants.Count() == 16)
        {
            OnError(node, box, TEXT("Too many VS to PS interpolants used."));
            value = Value::Zero;
            break;
        }

        // Check if can use interpolants
        const auto layer = GetRootLayer();
        if (!layer || layer->Domain == MaterialDomain::Decal || layer->Domain == MaterialDomain::PostProcess)
        {
            OnError(node, box, TEXT("VS to PS interpolants are not supported in Decal or Post Process materials."));
            value = Value::Zero;
            break;
        }

        // Indicate the interpolator slot usage
        value = Value(VariantType::Vector4, String::Format(TEXT("input.CustomVSToPS[{0}]"), _vsToPsInterpolants.Count()));
        _vsToPsInterpolants.Add(input);
        break;
    }
        // Terrain Holes Mask
    case 21:
    {
        MaterialLayer* baseLayer = GetRootLayer();
        if (baseLayer->Domain == MaterialDomain::Terrain)
            value = Value(VariantType::Float, TEXT("input.HolesMask"));
        else
            value = Value::One;
        break;
    }
        // Terrain Layer Weight
    case 22:
    {
        MaterialLayer* baseLayer = GetRootLayer();
        if (baseLayer->Domain != MaterialDomain::Terrain)
        {
            value = Value::One;
            break;
        }

        const int32 layer = node->Values[0].AsInt;
        if (layer < 0 || layer > 7)
        {
            value = Value::One;
            OnError(node, box, TEXT("Invalid terrain layer index."));
            break;
        }

        const int32 slotIndex = layer / 4;
        const int32 componentIndex = layer % 4;
        value = Value(VariantType::Float, String::Format(TEXT("input.Layers[{0}][{1}]"), slotIndex, componentIndex));
        break;
    }
        // Depth Fade
    case 23:
    {
        // Calculate screen-space UVs
        auto screenUVs = writeLocal(VariantType::Vector2, TEXT("input.SvPosition.xy * ScreenSize.zw"), node);

        // Sample scene depth buffer
        auto sceneDepthTexture = findOrAddSceneTexture(MaterialSceneTextures::SceneDepth);
        auto depthSample = writeLocal(VariantType::Float, String::Format(TEXT("{0}.SampleLevel(SamplerLinearClamp, {1}, 0).x"), sceneDepthTexture.ShaderName, screenUVs.Value), node);

        // Linearize raw device depth
        Value sceneDepth;
        linearizeSceneDepth(node, depthSample, sceneDepth);

        // Calculate pixel depth
        auto posVS = writeLocal(VariantType::Float, TEXT("mul(float4(input.WorldPosition.xyz, 1), ViewMatrix).z"), node);

        // Compute depth difference
        auto depthDiff = writeLocal(VariantType::Float, String::Format(TEXT("{0} * ViewFar - {1}"), sceneDepth.Value, posVS.Value), node);

        auto fadeDistance = tryGetValue(node->GetBox(0), node->Values[0]).AsFloat();

        // Apply smoothing factor and clamp the result
        value = writeLocal(VariantType::Float, String::Format(TEXT("saturate({0} / {1})"), depthDiff.Value, fadeDistance.Value), node);
        break;
    }
        // Material Function
    case 24:
    {
        // Load function asset
        const auto function = Assets.LoadAsync<MaterialFunction>((Guid)node->Values[0]);
        if (!function || function->WaitForLoaded())
        {
            OnError(node, box, TEXT("Missing or invalid function."));
            value = Value::Zero;
            break;
        }

#if 0
        // Prevent recursive calls
        for (int32 i = _callStack.Count() - 1; i >= 0; i--)
        {
            if (_callStack[i]->Type == GRAPH_NODE_MAKE_TYPE(1, 24))
            {
                const auto callFunc = Assets.LoadAsync<MaterialFunction>((Guid)_callStack[i]->Values[0]);
                if (callFunc == function)
                {
                    OnError(node, box, String::Format(TEXT("Recursive call to function '{0}'!"), function->ToString()));
                    value = Value::Zero;
                    return;
                }
            }
        }
#endif

        // Create a instanced version of the function graph
        Graph* graph;
        if (!_functions.TryGet(node, graph))
        {
            graph = New<MaterialGraph>();
            function->LoadSurface((MaterialGraph&)*graph);
            _functions.Add(node, graph);
        }

        // Peek the function output (function->Outputs maps the functions outputs to output nodes indices)
        const int32 outputIndex = box->ID - 16;
        if (outputIndex < 0 || outputIndex >= function->Outputs.Count())
        {
            OnError(node, box, TEXT("Invalid function output box."));
            value = Value::Zero;
            break;
        }
        Node* functionOutputNode = &graph->Nodes[function->Outputs[outputIndex]];
        Box* functionOutputBox = functionOutputNode->TryGetBox(0);

        // Evaluate the function output
        _graphStack.Push(graph);
        value = functionOutputBox && functionOutputBox->HasConnection() ? eatBox(node, functionOutputBox->FirstConnection()) : Value::Zero;
        _graphStack.Pop();
        break;
    }
        // Object Size
    case 25:
        value = Value(VariantType::Vector3, TEXT("GetObjectSize(input)"));
        break;
        // Blend Normals
    case 26:
    {
        const auto baseNormal = tryGetValue(node->GetBox(0), getNormalZero).AsVector3();
        const auto additionalNormal = tryGetValue(node->GetBox(1), getNormalZero).AsVector3();

        const String text1 = String::Format(TEXT("(float2({0}.xy) + float2({1}.xy) * 2.0)"), baseNormal.Value, additionalNormal.Value);
        const auto appendXY = writeLocal(ValueType::Vector2, text1, node);

        const String text2 = String::Format(TEXT("float3({0}, sqrt(saturate(1.0 - dot({0}.xy, {0}.xy))))"), appendXY.Value);
        value = writeLocal(ValueType::Vector3, text2, node);
        break;
    }
        // Rotator
    case 27:
    {
        const auto uv = tryGetValue(node->GetBox(0), getUVs).AsVector2();
        const auto center = tryGetValue(node->GetBox(1), Value::Zero).AsVector2();
        const auto rotationAngle = tryGetValue(node->GetBox(2), Value::Zero).AsFloat();

        auto x1 = writeLocal(ValueType::Vector2, String::Format(TEXT("({0} * -1) + {1}"), center.Value, uv.Value), node);
        auto raCosSin = writeLocal(ValueType::Vector2, String::Format(TEXT("float2(cos({0}), sin({0}))"), rotationAngle.Value), node);

        auto dotB1 = writeLocal(ValueType::Vector2, String::Format(TEXT("float2({0}.x, {0}.y * -1)"), raCosSin.Value), node);
        auto dotB2 = writeLocal(ValueType::Vector2, String::Format(TEXT("float2({0}.y, {0}.x)"), raCosSin.Value), node);

        value = writeLocal(ValueType::Vector2, String::Format(TEXT("{3} + float2(dot({0},{1}), dot({0},{2}))"), x1.Value, dotB1.Value, dotB2.Value, center.Value), node);
        break;
    }
        // Sphere Mask
    case 28:
    {
        const auto a = tryGetValue(node->GetBox(0), 0, Value::Zero);
        const auto b = tryGetValue(node->GetBox(1), 1, Value::Zero).Cast(a.Type);
        const auto radius = tryGetValue(node->GetBox(2), node->Values[0]).AsFloat();
        const auto hardness = tryGetValue(node->GetBox(3), node->Values[1]).AsFloat();
        const auto invert = tryGetValue(node->GetBox(4), node->Values[2]).AsBool();

        // Get distance and apply radius
        auto x1 = writeLocal(ValueType::Float, String::Format(TEXT("distance({0}, {1}) / (float){2}"), a.Value, b.Value, radius.Value), node);

        // Apply hardness, use 0.991 as max since any value above will result in harsh aliasing
        auto x2 = writeLocal(ValueType::Float, String::Format(TEXT("saturate((1 - {0}) * (1 / (1 - clamp({1}, 0, 0.991f))))"), x1.Value, hardness.Value), node);

        value = writeLocal(ValueType::Float, String::Format(TEXT("{0} ? (1 - {1}) : {1}"), invert.Value, x2.Value), node);
        break;
    }
        // Tiling & Offset
    case 29:
    {
        const auto uv = tryGetValue(node->GetBox(0), getUVs).AsVector2();
        const auto tiling = tryGetValue(node->GetBox(1), node->Values[0]).AsVector2();
        const auto offset = tryGetValue(node->GetBox(2), node->Values[1]).AsVector2();

        value = writeLocal(ValueType::Vector2, String::Format(TEXT("{0} * {1} + {2}"), uv.Value, tiling.Value, offset.Value), node);
        break;
    }
        // DDX
    case 30:
    {
        const auto inValue = tryGetValue(node->GetBox(0), 0, Value::Zero);

        value = writeLocal(inValue.Type, String::Format(TEXT("ddx({0})"), inValue.Value), node);
        break;
    }
        // DDY
    case 31:
    {
        const auto inValue = tryGetValue(node->GetBox(0), 0, Value::Zero);

        value = writeLocal(inValue.Type, String::Format(TEXT("ddy({0})"), inValue.Value), node);
        break;
    }
        // Sign
    case 32:
    {
        const auto inValue = tryGetValue(node->GetBox(0), 0, Value::Zero);

        value = writeLocal(ValueType::Float, String::Format(TEXT("sign({0})"), inValue.Value), node);
        break;
    }
        // Any
    case 33:
    {
        const auto inValue = tryGetValue(node->GetBox(0), 0, Value::Zero);

        value = writeLocal(ValueType::Bool, String::Format(TEXT("any({0})"), inValue.Value), node);
        break;
    }
        // All
    case 34:
    {
        const auto inValue = tryGetValue(node->GetBox(0), 0, Value::Zero);

        value = writeLocal(ValueType::Bool, String::Format(TEXT("all({0})"), inValue.Value), node);
        break;
    }
        // Blackbody
    case 35:
    {
        // Reference: Mitchell Charity, http://www.vendian.org/mncharity/dir3/blackbody/

        const auto temperature = tryGetValue(node->GetBox(0), node->Values[0]).AsFloat();

        // Value X
        auto x = writeLocal(ValueType::Float, String::Format(TEXT("56100000.0f * pow({0}, -1) + 148.0f"), temperature.Value), node);

        // Value Y
        auto y = writeLocal(ValueType::Float, String::Format(TEXT("{0} > 6500.0f ? 35200000.0f * pow({0}, -1) + 184.0f : 100.04f * log({0}) - 623.6f"), temperature.Value), node);

        // Value Z
        auto z = writeLocal(ValueType::Float, String::Format(TEXT("194.18f * log({0}) - 1448.6f"), temperature.Value), node);

        // Final color
        auto color = writeLocal(ValueType::Vector3, String::Format(TEXT("float3({0}, {1}, {2})"), x.Value, y.Value, z.Value), node);
        color = writeLocal(ValueType::Vector3, String::Format(TEXT("clamp({0}, 0.0f, 255.0f) / 255.0f"), color.Value), node);
        value = writeLocal(ValueType::Vector3, String::Format(TEXT("{1} < 1000.0f ? {0} * {1}/1000.0f : {0}"), color.Value, temperature.Value), node);
        break;
    }
        // HSVToRGB
    case 36:
    {
        const auto hsv = tryGetValue(node->GetBox(0), node->Values[0]).AsVector3();

        // Normalize from 360
        auto color = writeLocal(ValueType::Vector3, String::Format(TEXT("float3({0}.x / 360.0f, {0}.y, {0}.z)"), hsv.Value), node);

        auto x1 = writeLocal(ValueType::Vector3, String::Format(TEXT("clamp(abs(fmod({0}.x * 6.0 + float3(0.0f, 4.0f, 2.0f), 6.0f) - 3.0f) - 1.0f, 0.0f, 1.0f)"), color.Value), node);
        value = writeLocal(ValueType::Vector3, String::Format(TEXT("{1}.z * lerp(float3(1.0, 1.0, 1.0), {0}, {1}.y)"), x1.Value, color.Value), node);
        break;
    }
        // RGBToHSV
    case 37:
    {
        // Reference: Ian Taylor, https://www.chilliant.com/rgb2hsv.html

        const auto rgb = tryGetValue(node->GetBox(0), node->Values[0]).AsVector3();
        const auto epsilon = writeLocal(ValueType::Float, TEXT("1e-10"), node);

        auto p = writeLocal(ValueType::Vector4, String::Format(TEXT("({0}.g < {0}.b) ? float4({0}.bg, -1.0f, 2.0f/3.0f) : float4({0}.gb, 0.0f, -1.0f/3.0f)"), rgb.Value), node);
        auto q = writeLocal(ValueType::Vector4, String::Format(TEXT("({0}.r < {1}.x) ? float4({1}.xyw, {0}.r) : float4({0}.r, {1}.yzx)"), rgb.Value, p.Value), node);
        auto c = writeLocal(ValueType::Float, String::Format(TEXT("{0}.x - min({0}.w, {0}.y)"), q.Value), node);
        auto h = writeLocal(ValueType::Float , String::Format(TEXT("abs(({0}.w - {0}.y) / (6 * {1} + {2}) + {0}.z)"), q.Value, c.Value, epsilon.Value), node);
        
        auto hcv = writeLocal(ValueType::Vector3, String::Format(TEXT("float3({0}, {1}, {2}.x)"), h.Value, c.Value, q.Value), node);
        value = writeLocal(ValueType::Vector3, String::Format(TEXT("float3({0}.x * 360.0f, {0}.y / ({0}.z + {1}), {0}.z)"), hcv.Value, epsilon.Value), node);
        break;
    }
    default:
        break;
    }
}

void MaterialGenerator::ProcessGroupFunction(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
        // Function Input
    case 1:
    {
        // Find the function call
        Node* functionCallNode = nullptr;
        ASSERT(_graphStack.Count() >= 2);
        Graph* graph;
        for (int32 i = _callStack.Count() - 1; i >= 0; i--)
        {
            if (_callStack[i]->Type == GRAPH_NODE_MAKE_TYPE(1, 24) && _functions.TryGet(_callStack[i], graph) && _graphStack[_graphStack.Count() - 1] == graph)
            {
                functionCallNode = _callStack[i];
                break;
            }
        }
        if (!functionCallNode)
        {
            OnError(node, box, TEXT("Missing calling function node."));
            value = Value::Zero;
            break;
        }
        const auto function = Assets.LoadAsync<MaterialFunction>((Guid)functionCallNode->Values[0]);
        if (!_functions.TryGet(functionCallNode, graph) || !function)
        {
            OnError(node, box, TEXT("Missing calling function graph."));
            value = Value::Zero;
            break;
        }

        // Peek the input box to use
        int32 inputIndex = -1;
        for (int32 i = 0; i < function->Inputs.Count(); i++)
        {
            if (node->ID == graph->Nodes[function->Inputs[i]].ID)
            {
                inputIndex = i;
                break;
            }
        }
        if (inputIndex < 0 || inputIndex >= function->Inputs.Count())
        {
            OnError(node, box, TEXT("Invalid function input box."));
            value = Value::Zero;
            break;
        }
        Box* functionCallBox = functionCallNode->TryGetBox(inputIndex);
        if (functionCallBox->HasConnection())
        {
            // Use provided input value from the function call
            _graphStack.Pop();
            value = eatBox(node, functionCallBox->FirstConnection());
            _graphStack.Push(graph);
        }
        else
        {
            // Use the default value from the function graph
            value = tryGetValue(node->TryGetBox(1), Value::Zero);
        }
        break;
    }
    default:
        break;
    }
}

#endif
