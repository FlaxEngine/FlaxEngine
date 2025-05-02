// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("Guid")
{
    SECTION("Test Parse")
    {
        Guid a1;
        Guid::Parse(StringAnsiView("5094652a8d40275c9375bb9438653646"), a1);
        CHECK(a1.ToString() == TEXT("5094652a8d40275c9375bb9438653646"));
        CHECK(a1.ToString(Guid::FormatType::N) == TEXT("5094652a8d40275c9375bb9438653646"));
        CHECK(a1.ToString(Guid::FormatType::D) == TEXT("5094652a-8d40-275c-9375-bb9438653646"));
        CHECK(a1.ToString(Guid::FormatType::B) == TEXT("{5094652a-8d40-275c-9375-bb9438653646}"));
        CHECK(a1.ToString(Guid::FormatType::P) == TEXT("(5094652a-8d40-275c-9375-bb9438653646)"));
    }

    SECTION("Test IsValid")
    {
        CHECK(!Guid::Empty.IsValid());
        Guid a1;
        Guid::Parse(StringAnsiView("5094652a8d40275c9375bb9438653646"), a1);
        CHECK(a1.IsValid());
    }
}
