// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Json.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Core/Math/Ray.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Core/Math/Plane.h"
#include "Engine/Core/Math/Matrix.h"
#include "ISerializable.h"

struct CommonValue;

/// <summary>
/// Json value container utilities.
/// </summary>
class FLAXENGINE_API JsonTools
{
public:

    typedef rapidjson_flax::Document Document;
    typedef rapidjson_flax::Value Value;

public:

    FORCE_INLINE static void MergeDocuments(Document& target, Document& source)
    {
        MergeObjects(target, source, target.GetAllocator());
    }

    static void MergeObjects(Value& target, Value& source, Value::AllocatorType& allocator)
    {
        ASSERT(target.IsObject() && source.IsObject());
        for (auto itr = source.MemberBegin(); itr != source.MemberEnd(); ++itr)
            target.AddMember(itr->name, itr->value, allocator);
    }

    static void ChangeIds(Document& doc, const Dictionary<Guid, Guid>& mapping);

public:

    static Vector2 GetVector2(const Value& value)
    {
        Vector2 result;
        const auto mX = value.FindMember("X");
        const auto mY = value.FindMember("Y");
        result.X = mX != value.MemberEnd() ? mX->value.GetFloat() : 0.0f;
        result.Y = mY != value.MemberEnd() ? mY->value.GetFloat() : 0.0f;
        return result;
    }

    static Vector3 GetVector3(const Value& value)
    {
        Vector3 result;
        const auto mX = value.FindMember("X");
        const auto mY = value.FindMember("Y");
        const auto mZ = value.FindMember("Z");
        result.X = mX != value.MemberEnd() ? mX->value.GetFloat() : 0.0f;
        result.Y = mY != value.MemberEnd() ? mY->value.GetFloat() : 0.0f;
        result.Z = mZ != value.MemberEnd() ? mZ->value.GetFloat() : 0.0f;
        return result;
    }

    static Vector4 GetVector4(const Value& value)
    {
        Vector4 result;
        const auto mX = value.FindMember("X");
        const auto mY = value.FindMember("Y");
        const auto mZ = value.FindMember("Z");
        const auto mW = value.FindMember("W");
        result.X = mX != value.MemberEnd() ? mX->value.GetFloat() : 0.0f;
        result.Y = mY != value.MemberEnd() ? mY->value.GetFloat() : 0.0f;
        result.Z = mZ != value.MemberEnd() ? mZ->value.GetFloat() : 0.0f;
        result.W = mW != value.MemberEnd() ? mW->value.GetFloat() : 0.0f;
        return result;
    }

    static Color GetColor(const Value& value)
    {
        Color result;
        const auto mR = value.FindMember("R");
        const auto mG = value.FindMember("G");
        const auto mB = value.FindMember("B");
        const auto mA = value.FindMember("A");
        result.R = mR != value.MemberEnd() ? mR->value.GetFloat() : 0.0f;
        result.G = mG != value.MemberEnd() ? mG->value.GetFloat() : 0.0f;
        result.B = mB != value.MemberEnd() ? mB->value.GetFloat() : 0.0f;
        result.A = mA != value.MemberEnd() ? mA->value.GetFloat() : 0.0f;
        return result;
    }

    static Quaternion GetQuaternion(const Value& value)
    {
        Quaternion result;
        const auto mX = value.FindMember("X");
        const auto mY = value.FindMember("Y");
        const auto mZ = value.FindMember("Z");
        const auto mW = value.FindMember("W");
        result.X = mX != value.MemberEnd() ? mX->value.GetFloat() : 0.0f;
        result.Y = mY != value.MemberEnd() ? mY->value.GetFloat() : 0.0f;
        result.Z = mZ != value.MemberEnd() ? mZ->value.GetFloat() : 0.0f;
        result.W = mW != value.MemberEnd() ? mW->value.GetFloat() : 0.0f;
        return result;
    }

    static Ray GetRay(const Value& value)
    {
        return Ray(
            GetVector3(value, "Position", Vector3::Zero),
            GetVector3(value, "Direction", Vector3::One)
        );
    }

    static Matrix GetMatrix(const Value& value);

    static Transform GetTransform(const Value& value)
    {
        return Transform(
            GetVector3(value, "Translation", Vector3::Zero),
            GetQuaternion(value, "Orientation", Quaternion::Identity),
            GetVector3(value, "Scale", Vector3::One)
        );
    }

    static void GetTransform(Transform& result, const Value& value)
    {
        GetVector3(result.Translation, value, "Translation");
        GetQuaternion(result.Orientation, value, "Orientation");
        GetVector3(result.Scale, value, "Scale");
    }

    static Plane GetPlane(const Value& value)
    {
        Plane result;
        const auto mD = value.FindMember("D");
        result.Normal = GetVector3(value, "Normal", Vector3::One);
        result.D = mD != value.MemberEnd() ? mD->value.GetFloat() : 0.0f;
        return result;
    }

    static Rectangle GetRectangle(const Value& value)
    {
        return Rectangle(
            GetVector2(value, "Location", Vector2::Zero),
            GetVector2(value, "Size", Vector2::Zero)
        );
    }

    static BoundingSphere GetBoundingSphere(const Value& value)
    {
        BoundingSphere result;
        const auto mRadius = value.FindMember("Radius");
        result.Center = GetVector3(value, "Center", Vector3::Zero);
        result.Radius = mRadius != value.MemberEnd() ? mRadius->value.GetFloat() : 0.0f;
        return result;
    }

    static BoundingBox GetBoundingBox(const Value& value)
    {
        return BoundingBox(
            GetVector3(value, "Minimum", Vector3::Zero),
            GetVector3(value, "Maximum", Vector3::Zero)
        );
    }

    static Guid GetGuid(const Value& value)
    {
        CHECK_RETURN(value.GetStringLength() == 32, Guid::Empty);

        // Split
        const char* a = value.GetString();
        const char* b = a + 8;
        const char* c = b + 8;
        const char* d = c + 8;

        // Parse
        Guid result;
        StringUtils::ParseHex(a, 8, &result.A);
        StringUtils::ParseHex(b, 8, &result.B);
        StringUtils::ParseHex(c, 8, &result.C);
        StringUtils::ParseHex(d, 8, &result.D);
        return result;
    }

    static DateTime GetDate(const Value& value);

    static DateTime GetDateTime(const Value& value);

    static CommonValue GetCommonValue(const Value& value);

public:

    FORCE_INLINE static String GetString(const Value& node, const char* name)
    {
        auto member = node.FindMember(name);
        return member != node.MemberEnd() ? member->value.GetText() : String::Empty;
    }

public:

    FORCE_INLINE static bool GetBool(const Value& node, const char* name, const bool defaultValue)
    {
        auto member = node.FindMember(name);
        return member != node.MemberEnd() ? member->value.GetBool() : defaultValue;
    }

    FORCE_INLINE static float GetFloat(const Value& node, const char* name, const float defaultValue)
    {
        auto member = node.FindMember(name);
        return member != node.MemberEnd() && member->value.IsNumber() ? member->value.GetFloat() : defaultValue;
    }

    FORCE_INLINE static int32 GetInt(const Value& node, const char* name, const int32 defaultValue)
    {
        auto member = node.FindMember(name);
        return member != node.MemberEnd() && member->value.IsInt() ? member->value.GetInt() : defaultValue;
    }

    template<class T>
    FORCE_INLINE static T GetEnum(const Value& node, const char* name, const T defaultValue)
    {
        auto member = node.FindMember(name);
        return member != node.MemberEnd() && member->value.IsInt() ? static_cast<T>(member->value.GetInt()) : defaultValue;
    }

    FORCE_INLINE static String GetString(const Value& node, const char* name, const String& defaultValue)
    {
        auto member = node.FindMember(name);
        return member != node.MemberEnd() ? member->value.GetText() : defaultValue;
    }

#define DECLARE_GETTER(type) \
	FORCE_INLINE static type Get##type(const Value& node, const char* name, const type& defaultValue) \
	{ \
		const auto member = node.FindMember(name); \
		return member != node.MemberEnd() ? Get##type(member->value) : defaultValue; \
	}

    DECLARE_GETTER(Rectangle);
    DECLARE_GETTER(Transform);
    DECLARE_GETTER(Vector2);
    DECLARE_GETTER(Vector3);
    DECLARE_GETTER(Vector4);
    DECLARE_GETTER(Color);
    DECLARE_GETTER(Quaternion);
    DECLARE_GETTER(BoundingBox);
    DECLARE_GETTER(BoundingSphere);
    DECLARE_GETTER(Matrix);
    DECLARE_GETTER(Ray);
    DECLARE_GETTER(Plane);
    DECLARE_GETTER(DateTime);

#undef DECLARE_GETTER

    FORCE_INLINE static Guid GetGuid(const Value& node, const char* name)
    {
        auto member = node.FindMember(name);
        return member != node.MemberEnd() ? GetGuid(member->value) : Guid::Empty;
    }

    FORCE_INLINE static bool GetGuidIfValid(Guid& result, const Value& node, const char* name)
    {
        auto member = node.FindMember(name);
        if (member != node.MemberEnd())
        {
            result = GetGuid(member->value);
            return result.IsValid();
        }
        return false;
    }

public:

    FORCE_INLINE static void GetBool(bool& result, const Value& node, const char* name)
    {
        const auto member = node.FindMember(name);
        if (member != node.MemberEnd() && member->value.IsBool())
        {
            result = member->value.GetBool();
        }
    }

    FORCE_INLINE static void GetFloat(float& result, const Value& node, const char* name)
    {
        const auto member = node.FindMember(name);
        if (member != node.MemberEnd() && member->value.IsNumber())
        {
            result = member->value.GetFloat();
        }
    }

    FORCE_INLINE static void GetInt(byte& result, const Value& node, const char* name)
    {
        const auto member = node.FindMember(name);
        if (member != node.MemberEnd() && member->value.IsInt())
        {
            result = member->value.GetInt();
        }
    }

    FORCE_INLINE static void GetInt(int32& result, const Value& node, const char* name)
    {
        const auto member = node.FindMember(name);
        if (member != node.MemberEnd() && member->value.IsInt())
        {
            result = member->value.GetInt();
        }
    }

    FORCE_INLINE static void GetInt(uint32& result, const Value& node, const char* name)
    {
        const auto member = node.FindMember(name);
        if (member != node.MemberEnd() && member->value.IsInt())
        {
            result = member->value.GetInt();
        }
    }

    FORCE_INLINE static void GetInt(int16& result, const Value& node, const char* name)
    {
        const auto member = node.FindMember(name);
        if (member != node.MemberEnd() && member->value.IsInt())
        {
            result = member->value.GetInt();
        }
    }

    FORCE_INLINE static void GetInt(uint16& result, const Value& node, const char* name)
    {
        const auto member = node.FindMember(name);
        if (member != node.MemberEnd() && member->value.IsInt())
        {
            result = member->value.GetInt();
        }
    }

    template<class T>
    FORCE_INLINE static void GetEnum(T& result, const Value& node, const char* name)
    {
        const auto member = node.FindMember(name);
        if (member != node.MemberEnd() && member->value.IsInt())
        {
            result = static_cast<T>(member->value.GetInt());
        }
    }

    FORCE_INLINE static void GetString(String& result, const Value& node, const char* name)
    {
        const auto member = node.FindMember(name);
        if (member != node.MemberEnd())
        {
            result = member->value.GetText();
        }
    }

#define DECLARE_GETTER(type) \
	FORCE_INLINE static void Get##type(type& result, const Value& node, const char* name) \
	{ \
		const auto member = node.FindMember(name); \
		if (member != node.MemberEnd()) \
		{ \
			result = Get##type(member->value); \
		} \
	}

    DECLARE_GETTER(Rectangle);
    DECLARE_GETTER(Vector2);
    DECLARE_GETTER(Vector3);
    DECLARE_GETTER(Vector4);
    DECLARE_GETTER(Color);
    DECLARE_GETTER(Quaternion);
    DECLARE_GETTER(BoundingBox);
    DECLARE_GETTER(BoundingSphere);
    DECLARE_GETTER(Ray);
    DECLARE_GETTER(Plane);
    DECLARE_GETTER(DateTime);

#define DECLARE_GETTER(type) \
	FORCE_INLINE static void Get##type(type& result, const Value& node, const char* name) \
	{ \
		const auto member = node.FindMember(name); \
		if (member != node.MemberEnd()) \
		{ \
			Get##type(result, member->value); \
		} \
	}

    DECLARE_GETTER(Transform);

#undef DECLARE_GETTER

    template<typename T>
    FORCE_INLINE static void GetReference(T& result, const Value& node, const char* name)
    {
        const auto member = node.FindMember(name);
        if (member != node.MemberEnd())
        {
            result = GetGuid(member->value);
        }
    }

    FORCE_INLINE static void GetGuid(Guid& result, const Value& node, const char* name)
    {
        const auto member = node.FindMember(name);
        if (member != node.MemberEnd())
        {
            result = GetGuid(member->value);
        }
    }
};
