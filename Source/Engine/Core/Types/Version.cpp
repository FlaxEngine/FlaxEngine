// Copyright (c) Wojciech Figat. All rights reserved.

#include "Version.h"
#include "Engine/Core/Types/StringBuilder.h"

Version::Version(int32 major, int32 minor, int32 build, int32 revision)
{
    _major = Math::Max(major, 0);
    _minor = Math::Max(minor, 0);
    _build = Math::Max(build, -1);
    _revision = Math::Max(revision, -1);
}

Version::Version(int32 major, int32 minor, int32 build)
{
    _major = Math::Max(major, 0);
    _minor = Math::Max(minor, 0);
    _build = Math::Max(build, -1);
    _revision = -1;
}

Version::Version(int32 major, int32 minor)
{
    _major = Math::Max(major, 0);
    _minor = Math::Max(minor, 0);
    _build = -1;
    _revision = -1;
}

int32 Version::CompareTo(const Version& value) const
{
    if (_major != value._major)
    {
        if (_major > value._major)
        {
            return 1;
        }
        return -1;
    }
    if (_minor != value._minor)
    {
        if (_minor > value._minor)
        {
            return 1;
        }
        return -1;
    }
    if (_build != value._build)
    {
        if (_build > value._build)
        {
            return 1;
        }
        return -1;
    }
    if (_revision == value._revision)
    {
        return 0;
    }
    if (_revision > value._revision)
    {
        return 1;
    }
    return -1;
}

String Version::ToString(int32 fieldCount) const
{
    StringBuilder stringBuilder(32);
    switch (fieldCount)
    {
    case 0:
        break;
    case 1:
        stringBuilder.Append(Math::Max(_major, 0));
        break;
    case 2:
        stringBuilder.Append(Math::Max(_major, 0));
        stringBuilder.Append(TEXT('.'));
        stringBuilder.Append(Math::Max(_minor, 0));
        break;
    case 3:
        stringBuilder.Append(Math::Max(_major, 0));
        stringBuilder.Append(TEXT('.'));
        stringBuilder.Append(Math::Max(_minor, 0));
        stringBuilder.Append(TEXT('.'));
        stringBuilder.Append(Math::Max(_build, 0));
        break;
    default:
        stringBuilder.Append(Math::Max(_major, 0));
        stringBuilder.Append(TEXT('.'));
        stringBuilder.Append(Math::Max(_minor, 0));
        stringBuilder.Append(TEXT('.'));
        stringBuilder.Append(Math::Max(_build, 0));
        stringBuilder.Append(TEXT('.'));
        stringBuilder.Append(Math::Max(_revision, 0));
        break;
    }
    return stringBuilder.ToString();
}

String Version::ToString() const
{
    if (_build == -1)
    {
        return ToString(2);
    }
    if (_revision == -1)
    {
        return ToString(3);
    }
    return ToString(4);
}

bool Version::Parse(const String& text, Version* value)
{
    int32 major, minor, build, revision;

    Array<String> parsedComponents(4);
    text.Split('.', parsedComponents);

    int32 parsedComponentsLength = parsedComponents.Count();
    if (parsedComponentsLength < 2 || parsedComponentsLength > 4)
    {
        return true;
    }

    if (StringUtils::Parse(*parsedComponents[0], &major))
    {
        return true;
    }

    if (StringUtils::Parse(*parsedComponents[1], &minor))
    {
        return true;
    }

    parsedComponentsLength -= 2;

    if (parsedComponentsLength > 0)
    {
        if (StringUtils::Parse(*parsedComponents[2], &build))
        {
            return true;
        }

        parsedComponentsLength--;

        if (parsedComponentsLength > 0)
        {
            if (StringUtils::Parse(*parsedComponents[3], &revision))
            {
                return true;
            }

            *value = Version(major, minor, build, revision);
        }
        else
        {
            *value = Version(major, minor, build);
        }
    }
    else
    {
        *value = Version(major, minor);
    }

    return false;
}
