// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Platform/StringUtils.h"
#include "Engine/Platform/FileSystem.h"
#include <ThirdParty/catch2/catch.hpp>

String TestNormalizePath(const StringView& input)
{
    String path(input);
    FileSystem::NormalizePath(path);
    StringUtils::PathRemoveRelativeParts(path);
    return path;
}

TEST_CASE("StringUtils")
{
    SECTION("Test Length")
    {
        CHECK(StringUtils::Length("1234") == 4);
        CHECK(StringUtils::Length(TEXT("1234")) == 4);
    }

    SECTION("Test Paths")
    {
        CHECK(StringUtils::GetFileName(TEXT("file")) == TEXT("file"));
        CHECK(StringUtils::GetFileName(TEXT("file.ext")) == TEXT("file.ext"));
        CHECK(StringUtils::GetFileName(TEXT("folder/file.ext")) == TEXT("file.ext"));
        CHECK(StringUtils::GetFileName(TEXT("folder\\file.ext")) == TEXT("file.ext"));
        CHECK(StringUtils::GetFileName(TEXT("folder/d/file.ext")) == TEXT("file.ext"));
        CHECK(StringUtils::GetFileName(TEXT("folder/d//file.ext")) == TEXT("file.ext"));
        CHECK(StringUtils::GetFileName(TEXT("folder/d/../file.ext")) == TEXT("file.ext"));
        CHECK(StringUtils::GetFileName(TEXT("folder/d/./file.ext")) == TEXT("file.ext"));
        CHECK(StringUtils::GetFileName(TEXT("C:\\folder/d/./file.ext")) == TEXT("file.ext"));
        CHECK(StringUtils::GetFileName(TEXT("/folder/d/./file.ext")) == TEXT("file.ext"));
        CHECK(StringUtils::GetFileName(TEXT("./folder/d/./file.ext")) == TEXT("file.ext"));

        CHECK(StringUtils::GetDirectoryName(TEXT("file")) == TEXT(""));
        CHECK(StringUtils::GetDirectoryName(TEXT("file.ext")) == TEXT(""));
        CHECK(StringUtils::GetDirectoryName(TEXT("folder")) == TEXT(""));
        CHECK(StringUtils::GetDirectoryName(TEXT("folder\\file.ext")) == TEXT("folder"));
        CHECK(StringUtils::GetDirectoryName(TEXT("folder/d/file.ext")) == TEXT("folder/d"));
        CHECK(StringUtils::GetDirectoryName(TEXT("folder/d//file.ext")) == TEXT("folder/d/"));
        CHECK(StringUtils::GetDirectoryName(TEXT("folder/d/../file.ext")) == TEXT("folder/d/.."));
        CHECK(StringUtils::GetDirectoryName(TEXT("folder/d/./file.ext")) == TEXT("folder/d/."));
        CHECK(StringUtils::GetDirectoryName(TEXT("C:\\folder/d/./file.ext")) == TEXT("C:\\folder/d/."));
        CHECK(StringUtils::GetDirectoryName(TEXT("/folder/d/./file.ext")) == TEXT("/folder/d/."));
        CHECK(StringUtils::GetDirectoryName(TEXT("./folder/d/./file.ext")) == TEXT("./folder/d/."));

        CHECK(TestNormalizePath(TEXT("file")) == TEXT("file"));
        CHECK(TestNormalizePath(TEXT("file.ext")) == TEXT("file.ext"));
        CHECK(TestNormalizePath(TEXT("folder")) == TEXT("folder"));
        CHECK(TestNormalizePath(TEXT("folder\\file.ext")) == TEXT("folder/file.ext"));
        CHECK(TestNormalizePath(TEXT("folder/d/file.ext")) == TEXT("folder/d/file.ext"));
        CHECK(TestNormalizePath(TEXT("folder/d//file.ext")) == TEXT("folder/d/file.ext"));
        CHECK(TestNormalizePath(TEXT("folder/d/../file.ext")) == TEXT("folder/file.ext"));
        CHECK(TestNormalizePath(TEXT("folder/d/./file.ext")) == TEXT("folder/d/file.ext"));
        CHECK(TestNormalizePath(TEXT("C:\\folder/d/./file.ext")) == TEXT("C:\\folder/d/file.ext"));
        CHECK(TestNormalizePath(TEXT("/folder/d/./file.ext")) == TEXT("/folder/d/file.ext"));
        CHECK(TestNormalizePath(TEXT("./folder/d/./file.ext")) == TEXT("/folder/d/file.ext"));
    }
}
