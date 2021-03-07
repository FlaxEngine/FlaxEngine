// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "VisjectGraph.h"
#include "GraphUtilities.h"
#include "Engine/Core/Random.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Engine/GameplayGlobals.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Level/Actor.h"

#define RAND Random::Rand()

VisjectExecutor::VisjectExecutor()
{
    // Register per group type processing events
    // Note: index must match group id
#if BUILD_DEBUG
    Platform::MemoryClear(&_perGroupProcessCall, sizeof(_perGroupProcessCall));
#endif
    _perGroupProcessCall[2] = &VisjectExecutor::ProcessGroupConstants;
    _perGroupProcessCall[3] = &VisjectExecutor::ProcessGroupMath;
    _perGroupProcessCall[4] = &VisjectExecutor::ProcessGroupPacking;
    _perGroupProcessCall[7] = &VisjectExecutor::ProcessGroupTools;
    _perGroupProcessCall[10] = &VisjectExecutor::ProcessGroupBoolean;
    _perGroupProcessCall[11] = &VisjectExecutor::ProcessGroupBitwise;
    _perGroupProcessCall[12] = &VisjectExecutor::ProcessGroupComparisons;
    _perGroupProcessCall[14] = &VisjectExecutor::ProcessGroupParticles;
}

VisjectExecutor::~VisjectExecutor()
{
}

void VisjectExecutor::OnError(Node* node, Box* box, const StringView& message)
{
    Error(node, box, message);
}

void VisjectExecutor::ProcessGroupConstants(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
        // Constant value
    case 1:
    case 2:
    case 3:
    case 12:
        value = node->Values[0];
        break;
    case 4:
    {
        const Value& cv = node->Values[0];
        if (box->ID == 0)
            value = cv;
        else if (box->ID == 1)
            value = cv.AsVector2().X;
        else if (box->ID == 2)
            value = cv.AsVector2().Y;
        break;
    }
    case 5:
    {
        const Value& cv = node->Values[0];
        if (box->ID == 0)
            value = cv;
        else if (box->ID == 1)
            value = cv.AsVector3().X;
        else if (box->ID == 2)
            value = cv.AsVector3().Y;
        else if (box->ID == 3)
            value = cv.AsVector3().Z;
        break;
    }
    case 6:
    case 7:
    {
        const Value& cv = node->Values[0];
        if (box->ID == 0)
            value = cv;
        else if (box->ID == 1)
            value = cv.AsVector4().X;
        else if (box->ID == 2)
            value = cv.AsVector4().Y;
        else if (box->ID == 3)
            value = cv.AsVector4().Z;
        else if (box->ID == 4)
            value = cv.AsVector4().W;
        break;
    }
    case 8:
    {
        const float pitch = (float)node->Values[0];
        const float yaw = (float)node->Values[1];
        const float roll = (float)node->Values[2];
        value = Quaternion::Euler(pitch, yaw, roll);
        break;
    }
    case 9:
    {
        value = node->Values[0];
        break;
    }
        // PI
    case 10:
        value = PI;
        break;

    default:
        break;
    }
}

void VisjectExecutor::ProcessGroupMath(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
        // Add, Subtract, Multiply, Divide, Modulo, Max, Min, Pow, Fmod, Atan2
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 21:
    case 22:
    case 23:
    case 40:
    case 41:
    {
        Value v1 = tryGetValue(node->GetBox(0), 0, Value::Zero);
        Value v2 = tryGetValue(node->GetBox(1), 1, Value::Zero).Cast(v1.Type);
        GraphUtilities::ApplySomeMathHere(node->TypeID, value, v1, v2);
        break;
    }
        // Absolute Value, Ceil, Cosine, Floor, Round, Saturate, Sine, Sqrt, Tangent, Negate, 1 - Value, Asine, Acosine, Atan, Trunc, Frac, Degrees, Radians
    case 7:
    case 8:
    case 9:
    case 10:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 27:
    case 28:
    case 33:
    case 34:
    case 35:
    case 38:
    case 39:
    case 43:
    case 44:
    {
        Value v1 = tryGetValue(node->GetBox(0), Value::Zero);
        GraphUtilities::ApplySomeMathHere(node->TypeID, value, v1);
        break;
    }
        // Length, Normalize
    case 11:
    case 12:
    {
        Value v1 = tryGetValue(node->GetBox(0), Value::Zero);
        switch (node->TypeID)
        {
        case 11:
        {
            switch (v1.Type.Type)
            {
            case VariantType::Vector2:
                value = v1.AsVector2().Length();
                break;
            case VariantType::Vector3:
                value = v1.AsVector3().Length();
                break;
            case VariantType::Vector4:
                value = Vector3(v1.AsVector4()).Length();
                break;
            default: CRASH;
                break;
            }
            break;
        }
        case 12:
            switch (v1.Type.Type)
            {
            case VariantType::Int:
                value = Math::Saturate(v1.AsInt);
                break;
            case VariantType::Uint:
                value = Math::Saturate(v1.AsUint);
                break;
            case VariantType::Float:
                value = Math::Saturate(v1.AsFloat);
                break;
            case VariantType::Vector2:
                value = Vector2::Normalize(v1.AsVector2());
                break;
            case VariantType::Vector3:
                value = Vector3::Normalize(v1.AsVector3());
                break;
            case VariantType::Vector4:
                value = Vector4(Vector3::Normalize(Vector3(v1.AsVector4())), 0.0f);
                break;
            default: CRASH;
                break;
            }
            break;
        }
        break;
    }
        // Cross, Distance, Dot
    case 18:
    case 19:
    case 20:
    {
        Value v1 = tryGetValue(node->GetBox(0), Value::Zero);
        Value v2 = tryGetValue(node->GetBox(1), Value::Zero).Cast(v1.Type);
        switch (node->TypeID)
        {
        case 18:
            switch (v1.Type.Type)
            {
            case VariantType::Vector3:
                value = Vector3::Cross(v1.AsVector3(), v2.AsVector3());
                break;
            default: CRASH;
                break;
            }
            break;
        case 19:
            switch (v1.Type.Type)
            {
            case VariantType::Vector2:
                value = Vector2::Distance(v1.AsVector2(), v2.AsVector2());
                break;
            case VariantType::Vector3:
                value = Vector3::Distance(v1.AsVector3(), v2.AsVector3());
                break;
            case VariantType::Vector4:
            case VariantType::Color:
                value = Vector3::Distance((Vector3)v1, (Vector3)v2);
                break;
            default: CRASH;
                break;
            }
            break;
        case 20:
            switch (v1.Type.Type)
            {
            case VariantType::Vector2:
                value = Vector2::Dot(v1.AsVector2(), v2.AsVector2());
                break;
            case VariantType::Vector3:
                value = Vector3::Dot(v1.AsVector3(), v2.AsVector3());
                break;
            case VariantType::Vector4:
            case VariantType::Color:
                value = Vector3::Dot((Vector3)v1, (Vector3)v2);
                break;
            default: CRASH;
                break;
            }
            break;
        }
        break;
    }
        // Clamp
    case 24:
    {
        Value v1 = tryGetValue(node->GetBox(0), Value::Zero);
        Value v2 = tryGetValue(node->GetBox(1), 0, Value::Zero).Cast(v1.Type);
        Value v3 = tryGetValue(node->GetBox(2), 1, Value::One).Cast(v1.Type);
        GraphUtilities::ApplySomeMathHere(value, v1, v2, v3, [](float a, float b, float c)
        {
            return Math::Clamp(a, b, c);
        });
        break;
    }
        // Lerp
    case 25:
    {
        Value a = tryGetValue(node->GetBox(0), 0, Value::Zero);
        Value b = tryGetValue(node->GetBox(1), 1, Value::One).Cast(a.Type);
        Value alpha = tryGetValue(node->GetBox(2), 2, Value::Zero).Cast(ValueType(ValueType::Float));
        value = Value::Lerp(a, b, alpha.AsFloat);
        break;
    }
        // Reflect
    case 26:
    {
        Value v1 = tryGetValue(node->GetBox(0), Value::Zero);
        Value v2 = tryGetValue(node->GetBox(1), Value::Zero).Cast(v1.Type);
        switch (v1.Type.Type)
        {
        case VariantType::Vector2:
            value = v1.AsVector2() - 2 * v2.AsVector2() * Vector2::Dot(v1.AsVector2(), v2.AsVector2());
            break;
        case VariantType::Vector3:
            value = v1.AsVector3() - 2 * v2.AsVector3() * Vector3::Dot(v1.AsVector3(), v2.AsVector3());
            break;
        case VariantType::Vector4:
            value = Vector4(v1.AsVector4() - 2 * v2.AsVector4() * Vector3::Dot((Vector3)v1, (Vector3)v2));
            break;
        default: CRASH;
            break;
        }
        break;
    }
        // Mad
    case 31:
    {
        Value v1 = tryGetValue(node->GetBox(0), Value::Zero);
        Value v2 = tryGetValue(node->GetBox(1), 0, Value::One).Cast(v1.Type);
        Value v3 = tryGetValue(node->GetBox(2), 1, Value::Zero).Cast(v1.Type);
        GraphUtilities::ApplySomeMathHere(value, v1, v2, v3, [](float a, float b, float c)
        {
            return (a * b) + c;
        });
        break;
    }
        // Extract Largest Component
    case 32:
    {
        const auto v1 = (Vector3)tryGetValue(node->GetBox(0), Value::Zero);
        value = Math::ExtractLargestComponent(v1);
        break;
    }
        // Bias and Scale
    case 36:
    {
        ASSERT(node->Values.Count() == 2 && node->Values[0].Type == VariantType::Float && node->Values[1].Type == VariantType::Float);
        const auto bias = node->Values[0].AsFloat;
        const auto scale = node->Values[1].AsFloat;
        const auto input = (Vector3)tryGetValue(node->GetBox(0), Value::Zero);
        value = (input + bias) * scale;
        break;
    }
        // Rotate About Axis
    case 37:
    {
        const auto normalizedRotationAxis = (Vector3)tryGetValue(node->GetBox(0), Value::Zero);
        const auto rotationAngle = (float)tryGetValue(node->GetBox(1), Value::Zero);
        const auto pivotPoint = (Vector3)tryGetValue(node->GetBox(2), Value::Zero);
        const auto position = (Vector3)tryGetValue(node->GetBox(3), Value::Zero);
        value = Math::RotateAboutAxis(normalizedRotationAxis, rotationAngle, pivotPoint, position);
        break;
    }
        // Near Equal
    case 42:
    {
        const Value a = tryGetValue(node->GetBox(0), node->Values[0]);
        const Value b = tryGetValue(node->GetBox(1), node->Values[1]).Cast(a.Type);
        const float epsilon = (float)tryGetValue(node->GetBox(2), node->Values[2]);
        value = Value::NearEqual(a, b, epsilon);
        break;
    }
        // Enum Value
    case 45:
        value = (uint64)tryGetValue(node->GetBox(0), Value::Zero);
        break;
        // Enum AND
    case 46:
        value = tryGetValue(node->GetBox(0), Value::Zero);
        if (value.Type.Type == VariantType::Enum)
            value.AsUint64 = value.AsUint64 & (uint64)tryGetValue(node->GetBox(1), Value::Zero);
        break;
        // Enum OR
    case 47:
        value = tryGetValue(node->GetBox(0), Value::Zero);
        if (value.Type.Type == VariantType::Enum)
            value.AsUint64 = value.AsUint64 | (uint64)tryGetValue(node->GetBox(1), Value::Zero);
        break;
        // Remap
    case 48:
    {
        const float inVal  = tryGetValue(node->GetBox(0), node->Values[0]).AsFloat;
        const Vector2 rangeA = tryGetValue(node->GetBox(1), node->Values[1]).AsVector2();
        const Vector2 rangeB = tryGetValue(node->GetBox(2), node->Values[2]).AsVector2();
        const bool clamp = tryGetValue(node->GetBox(3), node->Values[3]).AsBool;

        auto mapFunc = Math::Remap(inVal, rangeA.X, rangeA.Y, rangeB.X, rangeB.Y);
        
        value = clamp ? Math::Clamp(mapFunc, rangeB.X, rangeB.Y) : mapFunc;
        break;
    }
    default:
        break;
    }
}

void VisjectExecutor::ProcessGroupPacking(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
        // Pack
    case 20:
    {
        float vX = (float)tryGetValue(node->GetBox(1), node->Values[0]);
        float vY = (float)tryGetValue(node->GetBox(2), node->Values[1]);
        value = Vector2(vX, vY);
        break;
    }
    case 21:
    {
        float vX = (float)tryGetValue(node->GetBox(1), node->Values[0]);
        float vY = (float)tryGetValue(node->GetBox(2), node->Values[1]);
        float vZ = (float)tryGetValue(node->GetBox(3), node->Values[2]);
        value = Vector3(vX, vY, vZ);
        break;
    }
    case 22:
    {
        float vX = (float)tryGetValue(node->GetBox(1), node->Values[0]);
        float vY = (float)tryGetValue(node->GetBox(2), node->Values[1]);
        float vZ = (float)tryGetValue(node->GetBox(3), node->Values[2]);
        float vW = (float)tryGetValue(node->GetBox(4), node->Values[3]);
        value = Vector4(vX, vY, vZ, vW);
        break;
    }
    case 23:
    {
        float vX = (float)tryGetValue(node->GetBox(1), node->Values[0]);
        float vY = (float)tryGetValue(node->GetBox(2), node->Values[1]);
        float vZ = (float)tryGetValue(node->GetBox(3), node->Values[2]);
        value = Quaternion::Euler(vX, vY, vZ);
        break;
    }
    case 24:
    {
        const Vector3 vX = (Vector3)tryGetValue(node->GetBox(1), Vector3::Zero);
        const Quaternion vY = (Quaternion)tryGetValue(node->GetBox(2), Quaternion::Identity);
        const Vector3 vZ = (Vector3)tryGetValue(node->GetBox(3), Vector3::One);
        value = Variant(Transform(vX, vY, vZ));
        break;
    }
    case 25:
    {
        const Vector3 vX = (Vector3)tryGetValue(node->GetBox(1), Vector3::Zero);
        const Vector3 vY = (Vector3)tryGetValue(node->GetBox(2), Vector3::Zero);
        value = Variant(BoundingBox(vX, vY));
        break;
    }
        // Unpack
    case 30:
    {
        Vector2 v = (Vector2)tryGetValue(node->GetBox(0), Vector2::Zero);
        int32 subIndex = box->ID - 1;
        ASSERT(subIndex >= 0 && subIndex < 2);
        value = v.Raw[subIndex];
        break;
    }
    case 31:
    {
        Vector3 v = (Vector3)tryGetValue(node->GetBox(0), Vector3::Zero);
        int32 subIndex = box->ID - 1;
        ASSERT(subIndex >= 0 && subIndex < 3);
        value = v.Raw[subIndex];
        break;
    }
    case 32:
    {
        Vector4 v = (Vector4)tryGetValue(node->GetBox(0), Vector4::Zero);
        int32 subIndex = box->ID - 1;
        ASSERT(subIndex >= 0 && subIndex < 4);
        value = v.Raw[subIndex];
        break;
    }
    case 33:
    {
        const Vector3 v = ((Quaternion)tryGetValue(node->GetBox(0), Quaternion::Identity)).GetEuler();
        const int32 subIndex = box->ID - 1;
        ASSERT(subIndex >= 0 && subIndex < 3);
        value = v.Raw[subIndex];
        break;
    }
    case 34:
    {
        const Transform v = (Transform)tryGetValue(node->GetBox(0), Variant::Zero);
        switch (box->ID)
        {
        case 1:
            value = v.Translation;
            break;
        case 2:
            value = v.Orientation;
            break;
        case 3:
            value = v.Scale;
            break;
        }
        break;
    }
    case 35:
    {
        const BoundingBox v = (BoundingBox)tryGetValue(node->GetBox(0), Variant::Zero);
        switch (box->ID)
        {
        case 1:
            value = v.Minimum;
            break;
        case 2:
            value = v.Maximum;
            break;
        }
        break;
    }
        // Mask X, Y, Z, W
    case 40:
    case 41:
    case 42:
    case 43:
    {
        const Vector4 v = (Vector4)tryGetValue(node->GetBox(0), Vector4::Zero);
        value = v.Raw[node->TypeID - 40];
        break;
    }
        // Mask XY, YZ, XZ,...
    case 44:
    {
        value = (Vector2)tryGetValue(node->GetBox(0), Vector2::Zero);
        break;
    }
    case 45:
    {
        const Vector4 v = (Vector4)tryGetValue(node->GetBox(0), Vector4::Zero);
        value = Vector2(v.X, v.Z);
        break;
    }
    case 46:
    {
        const Vector4 v = (Vector4)tryGetValue(node->GetBox(0), Vector4::Zero);
        value = Vector2(v.Y, v.Z);
        break;
    }
    case 47:
    {
        const Vector4 v = (Vector4)tryGetValue(node->GetBox(0), Vector4::Zero);
        value = Vector2(v.Z, v.W);
        break;
    }
        // Mask XYZ
    case 70:
    {
        value = (Vector3)tryGetValue(node->GetBox(0), Vector3::Zero);
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

        auto count0 = GraphUtilities::CountComponents(value0.Type.Type);
        auto count1 = GraphUtilities::CountComponents(value1.Type.Type);

        auto count = count0 + count1;
        switch (count)
        {
        case 1:
            value = count0 ? value0 : value1;
            break;
        case 2:
            value = Vector2((float)value0, (float)value1);
            break;
        case 3:
            if (count0 == 1)
                value = Vector3((float)value0, value1.AsVector2().X, value1.AsVector2().Y);
            else
                value = Vector3((Vector2)value0, (float)value1);
            break;
        case 4:
            if (count0 == 1)
                value = Vector4((float)value0, value1.AsVector3().X, value1.AsVector3().Y, value1.AsVector3().Z);
            else if (count0 == 2)
                value = Vector4(value0.AsVector2().X, value0.AsVector2().Y, value1.AsVector2().X, value1.AsVector2().Y);
            else
                value = Vector4((Vector3)value0, (float)value1);
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

void VisjectExecutor::ProcessGroupTools(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
        // Color Gradient
    case 10:
    {
        float time, prevTime, curTime;
        Color prevColor, curColor;
        const int32 count = (int32)node->Values[0];
        switch (count)
        {
        case 0:
            // No sample
            value = Value::Zero;
            break;
        case 1:
            // Single sample
            value = (Color)node->Values[2];
            break;
        case 2:
            // Linear blend between 2 samples
            time = (float)tryGetValue(node->GetBox(0), Value::Zero);
            prevTime = (float)node->Values[1];
            prevColor = (Color)node->Values[2];
            curTime = (float)node->Values[3];
            curColor = (Color)node->Values[4];
            value = Color::Lerp(prevColor, curColor, Math::Saturate((time - prevTime) / (curTime - prevTime)));
            break;
        default:
            time = (float)tryGetValue(node->GetBox(0), Value::Zero);
            if (time >= node->Values[1 + count * 2 - 2].AsFloat)
            {
                // Outside the range
                value = (Color)node->Values[1 + count * 2 - 2 + 1];
            }
            else
            {
                // Find 2 samples to blend between them
                prevTime = (float)node->Values[1];
                prevColor = (Color)node->Values[2];
                for (int32 i = 1; i < count; i++)
                {
                    curTime = (float)node->Values[i * 2 + 1];
                    curColor = (Color)node->Values[i * 2 + 2];

                    if (time <= curTime)
                    {
                        value = Color::Lerp(prevColor, curColor, Math::Saturate((time - prevTime) / (curTime - prevTime)));
                        break;
                    }

                    prevTime = curTime;
                    prevColor = curColor;
                }
            }
            break;
        }
        break;
    }
        // Curve
#define SAMPLE_CURVE(id, curves, type, graphType) \
		case id: \
		{ \
			const auto& curve = GetCurrentGraph()->curves[node->Data.Curve.CurveIndex]; \
			const float time = (float)tryGetValue(node->GetBox(0), Value::Zero); \
			value.Type = VariantType(VariantType::graphType); \
			curve.Evaluate(*(type*)value.AsData, time, false); \
			break; \
		}
    SAMPLE_CURVE(12, FloatCurves, float, Float)
    SAMPLE_CURVE(13, Vector2Curves, Vector2, Vector2)
    SAMPLE_CURVE(14, Vector3Curves, Vector3, Vector3)
    SAMPLE_CURVE(15, Vector4Curves, Vector4, Vector4)
#undef SETUP_CURVE
        // Get Gameplay Global
    case 16:
    {
        const auto asset = node->Assets[0].As<GameplayGlobals>();
        if (asset)
        {
            const StringView& name = (StringView)node->Values[1];
            const auto e = asset->Variables.Find(name);
            value = e != asset->Variables.End() ? e->Value.Value : Value::Zero;
        }
        else
        {
            value = Value::Zero;
        }
        break;
    }
        // Platform Switch
    case 17:
    {
        int32 boxId = 1;
        switch (PLATFORM_TYPE)
        {
        case PlatformType::Windows:
            boxId = 2;
            break;
        case PlatformType::XboxOne:
            boxId = 3;
            break;
        case PlatformType::UWP:
            boxId = 4;
            break;
        case PlatformType::Linux:
            boxId = 5;
            break;
        case PlatformType::PS4:
            boxId = 6;
            break;
        case PlatformType::XboxScarlett:
            boxId = 7;
            break;
        case PlatformType::Android:
            boxId = 8;
            break;
        default: ;
        }
        value = tryGetValue(node->GetBox(node->GetBox(boxId)->HasConnection() ? boxId : 1), Value::Zero);
        break;
    }
        // Asset Reference
    case 18:
    {
        value = ::LoadAsset((Guid)node->Values[0], Asset::TypeInitializer);
        break;
    }
        // To String
    case 20:
    {
        value.SetString(tryGetValue(node->GetBox(1), Value(StringView::Empty)).ToString());
        break;
    }
        // Actor Reference
    case 21:
    {
        value = Scripting::FindObject<Actor>((Guid)node->Values[0]);
        break;
    }

    case 29:
    {
        value = tryGetValue(node->GetBox(0), Value::Zero);
        break;
    }
    default:
        break;
    }
}

void VisjectExecutor::ProcessGroupBoolean(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
        // NOT
    case 1:
    {
        const bool a = (bool)tryGetValue(node->GetBox(0), Value::False);
        value = !a;
        break;
    }
        // AND, OR, XOR, NOR, NAND
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    {
        const bool a = (bool)tryGetValue(node->GetBox(0), 0, node->Values[0]);
        const bool b = (bool)tryGetValue(node->GetBox(1), 1, node->Values[1]);
        bool result = false;
        switch (node->TypeID)
        {
            // AND
        case 2:
            result = a && b;
            break;
            // OR
        case 3:
            result = a || b;
            break;
            // XOR
        case 4:
            result = !a != !b;
            break;
            // NOR
        case 5:
            result = !(a || b);
            break;
            // NAND
        case 6:
            result = !(a && b);
            break;
        }
        value = result;
        break;
    }
    default:
        break;
    }
}

void VisjectExecutor::ProcessGroupBitwise(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
        // NOT
    case 1:
    {
        const int32 a = (int32)tryGetValue(node->GetBox(0), Value(0));
        value = !a;
        break;
    }
        // AND, OR, XOR
    case 2:
    case 3:
    case 4:
    {
        const int32 a = (int32)tryGetValue(node->GetBox(0), 0, node->Values[0]);
        const int32 b = (int32)tryGetValue(node->GetBox(1), 1, node->Values[1]);
        int32 result = 0;
        switch (node->TypeID)
        {
            // AND
        case 2:
            result = a & b;
            break;
            // OR
        case 3:
            result = a | b;
            break;
            // XOR
        case 4:
            result = a ^ b;
            break;
        }
        value = result;
        break;
    }
    default:
        break;
    }
}

void VisjectExecutor::ProcessGroupComparisons(Box* box, Node* node, Value& value)
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
        const Value a = tryGetValue(node->GetBox(0), 0, node->Values[0]);
        const Value b = tryGetValue(node->GetBox(1), 1, node->Values[1]).Cast(a.Type);
        bool result = false;
        switch (node->TypeID)
        {
        case 1:
            result = a == b;
            break;
        case 2:
            result = a != b;
            break;
        case 3:
            result = a > b;
            break;
        case 4:
            result = a < b;
            break;
        case 5:
            result = a <= b;
            break;
        case 6:
            result = a >= b;
            break;
        }
        value = result;
        break;
    }
        // Switch On Bool
    case 7:
    {
        const Value condition = tryGetValue(node->GetBox(0), Value::False);
        if (condition)
            value = tryGetValue(node->GetBox(2), 1, Value::Zero);
        else
            value = tryGetValue(node->GetBox(1), 0, Value::Zero);
        break;
    }
        // Switch On Enum
    case 8:
    {
        const Value v = tryGetValue(node->GetBox(0), Value::Null);
        if (v.Type.Type == VariantType::Enum && node->Values.Count() == 1 && node->Values[0].Type.Type == VariantType::Blob)
        {
            int32* dataValues = (int32*)node->Values[0].AsBlob.Data;
            int32 dataValuesCount = node->Values[0].AsBlob.Length / 4;
            int32 vAsInt = (int32)v;
            for (int32 i = 0; i < dataValuesCount; i++)
            {
                if (dataValues[i] == vAsInt)
                {
                    value = tryGetValue(node->GetBox(i + 2), Value::Null);
                    break;
                }
            }
        }
        else
            value = Value::Null;
        break;
    }
    }
}

void VisjectExecutor::ProcessGroupParticles(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
        // Random Float
    case 208:
    {
        value = RAND;
        break;
    }
        // Random Vector2
    case 209:
    {
        value = Vector2(RAND, RAND);
        break;
    }
        // Random Vector3
    case 210:
    {
        value = Vector3(RAND, RAND, RAND);
        break;
    }
        // Random Vector4
    case 211:
    {
        value = Vector4(RAND, RAND, RAND, RAND);
        break;
    }
        // Random Float Range
    case 213:
    {
        auto& a = node->Values[0].AsFloat;
        auto& b = node->Values[1].AsFloat;
        value = Math::Lerp(a, b, RAND);
        break;
    }
        // Random Vector2 Range
    case 214:
    {
        auto a = (Vector2)node->Values[0];
        auto b = (Vector2)node->Values[1];
        value = Vector2(
            Math::Lerp(a.X, b.X, RAND),
            Math::Lerp(a.Y, b.Y, RAND)
        );
        break;
    }
        // Random Vector3 Range
    case 215:
    {
        auto a = (Vector3)node->Values[0];
        auto b = (Vector3)node->Values[1];
        value = Vector3(
            Math::Lerp(a.X, b.X, RAND),
            Math::Lerp(a.Y, b.Y, RAND),
            Math::Lerp(a.Z, b.Z, RAND)
        );
        break;
    }
        // Random Vector4 Range
    case 216:
    {
        auto a = (Vector4)node->Values[0];
        auto b = (Vector4)node->Values[1];
        value = Vector4(
            Math::Lerp(a.X, b.X, RAND),
            Math::Lerp(a.Y, b.Y, RAND),
            Math::Lerp(a.Z, b.Z, RAND),
            Math::Lerp(a.W, b.W, RAND)
        );
        break;
    }
    default:
        break;
    }
}

VisjectExecutor::Value VisjectExecutor::tryGetValue(Box* box, int32 defaultValueBoxIndex, const Value& defaultValue)
{
    const auto parentNode = box->GetParent<Node>();
    if (box->HasConnection())
        return eatBox(parentNode, box->FirstConnection());
    if (parentNode->Values.Count() > defaultValueBoxIndex)
        return Value(parentNode->Values[defaultValueBoxIndex]);
    return defaultValue;
}

VisjectExecutor::Value VisjectExecutor::tryGetValue(Box* box, const Value& defaultValue)
{
    return box && box->HasConnection() ? eatBox(box->GetParent<Node>(), box->FirstConnection()) : defaultValue;
}
