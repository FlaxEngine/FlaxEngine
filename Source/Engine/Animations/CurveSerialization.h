// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Curve.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Serialization/ReadStream.h"
#include "Engine/Serialization/WriteStream.h"
#include "Engine/Serialization/Serialization.h"

// @formatter:off

namespace Serialization
{
    // StepCurveKeyframe

    template<class T>
    inline bool ShouldSerialize(const StepCurveKeyframe<T>& v, const void* otherObj)
    {
        if (!otherObj)
            return true;
        const auto other = (const StepCurveKeyframe<T>*)otherObj;
        return !(v == *other);
    }
    template<class T>
    inline void Serialize(ISerializable::SerializeStream& stream, const StepCurveKeyframe<T>& v, const void* otherObj)
    {
        stream.StartObject();
        stream.JKEY("Time");
        Serialize(stream, v.Time, nullptr);
        stream.JKEY("Value");
        Serialize(stream, v.Value, nullptr);
        stream.EndObject();
    }
    template<class T>
    inline void Deserialize(ISerializable::DeserializeStream& stream, StepCurveKeyframe<T>& v, ISerializeModifier* modifier)
    {
        DESERIALIZE_MEMBER(Time, v.Time);
        DESERIALIZE_MEMBER(Value, v.Value);
    }

    // LinearCurveKeyframe

    template<class T>
    inline bool ShouldSerialize(const LinearCurveKeyframe<T>& v, const void* otherObj)
    {
        if (!otherObj)
            return true;
        const auto other = (const LinearCurveKeyframe<T>*)otherObj;
        return !(v == *other);
    }
    template<class T>
    inline void Serialize(ISerializable::SerializeStream& stream, const LinearCurveKeyframe<T>& v, const void* otherObj)
    {
        stream.StartObject();
        stream.JKEY("Time");
        Serialize(stream, v.Time, nullptr);
        stream.JKEY("Value");
        Serialize(stream, v.Value, nullptr);
        stream.EndObject();
    }
    template<class T>
    inline void Deserialize(ISerializable::DeserializeStream& stream, LinearCurveKeyframe<T>& v, ISerializeModifier* modifier)
    {
        DESERIALIZE_MEMBER(Time, v.Time);
        DESERIALIZE_MEMBER(Value, v.Value);
    }

    // HermiteCurveKeyframe

    template<class T>
    inline bool ShouldSerialize(const HermiteCurveKeyframe<T>& v, const void* otherObj)
    {
        if (!otherObj)
            return true;
        const auto other = (const HermiteCurveKeyframe<T>*)otherObj;
        return !(v == *other);
    }
    template<class T>
    inline void Serialize(ISerializable::SerializeStream& stream, const HermiteCurveKeyframe<T>& v, const void* otherObj)
    {
        stream.StartObject();
        stream.JKEY("Time");
        Serialize(stream, v.Time, nullptr);
        stream.JKEY("Value");
        Serialize(stream, v.Value, nullptr);
        stream.JKEY("TangentIn");
        Serialize(stream, v.TangentIn, nullptr);
        stream.JKEY("TangentOut");
        Serialize(stream, v.TangentOut, nullptr);
        stream.EndObject();
    }
    template<class T>
    inline void Deserialize(ISerializable::DeserializeStream& stream, HermiteCurveKeyframe<T>& v, ISerializeModifier* modifier)
    {
        DESERIALIZE_MEMBER(Time, v.Time);
        DESERIALIZE_MEMBER(Value, v.Value);
        DESERIALIZE_MEMBER(TangentIn, v.TangentIn);
        DESERIALIZE_MEMBER(TangentOut, v.TangentOut);
    }

    // BezierCurveKeyframe

    template<class T>
    inline bool ShouldSerialize(const BezierCurveKeyframe<T>& v, const void* otherObj)
    {
        if (!otherObj)
            return true;
        const auto other = (const BezierCurveKeyframe<T>*)otherObj;
        return !(v == *other);
    }
    template<class T>
    inline void Serialize(ISerializable::SerializeStream& stream, const BezierCurveKeyframe<T>& v, const void* otherObj)
    {
        stream.StartObject();
        stream.JKEY("Time");
        Serialize(stream, v.Time, nullptr);
        stream.JKEY("Value");
        Serialize(stream, v.Value, nullptr);
        stream.JKEY("TangentIn");
        Serialize(stream, v.TangentIn, nullptr);
        stream.JKEY("TangentOut");
        Serialize(stream, v.TangentOut, nullptr);
        stream.EndObject();
    }
    template<class T>
    inline void Deserialize(ISerializable::DeserializeStream& stream, BezierCurveKeyframe<T>& v, ISerializeModifier* modifier)
    {
        DESERIALIZE_MEMBER(Time, v.Time);
        DESERIALIZE_MEMBER(Value, v.Value);
        DESERIALIZE_MEMBER(TangentIn, v.TangentIn);
        DESERIALIZE_MEMBER(TangentOut, v.TangentOut);
    }

    // Curve

    template<class T, typename KeyFrame>
    inline bool ShouldSerialize(const Curve<T, KeyFrame>& v, const void* otherObj)
    {
        if (!otherObj)
            return true;
        const auto other = (const Curve<T, KeyFrame>*)otherObj;
        return !(v == *other);
    }
    template<class T, typename KeyFrame>
    inline void Serialize(ISerializable::SerializeStream& stream, const Curve<T, KeyFrame>& v, const void* otherObj)
    {
        stream.StartObject();
        stream.JKEY("Keyframes");
        stream.StartArray();
        for (auto& k : v.GetKeyframes())
            Serialize(stream, k, nullptr);
        stream.EndArray();
        stream.EndObject();
    }
    template<class T, typename KeyFrame>
    inline void Deserialize(ISerializable::DeserializeStream& stream, Curve<T, KeyFrame>& v, ISerializeModifier* modifier)
    {
        if (!stream.IsObject())
            return;
        const auto mKeyframes = SERIALIZE_FIND_MEMBER(stream, "Keyframes");
        if (mKeyframes != stream.MemberEnd())
        {
            const auto& keyframesArray = mKeyframes->value.GetArray();
            auto& keyframes = v.GetKeyframes();
            keyframes.Resize(keyframesArray.Size());
            for (rapidjson::SizeType i = 0; i < keyframesArray.Size(); i++)
                Deserialize(keyframesArray[i], keyframes[i], modifier);
        }
    }

    template<class T, typename KeyFrame>
    inline void Serialize(WriteStream& stream, const Curve<T, KeyFrame>& v)
    {
        auto& keyframes = v.GetKeyframes();

        // Version
        if (keyframes.IsEmpty())
        {
            stream.WriteInt32(0);
            return;
        }
        stream.WriteInt32(1);

        // TODO: support compression (serialize compression mode)

        // Raw keyframes data
        stream.WriteInt32(keyframes.Count());
        stream.WriteBytes(keyframes.Get(), keyframes.Count() * sizeof(KeyFrame));
    }

    template<class T, typename KeyFrame>
    inline bool Deserialize(ReadStream& stream, Curve<T, KeyFrame>& v)
    {
        auto& keyframes = v.GetKeyframes();
        keyframes.Resize(0);

        // Version
        int32 version;
        stream.ReadInt32(&version);
        if (version == 0)
            return false;
        if (version != 1)
        {
            return true;
        }

        // Raw keyframes data
        int32 keyframesCount;
        stream.ReadInt32(&keyframesCount);
        keyframes.Resize(keyframesCount, false);
        stream.ReadBytes(keyframes.Get(), keyframes.Count() * sizeof(KeyFrame));

        return false;
    }
}

// @formatter:on
