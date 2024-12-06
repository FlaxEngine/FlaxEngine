// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if USE_EDITOR

#include "ShaderGraph.h"
#include "GraphUtilities.h"
#include "ShaderGraphUtilities.h"
#include "Engine/Engine/GameplayGlobals.h"

const Char* ShaderGenerator::_mathFunctions[] =
{
    TEXT("abs"),
    TEXT("ceil"),
    TEXT("cos"),
    TEXT("floor"),
    TEXT("length"),
    TEXT("normalize"),
    TEXT("round"),
    TEXT("saturate"),
    TEXT("sin"),
    TEXT("sqrt"),
    TEXT("tan"),
    TEXT("cross"),
    TEXT("distance"),
    TEXT("dot"),
    TEXT("max"),
    TEXT("min"),
    TEXT("pow"),
    TEXT("reflect"),
};
const Char* ShaderGenerator::_subs[] =
{
    TEXT(".x"),
    TEXT(".y"),
    TEXT(".z"),
    TEXT(".w")
};

ShaderGenerator::ShaderGenerator()
    : _writer(2048)
{
    // Register per group type processing events
    // Note: index must match group id
    _perGroupProcessCall.Resize(17);
    _perGroupProcessCall[2].Bind<ShaderGenerator, &ShaderGenerator::ProcessGroupConstants>(this);
    _perGroupProcessCall[3].Bind<ShaderGenerator, &ShaderGenerator::ProcessGroupMath>(this);
    _perGroupProcessCall[4].Bind<ShaderGenerator, &ShaderGenerator::ProcessGroupPacking>(this);
    _perGroupProcessCall[10].Bind<ShaderGenerator, &ShaderGenerator::ProcessGroupBoolean>(this);
    _perGroupProcessCall[11].Bind<ShaderGenerator, &ShaderGenerator::ProcessGroupBitwise>(this);
    _perGroupProcessCall[12].Bind<ShaderGenerator, &ShaderGenerator::ProcessGroupComparisons>(this);
}

ShaderGenerator::~ShaderGenerator()
{
    _functions.ClearDelete();
}

void ShaderGenerator::OnError(Node* node, Box* box, const StringView& message)
{
    Error(node, box, message);
}

void ShaderGenerator::ProcessGroupConstants(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
    // Constant value
    case 1:
    case 2:
    case 3:
    case 12:
    case 15:
        value = Value(node->Values[0]);
        break;
    // Float2/3/4, Color
    case 4:
    case 5:
    case 6:
    case 7:
    {
        const Variant& v = node->Values[0];
        const Float4 cv = (Float4)v;
        if (box->ID == 0)
            value = Value(v);
        else if (box->ID == 1)
            value = Value(cv.X);
        else if (box->ID == 2)
            value = Value(cv.Y);
        else if (box->ID == 3)
            value = Value(cv.Z);
        else if (box->ID == 4)
            value = Value(cv.W);
        break;
    }
    // Rotation
    case 8:
    {
        const float pitch = (float)node->Values[0];
        const float yaw = (float)node->Values[1];
        const float roll = (float)node->Values[2];
        value = Value(Quaternion::Euler(pitch, yaw, roll));
        break;
    }
    // PI
    case 10:
    {
        value = Value(PI);
        break;
    }
    // Vector2/3/4
    case 16:
    case 17:
    case 18:
    {
        const Variant& v = node->Values[0];
        const Vector4 cv = (Vector4)v;
        if (box->ID == 0)
            value = Value(v);
        else if (box->ID == 1)
            value = Value(cv.X);
        else if (box->ID == 2)
            value = Value(cv.Y);
        else if (box->ID == 3)
            value = Value(cv.Z);
        else if (box->ID == 4)
            value = Value(cv.W);
        break;
    }
    default:
        break;
    }
}

void ShaderGenerator::ProcessGroupMath(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
    // Add, Subtract, Multiply, Divide, Modulo
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    {
        Box* b1 = node->GetBox(0);
        Box* b2 = node->GetBox(1);
        Value v1 = tryGetValue(b1, 0, Value::Zero);
        Value v2 = tryGetValue(b2, 1, Value::Zero);
        if (b1->HasConnection())
            v2 = v2.Cast(v1.Type);
        else
            v1 = v1.Cast(v2.Type);
        Char op = '?';
        switch (node->TypeID)
        {
        case 1:
            op = '+';
            break;
        case 2:
            op = '-';
            break;
        case 3:
            op = '*';
            break;
        case 4:
            op = '%';
            break;
        case 5:
        {
            op = '/';
            if (v2.IsZero())
            {
                OnError(node, b2, TEXT("Cannot divide by zero!"));
                v2 = Value::One;
            }
            break;
        }
        default:
            break;
        }
        value = writeOperation2(node, v1, v2, op);
        break;
    }
    // Absolute Value, Ceil, Cosine, Floor, Normalize, Round, Saturate, Sine, Sqrt, Tangent
    case 7:
    case 8:
    case 9:
    case 10:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    {
        Box* b1 = node->GetBox(0);
        Value v1 = tryGetValue(b1, Value::Zero);
        const Char* function = _mathFunctions[node->TypeID - 7];
        value = writeFunction1(node, v1, function);
        break;
    }
    // Length
    case 11:
    {
        String text = String::Format(TEXT("length({0})"), tryGetValue(node->GetBox(0), Value::Zero).Value);
        value = writeLocal(ValueType::Float, text, node);
        break;
    }
    // Cross
    case 18:
    {
        Value v1 = tryGetValue(node->GetBox(0), 0, Value::Zero).Cast(VariantType::Float3);
        Value v2 = tryGetValue(node->GetBox(1), 1, Value::Zero).Cast(VariantType::Float3);
        const Char* function = _mathFunctions[node->TypeID - 7];
        value = writeFunction2(node, v1, v2, function);
        break;
    }
    // Max, Min, Pow
    case 21:
    case 22:
    case 23:
    {
        Value v1 = tryGetValue(node->GetBox(0), 0, Value::Zero);
        Value v2 = tryGetValue(node->GetBox(1), 1, Value::Zero);
        const Char* function = _mathFunctions[node->TypeID - 7];
        value = writeFunction2(node, v1, v2, function);
        break;
    }
    // Distance, Dot
    case 19:
    case 20:
    {
        Value v1 = tryGetValue(node->GetBox(0), 0, Value::Zero);
        Value v2 = tryGetValue(node->GetBox(1), 1, Value::Zero);
        const Char* function = _mathFunctions[node->TypeID - 7];
        value = writeFunction2(node, v1, v2, function, ValueType::Float);
        break;
    }
    // Clamp
    case 24:
    {
        Box* b1 = node->GetBox(0);
        Box* b2 = node->GetBox(1);
        Box* b3 = node->GetBox(2);
        Value v1 = tryGetValue(b1, Value::Zero);
        Value v2 = tryGetValue(b2, 0, Value::Zero);
        Value v3 = tryGetValue(b3, 1, Value::One);
        value = writeFunction3(node, v1, v2, v3, TEXT("clamp"), v1.Type);
        break;
    }
    // Lerp
    case 25:
    {
        Value a = tryGetValue(node->GetBox(0), 0, Value::Zero);
        Value b = tryGetValue(node->GetBox(1), 1, Value::One).Cast(a.Type);
        Value alpha = tryGetValue(node->GetBox(2), 2, Value::Zero).Cast(ValueType::Float);
        String text = String::Format(TEXT("lerp({0}, {1}, {2})"), a.Value, b.Value, alpha.Value);
        value = writeLocal(a.Type, text, node);
        break;
    }
    // Reflect
    case 26:
    {
        Value v1 = tryGetValue(node->GetBox(0), Value::Zero);
        Value v2 = tryGetValue(node->GetBox(1), Value::Zero);
        const Char* function = _mathFunctions[17];
        value = writeFunction2(node, v1, v2, function);
        break;
    }
    // Negate
    case 27:
    {
        Box* b1 = node->GetBox(0);
        Value v1 = tryGetValue(b1, Value::Zero);
        value = writeLocal(v1.Type, String(TEXT("-")) + v1.Value, node);
        break;
    }
    // 1 - Value
    case 28:
    {
        Value v1 = tryGetValue(node->GetBox(0), Value::Zero);
        value = writeOperation2(node, Value::InitForOne(v1.Type), v1, '-');
        break;
    }
    // Derive Normal Z
    case 29:
    {
        Value inXY = tryGetValue(node->GetBox(0), Value::Zero).AsFloat2();
        value = writeLocal(ValueType::Float3, String::Format(TEXT("float3({0}, sqrt(saturate(1.0 - dot({0}.xy, {0}.xy))))"), inXY.Value), node);
        break;
    }
    // Mad
    case 31:
    {
        Value v1 = tryGetValue(node->GetBox(0), Value::Zero);
        Value v2 = tryGetValue(node->GetBox(1), 0, Value::One);
        Value v3 = tryGetValue(node->GetBox(2), 1, Value::Zero);
        String text = String::Format(TEXT("({0}) * ({1}) + ({2})"), v1.Value, Value::Cast(v2, v1.Type).Value, Value::Cast(v3, v1.Type).Value);
        value = writeLocal(v1.Type, text, node);
        break;
    }
    // Extract Largest Component
    case 32:
    {
        Value v1 = tryGetValue(node->GetBox(0), Value::Zero);
        String text = String::Format(TEXT("ExtractLargestComponent({0})"), Value::Cast(v1, ValueType::Float3).Value);
        value = writeLocal(ValueType::Float3, text, node);
        _includes.Add(TEXT("./Flax/Math.hlsl"));
        break;
    }
    // Asine
    case 33:
    {
        value = writeFunction1(node, tryGetValue(node->GetBox(0), Value::Zero), TEXT("asin"));
        break;
    }
    // Acosine
    case 34:
    {
        value = writeFunction1(node, tryGetValue(node->GetBox(0), Value::Zero), TEXT("acos"));
        break;
    }
    // Atan
    case 35:
    {
        value = writeFunction1(node, tryGetValue(node->GetBox(0), Value::Zero), TEXT("atan"));
        break;
    }
    // Bias and Scale
    case 36:
    {
        ASSERT(node->Values.Count() == 2 && node->Values[0].Type == VariantType::Float && node->Values[1].Type == VariantType::Float);
        const Value input = tryGetValue(node->GetBox(0), Value::Zero);
        const Value bias = Value(node->Values[0].AsFloat).Cast(input.Type);
        const Value scale = Value(node->Values[1].AsFloat).Cast(input.Type);
        const String text = String::Format(TEXT("({0} + {1}) * {2}"), input.Value, bias.Value, scale.Value);
        value = writeLocal(input.Type, text, node);
        break;
    }
    // Rotate About Axis
    case 37:
    {
        const auto normalizedRotationAxis = tryGetValue(node->GetBox(0), Value::Zero).AsFloat3();
        const auto shaderGraphValue = tryGetValue(node->GetBox(1), Value::Zero).AsFloat();
        const auto graphValue = tryGetValue(node->GetBox(2), Value::Zero).AsFloat3();
        const auto position = tryGetValue(node->GetBox(3), Value::Zero).AsFloat3();
        const String text = String::Format(TEXT("RotateAboutAxis(float4({0}, {1}), {2}, {3})"), normalizedRotationAxis.Value, shaderGraphValue.Value, graphValue.Value, position.Value);
        _includes.Add(TEXT("./Flax/Math.hlsl"));
        value = writeLocal(ValueType::Float3, text, node);
        break;
    }
    // Trunc
    case 38:
    {
        value = writeFunction1(node, tryGetValue(node->GetBox(0), Value::Zero), TEXT("trunc"));
        break;
    }
    // Frac
    case 39:
    {
        value = writeFunction1(node, tryGetValue(node->GetBox(0), Value::Zero), TEXT("frac"));
        break;
    }
    // Fmod
    case 40:
    {
        Value v1 = tryGetValue(node->GetBox(0), Value::Zero);
        Value v2 = tryGetValue(node->GetBox(1), Value::Zero);
        value = writeFunction2(node, v1, v2, TEXT("fmod"));
        break;
    }
    // Atan2
    case 41:
    {
        Value v1 = tryGetValue(node->GetBox(0), Value::Zero);
        Value v2 = tryGetValue(node->GetBox(1), Value::Zero);
        value = writeFunction2(node, v1, v2, TEXT("atan2"));
        break;
    }
    // Near Equal
    case 42:
    {
        Value v1 = tryGetValue(node->GetBox(0), Value::Zero);
        Value v2 = tryGetValue(node->GetBox(1), Value::Zero).Cast(v1.Type);
        Value epsilon = tryGetValue(node->GetBox(2), 2, Value::Zero);
        value = writeLocal(ValueType::Bool, String::Format(TEXT("distance({0},{1}) < {2}"), v1.Value, v2.Value, epsilon.Value), node);
        break;
    }
    // Degrees
    case 43:
    {
        value = writeFunction1(node, tryGetValue(node->GetBox(0), Value::Zero), TEXT("degrees"));
        break;
    }
    // Radians
    case 44:
    {
        value = writeFunction1(node, tryGetValue(node->GetBox(0), Value::Zero), TEXT("radians"));
        break;
    }
    // Remap
    case 48:
    {
        const auto inVal = tryGetValue(node->GetBox(0), node->Values[0].AsFloat);
        const auto rangeA = tryGetValue(node->GetBox(1), node->Values[1].AsFloat2());
        const auto rangeB = tryGetValue(node->GetBox(2), node->Values[2].AsFloat2());
        const auto clamp = tryGetValue(node->GetBox(3), node->Values[3]).AsBool();
        const auto mapFunc = String::Format(TEXT("{2}.x + ({0} - {1}.x) * ({2}.y - {2}.x) / ({1}.y - {1}.x)"), inVal.Value, rangeA.Value, rangeB.Value);
        value = writeLocal(ValueType::Float, String::Format(TEXT("{2} ? clamp({0}, {1}.x, {1}.y) : {0}"), mapFunc, rangeB.Value, clamp.Value), node);
        break;
    }
    // Rotate Vector
    case 49:
    {
        const Value quaternion = tryGetValue(node->GetBox(0), Value::InitForZero(VariantType::Quaternion)).Cast(VariantType::Quaternion);
        const Value vector = tryGetValue(node->GetBox(1), Float3::Forward).Cast(VariantType::Float3);
        value = writeLocal(ValueType::Float3, String::Format(TEXT("QuatRotateVector({0}, {1})"), quaternion.Value, vector.Value), node);
        break;
    }
    // Smoothstep
    case 50:
    {
        Value v1 = tryGetValue(node->GetBox(0), 0, Value::Zero);
        Value v2 = tryGetValue(node->GetBox(1), 1, Value::Zero);
        Value v3 = tryGetValue(node->GetBox(2), 2, Value::Zero);
        value = writeFunction3(node, v1, v2, v3, TEXT("smoothstep"), v1.Type);
        break;
    }
    // Step
    case 51:
    {
        Value v1 = tryGetValue(node->GetBox(0), 0, Value::Zero);
        Value v2 = tryGetValue(node->GetBox(1), 1, Value::Zero);
        value = writeFunction2(node, v1, v2, TEXT("step"));
        break;
    }
    default:
        break;
    }
}

void ShaderGenerator::ProcessGroupPacking(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
    // Pack
    case 20:
    {
        Box* bX = node->GetBox(1);
        Box* bY = node->GetBox(2);
        Value vX = tryGetValue(bX, node->Values[0]);
        Value vY = tryGetValue(bY, node->Values[1]);
        value = Value::Float2(vX, vY);
        break;
    }
    case 21:
    {
        Box* bX = node->GetBox(1);
        Box* bY = node->GetBox(2);
        Box* bZ = node->GetBox(3);
        Value vX = tryGetValue(bX, node->Values[0]);
        Value vY = tryGetValue(bY, node->Values[1]);
        Value vZ = tryGetValue(bZ, node->Values[2]);
        value = Value::Float3(vX, vY, vZ);
        break;
    }
    case 22:
    {
        Box* bX = node->GetBox(1);
        Box* bY = node->GetBox(2);
        Box* bZ = node->GetBox(3);
        Box* bW = node->GetBox(4);
        Value vX = tryGetValue(bX, node->Values[0]);
        Value vY = tryGetValue(bY, node->Values[1]);
        Value vZ = tryGetValue(bZ, node->Values[2]);
        Value vW = tryGetValue(bW, node->Values[3]);
        value = Value::Float4(vX, vY, vZ, vW);
        break;
    }
    case 23:
    case 24:
    case 25:
    {
        // Not supported
        value = Value::Zero;
        break;
    }
    // Unpack
    case 30:
    {
        Box* b = node->GetBox(0);
        Value v = tryGetValue(b, Float2::Zero).AsFloat2();

        int32 subIndex = box->ID - 1;
        ASSERT(subIndex >= 0 && subIndex < 2);

        value = Value(ValueType::Float, v.Value + _subs[subIndex]);
        break;
    }
    case 31:
    {
        Box* b = node->GetBox(0);
        Value v = tryGetValue(b, Float3::Zero).AsFloat3();

        int32 subIndex = box->ID - 1;
        ASSERT(subIndex >= 0 && subIndex < 3);

        value = Value(ValueType::Float, v.Value + _subs[subIndex]);
        break;
    }
    case 32:
    {
        Box* b = node->GetBox(0);
        Value v = tryGetValue(b, Float4::Zero).AsFloat4();

        int32 subIndex = box->ID - 1;
        ASSERT(subIndex >= 0 && subIndex < 4);

        value = Value(ValueType::Float, v.Value + _subs[subIndex]);
        break;
    }
    case 33:
    case 34:
    case 35:
    {
        // Not supported
        value = Value::Zero;
        break;
    }
    // Mask X, Y, Z, W
    case 40:
    case 41:
    case 42:
    case 43:
    {
        Value v = tryGetValue(node->GetBox(0), Float4::Zero).AsFloat4();
        value = Value(ValueType::Float, v.Value + _subs[node->TypeID - 40]);
        break;
    }
    // Mask XY, YZ, XZ,...
    case 44:
    {
        value = tryGetValue(node->GetBox(0), Float2::Zero).AsFloat2();
        break;
    }
    case 45:
    {
        Value v = tryGetValue(node->GetBox(0), Float4::Zero).AsFloat4();
        value = Value(ValueType::Float2, v.Value + TEXT(".xz"));
        break;
    }
    case 46:
    {
        Value v = tryGetValue(node->GetBox(0), Float4::Zero).AsFloat4();
        value = Value(ValueType::Float2, v.Value + TEXT(".yz"));
        break;
    }
    case 47:
    {
        Value v = tryGetValue(node->GetBox(0), Float4::Zero).AsFloat4();
        value = Value(ValueType::Float2, v.Value + TEXT(".zw"));
        break;
    }
    // Mask XYZ
    case 70:
    {
        value = tryGetValue(node->GetBox(0), Float4::Zero).AsFloat3();
        break;
    }
    // Append
    case 100:
    {
        auto in0 = node->GetBox(0);
        auto in1 = node->GetBox(1);
        if (!in0->HasConnection() || !in1->HasConnection())
        {
            value = Value::Zero;
            break;
        }

        auto value0 = eatBox(in0->GetParent<Node>(), in0->FirstConnection());
        auto value1 = eatBox(in1->GetParent<Node>(), in1->FirstConnection());

        auto count0 = GraphUtilities::CountComponents(value0.Type);
        auto count1 = GraphUtilities::CountComponents(value1.Type);

        auto count = count0 + count1;
        switch (count)
        {
        case 2:
            value = writeLocal(ValueType::Float2, String::Format(TEXT("float2({0}, {1})"), value0.Value, value1.Value), node);
            break;
        case 3:
            value = writeLocal(ValueType::Float3, String::Format(TEXT("float3({0}, {1})"), value0.Value, value1.Value), node);
            break;
        case 4:
            value = writeLocal(ValueType::Float4, String::Format(TEXT("float4({0}, {1})"), value0.Value, value1.Value), node);
            break;
        default:
            value = Value::Zero;
            break;
        }
        break;
    }
    default:
        break;
    }
}

void ShaderGenerator::ProcessGroupTools(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
    // Desaturation
    case 2:
    {
        Value input = tryGetValue(node->GetBox(0), Value::Zero).AsFloat3();
        Value scale = tryGetValue(node->GetBox(1), Value::Zero).AsFloat();
        Value luminanceFactors = Value(node->Values[0]);
        auto dot = writeFunction2(node, input, luminanceFactors, TEXT("dot"), ValueType::Float);
        value = writeFunction3(node, input, dot, scale, TEXT("lerp"), ValueType::Float3);
        break;
    }
    // Color Gradient
    case 10:
    {
        Value time, prevTime, curTime;
        Value prevColor, curColor;
        const int32 count = (int32)node->Values[0];
        switch (count)
        {
        case 0:
            // No sample
            value = Value::Zero;
            break;
        case 1:
            // Single sample
            value = Value(node->Values[2]);
            break;
        case 2:
            // Linear blend between 2 samples
            time = tryGetValue(node->GetBox(0), Value::Zero).AsFloat();
            prevTime = Value(node->Values[1]);
            prevColor = Value(node->Values[2]);
            curTime = Value(node->Values[3]);
            curColor = Value(node->Values[4]);
            value = writeLocal(ValueType::Float4, String::Format(
                                   TEXT("lerp({0}, {1}, saturate(({2} - {3}) / ({4} - {3})))"),
                                   prevColor.Value,
                                   curColor.Value,
                                   time.Value,
                                   prevTime.Value,
                                   curTime.Value,
                                   prevTime.Value),
                               node);
            break;
        default:
            time = tryGetValue(node->GetBox(0), Value::Zero).AsFloat();
            prevTime = Value(node->Values[1]);
            prevColor = Value(node->Values[2]);
            value = writeLocal(ValueType::Float4, node);
            for (int32 i = 1; i < count; i++)
            {
                curTime = Value(node->Values[i * 2 + 1]);
                curColor = Value(node->Values[i * 2 + 2]);

                _writer.Write(
                    TEXT(
                        "	if ({1} <= {3})\n"
                        "	{{\n"
                        "		{0} = lerp({4}, {5}, saturate(({1} - {2}) / ({3} - {2})));\n"
                        "	}}\n"
                        "	else\n"
                    ), value.Value, time.Value, prevTime.Value, curTime.Value, prevColor.Value, curColor.Value);

                prevTime = curTime;
                prevColor = curColor;
            }
            _writer.Write(
                TEXT(
                    "	{{\n"
                    "		{0} = {1};\n"
                    "	}}\n"
                ), value.Value, prevColor.Value);
            break;
        }
        break;
    }
    // Curve
#define SAMPLE_CURVE(id, curves, type, graphType) \
		case id: \
		{ \
			const Value time = tryGetValue(node->GetBox(0), Value::Zero).type(); \
			value = writeLocal(ValueType::graphType, node); \
			ShaderGraphUtilities::SampleCurve(_writer, _graphStack.Peek()->curves[node->Data.Curve.CurveIndex], time.Value, value.Value); \
			break; \
		}
    SAMPLE_CURVE(12, FloatCurves, AsFloat, Float)
    SAMPLE_CURVE(13, Float2Curves, AsFloat2, Float2)
    SAMPLE_CURVE(14, Float3Curves, AsFloat3, Float3)
    SAMPLE_CURVE(15, Float4Curves, AsFloat4, Float4)
#undef SETUP_CURVE
    // Get Gameplay Global
    case 16:
    {
        // Get the variable type
        auto asset = Assets.LoadAsync<GameplayGlobals>((Guid)node->Values[0]);
        if (!asset || asset->WaitForLoaded())
        {
            OnError(node, box, TEXT("Failed to load Gameplay Global asset."));
            value = Value::Zero;
            break;
        }
        const StringView& name = (StringView)node->Values[1];
        GameplayGlobals::Variable variable;
        if (!asset->Variables.TryGet(name, variable))
        {
            OnError(node, box, TEXT("Missing Gameplay Global variable."));
            value = Value::Zero;
            break;
        }

        // Find or create a parameter for the gameplay global binding
        SerializedMaterialParam* param = nullptr;
        for (auto& p : _parameters)
        {
            if (!p.IsPublic && p.Type == MaterialParameterType::GameplayGlobal && p.Name == name)
            {
                param = &p;
                break;
            }
        }
        if (!param)
        {
            auto& p = _parameters.AddOne();
            p.Type = MaterialParameterType::GameplayGlobal;
            p.IsPublic = false;
            p.Override = true;
            p.Name = name;
            p.ShaderName = getParamName(_parameters.Count());
            p.AsGuid = asset->GetID();
            p.ID = Guid::New();
            param = &p;
        }

        // Get param value
        value.Type = variable.DefaultValue.Type.Type;
        value.Value = param->ShaderName;
        break;
    }
    // Platform Switch
    case 17:
    {
        bool usesAnyPlatformSpecificInput = false;
        Array<Value, FixedAllocation<32>> values;
        values.Resize(node->Boxes.Count());
        values[1] = Value::Zero;
        ValueType type = ValueType::Float;
        for (int32 i = 1; i < node->Boxes.Count(); i++)
        {
            if (node->Boxes[i].HasConnection())
            {
                values[i] = tryGetValue(&node->Boxes[i], values[1]);
                if (node->Boxes[i].HasConnection())
                    type = values[i].Type;
                usesAnyPlatformSpecificInput |= i > 1;
            }
        }
        if (!usesAnyPlatformSpecificInput)
        {
            // Use default if no platform switches used
            value = values[1];
            break;
        }

        // Create local variable as output (initialized to default value)
        value = writeLocal(type, values[1], node);

        // Create series of compile-time switches based on PLATFORM_XXX defines
#define PLATFORM_CASE(boxIndex, defineName) \
    if (node->GetBox(boxIndex)->HasConnection()) \
        _writer.Write(TEXT("#ifdef " defineName "\n\t{0} = {1};\n#endif\n"), value.Value, values[boxIndex].Value)
        PLATFORM_CASE(2, "PLATFORM_WINDOWS");
        PLATFORM_CASE(3, "PLATFORM_XBOX_ONE");
        PLATFORM_CASE(4, "PLATFORM_UWP");
        PLATFORM_CASE(5, "PLATFORM_LINUX");
        PLATFORM_CASE(6, "PLATFORM_PS4");
        PLATFORM_CASE(7, "PLATFORM_XBOX_SCARLETT");
        PLATFORM_CASE(8, "PLATFORM_ANDROID");
        PLATFORM_CASE(9, "PLATFORM_SWITCH");
        PLATFORM_CASE(10, "PLATFORM_PS5");
        PLATFORM_CASE(11, "PLATFORM_MAC");
        PLATFORM_CASE(12, "PLATFORM_IOS");
#undef PLATFORM_CASE
        break;
    }
    // Reroute
    case 29:
        value = tryGetValue(node->GetBox(0), Value::Zero);
        break;
    // Noises
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    {
        _includes.Add(TEXT("./Flax/Noise.hlsl"));
        const Char* format;
        ValueType pointType = VariantType::Float2;
        ValueType resultType = VariantType::Float;
        switch (node->TypeID)
        {
        case 30:
            format = TEXT("PerlinNoise({0})");
            break;
        case 31:
            format = TEXT("SimplexNoise({0})");
            break;
        case 32:
            format = TEXT("WorleyNoise({0})");
            resultType = VariantType::Float2;
            break;
        case 33:
            format = TEXT("VoronoiNoise({0})");
            resultType = VariantType::Float3;
            break;
        case 34:
            format = TEXT("CustomNoise({0})");
            pointType = VariantType::Float3;
            break;
        }
        value = writeLocal(resultType, String::Format(format, tryGetValue(node->GetBox(0), Value::Zero).Cast(pointType).Value), node);
        break;
    }
    default:
        break;
    }
}

void ShaderGenerator::ProcessGroupBoolean(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
    // NOT
    case 1:
    {
        // Get A value
        const Value a = tryGetValue(node->GetBox(0), Value::False).AsBool();

        // Perform operation
        value = writeLocal(ValueType::Bool, String::Format(TEXT("!{0}"), a.Value), node);
        break;
    }
    // AND, OR, XOR, NOR, NAND
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    {
        // Get A and B values
        const Value a = tryGetValue(node->GetBox(0), node->Values[0]).AsBool();
        const Value b = tryGetValue(node->GetBox(1), node->Values[1]).AsBool();

        // Perform operation
        const Char* op;
        switch (node->TypeID)
        {
        // AND
        case 2:
            op = TEXT("{0} && {1}");
            break;
        // OR
        case 3:
            op = TEXT("{0} || {1}");
            break;
        // XOR
        case 4:
            op = TEXT("!{0} != !{1}");
            break;
        // NOR
        case 5:
            op = TEXT("!({0} || {1})");
            break;
        // NAND
        case 6:
            op = TEXT("!({0} && {1})");
            break;
        default:
            CRASH;
            return;
        }
        value = writeLocal(ValueType::Bool, String::Format(op, a.Value, b.Value), node);
        break;
    }
    default:
        break;
    }
}

void ShaderGenerator::ProcessGroupBitwise(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
    // NOT
    case 1:
    {
        // Get A value
        const Value a = tryGetValue(node->GetBox(0), Value(ValueType::Int, TEXT("0"))).AsInt();

        // Perform operation
        value = writeLocal(ValueType::Int, String::Format(TEXT("!{0}"), a.Value), node);
        break;
    }
    // AND, OR, XOR
    case 2:
    case 3:
    case 4:
    {
        // Get A and B values
        const Value a = tryGetValue(node->GetBox(0), node->Values[0]).AsInt();
        const Value b = tryGetValue(node->GetBox(1), node->Values[1]).AsInt();

        // Perform operation
        const Char* op;
        switch (node->TypeID)
        {
        // AND
        case 2:
            op = TEXT("{0} & {1}");
            break;
        // OR
        case 3:
            op = TEXT("{0} | {1}");
            break;
        // XOR
        case 4:
            op = TEXT("{0} ^ {1}");
            break;
        default:
            CRASH;
            return;
        }
        value = writeLocal(ValueType::Int, String::Format(op, a.Value, b.Value), node);
        break;
    }
    default:
        break;
    }
}

void ShaderGenerator::ProcessGroupComparisons(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
    // ==, !=, >, <=, >=
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    {
        // Get A and B values
        const Value a = tryGetValue(node->GetBox(0), node->Values[0]);
        const Value b = tryGetValue(node->GetBox(1), node->Values[1]).Cast(a.Type);

        // Perform operation
        const Char* op;
        switch (node->TypeID)
        {
        case 1:
            op = TEXT("{0} == {1}");
            break;
        case 2:
            op = TEXT("{0} != {1}");
            break;
        case 3:
            op = TEXT("{0} > {1}");
            break;
        case 4:
            op = TEXT("{0} < {1}");
            break;
        case 5:
            op = TEXT("{0} <= {1}");
            break;
        case 6:
            op = TEXT("{0} >= {1}");
            break;
        default:
            CRASH;
            return;
        }
        value = writeLocal(ValueType::Bool, String::Format(op, a.Value, b.Value), node);
        break;
    }
    // Switch On Bool
    case 7:
    {
        const Value condition = tryGetValue(node->GetBox(0), Value::False).AsBool();
        const Value onTrue = tryGetValue(node->GetBox(2), 1, Value::Zero);
        const Value onFalse = tryGetValue(node->GetBox(1), 0, Value::Zero).Cast(onTrue.Type);
        value = writeLocal(onTrue.Type, String::Format(TEXT("{0} ? {1} : {2}"), condition.Value, onTrue.Value, onFalse.Value), node);
        break;
    }
    }
}

ShaderGenerator::Value ShaderGenerator::eatBox(Node* caller, Box* box)
{
    // Check if graph is looped or is too deep
    if (_callStack.Count() >= SHADER_GRAPH_MAX_CALL_STACK)
    {
        OnError(caller, box, TEXT("Graph is looped or too deep!"));
        return Value::Zero;
    }

    // Check if box is invalid (better way to prevent crashes)
    if (box == nullptr)
    {
        return Value::Zero;
    }

    // Check if box has cached value
    if (box->Cache.IsValid())
    {
        return box->Cache;
    }

    // Add to the calling stack
    _callStack.Push(caller);

    // Call per group custom processing event
    Value value;
    const auto parentNode = box->GetParent<Node>();
    _perGroupProcessCall[parentNode->GroupID](box, parentNode, value);

    // Ensure value is valid
    if (value.IsInvalid())
    {
        OnError(parentNode, box, TEXT("Unknown box to resolve."));
    }

    // Cache value
    box->Cache = value;

    // Remove from the calling stack
    _callStack.Pop();

    return value;
}

ShaderGenerator::Value ShaderGenerator::tryGetValue(Box* box, int32 defaultValueBoxIndex, const Value& defaultValue)
{
    if (!box)
        return defaultValue;
    const auto parentNode = box->GetParent<Node>();
    if (box->HasConnection())
        return eatBox(parentNode, box->FirstConnection());
    if (parentNode->Values.Count() > defaultValueBoxIndex)
        return Value(parentNode->Values[defaultValueBoxIndex]);
    return defaultValue;
}

ShaderGenerator::Value ShaderGenerator::tryGetValue(Box* box, const Value& defaultValue)
{
    return box && box->HasConnection() ? eatBox(box->GetParent<Node>(), box->FirstConnection()) : defaultValue;
}

ShaderGenerator::Value ShaderGenerator::tryGetValue(Box* box, const Variant& defaultValue)
{
    return box && box->HasConnection() ? eatBox(box->GetParent<Node>(), box->FirstConnection()) : Value(defaultValue);
}

ShaderGenerator::Value ShaderGenerator::writeLocal(ValueType type, Node* caller)
{
    return writeLocal(type, caller, getLocalName(_localIndex++));
}

ShaderGenerator::Value ShaderGenerator::writeLocal(ValueType type, Node* caller, const String& name)
{
    const Char* typeName;
    switch (type)
    {
    case ValueType::Bool:
        typeName = TEXT("bool");
        break;
    case ValueType::Int:
        typeName = TEXT("int");
        break;
    case ValueType::Uint:
        typeName = TEXT("uint");
        break;
    case ValueType::Float:
        typeName = TEXT("float");
        break;
    case ValueType::Float2:
        typeName = TEXT("float2");
        break;
    case ValueType::Float3:
        typeName = TEXT("float3");
        break;
    case ValueType::Float4:
    case ValueType::Color:
        typeName = TEXT("float4");
        break;
    case ValueType::Object:
        typeName = TEXT("Texture2D");
        break;
    case ValueType::Void:
        typeName = TEXT("Material");
        break;
    default:
        OnError(caller, nullptr, *String::Format(TEXT("Unsupported value type: {0}"), VariantType(type)));
        return ShaderGraphValue::Zero;
    }
    _writer.Write(TEXT("\t{0} {1};\n"), typeName, name);
    return ShaderGraphValue(type, name);
}

ShaderGenerator::Value ShaderGenerator::writeLocal(ValueType type, const Value& value, Node* caller)
{
    return writeLocal(Value::Cast(value, type), caller);
}

ShaderGenerator::Value ShaderGenerator::writeLocal(const Value& value, Node* caller)
{
    return writeLocal(value.Type, value.Value, caller);
}

ShaderGenerator::Value ShaderGenerator::writeLocal(ValueType type, const String& value, Node* caller)
{
    return writeLocal(type, value, caller, getLocalName(_localIndex++));
}

ShaderGenerator::Value ShaderGenerator::writeLocal(ValueType type, const String& value, Node* caller, const String& name)
{
    const Char* typeName;
    switch (type)
    {
    case ValueType::Bool:
        typeName = TEXT("bool");
        break;
    case ValueType::Int:
        typeName = TEXT("int");
        break;
    case ValueType::Uint:
        typeName = TEXT("uint");
        break;
    case ValueType::Float:
        typeName = TEXT("float");
        break;
    case ValueType::Float2:
        typeName = TEXT("float2");
        break;
    case ValueType::Float3:
        typeName = TEXT("float3");
        break;
    case ValueType::Float4:
    case ValueType::Color:
        typeName = TEXT("float4");
        break;
    case ValueType::Object:
        typeName = TEXT("Texture2D");
        break;
    case ValueType::Void:
        typeName = TEXT("Material");
        break;
    default:
        OnError(caller, nullptr, String::Format(TEXT("Unsupported value type: {0}"), VariantType(type)));
        return ShaderGraphValue::Zero;
    }
    _writer.Write(TEXT("\t{0} {1} = {2};\n"), typeName, name, value);
    return ShaderGraphValue(type, name);
}

ShaderGenerator::Value ShaderGenerator::writeOperation2(Node* caller, const Value& valueA, const Value& valueB, Char op1)
{
    const Char op1Str[2] = { op1, 0 };
    const String value = String::Format(TEXT("{0} {1} {2}"), valueA.Value, op1Str, Value::Cast(valueB, valueA.Type).Value);
    return writeLocal(valueA.Type, value, caller);
}

ShaderGenerator::Value ShaderGenerator::writeFunction1(Node* caller, const Value& valueA, const String& function)
{
    const String value = String::Format(TEXT("{0}({1})"), function, valueA.Value);
    return writeLocal(valueA.Type, value, caller);
}

ShaderGenerator::Value ShaderGenerator::writeFunction2(Node* caller, const Value& valueA, const Value& valueB, const String& function)
{
    const String value = String::Format(TEXT("{0}({1}, {2})"), function, valueA.Value, Value::Cast(valueB, valueA.Type).Value);
    return writeLocal(valueA.Type, value, caller);
}

ShaderGenerator::Value ShaderGenerator::writeFunction2(Node* caller, const Value& valueA, const Value& valueB, const String& function, ValueType resultType)
{
    const String value = String::Format(TEXT("{0}({1}, {2})"), function, valueA.Value, Value::Cast(valueB, valueA.Type).Value);
    return writeLocal(resultType, value, caller);
}

ShaderGenerator::Value ShaderGenerator::writeFunction3(Node* caller, const Value& valueA, const Value& valueB, const Value& valueC, const String& function, ValueType resultType)
{
    const String value = String::Format(TEXT("{0}({1}, {2}, {3})"), function, valueA.Value, Value::Cast(valueB, valueA.Type).Value, Value::Cast(valueC, valueA.Type).Value);
    return writeLocal(resultType, value, caller);
}

SerializedMaterialParam* ShaderGenerator::findParam(const String& shaderName)
{
    for (int32 i = 0; i < _parameters.Count(); i++)
    {
        SerializedMaterialParam& param = _parameters[i];
        if (param.ShaderName == shaderName)
            return &param;
    }
    return nullptr;
}

SerializedMaterialParam* ShaderGenerator::findParam(const Guid& id)
{
    SerializedMaterialParam* result = nullptr;
    for (auto& param : _parameters)
    {
        if (param.ID == id)
        {
            result = &param;
            break;
        }
    }
    return result;
}

SerializedMaterialParam ShaderGenerator::findOrAddTexture(const Guid& id)
{
    // Find
    for (int32 i = 0; i < _parameters.Count(); i++)
    {
        SerializedMaterialParam& param = _parameters[i];
        if (!param.IsPublic && param.Type == MaterialParameterType::Texture && param.AsGuid == id)
            return param;
    }

    // Create
    SerializedMaterialParam& param = _parameters.AddOne();
    param.Type = MaterialParameterType::Texture;
    param.IsPublic = false;
    param.Override = true;
    param.Name = TEXT("Texture");
    param.ShaderName = getParamName(_parameters.Count());
    param.AsGuid = id;
    param.ID = Guid(_parameters.Count(), 0, 0, 1); // Assign temporary id
    return param;
}

SerializedMaterialParam ShaderGenerator::findOrAddNormalMap(const Guid& id)
{
    // Find
    for (int32 i = 0; i < _parameters.Count(); i++)
    {
        SerializedMaterialParam& param = _parameters[i];
        if (!param.IsPublic && param.Type == MaterialParameterType::NormalMap && param.AsGuid == id)
            return param;
    }

    // Create
    SerializedMaterialParam& param = _parameters.AddOne();
    param.Type = MaterialParameterType::NormalMap;
    param.IsPublic = false;
    param.Override = true;
    param.Name = TEXT("Normal Map");
    param.ShaderName = getParamName(_parameters.Count());
    param.AsGuid = id;
    param.ID = Guid(_parameters.Count(), 0, 0, 2); // Assign temporary id
    return param;
}

SerializedMaterialParam ShaderGenerator::findOrAddCubeTexture(const Guid& id)
{
    // Find
    for (int32 i = 0; i < _parameters.Count(); i++)
    {
        SerializedMaterialParam& param = _parameters[i];
        if (!param.IsPublic && param.Type == MaterialParameterType::CubeTexture && param.AsGuid == id)
            return param;
    }

    // Create
    SerializedMaterialParam& param = _parameters.AddOne();
    param.Type = MaterialParameterType::CubeTexture;
    param.IsPublic = false;
    param.Override = true;
    param.Name = TEXT("Cube Texture");
    param.ShaderName = getParamName(_parameters.Count());
    param.AsGuid = id;
    param.ID = Guid(_parameters.Count(), 0, 0, 3); // Assign temporary id
    return param;
}

SerializedMaterialParam ShaderGenerator::findOrAddSceneTexture(MaterialSceneTextures type)
{
    int32 asInt = (int32)type;

    // Find
    for (int32 i = 0; i < _parameters.Count(); i++)
    {
        SerializedMaterialParam& param = _parameters[i];
        if (!param.IsPublic && param.Type == MaterialParameterType::SceneTexture && param.AsInteger == asInt)
            return param;
    }

    // Create
    SerializedMaterialParam& param = _parameters.AddOne();
    param.Type = MaterialParameterType::SceneTexture;
    param.IsPublic = false;
    param.Override = true;
    param.Name = TEXT("Scene Texture");
    param.ShaderName = getParamName(_parameters.Count());
    param.AsInteger = asInt;
    param.ID = Guid(_parameters.Count(), 0, 0, 3); // Assign temporary id
    return param;
}

SerializedMaterialParam& ShaderGenerator::findOrAddTextureGroupSampler(int32 index)
{
    // Find
    for (int32 i = 0; i < _parameters.Count(); i++)
    {
        SerializedMaterialParam& param = _parameters[i];
        if (!param.IsPublic && param.Type == MaterialParameterType::TextureGroupSampler && param.AsInteger == index)
            return param;
    }

    // Create
    SerializedMaterialParam& param = _parameters.AddOne();
    param.Type = MaterialParameterType::TextureGroupSampler;
    param.IsPublic = false;
    param.Override = true;
    param.Name = TEXT("Texture Group Sampler");
    param.ShaderName = getParamName(_parameters.Count());
    param.AsInteger = index;
    param.ID = Guid(_parameters.Count(), 0, 0, 3); // Assign temporary id
    return param;
}

SerializedMaterialParam& ShaderGenerator::findOrAddGlobalSDF()
{
    // Find
    for (int32 i = 0; i < _parameters.Count(); i++)
    {
        SerializedMaterialParam& param = _parameters[i];
        if (!param.IsPublic && param.Type == MaterialParameterType::GlobalSDF)
            return param;
    }

    // Create
    SerializedMaterialParam& param = _parameters.AddOne();
    param.Type = MaterialParameterType::GlobalSDF;
    param.IsPublic = false;
    param.Override = true;
    param.Name = TEXT("Global SDF");
    param.ShaderName = getParamName(_parameters.Count());
    param.ID = Guid(_parameters.Count(), 0, 0, 3); // Assign temporary id
    return param;
}

String ShaderGenerator::getLocalName(int32 index)
{
    return TEXT("local") + StringUtils::ToString(index);
}

String ShaderGenerator::getParamName(int32 index)
{
    return TEXT("In") + StringUtils::ToString(index);
}

#endif
