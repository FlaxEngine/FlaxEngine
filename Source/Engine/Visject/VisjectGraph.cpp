// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "VisjectGraph.h"
#include "GraphUtilities.h"
#include "Engine/Core/Random.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Engine/GameplayGlobals.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Level/Actor.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MField.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Utilities/Noise.h"

#define RAND Random::Rand()
#define ENSURE(condition, errorMsg)  if (!(condition)) { OnError(node, box, errorMsg); return; }

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
    _perGroupProcessCall[18] = &VisjectExecutor::ProcessGroupCollections;
}

VisjectExecutor::~VisjectExecutor()
{
}

void VisjectExecutor::OnError(Node* node, Box* box, const StringView& message)
{
    Error(node, box, message);
    LOG_STR(Error, message);
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
    case 15:
        value = node->Values[0];
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
            value = v;
        else if (box->ID == 1)
            value = cv.X;
        else if (box->ID == 2)
            value = cv.Y;
        else if (box->ID == 3)
            value = cv.Z;
        else if (box->ID == 4)
            value = cv.W;
        break;
    }
    // Rotation
    case 8:
    {
        const float pitch = (float)node->Values[0];
        const float yaw = (float)node->Values[1];
        const float roll = (float)node->Values[2];
        value = Quaternion::Euler(pitch, yaw, roll);
        break;
    }
    case 9:
        value = node->Values[0];
        break;
    // PI
    case 10:
        value = PI;
        break;
    // Enum
    case 11:
        value = node->Values[0];
        break;
    // Array
    case 13:
        value = node->Values[0];
        if (value.Type.Type == VariantType::Array)
        {
            auto& array = value.AsArray();
            const int32 count = Math::Min(array.Count(), node->Boxes.Count() - 1);
            const VariantType elementType = value.Type.GetElementType();
            for (int32 i = 0; i < count; i++)
            {
                auto b = &node->Boxes[i + 1];
                if (b && b->HasConnection())
                    array[i] = eatBox(node, b->FirstConnection()).Cast(elementType);
            }
        }
        break;
    // Dictionary
    case 14:
    {
        value = Variant(Dictionary<Variant, Variant>());
        String typeName = TEXT("System.Collections.Generic.Dictionary`2[");
        typeName += (StringView)node->Values[0];
        typeName += ',';
        typeName += (StringView)node->Values[1];
        typeName += ']';
        value.Type.SetTypeName(typeName);
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
            value = v;
        else if (box->ID == 1)
            value = cv.X;
        else if (box->ID == 2)
            value = cv.Y;
        else if (box->ID == 3)
            value = cv.Z;
        else if (box->ID == 4)
            value = cv.W;
        break;
    }
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
        auto b1 = node->GetBox(0);
        Value v1 = tryGetValue(b1, 0, Value::Zero);
        Value v2 = tryGetValue(node->GetBox(1), 1, Value::Zero);
        if (b1->HasConnection())
            v2 = v2.Cast(v1.Type);
        else
            v1 = v1.Cast(v2.Type);
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
            case VariantType::Float2:
                value = v1.AsFloat2().Length();
                break;
            case VariantType::Float3:
                value = v1.AsFloat3().Length();
                break;
            case VariantType::Float4:
                value = Float3(v1.AsFloat4()).Length();
                break;
            case VariantType::Double2:
                value = v1.AsDouble2().Length();
                break;
            case VariantType::Double3:
                value = v1.AsDouble3().Length();
                break;
            case VariantType::Double4:
                value = Double3(v1.AsDouble4()).Length();
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
            case VariantType::Float2:
                value = Float2::Normalize(v1.AsFloat2());
                break;
            case VariantType::Float3:
                value = Float3::Normalize(v1.AsFloat3());
                break;
            case VariantType::Float4:
                value = Float4(Float3::Normalize(Float3(v1.AsFloat4())), 0.0f);
                break;
            case VariantType::Double2:
                value = Double2::Normalize(v1.AsDouble2());
                break;
            case VariantType::Double3:
                value = Double3::Normalize(v1.AsDouble3());
                break;
            case VariantType::Double4:
                value = Double4(Double3::Normalize(Double3(v1.AsDouble3())), 0.0f);
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
        Value v1 = tryGetValue(node->GetBox(0), 0, Value::Zero);
        Value v2 = tryGetValue(node->GetBox(1), 1, Value::Zero).Cast(v1.Type);
        switch (node->TypeID)
        {
        case 18:
            switch (v1.Type.Type)
            {
            case VariantType::Float3:
                value = Float3::Cross(v1.AsFloat3(), v2.AsFloat3());
                break;
            case VariantType::Double3:
                value = Double3::Cross(v1.AsDouble3(), v2.AsDouble3());
                break;
            default: CRASH;
                break;
            }
            break;
        case 19:
            switch (v1.Type.Type)
            {
            case VariantType::Float2:
                value = Float2::Distance(v1.AsFloat2(), v2.AsFloat2());
                break;
            case VariantType::Float3:
                value = Float3::Distance(v1.AsFloat3(), v2.AsFloat3());
                break;
            case VariantType::Float4:
            case VariantType::Color:
                value = Float3::Distance((Float3)v1, (Float3)v2);
                break;
            case VariantType::Double2:
                value = Double2::Distance(v1.AsDouble2(), v2.AsDouble2());
                break;
            case VariantType::Double3:
                value = Double3::Distance(v1.AsDouble3(), v2.AsDouble3());
                break;
            case VariantType::Double4:
                value = Double3::Distance((Double3)v1, (Double3)v2);
                break;
            default: CRASH;
                break;
            }
            break;
        case 20:
            switch (v1.Type.Type)
            {
            case VariantType::Float2:
                value = Float2::Dot(v1.AsFloat2(), v2.AsFloat2());
                break;
            case VariantType::Float3:
                value = Float3::Dot(v1.AsFloat3(), v2.AsFloat3());
                break;
            case VariantType::Float4:
            case VariantType::Color:
                value = Float3::Dot((Float3)v1, (Float3)v2);
                break;
            case VariantType::Double2:
                value = Double2::Dot(v1.AsDouble2(), v2.AsDouble2());
                break;
            case VariantType::Double3:
                value = Double3::Dot(v1.AsDouble3(), v2.AsDouble3());
                break;
            case VariantType::Double4:
                value = Double3::Dot((Double3)v1, (Double3)v2);
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
        case VariantType::Float2:
            value = v1.AsFloat2() - 2.0f * v2.AsFloat2() * Float2::Dot(v1.AsFloat2(), v2.AsFloat2());
            break;
        case VariantType::Float3:
            value = v1.AsFloat3() - 2.0f * v2.AsFloat3() * Float3::Dot(v1.AsFloat3(), v2.AsFloat3());
            break;
        case VariantType::Float4:
            value = Float4(v1.AsFloat4() - 2.0f * v2.AsFloat4() * Float3::Dot((Float3)v1, (Float3)v2));
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
        const auto v1 = (Float3)tryGetValue(node->GetBox(0), Value::Zero);
        value = Math::ExtractLargestComponent(v1);
        break;
    }
    // Bias and Scale
    case 36:
    {
        ASSERT(node->Values.Count() == 2 && node->Values[0].Type == VariantType::Float && node->Values[1].Type == VariantType::Float);
        const auto bias = node->Values[0].AsFloat;
        const auto scale = node->Values[1].AsFloat;
        const auto input = (Float3)tryGetValue(node->GetBox(0), Value::Zero);
        value = (input + bias) * scale;
        break;
    }
    // Rotate About Axis
    case 37:
    {
        const auto normalizedRotationAxis = (Float3)tryGetValue(node->GetBox(0), Value::Zero);
        const auto rotationAngle = (float)tryGetValue(node->GetBox(1), Value::Zero);
        const auto pivotPoint = (Float3)tryGetValue(node->GetBox(2), Value::Zero);
        const auto position = (Float3)tryGetValue(node->GetBox(3), Value::Zero);
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
        const float inVal = tryGetValue(node->GetBox(0), node->Values[0]).AsFloat;
        const Float2 rangeA = tryGetValue(node->GetBox(1), node->Values[1]).AsFloat2();
        const Float2 rangeB = tryGetValue(node->GetBox(2), node->Values[2]).AsFloat2();
        const bool clamp = tryGetValue(node->GetBox(3), node->Values[3]).AsBool;
        auto mapFunc = Math::Remap(inVal, rangeA.X, rangeA.Y, rangeB.X, rangeB.Y);
        value = clamp ? Math::Clamp(mapFunc, rangeB.X, rangeB.Y) : mapFunc;
        break;
    }
    // Rotate Vector
    case 49:
    {
        const Quaternion quaternion = (Quaternion)tryGetValue(node->GetBox(0), Quaternion::Identity);
        const Float3 vector = (Float3)tryGetValue(node->GetBox(1), Float3::Forward);
        value = quaternion * vector;
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
        value = Float2(vX, vY);
        break;
    }
    case 21:
    {
        float vX = (float)tryGetValue(node->GetBox(1), node->Values[0]);
        float vY = (float)tryGetValue(node->GetBox(2), node->Values[1]);
        float vZ = (float)tryGetValue(node->GetBox(3), node->Values[2]);
        value = Float3(vX, vY, vZ);
        break;
    }
    case 22:
    {
        float vX = (float)tryGetValue(node->GetBox(1), node->Values[0]);
        float vY = (float)tryGetValue(node->GetBox(2), node->Values[1]);
        float vZ = (float)tryGetValue(node->GetBox(3), node->Values[2]);
        float vW = (float)tryGetValue(node->GetBox(4), node->Values[3]);
        value = Float4(vX, vY, vZ, vW);
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
        const Float3 vZ = (Float3)tryGetValue(node->GetBox(3), Float3::One);
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
        Float2 v = (Float2)tryGetValue(node->GetBox(0), Float2::Zero);
        int32 subIndex = box->ID - 1;
        ASSERT(subIndex >= 0 && subIndex < 2);
        value = v.Raw[subIndex];
        break;
    }
    case 31:
    {
        Float3 v = (Float3)tryGetValue(node->GetBox(0), Float3::Zero);
        int32 subIndex = box->ID - 1;
        ASSERT(subIndex >= 0 && subIndex < 3);
        value = v.Raw[subIndex];
        break;
    }
    case 32:
    {
        Float4 v = (Float4)tryGetValue(node->GetBox(0), Float4::Zero);
        int32 subIndex = box->ID - 1;
        ASSERT(subIndex >= 0 && subIndex < 4);
        value = v.Raw[subIndex];
        break;
    }
    case 33:
    {
        const Float3 v = ((Quaternion)tryGetValue(node->GetBox(0), Quaternion::Identity)).GetEuler();
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
    // Pack Structure
    case 26:
    {
        // Find type
        const StringView typeName(node->Values[0]);
        const StringAsANSI<100> typeNameAnsi(typeName.Get(), typeName.Length());
        const StringAnsiView typeNameAnsiView(typeNameAnsi.Get(), typeName.Length());
        const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(typeNameAnsiView);
        if (!typeHandle)
        {
#if !COMPILE_WITHOUT_CSHARP
            const auto mclass = Scripting::FindClass(typeNameAnsiView);
            if (mclass)
            {
                // Fallback to C#-only types
                bool failed = false;
                auto instance = mclass->CreateInstance();
                value = instance;
                auto& layoutCache = node->Values[1];
                CHECK(layoutCache.Type.Type == VariantType::Blob);
                MemoryReadStream stream((byte*)layoutCache.AsBlob.Data, layoutCache.AsBlob.Length);
                const byte version = stream.ReadByte();
                if (version == 1)
                {
                    int32 fieldsCount;
                    stream.ReadInt32(&fieldsCount);
                    for (int32 boxId = 1; boxId < node->Boxes.Count(); boxId++)
                    {
                        box = &node->Boxes[boxId];
                        String fieldName;
                        stream.ReadString(&fieldName, 11);
                        VariantType fieldType;
                        stream.ReadVariantType(&fieldType);
                        if (box && box->HasConnection())
                        {
                            StringAsANSI<40> fieldNameAnsi(*fieldName, fieldName.Length());
                            auto field = mclass->GetField(fieldNameAnsi.Get());
                            if (field)
                            {
                                Variant fieldValue = eatBox(node, box->FirstConnection());
                                field->SetValue(instance, MUtils::VariantToManagedArgPtr(fieldValue, field->GetType(), failed));
                            }
                        }
                    }
                }
            }
            else if (typeName.HasChars())
#endif
            {
                OnError(node, box, String::Format(TEXT("Missing type '{0}'"), typeName));
            }
            return;
        }
        const ScriptingType& type = typeHandle.GetType();

        // Allocate structure data and initialize it with native constructor
        value.SetType(VariantType(VariantType::Structure, typeNameAnsiView));

        // Setup structure fields
        auto& layoutCache = node->Values[1];
        CHECK(layoutCache.Type.Type == VariantType::Blob);
        MemoryReadStream stream((byte*)layoutCache.AsBlob.Data, layoutCache.AsBlob.Length);
        const byte version = stream.ReadByte();
        if (version == 1)
        {
            int32 fieldsCount;
            stream.ReadInt32(&fieldsCount);
            for (int32 boxId = 1; boxId < node->Boxes.Count(); boxId++)
            {
                box = &node->Boxes[boxId];
                String fieldName;
                stream.ReadString(&fieldName, 11);
                VariantType fieldType;
                stream.ReadVariantType(&fieldType);
                if (box && box->HasConnection())
                {
                    const Variant fieldValue = eatBox(node, box->FirstConnection());
                    type.Struct.SetField(value.AsBlob.Data, fieldName, fieldValue);
                }
            }
        }

        // For in-built structures try to convert it into internal format for better comparability with the scripting
        value.Inline();

        break;
    }
    // Unpack Structure
    case 36:
    {
        // Get value with structure data
        if (!node->GetBox(0)->HasConnection())
            return;
        Variant structureValue = eatBox(node, node->GetBox(0)->FirstConnection());

        // Find type
        const StringView typeName(node->Values[0]);
        const StringAsANSI<100> typeNameAnsi(typeName.Get(), typeName.Length());
        const StringAnsiView typeNameAnsiView(typeNameAnsi.Get(), typeName.Length());
        const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(typeNameAnsiView);
        if (!typeHandle)
        {
#if USE_CSHARP
            const auto mclass = Scripting::FindClass(typeNameAnsiView);
            if (mclass)
            {
                // Fallback to C#-only types
                auto instance = (MObject*)structureValue;
                CHECK(instance);
                if (structureValue.Type.Type != VariantType::ManagedObject || MCore::Object::GetClass(instance) != mclass)
                {
                    OnError(node, box, String::Format(TEXT("Cannot unpack value of type {0} to structure of type {1}"), String(MUtils::GetClassFullname(instance)), typeName));
                    return;
                }
                auto& layoutCache = node->Values[1];
                CHECK(layoutCache.Type.Type == VariantType::Blob);
                MemoryReadStream stream((byte*)layoutCache.AsBlob.Data, layoutCache.AsBlob.Length);
                const byte version = stream.ReadByte();
                if (version == 1)
                {
                    int32 fieldsCount;
                    stream.ReadInt32(&fieldsCount);
                    for (int32 boxId = 1; boxId < node->Boxes.Count(); boxId++)
                    {
                        String fieldName;
                        stream.ReadString(&fieldName, 11);
                        VariantType fieldType;
                        stream.ReadVariantType(&fieldType);
                        if (box->ID == boxId)
                        {
                            StringAsANSI<40> fieldNameAnsi(*fieldName, fieldName.Length());
                            auto field = mclass->GetField(fieldNameAnsi.Get());
                            if (field)
                                value = MUtils::UnboxVariant(field->GetValueBoxed(instance));
                            break;
                        }
                    }
                }
            }
            else if (typeName.HasChars())
#endif
            {
                OnError(node, box, String::Format(TEXT("Missing type '{0}'"), typeName));
            }
            return;
        }
        const ScriptingType& type = typeHandle.GetType();
        if (structureValue.Type.Type != VariantType::Structure) // If structureValue is eg. Float we can try to cast it to a required structure type
        {
            VariantType typeVariantType(typeNameAnsiView);
            if (Variant::CanCast(structureValue, typeVariantType))
                structureValue = Variant::Cast(structureValue, typeVariantType);
        }
        structureValue.InvertInline(); // Extract any Float3/Int32 into Structure type from inlined format
        const ScriptingTypeHandle structureValueTypeHandle = Scripting::FindScriptingType(structureValue.Type.GetTypeName());
        if (structureValue.Type.Type != VariantType::Structure || typeHandle != structureValueTypeHandle)
        {
            OnError(node, box, String::Format(TEXT("Cannot unpack value of type {0} to structure of type {1}"), structureValue.Type, typeName));
            return;
        }

        // Read structure field
        auto& layoutCache = node->Values[1];
        CHECK(layoutCache.Type.Type == VariantType::Blob);
        MemoryReadStream stream((byte*)layoutCache.AsBlob.Data, layoutCache.AsBlob.Length);
        const byte version = stream.ReadByte();
        if (version == 1)
        {
            int32 fieldsCount;
            stream.ReadInt32(&fieldsCount);
            for (int32 boxId = 1; boxId < node->Boxes.Count(); boxId++)
            {
                String fieldName;
                stream.ReadString(&fieldName, 11);
                VariantType fieldType;
                stream.ReadVariantType(&fieldType);
                if (box->ID == boxId)
                {
                    type.Struct.GetField(structureValue.AsBlob.Data, fieldName, value);
                    break;
                }
            }
        }
        break;
    }
    // Mask X, Y, Z, W
    case 40:
    case 41:
    case 42:
    case 43:
    {
        const Float4 v = (Float4)tryGetValue(node->GetBox(0), Float4::Zero);
        value = v.Raw[node->TypeID - 40];
        break;
    }
    // Mask XY, YZ, XZ,...
    case 44:
    {
        value = (Float2)tryGetValue(node->GetBox(0), Float2::Zero);
        break;
    }
    case 45:
    {
        const Float4 v = (Float4)tryGetValue(node->GetBox(0), Float4::Zero);
        value = Float2(v.X, v.Z);
        break;
    }
    case 46:
    {
        const Float4 v = (Float4)tryGetValue(node->GetBox(0), Float4::Zero);
        value = Float2(v.Y, v.Z);
        break;
    }
    case 47:
    {
        const Float4 v = (Float4)tryGetValue(node->GetBox(0), Float4::Zero);
        value = Float2(v.Z, v.W);
        break;
    }
    // Mask XYZ
    case 70:
    {
        value = (Float3)tryGetValue(node->GetBox(0), Float3::Zero);
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
            value = Float2((float)value0, (float)value1);
            break;
        case 3:
            if (count0 == 1)
                value = Float3((float)value0, value1.AsFloat2().X, value1.AsFloat2().Y);
            else
                value = Float3((Float2)value0, (float)value1);
            break;
        case 4:
            if (count0 == 1)
                value = Float4((float)value0, value1.AsFloat3().X, value1.AsFloat3().Y, value1.AsFloat3().Z);
            else if (count0 == 2)
                value = Float4(value0.AsFloat2().X, value0.AsFloat2().Y, value1.AsFloat2().X, value1.AsFloat2().Y);
            else
                value = Float4((Float3)value0, (float)value1);
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
    SAMPLE_CURVE(13, Float2Curves, Float2, Float2)
    SAMPLE_CURVE(14, Float3Curves, Float3, Float3)
    SAMPLE_CURVE(15, Float4Curves, Float4, Float4)
#undef SETUP_CURVE
    // Get Gameplay Global
    case 16:
        if (const auto asset = node->Assets[0].As<GameplayGlobals>())
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
        case PlatformType::Switch:
            boxId = 9;
            break;
        case PlatformType::PS5:
            boxId = 10;
            break;
        case PlatformType::Mac:
            boxId = 11;
            break;
        case PlatformType::iOS:
            boxId = 12;
            break;
        default: ;
        }
        value = tryGetValue(node->GetBox(node->GetBox(boxId)->HasConnection() ? boxId : 1), Value::Zero);
        break;
    }
    // Asset Reference
    case 18:
        value = ::LoadAsset((Guid)node->Values[0], Asset::TypeInitializer);
        break;
    // To String
    case 20:
        value.SetString(tryGetValue(node->GetBox(1), Value(StringView::Empty)).ToString());
        break;
    // Actor Reference
    case 21:
        value = Scripting::FindObject<Actor>((Guid)node->Values[0]);
        break;
    // As
    case 22:
    {
        value = Value::Null;
        const auto obj = (ScriptingObject*)tryGetValue(node->GetBox(1), Value::Null);
        if (obj)
        {
            const StringView typeName(node->Values[0]);
            const StringAsANSI<100> typeNameAnsi(typeName.Get(), typeName.Length());
            const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(typeNameAnsi.Get(), typeName.Length()));
            const auto objClass = obj->GetClass();
            if (typeHandle && objClass && objClass->IsSubClassOf(typeHandle.GetType().ManagedClass))
                value = obj;
        }
        break;
    }
    // Type Reference node
    case 23:
    {
        const StringView typeName(node->Values[0]);
        if (box->ID == 0)
            value.SetTypename(typeName);
        else
            value = typeName;
        break;
    }
    // Is
    case 24:
    {
        value = Value::False;
        const auto obj = (ScriptingObject*)tryGetValue(node->GetBox(1), Value::Null);
        if (obj)
        {
            const StringView typeName(node->Values[0]);
            const StringAsANSI<100> typeNameAnsi(typeName.Get(), typeName.Length());
            const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(typeNameAnsi.Get(), typeName.Length()));
            const auto objClass = obj->GetClass();
            value.AsBool = typeHandle && objClass && objClass->IsSubClassOf(typeHandle.GetType().ManagedClass);
        }
        break;
    }
    // Is Null
    case 27:
        value = (void*)tryGetValue(node->GetBox(1), Value::Null) == nullptr;
        break;
    // Is Valid
    case 28:
        value = (void*)tryGetValue(node->GetBox(1), Value::Null) != nullptr;
        break;
    // Reroute
    case 29:
        value = tryGetValue(node->GetBox(0), Value::Zero);
        break;
    // Noises
    case 30:
        value = Noise::PerlinNoise((Float2)tryGetValue(node->GetBox(0)));
        break;
    case 31:
        value = Noise::SimplexNoise((Float2)tryGetValue(node->GetBox(0)));
        break;
    case 32:
        value = Noise::WorleyNoise((Float2)tryGetValue(node->GetBox(0)));
        break;
    case 33:
        value = Noise::VoronoiNoise((Float2)tryGetValue(node->GetBox(0)));
        break;
    case 34:
        value = Noise::CustomNoise((Float3)tryGetValue(node->GetBox(0)));
        break;
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
        value = Float2(RAND, RAND);
        break;
    }
    // Random Vector3
    case 210:
    {
        value = Float3(RAND, RAND, RAND);
        break;
    }
    // Random Vector4
    case 211:
    {
        value = Float4(RAND, RAND, RAND, RAND);
        break;
    }
    // Random Float Range
    case 213:
    {
        auto a = (float)tryGetValue(node->TryGetBox(1), node->Values[0]);
        auto b = (float)tryGetValue(node->TryGetBox(2), node->Values[1]);
        value = Math::Lerp(a, b, RAND);
        break;
    }
    // Random Vector2 Range
    case 214:
    {
        auto a = (Float2)tryGetValue(node->TryGetBox(1),  node->Values[0]);
        auto b = (Float2)tryGetValue(node->TryGetBox(2),  node->Values[1]);
        value = Float2(
            Math::Lerp(a.X, b.X, RAND),
            Math::Lerp(a.Y, b.Y, RAND)
        );
        break;
    }
    // Random Vector3 Range
    case 215:
    {
        auto a = (Float3)tryGetValue(node->TryGetBox(1), node->Values[0]);
        auto b = (Float3)tryGetValue(node->TryGetBox(2), node->Values[1]);
        value = Float3(
            Math::Lerp(a.X, b.X, RAND),
            Math::Lerp(a.Y, b.Y, RAND),
            Math::Lerp(a.Z, b.Z, RAND)
        );
        break;
    }
    // Random Vector4 Range
    case 216:
    {
        auto a = (Float4)tryGetValue(node->TryGetBox(1), node->Values[0]);
        auto b = (Float4)tryGetValue(node->TryGetBox(2), node->Values[1]);
        value = Float4(
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

void VisjectExecutor::ProcessGroupCollections(Box* box, Node* node, Value& value)
{
    if (node->TypeID < 100)
    {
        // Array
        Variant v = tryGetValue(node->GetBox(0), Value::Null);
        if (v.Type.Type == VariantType::Null)
            v = Variant(Array<Variant>());
        ENSURE(v.Type.Type == VariantType::Array, String::Format(TEXT("Input value {0} is not an array."), v));
        auto& array = v.AsArray();
        Box* b;
        switch (node->TypeID)
        {
        // Count
        case 1:
            value = array.Count();
            break;
        // Contains
        case 2:
            value = array.Contains(tryGetValue(node->GetBox(1), Value::Null));
            break;
        // Find
        case 3:
            b = node->GetBox(1);
            ENSURE(b->HasConnection(), TEXT("Missing value to find."));
            value = array.Find(eatBox(b->GetParent<Node>(), b->FirstConnection()));
            break;
        // Find Last
        case 4:
            b = node->GetBox(1);
            ENSURE(b->HasConnection(), TEXT("Missing value to find."));
            value = array.FindLast(eatBox(b->GetParent<Node>(), b->FirstConnection()));
            break;
        // Clear
        case 5:
            array.Clear();
            value = MoveTemp(v);
            break;
        // Remove
        case 6:
            b = node->GetBox(1);
            ENSURE(b->HasConnection(), TEXT("Missing value to remove."));
            array.Remove(eatBox(b->GetParent<Node>(), b->FirstConnection()));
            value = MoveTemp(v);
            break;
        // Remove At
        case 7:
        {
            const int32 index = (int32)tryGetValue(node->GetBox(1), 0, Value::Null);
            ENSURE(index >= 0 && index < array.Count(), String::Format(TEXT("Array index {0} is out of range [0;{1}]."), index, array.Count() - 1));
            array.RemoveAt(index);
            value = MoveTemp(v);
            break;
        }
        // Add
        case 8:
            b = node->GetBox(1);
            ENSURE(b->HasConnection(), TEXT("Missing value to add."));
            array.Add(eatBox(b->GetParent<Node>(), b->FirstConnection()));
            value = MoveTemp(v);
            break;
        // Insert
        case 9:
        {
            b = node->GetBox(1);
            ENSURE(b->HasConnection(), TEXT("Missing value to add."));
            const int32 index = (int32)tryGetValue(node->GetBox(2), 0, Value::Null);
            ENSURE(index >= 0 && index <= array.Count(), String::Format(TEXT("Array index {0} is out of range [0;{1}]."), index, array.Count()));
            array.Insert(index, eatBox(b->GetParent<Node>(), b->FirstConnection()));
            value = MoveTemp(v);
            break;
        }
        // Get
        case 10:
        {
            const int32 index = (int32)tryGetValue(node->GetBox(1), 0, Value::Null);
            ENSURE(index >= 0 && index < array.Count(), String::Format(TEXT("Array index {0} is out of range [0;{1}]."), index, array.Count() - 1));
            value = MoveTemp(array[index]);
            break;
        }
        // Set
        case 11:
        {
            b = node->GetBox(2);
            ENSURE(b->HasConnection(), TEXT("Missing value to set."));
            const int32 index = (int32)tryGetValue(node->GetBox(1), 0, Value::Null);
            ENSURE(index >= 0 && index < array.Count(), String::Format(TEXT("Array index {0} is out of range [0;{1}]."), index, array.Count() - 1));
            array[index] = MoveTemp(eatBox(b->GetParent<Node>(), b->FirstConnection()));
            value = MoveTemp(v);
            break;
        }
        // Sort
        case 12:
            Sorting::QuickSort(array);
            value = MoveTemp(v);
            break;
        // Reverse
        case 13:
            array.Reverse();
            value = MoveTemp(v);
            break;
        // Add Unique
        case 14:
            b = node->GetBox(1);
            ENSURE(b->HasConnection(), TEXT("Missing value to add."));
            array.AddUnique(eatBox(b->GetParent<Node>(), b->FirstConnection()));
            value = MoveTemp(v);
            break;
        }
    }
    else if (node->TypeID < 200)
    {
        // Dictionary
        Variant v = tryGetValue(node->GetBox(0), Value::Null);
        if (v.Type.Type == VariantType::Null)
            v = Variant(Dictionary<Variant, Variant>());
        ENSURE(v.Type.Type == VariantType::Dictionary, String::Format(TEXT("Input value {0} is not a dictionary."), v));
        auto& dictionary = *v.AsDictionary;
        switch (node->TypeID)
        {
        // Count
        case 101:
            value = dictionary.Count();
            break;
        // Contains Key
        case 102:
        {
            Variant inKey = tryGetValue(node->GetBox(1), 0, Value::Null);
            value = dictionary.ContainsKey(inKey);
            break;
        }
        // Contains Value
        case 103:
        {
            Variant inValue = tryGetValue(node->GetBox(2), 0, Value::Null);
            value = dictionary.ContainsValue(inValue);
            break;
        }
        // Clear
        case 104:
            dictionary.Clear();
            value = MoveTemp(v);
            break;
        // Remove
        case 105:
        {
            Variant inKey = tryGetValue(node->GetBox(1), 0, Value::Null);
            dictionary.Remove(inKey);
            value = MoveTemp(v);
            break;
        }
        // Set
        case 106:
        {
            Variant inKey = tryGetValue(node->GetBox(1), 0, Value::Null);
            Variant inValue = tryGetValue(node->GetBox(2), 1, Value::Null);
            dictionary[MoveTemp(inKey)] = MoveTemp(inValue);
            value = MoveTemp(v);
            break;
        }
        // Get
        case 107:
        {
            Variant key = tryGetValue(node->GetBox(1), 0, Value::Null);
            Variant* valuePtr = dictionary.TryGet(key);
            ENSURE(valuePtr, TEXT("Missing key to get."));
            value = MoveTemp(*valuePtr);
            break;
        }
        }
    }
}
