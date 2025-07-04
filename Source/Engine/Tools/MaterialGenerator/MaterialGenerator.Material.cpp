// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MATERIAL_GRAPH

#include "MaterialGenerator.h"
#include "Engine/Content/Assets/MaterialFunction.h"
#include "Engine/Visject/ShaderStringBuilder.h"

void MaterialGenerator::ProcessGroupMaterial(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
    // Material
    case 1:
        value = tryGetValue(box, Value::Zero);
        break;
    // World Position
    case 2:
        value = Value(VariantType::Float3, TEXT("input.WorldPosition.xyz"));
        break;
    // View
    case 3:
    {
        switch (box->ID)
        {
        // Position
        case 0:
            value = Value(VariantType::Float3, TEXT("ViewPos"));
            break;
        // Direction
        case 1:
            value = Value(VariantType::Float3, TEXT("ViewDir"));
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
        // Check if use main view position
        const auto layer = GetRootLayer();
        if (layer && layer->Domain == MaterialDomain::Surface && node->Values.Count() > 0 && node->Values[0].AsBool)
        {
            // Transform world position into main viewport texcoord space
            Value clipPosition = writeLocal(VariantType::Float4, TEXT("mul(float4(input.WorldPosition.xyz, 1), MainViewProjectionMatrix)"), node);
            Value uvPos = writeLocal(VariantType::Float2, String::Format(TEXT("(({0}.xy / {0}.w) * float2(0.5, -0.5) + float2(0.5, 0.5))"), clipPosition.Value), node);

            // Position
            if (box->ID == 0)
                value = writeLocal(VariantType::Float2, String::Format(TEXT("{0} * MainScreenSize.xy"), uvPos.Value), node);
                // Texcoord
            else if (box->ID == 1)
                value = uvPos;
        }
        else
        {
            // Position
            if (box->ID == 0)
                value = Value(VariantType::Float2, TEXT("input.SvPosition.xy"));
                // Texcoord
            else if (box->ID == 1)
                value = writeLocal(VariantType::Float2, TEXT("input.SvPosition.xy * ScreenSize.zw"), node);
        }
        break;
    }
    // Screen Size
    case 7:
    {
        // Check if use main view position
        const auto layer = GetRootLayer();
        if (layer && layer->Domain == MaterialDomain::Surface && node->Values.Count() > 0 && node->Values[0].AsBool)
            value = Value(VariantType::Float2, box->ID == 0 ? TEXT("MainScreenSize.xy") : TEXT("MainScreenSize.zw"));
        else
            value = Value(VariantType::Float2, box->ID == 0 ? TEXT("ScreenSize.xy") : TEXT("ScreenSize.zw"));
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
                values[i] = writeLocal(VariantType::Float4, node);
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
                if (inputValue.Type != VariantType::Float4)
                    inputValue = inputValue.Cast(VariantType::Float4);
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
        value = Value(VariantType::Float3, TEXT("GetObjectPosition(input)"));
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

        auto x1 = writeLocal(VariantType::Float3, TEXT("ViewPos - input.WorldPosition"), node);
        auto x2 = writeLocal(VariantType::Float3, TEXT("TransformViewVectorToWorld(input, float3(0, 0, -1))"), node);
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
        value = Value(VariantType::Float3, TEXT("input.PreSkinnedPosition"));
        if (_treeType != MaterialTreeType::VertexShader)
            value = VsToPs(node, box).AsFloat3();
        break;
    // Pre-skinned Local Normal
    case 14:
        value = Value(VariantType::Float3, TEXT("input.PreSkinnedNormal"));
        if (_treeType != MaterialTreeType::VertexShader)
            value = VsToPs(node, box).AsFloat3();
        break;
    // Depth
    case 15:
        value = writeLocal(VariantType::Float, TEXT("distance(ViewPos, input.WorldPosition)"), node);
        break;
    // Tangent
    case 16:
        value = Value(VariantType::Float3, TEXT("input.TBN[0]"));
        break;
    // Bitangent
    case 17:
        value = Value(VariantType::Float3, TEXT("input.TBN[1]"));
        break;
    // Camera Position
    case 18:
        value = Value(VariantType::Float3, TEXT("ViewPos"));
        break;
    // Per Instance Random
    case 19:
        value = Value(VariantType::Float, TEXT("GetPerInstanceRandom(input)"));
        break;
    // Interpolate VS To PS
    case 20:
        value = VsToPs(node, node->GetBox(0));
        break;
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
        auto screenUVs = writeLocal(VariantType::Float2, TEXT("input.SvPosition.xy * ScreenSize.zw"), node);

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
        value = Value(VariantType::Float3, TEXT("GetObjectSize(input)"));
        break;
    // Blend Normals
    case 26:
    {
        const auto baseNormal = tryGetValue(node->GetBox(0), getNormalZero).AsFloat3();
        const auto additionalNormal = tryGetValue(node->GetBox(1), getNormalZero).AsFloat3();

        const String text1 = String::Format(TEXT("(float2({0}.xy) + float2({1}.xy) * 2.0)"), baseNormal.Value, additionalNormal.Value);
        const auto appendXY = writeLocal(ValueType::Float2, text1, node);

        const String text2 = String::Format(TEXT("float3({0}, sqrt(saturate(1.0 - dot({0}.xy, {0}.xy))))"), appendXY.Value);
        value = writeLocal(ValueType::Float3, text2, node);
        break;
    }
    // Rotator
    case 27:
    {
        const auto uv = tryGetValue(node->GetBox(0), getUVs).AsFloat2();
        const auto center = tryGetValue(node->GetBox(1), Value::Zero).AsFloat2();
        const auto rotationAngle = tryGetValue(node->GetBox(2), Value::Zero).AsFloat();

        auto x1 = writeLocal(ValueType::Float2, String::Format(TEXT("({0} * -1) + {1}"), center.Value, uv.Value), node);
        auto raCosSin = writeLocal(ValueType::Float2, String::Format(TEXT("float2(cos({0}), sin({0}))"), rotationAngle.Value), node);

        auto dotB1 = writeLocal(ValueType::Float2, String::Format(TEXT("float2({0}.x, {0}.y * -1)"), raCosSin.Value), node);
        auto dotB2 = writeLocal(ValueType::Float2, String::Format(TEXT("float2({0}.y, {0}.x)"), raCosSin.Value), node);

        value = writeLocal(ValueType::Float2, String::Format(TEXT("{3} + float2(dot({0},{1}), dot({0},{2}))"), x1.Value, dotB1.Value, dotB2.Value, center.Value), node);
        break;
    }
    // Sphere Mask
    case 28:
    {
        const auto a = tryGetValue(node->GetBox(0), getUVs);
        const auto b = tryGetValue(node->GetBox(1), Value::Half).Cast(a.Type);
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
        const auto uv = tryGetValue(node->GetBox(0), getUVs).AsFloat2();
        const auto tiling = tryGetValue(node->GetBox(1), node->Values[0]).AsFloat2();
        const auto offset = tryGetValue(node->GetBox(2), node->Values[1]).AsFloat2();

        value = writeLocal(ValueType::Float2, String::Format(TEXT("{0} * {1} + {2}"), uv.Value, tiling.Value, offset.Value), node);
        break;
    }
    // DDX
    case 30:
    {
        if (_treeType == MaterialTreeType::PixelShader)
        {
            const auto inValue = tryGetValue(node->GetBox(0), 0, Value::Zero);
            value = writeLocal(inValue.Type, String::Format(TEXT("ddx({0})"), inValue.Value), node);
        }
        else
        {
            // No derivatives support in VS/DS
            value = Value::Zero;
        }
        break;
    }
    // DDY
    case 31:
    {
        if (_treeType == MaterialTreeType::PixelShader)
        {
            const auto inValue = tryGetValue(node->GetBox(0), 0, Value::Zero);
            value = writeLocal(inValue.Type, String::Format(TEXT("ddy({0})"), inValue.Value), node);
        }
        else
        {
            // No derivatives support in VS/DS
            value = Value::Zero;
        }
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
        auto color = writeLocal(ValueType::Float3, String::Format(TEXT("float3({0}, {1}, {2})"), x.Value, y.Value, z.Value), node);
        color = writeLocal(ValueType::Float3, String::Format(TEXT("clamp({0}, 0.0f, 255.0f) / 255.0f"), color.Value), node);
        value = writeLocal(ValueType::Float3, String::Format(TEXT("{1} < 1000.0f ? {0} * {1}/1000.0f : {0}"), color.Value, temperature.Value), node);
        break;
    }
    // HSVToRGB
    case 36:
    {
        const auto hsv = tryGetValue(node->GetBox(0), node->Values[0]).AsFloat3();

        // Normalize from 360
        auto color = writeLocal(ValueType::Float3, String::Format(TEXT("float3({0}.x / 360.0f, {0}.y, {0}.z)"), hsv.Value), node);

        auto x1 = writeLocal(ValueType::Float3, String::Format(TEXT("clamp(abs(fmod({0}.x * 6.0 + float3(0.0f, 4.0f, 2.0f), 6.0f) - 3.0f) - 1.0f, 0.0f, 1.0f)"), color.Value), node);
        value = writeLocal(ValueType::Float3, String::Format(TEXT("{1}.z * lerp(float3(1.0, 1.0, 1.0), {0}, {1}.y)"), x1.Value, color.Value), node);
        break;
    }
    // RGBToHSV
    case 37:
    {
        // Reference: Ian Taylor, https://www.chilliant.com/rgb2hsv.html

        const auto rgb = tryGetValue(node->GetBox(0), node->Values[0]).AsFloat3();
        const auto epsilon = writeLocal(ValueType::Float, TEXT("1e-10"), node);

        auto p = writeLocal(ValueType::Float4, String::Format(TEXT("({0}.g < {0}.b) ? float4({0}.bg, -1.0f, 2.0f/3.0f) : float4({0}.gb, 0.0f, -1.0f/3.0f)"), rgb.Value), node);
        auto q = writeLocal(ValueType::Float4, String::Format(TEXT("({0}.r < {1}.x) ? float4({1}.xyw, {0}.r) : float4({0}.r, {1}.yzx)"), rgb.Value, p.Value), node);
        auto c = writeLocal(ValueType::Float, String::Format(TEXT("{0}.x - min({0}.w, {0}.y)"), q.Value), node);
        auto h = writeLocal(ValueType::Float, String::Format(TEXT("abs(({0}.w - {0}.y) / (6 * {1} + {2}) + {0}.z)"), q.Value, c.Value, epsilon.Value), node);

        auto hcv = writeLocal(ValueType::Float3, String::Format(TEXT("float3({0}, {1}, {2}.x)"), h.Value, c.Value, q.Value), node);
        value = writeLocal(ValueType::Float3, String::Format(TEXT("float3({0}.x * 360.0f, {0}.y / ({0}.z + {1}), {0}.z)"), hcv.Value, epsilon.Value), node);
        break;
    }
    // View Size
    case 39:
    {
        const auto layer = GetRootLayer();
        if (layer && layer->Domain == MaterialDomain::GUI)
        {
            value = Value(VariantType::Float2, box->ID == 0 ? TEXT("ViewSize.xy") : TEXT("ViewSize.zw"));
        }
        else
        {
            // Fallback to Screen Size
            value = Value(VariantType::Float2, box->ID == 0 ? TEXT("ScreenSize.xy") : TEXT("ScreenSize.zw"));
        }
        break;
    }
    // Rectangle Mask
    case 40:
    {
        const auto uv = tryGetValue(node->GetBox(0), getUVs).AsFloat2();
        const auto rectangle = tryGetValue(node->GetBox(1), node->Values[0]).AsFloat2();
        auto d = writeLocal(ValueType::Float2, String::Format(TEXT("abs({0} * 2 - 1) - {1}"), uv.Value, rectangle.Value), node);
        auto d2 = writeLocal(ValueType::Float2, String::Format(TEXT("1 - {0} / fwidth({0})"), d.Value), node);
        value = writeLocal(ValueType::Float, String::Format(TEXT("saturate(min({0}.x, {0}.y))"), d2.Value), node);
        break;
    }
    // FWidth
    case 41:
    {
        const auto inValue = tryGetValue(node->GetBox(0), 0, Value::Zero);
        value = writeLocal(inValue.Type, String::Format(TEXT("fwidth({0})"), inValue.Value), node);
        break;
    }
    // AA Step
    case 42:
    {
        // Reference: https://www.ronja-tutorials.com/post/046-fwidth/#a-better-step

        const auto compValue = tryGetValue(node->GetBox(0), getUVs).AsFloat();
        const auto gradient = tryGetValue(node->GetBox(1), node->Values[0]).AsFloat();

        auto change = writeLocal(ValueType::Float, String::Format(TEXT("fwidth({0})"), gradient.Value), node);

        // Base the range of the inverse lerp on the change over two pixels
        auto lowerEdge = writeLocal(ValueType::Float, String::Format(TEXT("{0} - {1}"), compValue.Value, change.Value), node);
        auto upperEdge = writeLocal(ValueType::Float, String::Format(TEXT("{0} + {1}"), compValue.Value, change.Value), node);

        // Do the inverse interpolation and saturate it
        value = writeLocal(ValueType::Float, String::Format(TEXT("saturate((({0} - {1}) / ({2} - {1})))"), gradient.Value, lowerEdge.Value, upperEdge.Value), node);
    }
    // Rotate UV [Rotator Simple]
    case 43:
    {
        //cosine = cos(rotation);
        //sine = sin(rotation);
        //float2 out = float2(cosine * uv.x + sine * uv.y,cosine * uv.y - sine * uv.x);
        const auto uv = tryGetValue(node->GetBox(0), getUVs).AsFloat2();
        const auto rotationAngle = tryGetValue(node->GetBox(1), node->Values[0].AsFloat).AsFloat();
        auto c = writeLocal(ValueType::Float, String::Format(TEXT("cos({0})"), rotationAngle.Value), node);
        auto s = writeLocal(ValueType::Float, String::Format(TEXT("sin({0})"), rotationAngle.Value), node);
        value = writeLocal(ValueType::Float2, String::Format(TEXT("float2({1} * {0}.x + {2} * {0}.y, {1} * {0}.y - {2} * {0}.x)"), uv.Value, c.Value, s.Value), node);
        break;
    }
    // Cone Gradient
    case 44:
    {
        //float gradient = angle - abs(atan2(uv.x,uv.y));
        const auto uv = tryGetValue(node->GetBox(0), getUVs).AsFloat2();
        const auto rotationAngle = tryGetValue(node->GetBox(1), node->Values[0].AsFloat).AsFloat();
        value = writeLocal(ValueType::Float, String::Format(TEXT("{1} - abs(atan2({0}.x, {0}.y))"), uv.Value, rotationAngle.Value), node);
        break;
    }
    // Cycle Gradient
    case 45:
    {
        //float gradient = 1 - length(uv * 2);
        const auto uv = tryGetValue(node->GetBox(0), getUVs).AsFloat2();
        value = writeLocal(ValueType::Float, String::Format(TEXT("1 - length({0} * 2.0)"), uv.Value), node);
        break;
    }
    // Falloff and Offset
    case 46:
    {
        //float out = clamp((((Value - (1 - Offset)) + Falloff) / Falloff),0,1)
        const auto in = tryGetValue(node->GetBox(0), ShaderGraphValue::Zero);
        const auto graphValue = tryGetValue(node->GetBox(1), node->Values[0].AsFloat);
        const auto falloff = tryGetValue(node->GetBox(2), node->Values[1].AsFloat);
        value = writeLocal(ValueType::Float, String::Format(TEXT("saturate(((({0} - (1.0 - {1})) + {2}) / {2}))"), in.Value, graphValue.Value, falloff.Value), node);
        break;
    }
    // Linear Gradient
    case 47:
    {
        // float2 uv = Input0.xy;
        // float r = Input0.z;
        // float2 A = 1.0 - float2(cos(r) * uv.x + sin(r) * uv.y, cos(r) * uv.y - sin(r) * uv.x);
        // float2 out  = float2(Mirror ? abs(A.x < 1.0 ? (A.x - 0.5) * 2 : (2 - ((A.x - 0.5) * 2)) * -1) : A.x < 1.0 ? (A.x - 0.5) * 2 : 1,Mirror ? abs(A.y < 1.0 ? (A.y - 0.5) * 2 : (2 - ((A.y - 0.5) * 2)) * -1) : A.y < 1.0 ? (A.y - 0.5) * 2 : 1);

        const auto uv = tryGetValue(node->GetBox(0), getUVs).AsFloat2();
        const auto rotationAngle = tryGetValue(node->GetBox(1), node->Values[0].AsFloat).AsFloat();
        const auto mirror = tryGetValue(node->GetBox(2), node->Values[1].AsBool).AsBool();

        auto c = writeLocal(ValueType::Float, String::Format(TEXT("cos({0})"), rotationAngle.Value), node);
        auto s = writeLocal(ValueType::Float, String::Format(TEXT("sin({0})"), rotationAngle.Value), node);
        auto a = writeLocal(ValueType::Float2, String::Format(TEXT("1.0 - float2({1} * {0}.x + {2} * {0}.y, {1} * {0}.y - {2} * {0}.x)"), uv.Value, c.Value, s.Value), node);
        value = writeLocal(
            ValueType::Float2, String::Format(TEXT
                (
                    "float2({0} ? abs({1}.x < 1.0 ? ({1}.x - 0.5) * 2 : (2 - (({1}.x - 0.5) * 2)) * -1) : {1}.x < 1.0 ? ({1}.x - 0.5) * 2 : 1,{0} ? abs({1}.y < 1.0 ? ({1}.y - 0.5) * 2 : (2 - (({1}.y - 0.5) * 2)) * -1) : {1}.y < 1.0 ? ({1}.y - 0.5) * 2 : 1)"
                ), mirror.Value, a.Value),
            node);
        break;
    }
    // Radial Gradient
    case 48:
    {
        //float gradient = clamp(atan2(uv.x,uv.y) - angle,0.0,1.0);
        const auto uv = tryGetValue(node->GetBox(0), getUVs).AsFloat2();
        const auto rotationAngle = tryGetValue(node->GetBox(1), node->Values[0].AsFloat).AsFloat();
        value = writeLocal(ValueType::Float, String::Format(TEXT("saturate(atan2({0}.x, {0}.y) - {1})"), uv.Value, rotationAngle.Value), node);
        break;
    }
    // Ring Gradient
    case 49:
    {
        // Nodes:
        // float c = CycleGradient(uv)
        // float InnerMask = FalloffAndOffset(c,(OuterBounds - Falloff),Falloff)
        // float OuterMask = FalloffAndOffset(1-c,1-InnerBounds,Falloff)
        // float Mask      = OuterMask * InnerMask;

        // TODO: check if there is some useless operators

        //expanded
        //float cycleGradient  = 1 - length(uv * 2);
        //float InnerMask      = clamp((((c - (1 - (OuterBounds - Falloff))) + Falloff) / Falloff),0,1)
        //float OuterMask      = clamp(((((1-c) - (1 - (1-InnerBounds))) + Falloff) / Falloff),0,1)
        //float Mask           = OuterMask * InnerMask;

        const auto uv = tryGetValue(node->GetBox(0), getUVs).AsFloat2();
        const auto outerBounds = tryGetValue(node->GetBox(1), node->Values[0].AsFloat).AsFloat();
        const auto innerBounds = tryGetValue(node->GetBox(2), node->Values[1].AsFloat).AsFloat();
        const auto falloff = tryGetValue(node->GetBox(3), node->Values[2].AsFloat).AsFloat();
        auto c = writeLocal(ValueType::Float, String::Format(TEXT("1 - length({0} * 2.0)"), uv.Value), node);
        auto innerMask = writeLocal(ValueType::Float, String::Format(TEXT("saturate(((({0} - (1.0 - ({1} - {2}))) + {2}) / {2}))"), c.Value, outerBounds.Value, falloff.Value), node);
        auto outerMask = writeLocal(ValueType::Float, String::Format(TEXT("saturate(((((1.0 - {0}) - (1.0 - (1.0 - {1}))) + {2}) / {2}))"), c.Value, innerBounds.Value, falloff.Value), node);
        auto mask = writeLocal(ValueType::Float, String::Format(TEXT("{0} * {1}"), innerMask.Value, outerMask.Value), node);
        value = writeLocal(ValueType::Float3, String::Format(TEXT("float3({0}, {1}, {2})"), innerMask.Value, outerMask.Value, mask.Value), node);
        break;
    }
    // Shift HSV
    case 50:
    {
        const auto color = tryGetValue(node->GetBox(0), Value::One).AsFloat4();
        if (!color.IsValid())
        {
            value = Value::Zero;
            break;
        }
        const auto hue = tryGetValue(node->GetBox(1), node->Values[0]).AsFloat();
        const auto saturation = tryGetValue(node->GetBox(2), node->Values[1]).AsFloat();
        const auto val = tryGetValue(node->GetBox(3), node->Values[2]).AsFloat();
        auto result = writeLocal(Value::InitForZero(ValueType::Float4), node);

        const String hsvAdjust = ShaderStringBuilder()
            .Code(TEXT(R"(
    {
        float3 rgb = %COLOR%.rgb;
        float minc = min(min(rgb.r, rgb.g), rgb.b);
        float maxc = max(max(rgb.r, rgb.g), rgb.b);
        float delta = maxc - minc;

        float3 grb = float3(rgb.g - rgb.b, rgb.r - rgb.b, rgb.b - rgb.g);
        float3 cmps = float3(maxc == rgb.r, maxc == rgb.g, maxc == rgb.b);
        float h = dot(grb * rcp(delta), cmps);
        h += 6.0 * (h < 0);
        h = frac(h * (1.0/6.0) * step(0, delta) + %HUE% * 0.5);
    
        float s = saturate(delta * rcp(maxc + step(maxc, 0)) * (1.0 + %SATURATION%));
        float v = maxc * (1.0 + %VALUE%);
    
        float3 k = float3(1.0, 2.0 / 3.0, 1.0 / 3.0);
        %RESULT% = float4(v * lerp(1.0, saturate(abs(frac(h + k) * 6.0 - 3.0) - 1.0), s), %COLOR%.a);
    }
    )"))
            .Replace(TEXT("%COLOR%"), color.Value)
            .Replace(TEXT("%HUE%"), hue.Value)
            .Replace(TEXT("%SATURATION%"), saturation.Value)
            .Replace(TEXT("%VALUE%"), val.Value)
            .Replace(TEXT("%RESULT%"), result.Value)
            .Build();
        _writer.Write(*hsvAdjust);
        value = result;
        break;
    }
    // Color Blend
    case 51:
    {
        const auto baseColor = tryGetValue(node->GetBox(0), Value::One).AsFloat4();
        const auto blendColor = tryGetValue(node->GetBox(1), Value::One).AsFloat4();
        const auto blendAmount = tryGetValue(node->GetBox(2), node->Values[1]).AsFloat();
        const auto blendMode = node->Values[0].AsInt;
        auto result = writeLocal(Value::InitForZero(ValueType::Float4), node);

        String blendFormula;
        switch (blendMode)
        {
        case 0: // Normal
            blendFormula = TEXT("blend");
            break;
        case 1: // Add
            blendFormula = TEXT("base + blend");
            break;
        case 2: // Subtract
            blendFormula = TEXT("base - blend");
            break;
        case 3: // Multiply
            blendFormula = TEXT("base * blend");
            break;
        case 4: // Screen
            blendFormula = TEXT("1.0 - (1.0 - base) * (1.0 - blend)");
            break;
        case 5: // Overlay
            blendFormula = TEXT("base <= 0.5 ? 2.0 * base * blend : 1.0 - 2.0 * (1.0 - base) * (1.0 - blend)");
            break;
        case 6: // Linear Burn
            blendFormula = TEXT("base + blend - 1.0");
            break;
        case 7: // Linear Light
            blendFormula = TEXT("blend < 0.5 ? max(base + (2.0 * blend) - 1.0, 0.0) : min(base + 2.0 * (blend - 0.5), 1.0)");
            break;
        case 8: // Darken
            blendFormula = TEXT("min(base, blend)");
            break;
        case 9: // Lighten
            blendFormula = TEXT("max(base, blend)");
            break;
        case 10: // Difference
            blendFormula = TEXT("abs(base - blend)");
            break;
        case 11: // Exclusion
            blendFormula = TEXT("base + blend - (2.0 * base * blend)");
            break;
        case 12: // Divide
            blendFormula = TEXT("base / (blend + 0.000001)");
            break;
        case 13: // Hard Light
            blendFormula = TEXT("blend <= 0.5 ? 2.0 * base * blend : 1.0 - 2.0 * (1.0 - base) * (1.0 - blend)");
            break;
        case 14: // Pin Light
            blendFormula = TEXT("blend <= 0.5 ? min(base, 2.0 * blend) : max(base, 2.0 * (blend - 0.5))");
            break;
        case 15: // Hard Mix
            blendFormula = TEXT("step(1.0 - base, blend)");
            break;
        default:
            blendFormula = TEXT("blend");
            break;
        }

        const String blendImpl = ShaderStringBuilder()
            .Code(TEXT(R"(
    {
        float3 base = %BASE%.rgb;
        float3 blend = %BLEND%.rgb;
        float alpha = %BASE%.a;
        float3 final = %BLEND_FORMULA%;
        %RESULT% = float4(lerp(base, final, %AMOUNT%), alpha);
    }
    )"))
            .Replace(TEXT("%BASE%"), baseColor.Value)
            .Replace(TEXT("%BLEND%"), blendColor.Value)
            .Replace(TEXT("%AMOUNT%"), blendAmount.Value)
            .Replace(TEXT("%BLEND_FORMULA%"), *blendFormula)
            .Replace(TEXT("%RESULT%"), result.Value)
            .Build();
        _writer.Write(*blendImpl);
        value = result;
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
        if (functionCallBox && functionCallBox->HasConnection())
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
