// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "Engine/Core/Log.h"
#include "Engine/Core/RandomStream.h"
#include "Engine/Core/Enums.h"
#include "Engine/Core/Math/Packed.h"
#include "IESLoader.h"

/// <summary>
/// IES profile format version
/// </summary>
DECLARE_ENUM_4(IESVersion, EIESV_1986, EIESV_1991, EIESV_1995, EIESV_2002);

static void JumpOverWhiteSpace(const uint8*& bufferPos)
{
    // Space and return

    while (*bufferPos)
    {
        if (*bufferPos == 13 && *(bufferPos + 1) == 10)
        {
            bufferPos += 2;
            continue;
        }

        if (*bufferPos == 10)
        {
            // No valid MSDOS return file
            CRASH;
        }
        else if (*bufferPos <= ' ')
        {
            // Tab, space, invisible characters
            bufferPos++;
            continue;
        }

        break;
    }
}

static void GetLineContent(const uint8*& bufferPos, char Line[256], bool bStopOnWhitespace)
{
    JumpOverWhiteSpace(bufferPos);

    char* linePtr = Line;

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
            // No valid MSDOS return file
            CRASH;
        }
        else if (bStopOnWhitespace && (*bufferPos <= ' '))
        {
            // Tab, space, invisible characters
            bufferPos++;
            break;
        }

        *linePtr++ = *bufferPos++;
    }

    Line[i] = 0;
}

static bool GetFloat(const uint8*& bufferPos, float& ret)
{
    char line[256];

    GetLineContent(bufferPos, line, true);

    ret = static_cast<float>(atof(line));
    return true;
}

static bool GetInt(const uint8*& bufferPos, int32& ret)
{
    char line[256];

    GetLineContent(bufferPos, line, true);

    ret = atoi(line);
    return true;
}

IESLoader::IESLoader()
    : _brightness(0)
    , _cachedIntegral(-1)
{
}

#define LOG_IES_IMPORT_ERROR(error) LOG(Warning, "Cannot import IES profile. {0}", error)
#define PARSE_FLOAT(x) float x; if (!GetFloat(bufferPos, x)) { LOG_IES_IMPORT_ERROR(error); return true; }
#define PARSE_INT(x) int32 x; if (!GetInt(bufferPos, x)) { LOG_IES_IMPORT_ERROR(error);  return true; }

bool IESLoader::Load(const byte* buffer)
{
    // File format as described here:
    // http://www.ltblight.com/English.lproj/LTBLhelp/pages/iesformat.html

    const uint8* bufferPos = buffer;
    const Char* error = nullptr;

    IESVersion version = IESVersion::EIESV_1986;
    {
        error = TEXT("VersionError");

        char line1[256];

        GetLineContent(bufferPos, line1, false);

        if (StringUtils::CompareIgnoreCase(line1, "IESNA:LM-63-1995") == 0)
        {
            version = IESVersion::EIESV_1995;
        }
        else if (StringUtils::CompareIgnoreCase(line1, "IESNA91") == 0)
        {
            version = IESVersion::EIESV_1991;
        }
        else if (StringUtils::CompareIgnoreCase(line1, "IESNA:LM-63-2002") == 0)
        {
            version = IESVersion::EIESV_2002;
        }
        else
        {
            // EIESV_1986
        }
    }

    error = TEXT("HeaderError");

    while (*bufferPos)
    {
        char line[256];

        GetLineContent(bufferPos, line, false);

        if (strcmp(line, "TILT=NONE") == 0)
        {
            // At the moment we don't support only profiles with TILT=NONE
            break;
        }
        if (strncmp(line, "TILT=", 5) == 0)
        {
            // "TILT=NONE", "TILT=INCLUDE", and "TILT={filename}"
            // not supported yet, seems rare
            LOG(Warning, "Cannot import IES profile. {0}", String(error));
            return true;
        }
    }

    error = TEXT("HeaderParameterError");

    PARSE_INT(LightCount);

    if (LightCount < 1)
    {
        error = TEXT("Light count needs to be positive.");
        LOG(Warning, "Cannot import IES profile. {0}", error);
        return true;
    }

    // if there is any file with that - do we need to parse it differently?
    ASSERT(LightCount >= 1);

    PARSE_FLOAT(LumensPerLamp);

    _brightness = LumensPerLamp / LightCount;

    PARSE_FLOAT(CandalaMult);

    if (CandalaMult < 0)
    {
        error = TEXT("CandalaMult is negative");
        LOG_IES_IMPORT_ERROR(error);
        return true;
    }

    PARSE_INT(VAnglesNum);
    PARSE_INT(HAnglesNum);

    if (VAnglesNum < 0)
    {
        error = TEXT("VAnglesNum is not valid");
        LOG_IES_IMPORT_ERROR(error);
        return true;
    }

    if (HAnglesNum < 0)
    {
        error = TEXT("HAnglesNum is not valid");
        LOG_IES_IMPORT_ERROR(error);
        return true;
    }

    PARSE_INT(PhotometricType);

    // 1:feet, 2:meter
    PARSE_INT(UnitType);

    PARSE_FLOAT(Width);
    PARSE_FLOAT(Length);
    PARSE_FLOAT(Height);

    PARSE_FLOAT(BallastFactor);
    PARSE_FLOAT(FutureUse);

    PARSE_FLOAT(InputWatts);

    LOG(Info, "IES profile version: {0}, VAngles: {1}, HAngles: {2}", ::ToString(version), VAnglesNum, HAnglesNum);

    error = TEXT("ContentError");

    {
        float minSoFar = MIN_float;

        _vAngles.SetCapacity(VAnglesNum, false);
        for (int32 y = 0; y < VAnglesNum; y++)
        {
            PARSE_FLOAT(Value);

            if (Value < minSoFar)
            {
                // binary search later relies on that
                error = TEXT("V Values are not in increasing order");
                LOG_IES_IMPORT_ERROR(error);
                return true;
            }

            minSoFar = Value;
            _vAngles.Add(Value);
        }
    }

    {
        float minSoFar = MIN_float;

        _hAngles.SetCapacity(HAnglesNum, false);
        for (int32 x = 0; x < HAnglesNum; x++)
        {
            PARSE_FLOAT(Value);

            if (Value < minSoFar)
            {
                // binary search later relies on that
                error = TEXT("H Values are not in increasing order");
                LOG_IES_IMPORT_ERROR(error);
                return true;
            }

            minSoFar = Value;
            _hAngles.Add(Value);
        }
    }

    _candalaValues.SetCapacity(HAnglesNum * VAnglesNum, false);
    for (int32 y = 0; y < HAnglesNum; y++)
    {
        for (int32 x = 0; x < VAnglesNum; x++)
        {
            PARSE_FLOAT(Value);

            _candalaValues.Add(Value * CandalaMult);
        }
    }

    error = TEXT("Unexpected content after candala values.");

    JumpOverWhiteSpace(bufferPos);

    if (*bufferPos)
    {
        // some files are terminated with "END"
        char Line[256];

        GetLineContent(bufferPos, Line, true);

        if (strcmp(Line, "END") == 0)
        {
            JumpOverWhiteSpace(bufferPos);
        }
    }

    if (*bufferPos)
    {
        error = TEXT("Unexpected content after END.");
        LOG_IES_IMPORT_ERROR(error);
        return true;
    }

    if (_brightness <= 0)
    {
        // Some samples have -1, then the brightness comes from the samples
        // Brightness = ComputeFullIntegral();

        // Use some reasonable value
        _brightness = 1000;
    }

    return false;
}
#undef PARSE_FLOAT
#undef PARSE_INT

float IESLoader::ExtractInR16F(Array<byte>& output)
{
    uint32 width = GetWidth();
    uint32 height = GetHeight();

    ASSERT(output.IsEmpty());
    output.Resize(width * height * sizeof(Half), false);

    Half* out = reinterpret_cast<Half*>(output.Get());

    float invWidth = 1.0f / width;
    float maxValue = ComputeMax();
    float invMaxValue = 1.0f / maxValue;

    for (uint32 y = 0; y < height; y++)
    {
        for (uint32 x = 0; x < width; x++)
        {
            // 0..1
            float fraction = x * invWidth;

            // TODO: distort for better quality? eg. Fraction = Square(Fraction);

            float value = invMaxValue * Interpolate1D(fraction * 180.0f);

            *out++ = ConvertFloatToHalf(value);
        }
    }

    float integral = ComputeFullIntegral();

    return maxValue / integral;
}

float IESLoader::InterpolatePoint(int32 X, int32 Y) const
{
    int32 HAnglesNum = _hAngles.Count();
    int32 VAnglesNum = _vAngles.Count();

    ASSERT(X >= 0);
    ASSERT(Y >= 0);

    X %= HAnglesNum;
    Y %= VAnglesNum;

    ASSERT(X < HAnglesNum);
    ASSERT(Y < VAnglesNum);

    return _candalaValues[Y + VAnglesNum * X];
}

float IESLoader::InterpolateBilinear(float fX, float fY) const
{
    int32 X = static_cast<int32>(fX);
    int32 Y = static_cast<int32>(fY);

    float fracX = fX - X;
    float fracY = fY - Y;

    float p00 = InterpolatePoint(X + 0, Y + 0);
    float p10 = InterpolatePoint(X + 1, Y + 0);
    float p01 = InterpolatePoint(X + 0, Y + 1);
    float p11 = InterpolatePoint(X + 1, Y + 1);

    float p0 = Math::Lerp(p00, p01, fracY);
    float p1 = Math::Lerp(p10, p11, fracY);

    return Math::Lerp(p0, p1, fracX);
}

float IESLoader::Interpolate2D(float HAngle, float VAngle) const
{
    float u = ComputeFilterPos(HAngle, _hAngles);
    float v = ComputeFilterPos(VAngle, _vAngles);

    return InterpolateBilinear(u, v);
}

float IESLoader::Interpolate1D(float VAngle) const
{
    float v = ComputeFilterPos(VAngle, _vAngles);

    float ret = 0.0f;

    uint32 HAnglesNum = static_cast<uint32>(_hAngles.Count());

    for (uint32 i = 0; i < HAnglesNum; i++)
    {
        ret += InterpolateBilinear(static_cast<float>(i), v);
    }

    return ret / HAnglesNum;
}

float IESLoader::ComputeMax() const
{
    float result = 0.0f;

    for (int32 i = 0; i < _candalaValues.Count(); i++)
        result = Math::Max(result, _candalaValues[i]);

    return result;
}

float IESLoader::ComputeFullIntegral()
{
    // Compute only if needed
    if (_cachedIntegral < 0)
    {
        // Monte carlo integration
        // If quality is a problem we can improve on this algorithm or increase SampleCount

        // Larger number costs more time but improves quality
        const int32 SamplesCount = 1000000;

        RandomStream randomStream(0x1234);

        double sum = 0;
        for (uint32 i = 0; i < SamplesCount; i++)
        {
            Vector3 v = randomStream.GetUnitVector();

            // http://en.wikipedia.org/wiki/Spherical_coordinate_system

            // 0..180
            float HAngle = Math::Acos(v.Z) / PI * 180;
            // 0..360
            float VAngle = Math::Atan2(v.Y, v.X) / PI * 180 + 180;

            ASSERT(HAngle >= 0 && HAngle <= 180);
            ASSERT(VAngle >= 0 && VAngle <= 360);

            sum += Interpolate2D(HAngle, VAngle);
        }

        _cachedIntegral = static_cast<float>(sum / SamplesCount);
    }

    return _cachedIntegral;
}

float IESLoader::ComputeFilterPos(float value, const Array<float>& sortedValues)
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

    // Binary search
    while (startPos < endPos)
    {
        uint32 testPos = (startPos + endPos + 1) / 2;

        float testValue = sortedValues[testPos];

        if (value >= testValue)
        {
            // Prevent endless loop
            ASSERT(startPos != testPos);

            startPos = testPos;
        }
        else
        {
            // Prevent endless loop
            ASSERT(endPos != testPos - 1);

            endPos = testPos - 1;
        }
    }

    float leftValue = sortedValues[startPos];

    float fraction = 0.0f;

    if (startPos + 1 < static_cast<uint32>(sortedValues.Count()))
    {
        // If not at right border
        float rightValue = sortedValues[startPos + 1];
        float deltaValue = rightValue - leftValue;

        if (deltaValue > 0.0001f)
        {
            fraction = (value - leftValue) / deltaValue;
        }
    }

    return startPos + fraction;
}
