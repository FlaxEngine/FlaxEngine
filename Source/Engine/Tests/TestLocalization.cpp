// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Core/Formatting.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Localization/Localization.h"
#include <ThirdParty/catch2/catch.hpp>

struct MyStruct
{
    Vector2 Direction;
    float Speed;
};

DEFINE_DEFAULT_FORMATTING(MyStruct, "Direction:{0} Speed:{1}", v.Direction, v.Speed);

TEST_CASE("Localization")
{
    SECTION("Test Fallback Values")
    {
        String myStr = Localization::GetString(TEXT("localized_id"), TEXT("Fallback value"));
        String myStrPlural = Localization::GetPluralString(TEXT("localized_id_n"), 2, TEXT("Count: {}"));
        CHECK(myStr == TEXT("Fallback value"));
        CHECK(myStrPlural == TEXT("Count: 2"));
    }
    SECTION("Test String Formatting")
    {
        // https://docs.flaxengine.com/manual/scripting/cpp/string-formatting.html

        // Values formatting
        auto str1 = String::Format(TEXT("a: {0}, b: {1}, a: {0}"), TEXT("a"), TEXT("b"));
        CHECK(str1 == TEXT("a: a, b: b, a: a"));
        auto str2 = String::Format(TEXT("1: {}, 2: {}, 3: {}"), 1, 2, 3);
        CHECK(str2 == TEXT("1: 1, 2: 2, 3: 3"));
        auto str3 = String::Format(TEXT("vector: {0}"), Vector3(1, 2, 3));
        CHECK(str3 == TEXT("vector: X:1 Y:2 Z:3"));
        String str = TEXT("hello");
        auto str4 = String::Format(TEXT("string: {0}"), str.ToString());
        CHECK(str4 == TEXT("string: hello"));
        auto str5 = String::Format(TEXT("boolean: {0}"), true);
        CHECK(str5 == TEXT("boolean: true"));

        // Custom type formatting
        MyStruct data = { Vector2(1, 2), 10.0f };
        auto str6 = String::Format(TEXT("{0}"), data);
        CHECK(str6 == TEXT("Direction:X:1 Y:2 Speed:10"));

        // Named arguments formatting
        String text1 = String::Format(TEXT("text: {0}, {1}"), TEXT("one"), TEXT("two"));
        String text2 = String::Format(TEXT("text: {arg0}, {arg1}"), fmt::arg(TEXT("arg0"), TEXT("one")), fmt::arg(TEXT("arg1"), TEXT("two")));
        CHECK(text1 == text2);
    }
    SECTION("Test Guid String")
    {
        const StringView text = TEXT("665bb01c49a3370f14a023b5395de261");
        Guid guid;
        Guid::Parse(text, guid);
        String guidText1 = guid.ToString();
        String guidText2 = String::Format(TEXT("{}"), guid);
        CHECK(text == guidText1);
        CHECK(text == guidText2);
    }
}
