// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_ASSETS_IMPORTER

#include "ImportIES.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/RandomStream.h"
#include "Engine/Core/Math/Packed.h"

#define MAX_LINE 200

static void SkipWhiteSpace(const uint8*& bufferPos)
{
    while (*bufferPos)
    {
        if (*bufferPos == 13 && *(bufferPos + 1) == 10)
        {
            bufferPos += 2;
            continue;
        }
        if (*bufferPos == 10)
        {
            bufferPos++;
            continue;
        }
        if (*bufferPos <= ' ')
        {
            bufferPos++;
            continue;
        }

        break;
    }
}

static void ReadLine(const uint8*& bufferPos, char line[MAX_LINE], bool skipUntilWhitespace)
{
    SkipWhiteSpace(bufferPos);

    char* linePtr = line;
    uint32 i;
    for (i = 0; i < 255; i++)
    {
        if (*bufferPos == 0)
        {
            break;
        }
        if (*bufferPos == 13 && *(bufferPos + 1) == 10)
        {
            bufferPos += 2;
            break;
        }
        if (*bufferPos == 10)
        {
            bufferPos++;
            continue;
        }
        if (skipUntilWhitespace && *bufferPos <= ' ')
        {
            bufferPos++;
            break;
        }

        *linePtr++ = *bufferPos++;
    }

    line[i] = 0;
}

static bool ReadFloat(const uint8*& bufferPos, float& ret)
{
    char line[MAX_LINE];
    ReadLine(bufferPos, line, true);
    ret = static_cast<float>(atof(line));
    return true;
}

static bool ReadLine(const uint8*& bufferPos, int32& ret)
{
    char line[MAX_LINE];
    ReadLine(bufferPos, line, true);
    ret = atoi(line);
    return true;
}

#define PARSE_FLOAT(x) float x; if (!ReadFloat(bufferPos, x)) { return true; }
#define PARSE_INT(x) int32 x; if (!ReadLine(bufferPos, x)) { return true; }

bool ImportIES::Load(const byte* buffer)
{
    // Referenced IES file format:
    // http://www.ltblight.com/English.lproj/LTBLhelp/pages/iesformat.html

    const uint8* bufferPos = buffer;
    const char* version;
    {
        char line[MAX_LINE];
        ReadLine(bufferPos, line, false);
        if (StringUtils::CompareIgnoreCase(line, "IESNA:LM-63-1995") == 0)
        {
            version = "EIESV_1995";
        }
        else if (StringUtils::CompareIgnoreCase(line, "IESNA91") == 0)
        {
            version = "EIESV_1991";
        }
        else if (StringUtils::CompareIgnoreCase(line, "IESNA:LM-63-2002") == 0)
        {
            version = "EIESV_2002";
        }
        else
        {
            version = "EIESV_1986";
        }
    }

    while (*bufferPos)
    {
        char line[MAX_LINE];
        ReadLine(bufferPos, line, false);
        if (StringUtils::Compare(line, "TILT=NONE") == 0)
        {
            break;
        }
        if (StringUtils::Compare(line, "TILT=", 5) == 0)
        {
            return true;
        }
    }

    PARSE_INT(lights);
    PARSE_FLOAT(lumensPerLight);
    PARSE_FLOAT(candalaScale);
    PARSE_INT(vAnglesCount);
    PARSE_INT(hAnglesCount);
    PARSE_INT(photometricType);
    PARSE_INT(unitType);
    PARSE_FLOAT(width);
    PARSE_FLOAT(length);
    PARSE_FLOAT(height);
    PARSE_FLOAT(ballastWeight);
    PARSE_FLOAT(dummy);
    PARSE_FLOAT(watts);

    if (lights < 1)
    {
        return true;
    }

    if (candalaScale < 0 || vAnglesCount < 0 || hAnglesCount < 0)
    {
        return true;
    }

    _brightness = lumensPerLight / lights;

    {
        float minValue = MIN_float;
        _vAngles.SetCapacity(vAnglesCount, false);
        for (int32 y = 0; y < vAnglesCount; y++)
        {
            PARSE_FLOAT(value);
            if (value < minValue)
                return true;
            minValue = value;
            _vAngles.Add(value);
        }
    }

    {
        float minValue = MIN_float;
        _hAngles.SetCapacity(hAnglesCount, false);
        for (int32 x = 0; x < hAnglesCount; x++)
        {
            PARSE_FLOAT(value);
            if (value < minValue)
                return true;
            minValue = value;
            _hAngles.Add(value);
        }
    }

    _candalaValues.SetCapacity(hAnglesCount * vAnglesCount, false);
    for (int32 y = 0; y < hAnglesCount; y++)
    {
        for (int32 x = 0; x < vAnglesCount; x++)
        {
            PARSE_FLOAT(value);
            _candalaValues.Add(value * candalaScale);
        }
    }

    SkipWhiteSpace(bufferPos);
    if (*bufferPos)
    {
        char line[MAX_LINE];
        ReadLine(bufferPos, line, true);
        if (StringUtils::Compare(line, "END") == 0)
        {
            SkipWhiteSpace(bufferPos);
        }
    }
    if (*bufferPos)
    {
        return true;
    }

    if (_brightness <= 0)
    {
        _brightness = 1000;
    }

    return false;
}

#undef PARSE_FLOAT
#undef PARSE_INT

float ImportIES::ExtractInR16(Array<byte>& output)
{
    const uint32 width = GetWidth();
    const uint32 height = GetHeight();

    output.Clear();
    output.Resize(width * height * sizeof(Half), false);

    Half* out = reinterpret_cast<Half*>(output.Get());

    const float invWidth = 1.0f / (float)width;
    float maxValue = _candalaValues[0];
    for (int32 i = 1; i < _candalaValues.Count(); i++)
        maxValue = Math::Max(maxValue, _candalaValues[i]);
    const float invMaxValue = 1.0f / maxValue;
    const uint32 hAnglesCount = static_cast<uint32>(_hAngles.Count());

    for (uint32 y = 0; y < height; y++)
    {
        for (uint32 x = 0; x < width; x++)
        {
            const float vAngle = (float)x * invWidth * 180.0f;
            const float v = ComputeFilterPos(vAngle, _vAngles);
            float result = 0.0f;
            for (uint32 i = 0; i < hAnglesCount; i++)
                result += InterpolateBilinear(static_cast<float>(i), v);
            *out++ = Float16Compressor::Compress(invMaxValue * result / (float)hAnglesCount);
        }
    }

    float integral;
    {
        // Calculate integral using Monte Carlo
        const int32 count = 500000;
        const RandomStream randomStream(0x1234);
        double sum = 0;
        for (uint32 i = 0; i < count; i++)
        {
            const Float3 v = randomStream.GetUnitVector();
            const float hAngle = Math::Acos(v.Z) / PI * 180;
            const float vAngle = Math::Atan2(v.Y, v.X) / PI * 180 + 180;
            sum += InterpolateBilinear(ComputeFilterPos(hAngle, _hAngles), ComputeFilterPos(vAngle, _vAngles));
        }
        integral = static_cast<float>(sum / count);
    }

    return maxValue / integral;
}

float ImportIES::InterpolatePoint(int32 x, int32 y) const
{
    x %= _hAngles.Count();
    y %= _vAngles.Count();
    return _candalaValues[y + _vAngles.Count() * x];
}

float ImportIES::InterpolateBilinear(float x, float y) const
{
    const int32 xInt = static_cast<int32>(x);
    const int32 yInt = static_cast<int32>(y);

    const float xFrac = x - xInt;
    const float yFrac = y - yInt;

    const float p00 = InterpolatePoint(xInt + 0, yInt + 0);
    const float p10 = InterpolatePoint(xInt + 1, yInt + 0);
    const float p01 = InterpolatePoint(xInt + 0, yInt + 1);
    const float p11 = InterpolatePoint(xInt + 1, yInt + 1);

    const float p0 = Math::Lerp(p00, p01, yFrac);
    const float p1 = Math::Lerp(p10, p11, yFrac);

    return Math::Lerp(p0, p1, xFrac);
}

float ImportIES::ComputeFilterPos(float value, const Array<float>& sortedValues)
{
    ASSERT(sortedValues.HasItems());

    uint32 startPos = 0;
    uint32 endPos = sortedValues.Count() - 1;

    if (value < sortedValues[startPos])
    {
        return 0.0f;
    }
    if (value > sortedValues[endPos])
    {
        return static_cast<float>(endPos);
    }

    while (startPos < endPos)
    {
        const uint32 testPos = (startPos + endPos + 1) / 2;
        const float testValue = sortedValues[testPos];

        if (value >= testValue)
        {
            ASSERT(startPos != testPos);
            startPos = testPos;
        }
        else
        {
            ASSERT(endPos != testPos - 1);
            endPos = testPos - 1;
        }
    }

    const float leftValue = sortedValues[startPos];
    float fraction = 0.0f;
    if (startPos + 1 < static_cast<uint32>(sortedValues.Count()))
    {
        const float rightValue = sortedValues[startPos + 1];
        const float deltaValue = rightValue - leftValue;
        if (deltaValue > 0.00005f)
        {
            fraction = (value - leftValue) / deltaValue;
        }
    }

    return startPos + fraction;
}

#endif
