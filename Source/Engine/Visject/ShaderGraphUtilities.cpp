// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if USE_EDITOR

#include "ShaderGraphUtilities.h"
#include "ShaderGraphValue.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Content/Content.h"
#include "Engine/Engine/GameplayGlobals.h"

void ShaderGraphUtilities::GenerateShaderConstantBuffer(TextWriterUnicode& writer, Array<SerializedMaterialParam>& parameters)
{
    int32 constantsOffset = 0;
    int32 paddingIndex = 0;

    for (int32 i = 0; i < parameters.Count(); i++)
    {
        auto& param = parameters[i];

        const Char* format = nullptr;
        int32 size;
        int32 alignment;
        switch (param.Type)
        {
        case MaterialParameterType::Bool:
            size = 4;
            alignment = 4;
            format = TEXT("bool {0};");
            break;
        case MaterialParameterType::Integer:
            size = 4;
            alignment = 4;
            format = TEXT("int {0};");
            break;
        case MaterialParameterType::Float:
            size = 4;
            alignment = 4;
            format = TEXT("float {0};");
            break;
        case MaterialParameterType::Vector2:
            size = 8;
            alignment = 8;
            format = TEXT("float2 {0};");
            break;
        case MaterialParameterType::Vector3:
            size = 12;
            alignment = 16;
            format = TEXT("float3 {0};");
            break;
        case MaterialParameterType::Vector4:
        case MaterialParameterType::ChannelMask:
        case MaterialParameterType::Color:
            size = 16;
            alignment = 16;
            format = TEXT("float4 {0};");
            break;
        case MaterialParameterType::Matrix:
            size = 16 * 4;
            alignment = 16;
            format = TEXT("float4x4 {0};");
            break;
        case MaterialParameterType::GameplayGlobal:
        {
            auto asset = Content::LoadAsync<GameplayGlobals>(param.AsGuid);
            if (!asset || asset->WaitForLoaded())
                break;
            GameplayGlobals::Variable variable;
            if (!asset->Variables.TryGet(param.Name, variable))
                break;
            switch (variable.DefaultValue.Type.Type)
            {
            case VariantType::Bool:
                size = 4;
                alignment = 4;
                format = TEXT("bool {0};");
                break;
            case VariantType::Int:
                size = 4;
                alignment = 4;
                format = TEXT("int {0};");
                break;
            case VariantType::Uint:
                size = 4;
                alignment = 4;
                format = TEXT("uint {0};");
                break;
            case VariantType::Float:
                size = 4;
                alignment = 4;
                format = TEXT("float {0};");
                break;
            case VariantType::Vector2:
                size = 8;
                alignment = 8;
                format = TEXT("float2 {0};");
                break;
            case VariantType::Vector3:
                size = 12;
                alignment = 16;
                format = TEXT("float3 {0};");
                break;
            case VariantType::Vector4:
            case VariantType::Color:
                size = 16;
                alignment = 16;
                format = TEXT("float4 {0};");
                break;
            default: ;
            }
            break;
        }
        default: ;
        }
        if (format)
        {
            int32 padding = Math::Abs((alignment - (constantsOffset % 16))) % alignment;
            if (padding != 0)
            {
                constantsOffset += padding;
                padding /= 4;
                while (padding-- > 0)
                {
                    writer.WriteLine(TEXT("uint PADDING_{0};"), paddingIndex++);
                }
            }

            param.RegisterIndex = 0;
            param.Offset = constantsOffset;
            writer.WriteLine(format, param.ShaderName);
            constantsOffset += size;
        }
    }
}

const Char* ShaderGraphUtilities::GenerateShaderResources(TextWriterUnicode& writer, Array<SerializedMaterialParam>& parameters, int32 startRegister)
{
    int32 registerIndex = startRegister;
    for (int32 i = 0; i < parameters.Count(); i++)
    {
        auto& param = parameters[i];

        const Char* format;
        switch (param.Type)
        {
        case MaterialParameterType::NormalMap:
        case MaterialParameterType::GPUTexture:
        case MaterialParameterType::SceneTexture:
        case MaterialParameterType::Texture:
            format = TEXT("Texture2D {0} : register(t{1});");
            break;
        case MaterialParameterType::GPUTextureCube:
        case MaterialParameterType::CubeTexture:
            format = TEXT("TextureCube {0} : register(t{1});");
            break;
        case MaterialParameterType::GPUTextureArray:
            format = TEXT("Texture2DArray {0} : register(t{1});");
            break;
        case MaterialParameterType::GPUTextureVolume:
            format = TEXT("Texture3D {0} : register(t{1});");
            break;
        default:
            format = nullptr;
            break;
        }

        if (format)
        {
            param.Offset = 0;
            param.RegisterIndex = registerIndex;
            writer.WriteLine(format, param.ShaderName, static_cast<int32>(registerIndex));
            registerIndex++;

            // Validate Shader Resource count limit
            if (param.RegisterIndex >= GPU_MAX_SR_BINDED)
            {
                return TEXT("Too many textures used. The maximum supported amount is " MACRO_TO_STR(GPU_MAX_SR_BINDED) " (including lightmaps and utility textures for lighting).");
            }
        }
    }
    return nullptr;
}

template<typename T>
const Char* GetTypename()
{
    return TEXT("");
}

template<>
const Char* GetTypename<float>()
{
    return TEXT("float");
}

template<>
const Char* GetTypename<Vector2>()
{
    return TEXT("float2");
}

template<>
const Char* GetTypename<Vector3>()
{
    return TEXT("float3");
}

template<>
const Char* GetTypename<Vector4>()
{
    return TEXT("float4");
}

template<typename T>
void ShaderGraphUtilities::SampleCurve(TextWriterUnicode& writer, const BezierCurve<T>& curve, const String& time, const String& value)
{
    const auto& keyframes = curve.GetKeyframes();

    if (keyframes.Count() == 0)
    {
        writer.Write(
            TEXT(
                "	{{\n"
                "		// Curve ({1})\n"
                "		{0} = 0;\n"
                "	}}\n"
            ),
            value, // {0}
            GetTypename<T>() // {1}
        );
    }
    else if (keyframes.Count() == 1)
    {
        writer.Write(
            TEXT(
                "	{{\n"
                "		// Curve ({1})\n"
                "		{0} = {2};\n"
                "	}}\n"
            ),
            value, // {0}
            GetTypename<T>(), // {1}
            ShaderGraphValue(keyframes[0].Value).Value // {2}
        );
    }
    else if (keyframes.Count() == 2)
    {
        writer.Write(
            TEXT(
                "	{{\n"
                "		// Curve ({4})\n"
                "		const float leftTime = {3};\n"
                "		const float rightTime = {5};\n"
                "		const float lengthTime = rightTime - leftTime;\n"
                "		float time = clamp({0}, leftTime, rightTime);\n"
                "		float alpha = lengthTime < 0.0000001 ? 0.0f : (time - leftTime) / lengthTime;\n"

                "		const {4} leftValue = {6};\n"
                "		const {4} rightValue = {7};\n"
                "		const float oneThird = 1.0f / 3.0f;\n"
                "		{4} leftTangent = leftValue + {8} * (lengthTime * oneThird);\n"
                "		{4} rightTangent = rightValue + {1} * (lengthTime * oneThird);\n"

                "		{4} p01 = lerp(leftValue, leftTangent, alpha);\n"
                "		{4} p12 = lerp(leftTangent, rightTangent, alpha);\n"
                "		{4} p23 = lerp(rightTangent, rightValue, alpha);\n"
                "		{4} p012 = lerp(p01, p12, alpha);\n"
                "		{4} p123 = lerp(p12, p23, alpha);\n"
                "		{2} = lerp(p012, p123, alpha);\n"
                "	}}\n"
            ),
            time, // {0}
            ShaderGraphValue(keyframes[1].TangentIn).Value, // {1}
            value, // {2}
            StringUtils::ToString(keyframes[0].Time), // {3}
            GetTypename<T>(), // {4}
            StringUtils::ToString(keyframes[1].Time), // {5}
            ShaderGraphValue(keyframes[0].Value).Value, // {6}
            ShaderGraphValue(keyframes[1].Value).Value, // {7}
            ShaderGraphValue(keyframes[0].TangentOut).Value // {8}
        );
    }
    else
    {
        StringBuilder keyframesTime, keyframesValue, keyframesTangentIn, keyframesTangentOut;
        for (int32 i = 0; i < keyframes.Count(); i++)
        {
            const auto& keyframe = keyframes[i];
            if (i != 0)
            {
                keyframesTime.Append(',');
                keyframesValue.Append(',');
                keyframesTangentIn.Append(',');
                keyframesTangentOut.Append(',');
            }
            keyframesTime.Append(StringUtils::ToString(keyframe.Time));
            keyframesValue.Append(ShaderGraphValue(keyframe.Value).Value);
            keyframesTangentIn.Append(ShaderGraphValue(keyframe.TangentIn).Value);
            keyframesTangentOut.Append(ShaderGraphValue(keyframe.TangentOut).Value);
        }
        keyframesTime.Append('\0');
        keyframesValue.Append('\0');
        keyframesTangentIn.Append('\0');
        keyframesTangentOut.Append('\0');

        writer.Write(
            TEXT(
                "	{{\n"
                "		// Curve ({4})\n"
                "		int count = {0};\n"
                "		float time = clamp({1}, 0.0, {2});\n"

                "		static float keyframesTime[] = {{ {5} }};\n"
                "		static {4} keyframesValue[] = {{ {6} }};\n"
                "		static {4} keyframesTangentIn[] = {{ {7} }};\n"
                "		static {4} keyframesTangentOut[] = {{ {8} }};\n"

                "		int start = 0;\n"
                "		int searchLength = count;\n"
                "		while (searchLength > 0)\n"
                "		{{\n"
                "			int halfPos = searchLength >> 1;\n"
                "			int midPos = start + halfPos;\n"
                "			if (time < keyframesTime[midPos])\n"
                "			{{\n"
                "				searchLength = halfPos;\n"
                "			}}\n"
                "			else\n"
                "			{{\n"
                "				start = midPos + 1;\n"
                "				searchLength -= halfPos + 1;\n"
                "			}}\n"
                "		}}\n"
                "		int leftKey = max(0, start - 1);\n"
                "		int rightKey = min(start, count - 1);\n"

                "		const float leftTime = keyframesTime[leftKey];\n"
                "		const float rightTime = keyframesTime[rightKey];\n"
                "		const float lengthTime = rightTime - leftTime;\n"
                "		float alpha = lengthTime < 0.0000001 ? 0.0f : (time - leftTime) / lengthTime;\n"

                "		const {4} leftValue = keyframesValue[leftKey];\n"
                "		const {4} rightValue = keyframesValue[rightKey];\n"
                "		const float oneThird = 1.0f / 3.0f;\n"
                "		{4} leftTangent = leftValue + keyframesTangentOut[leftKey] * (lengthTime * oneThird);\n"
                "		{4} rightTangent = rightValue + keyframesTangentIn[rightKey] * (lengthTime * oneThird);\n"

                "		{4} p01 = lerp(leftValue, leftTangent, alpha);\n"
                "		{4} p12 = lerp(leftTangent, rightTangent, alpha);\n"
                "		{4} p23 = lerp(rightTangent, rightValue, alpha);\n"
                "		{4} p012 = lerp(p01, p12, alpha);\n"
                "		{4} p123 = lerp(p12, p23, alpha);\n"
                "		{3} = lerp(p012, p123, alpha);\n"
                "	}}\n"
            ),
            keyframes.Count(), // {0}
            time, // {1}
            curve.GetLength(), // {2}
            value, // {3}
            GetTypename<T>(), // {4}
            *keyframesTime, // {5}
            *keyframesValue, // {6}
            *keyframesTangentIn, // {7}
            *keyframesTangentOut // {8}
        );
    }
}

template void ShaderGraphUtilities::SampleCurve(TextWriterUnicode& writer, const BezierCurve<float>& curve, const String& time, const String& value);
template void ShaderGraphUtilities::SampleCurve(TextWriterUnicode& writer, const BezierCurve<Vector2>& curve, const String& time, const String& value);
template void ShaderGraphUtilities::SampleCurve(TextWriterUnicode& writer, const BezierCurve<Vector3>& curve, const String& time, const String& value);
template void ShaderGraphUtilities::SampleCurve(TextWriterUnicode& writer, const BezierCurve<Vector4>& curve, const String& time, const String& value);

#endif
