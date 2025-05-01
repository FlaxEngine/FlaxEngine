// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("String Replace")
{
    SECTION("Char, case sensitive")
    {
        String str("hello HELLO");
        CHECK(str.Replace('l', 'x', StringSearchCase::CaseSensitive) == 2);
        CHECK(str == String("hexxo HELLO"));
    }

    SECTION("Char, ignore case")
    {
        String str("hello HELLO");
        CHECK(str.Replace('l', 'x', StringSearchCase::IgnoreCase) == 4);
        CHECK(str == String("hexxo HExxO"));
    }

    SECTION("case sensitive")
    {
        String str("hello HELLO this is me saying hello");
        CHECK(str.Replace(TEXT("hello"), TEXT("hi"), StringSearchCase::CaseSensitive) == 2);
        CHECK(str == String("hi HELLO this is me saying hi"));
    }

    SECTION("ignore case")
    {
        String str("hello HELLO this is me saying hello");
        CHECK(str.Replace(TEXT("hello"), TEXT("hi"), StringSearchCase::IgnoreCase) == 3);
        CHECK(str == String("hi hi this is me saying hi"));
    }

    SECTION("case sensitive, search and replace texts identical")
    {
        String str("hello HELLO this is me saying hello");
        CHECK(str.Replace(TEXT("hello"), TEXT("hello"), StringSearchCase::CaseSensitive) == 2);
        CHECK(str == String("hello HELLO this is me saying hello"));
    }

    SECTION("ignore case, search and replace texts identical")
    {
        String str("hello HELLO this is me saying hello");
        CHECK(str.Replace(TEXT("hello"), TEXT("hello"), StringSearchCase::IgnoreCase) == 3);
        CHECK(str == String("hello hello this is me saying hello"));
    }

    SECTION("case sensitive, replace text empty")
    {
        String str("hello HELLO this is me saying hello");
        CHECK(str.Replace(TEXT("hello"), TEXT(""), StringSearchCase::CaseSensitive) == 2);
        CHECK(str == String(" HELLO this is me saying "));
    }

    SECTION("ignore case, replace text empty")
    {
        String str("hello HELLO this is me saying hello");
        CHECK(str.Replace(TEXT("hello"), TEXT(""), StringSearchCase::IgnoreCase) == 3);
        CHECK(str == String("  this is me saying "));
    }

    SECTION("no finds")
    {
        String str("hello HELLO this is me saying hello");
        CHECK(str.Replace(TEXT("bye"), TEXT("hi"), StringSearchCase::CaseSensitive) == 0);
        CHECK(str.Replace(TEXT("bye"), TEXT("hi"), StringSearchCase::IgnoreCase) == 0);
        CHECK(str == String("hello HELLO this is me saying hello"));
    }

    SECTION("empty input")
    {
        String str("");
        CHECK(str.Replace(TEXT("bye"), TEXT("hi"), StringSearchCase::CaseSensitive) == 0);
        CHECK(str.Replace(TEXT("bye"), TEXT("hi"), StringSearchCase::IgnoreCase) == 0);
        CHECK(str == String(""));
    }
}

TEST_CASE("String Starts/EndsWith")
{
    SECTION("StartsWith, case sensitive")
    {
        SECTION("Char")
        {
            CHECK(String("").StartsWith('h', StringSearchCase::CaseSensitive) == false);
            CHECK(String("hello HELLO").StartsWith('h', StringSearchCase::CaseSensitive) == true);
            CHECK(String("hello HELLO").StartsWith('H', StringSearchCase::CaseSensitive) == false);
        }

        SECTION("String")
        {
            CHECK(String("hello HELLO").StartsWith(String(TEXT("hello")), StringSearchCase::CaseSensitive) == true);
            CHECK(String("hello HELLO").StartsWith(String(TEXT("HELLO")), StringSearchCase::CaseSensitive) == false);
            CHECK(String("hello HELLO").StartsWith(String(TEXT("")), StringSearchCase::CaseSensitive) == true);
            CHECK(String("hello HELLO").StartsWith(String(TEXT("xxx")), StringSearchCase::CaseSensitive) == false);

            CHECK(String("").StartsWith(String(TEXT("x")), StringSearchCase::CaseSensitive) == false);
            CHECK(String("hello HELLO").StartsWith(String(TEXT("hello HELLOx")), StringSearchCase::CaseSensitive) == false);
            CHECK(String("hello HELLO").StartsWith(String(TEXT("xhello HELLO")), StringSearchCase::CaseSensitive) == false);
        }

        SECTION("StringView")
        {
            CHECK(String("hello HELLO").StartsWith(StringView(TEXT("hello")), StringSearchCase::CaseSensitive) == true);
            CHECK(String("hello HELLO").StartsWith(StringView(TEXT("HELLO")), StringSearchCase::CaseSensitive) == false);
            CHECK(String("hello HELLO").StartsWith(StringView(), StringSearchCase::CaseSensitive) == true);
            CHECK(String("hello HELLO").StartsWith(StringView(TEXT("")), StringSearchCase::CaseSensitive) == true);
            CHECK(String("hello HELLO").StartsWith(StringView(TEXT("xxx")), StringSearchCase::CaseSensitive) == false);

            CHECK(String("").StartsWith(StringView(TEXT("x")), StringSearchCase::CaseSensitive) == false);
            CHECK(String("hello HELLO").StartsWith(StringView(TEXT("hello HELLOx")), StringSearchCase::CaseSensitive) == false);
            CHECK(String("hello HELLO").StartsWith(StringView(TEXT("xhello HELLO")), StringSearchCase::CaseSensitive) == false);
        }
    }

    SECTION("StartsWith, ignore case")
    {
        SECTION("Char")
        {
            CHECK(String("").StartsWith('h', StringSearchCase::IgnoreCase) == false);
            CHECK(String("hello HELLO").StartsWith('h', StringSearchCase::IgnoreCase) == true);
            CHECK(String("hello HELLO").StartsWith('H', StringSearchCase::IgnoreCase) == true);
        }

        SECTION("String")
        {
            CHECK(String("hello HELLO").StartsWith(String(TEXT("hello")), StringSearchCase::IgnoreCase) == true);
            CHECK(String("hello HELLO").StartsWith(String(TEXT("HELLO")), StringSearchCase::IgnoreCase) == true);
            CHECK(String("hello HELLO").StartsWith(String(TEXT("")), StringSearchCase::IgnoreCase) == true);
            CHECK(String("hello HELLO").StartsWith(String(TEXT("xxx")), StringSearchCase::IgnoreCase) == false);

            CHECK(String("").StartsWith(String(TEXT("x")), StringSearchCase::IgnoreCase) == false);
            CHECK(String("hello HELLO").StartsWith(String(TEXT("hello HELLOx")), StringSearchCase::IgnoreCase) == false);
            CHECK(String("hello HELLO").StartsWith(String(TEXT("xhello HELLO")), StringSearchCase::IgnoreCase) == false);
        }

        SECTION("StringView")
        {
            CHECK(String("hello HELLO").StartsWith(StringView(TEXT("hello")), StringSearchCase::IgnoreCase) == true);
            CHECK(String("hello HELLO").StartsWith(StringView(TEXT("HELLO")), StringSearchCase::IgnoreCase) == true);
            CHECK(String("hello HELLO").StartsWith(StringView(), StringSearchCase::IgnoreCase) == true);
            CHECK(String("hello HELLO").StartsWith(StringView(TEXT("")), StringSearchCase::IgnoreCase) == true);
            CHECK(String("hello HELLO").StartsWith(StringView(TEXT("xxx")), StringSearchCase::IgnoreCase) == false);

            CHECK(String("").StartsWith(StringView(TEXT("x")), StringSearchCase::IgnoreCase) == false);
            CHECK(String("hello HELLO").StartsWith(StringView(TEXT("hello HELLOx")), StringSearchCase::IgnoreCase) == false);
            CHECK(String("hello HELLO").StartsWith(StringView(TEXT("xhello HELLO")), StringSearchCase::IgnoreCase) == false);
        }
    }

    SECTION("EndsWith, case sensitive")
    {
        SECTION("Char")
        {
            CHECK(String("").EndsWith('h', StringSearchCase::CaseSensitive) == false);
            CHECK(String("hello HELLO").EndsWith('O', StringSearchCase::CaseSensitive) == true);
            CHECK(String("hello HELLO").EndsWith('o', StringSearchCase::CaseSensitive) == false);
        }

        SECTION("String")
        {
            CHECK(String("hello HELLO").EndsWith(String(TEXT("HELLO")), StringSearchCase::CaseSensitive) == true);
            CHECK(String("hello HELLO").EndsWith(String(TEXT("hello")), StringSearchCase::CaseSensitive) == false);
            CHECK(String("hello HELLO").EndsWith(String(TEXT("")), StringSearchCase::CaseSensitive) == true);
            CHECK(String("hello HELLO").EndsWith(String(TEXT("xxx")), StringSearchCase::CaseSensitive) == false);

            CHECK(String("").EndsWith(String(TEXT("x")), StringSearchCase::CaseSensitive) == false);
            CHECK(String("hello HELLO").EndsWith(String(TEXT("hello HELLOx")), StringSearchCase::CaseSensitive) == false);
            CHECK(String("hello HELLO").EndsWith(String(TEXT("xhello HELLO")), StringSearchCase::CaseSensitive) == false);
        }

        SECTION("StringView")
        {
            CHECK(String("hello HELLO").EndsWith(StringView(TEXT("HELLO")), StringSearchCase::CaseSensitive) == true);
            CHECK(String("hello HELLO").EndsWith(StringView(TEXT("hello")), StringSearchCase::CaseSensitive) == false);
            CHECK(String("hello HELLO").EndsWith(StringView(), StringSearchCase::CaseSensitive) == true);
            CHECK(String("hello HELLO").EndsWith(StringView(TEXT("")), StringSearchCase::CaseSensitive) == true);
            CHECK(String("hello HELLO").EndsWith(StringView(TEXT("xxx")), StringSearchCase::CaseSensitive) == false);

            CHECK(String("").EndsWith(StringView(TEXT("x")), StringSearchCase::CaseSensitive) == false);
            CHECK(String("hello HELLO").EndsWith(StringView(TEXT("hello HELLOx")), StringSearchCase::CaseSensitive) == false);
            CHECK(String("hello HELLO").EndsWith(StringView(TEXT("xhello HELLO")), StringSearchCase::CaseSensitive) == false);
        }
    }

    SECTION("EndsWith, ignore case")
    {
        SECTION("Char")
        {
            CHECK(String("").EndsWith('h', StringSearchCase::IgnoreCase) == false);
            CHECK(String("hello HELLO").EndsWith('O', StringSearchCase::IgnoreCase) == true);
            CHECK(String("hello HELLO").EndsWith('o', StringSearchCase::IgnoreCase) == true);
        }

        SECTION("String")
        {
            CHECK(String("hello HELLO").EndsWith(String(TEXT("HELLO")), StringSearchCase::IgnoreCase) == true);
            CHECK(String("hello HELLO").EndsWith(String(TEXT("hello")), StringSearchCase::IgnoreCase) == true);
            CHECK(String("hello HELLO").EndsWith(String(TEXT("")), StringSearchCase::IgnoreCase) == true);
            CHECK(String("hello HELLO").EndsWith(String(TEXT("xxx")), StringSearchCase::IgnoreCase) == false);

            CHECK(String("").EndsWith(String(TEXT("x")), StringSearchCase::IgnoreCase) == false);
            CHECK(String("hello HELLO").EndsWith(String(TEXT("hello HELLOx")), StringSearchCase::IgnoreCase) == false);
            CHECK(String("hello HELLO").EndsWith(String(TEXT("xhello HELLO")), StringSearchCase::IgnoreCase) == false);
        }

        SECTION("StringView")
        {
            CHECK(String("hello HELLO").EndsWith(StringView(TEXT("HELLO")), StringSearchCase::IgnoreCase) == true);
            CHECK(String("hello HELLO").EndsWith(StringView(TEXT("hello")), StringSearchCase::IgnoreCase) == true);
            CHECK(String("hello HELLO").EndsWith(StringView(), StringSearchCase::IgnoreCase) == true);
            CHECK(String("hello HELLO").EndsWith(StringView(TEXT("")), StringSearchCase::IgnoreCase) == true);
            CHECK(String("hello HELLO").EndsWith(StringView(TEXT("xxx")), StringSearchCase::IgnoreCase) == false);

            CHECK(String("").EndsWith(StringView(TEXT("x")), StringSearchCase::IgnoreCase) == false);
            CHECK(String("hello HELLO").EndsWith(StringView(TEXT("hello HELLOx")), StringSearchCase::IgnoreCase) == false);
            CHECK(String("hello HELLO").EndsWith(StringView(TEXT("xhello HELLO")), StringSearchCase::IgnoreCase) == false);
        }
    }
}

TEST_CASE("String Compare")
{
    SECTION("String")
    {
        SECTION("case sensitive")
        {
            // Empty strings
            CHECK(String("").Compare(String(TEXT("")), StringSearchCase::CaseSensitive) == 0);
            CHECK(String("").Compare(String(TEXT("xxx")), StringSearchCase::CaseSensitive) < 0);
            CHECK(String("xxx").Compare(String(TEXT("")), StringSearchCase::CaseSensitive) > 0);

            // Equal lengths, difference at end
            CHECK(String("xxx").Compare(String(TEXT("xxx")), StringSearchCase::CaseSensitive) == 0);
            CHECK(String("abc").Compare(String(TEXT("abd")), StringSearchCase::CaseSensitive) < 0);
            CHECK(String("abd").Compare(String(TEXT("abc")), StringSearchCase::CaseSensitive) > 0);

            // Equal lengths, difference in the middle
            CHECK(String("abcx").Compare(String(TEXT("abdx")), StringSearchCase::CaseSensitive) < 0);
            CHECK(String("abdx").Compare(String(TEXT("abcx")), StringSearchCase::CaseSensitive) > 0);

            // Different lengths, same prefix
            CHECK(String("abcxx").Compare(String(TEXT("abc")), StringSearchCase::CaseSensitive) > 0);
            CHECK(String("abc").Compare(String(TEXT("abcxx")), StringSearchCase::CaseSensitive) < 0);

            // Different lengths, different prefix
            CHECK(String("abcx").Compare(String(TEXT("abd")), StringSearchCase::CaseSensitive) < 0);
            CHECK(String("abd").Compare(String(TEXT("abcx")), StringSearchCase::CaseSensitive) > 0);
            CHECK(String("abc").Compare(String(TEXT("abdx")), StringSearchCase::CaseSensitive) < 0);
            CHECK(String("abdx").Compare(String(TEXT("abc")), StringSearchCase::CaseSensitive) > 0);

            // Case differences
            CHECK(String("a").Compare(String(TEXT("A")), StringSearchCase::CaseSensitive) > 0);
            CHECK(String("A").Compare(String(TEXT("a")), StringSearchCase::CaseSensitive) < 0);

            // Operators
            CHECK(String(TEXT("")) == String(TEXT("")));
            CHECK(String(TEXT("xx")) != String(TEXT("")));
            CHECK(!(String(TEXT("abcx")) == String(TEXT("xxx"))));
            CHECK(String(TEXT("abcx")) != String(TEXT("xxx")));
            CHECK(String(TEXT("xxx")) == String(TEXT("xxx")));
            CHECK(!(String(TEXT("xxx")) != String(TEXT("xxx"))));
        }

        SECTION("ignore case")
        {
            // Empty strings
            CHECK(String("").Compare(String(TEXT("")), StringSearchCase::IgnoreCase) == 0);
            CHECK(String("").Compare(String(TEXT("xxx")), StringSearchCase::IgnoreCase) < 0);
            CHECK(String("xxx").Compare(String(TEXT("")), StringSearchCase::IgnoreCase) > 0);

            // Equal lengths, difference at end
            CHECK(String("xxx").Compare(String(TEXT("xxx")), StringSearchCase::IgnoreCase) == 0);
            CHECK(String("abc").Compare(String(TEXT("abd")), StringSearchCase::IgnoreCase) < 0);
            CHECK(String("abd").Compare(String(TEXT("abc")), StringSearchCase::IgnoreCase) > 0);

            // Equal lengths, difference in the middle
            CHECK(String("abcx").Compare(String(TEXT("abdx")), StringSearchCase::IgnoreCase) < 0);
            CHECK(String("abdx").Compare(String(TEXT("abcx")), StringSearchCase::IgnoreCase) > 0);

            // Different lengths, same prefix
            CHECK(String("abcxx").Compare(String(TEXT("abc")), StringSearchCase::IgnoreCase) > 0);
            CHECK(String("abc").Compare(String(TEXT("abcxx")), StringSearchCase::IgnoreCase) < 0);

            // Different lengths, different prefix
            CHECK(String("abcx").Compare(String(TEXT("abd")), StringSearchCase::IgnoreCase) < 0);
            CHECK(String("abd").Compare(String(TEXT("abcx")), StringSearchCase::IgnoreCase) > 0);
            CHECK(String("abc").Compare(String(TEXT("abdx")), StringSearchCase::IgnoreCase) < 0);
            CHECK(String("abdx").Compare(String(TEXT("abc")), StringSearchCase::IgnoreCase) > 0);

            // Case differences
            CHECK(String("a").Compare(String(TEXT("A")), StringSearchCase::IgnoreCase) == 0);
            CHECK(String("A").Compare(String(TEXT("a")), StringSearchCase::IgnoreCase) == 0);
        }
    }

    SECTION("StringView")
    {
        SECTION("case sensitive")
        {
            // Null string views
            CHECK(StringView().Compare(StringView(), StringSearchCase::CaseSensitive) == 0);
            CHECK(StringView().Compare(StringView(TEXT("xxx")), StringSearchCase::CaseSensitive) < 0);
            CHECK(StringView(TEXT("xxx")).Compare(StringView(), StringSearchCase::CaseSensitive) > 0);

            // Empty strings
            CHECK(StringView(TEXT("")).Compare(StringView(TEXT("")), StringSearchCase::CaseSensitive) == 0);
            CHECK(StringView(TEXT("")).Compare(StringView(TEXT("xxx")), StringSearchCase::CaseSensitive) < 0);
            CHECK(StringView(TEXT("xxx")).Compare(StringView(TEXT("")), StringSearchCase::CaseSensitive) > 0);

            // Equal lengths, difference at end
            CHECK(StringView(TEXT("xxx")).Compare(StringView(TEXT("xxx")), StringSearchCase::CaseSensitive) == 0);
            CHECK(StringView(TEXT("abc")).Compare(StringView(TEXT("abd")), StringSearchCase::CaseSensitive) < 0);
            CHECK(StringView(TEXT("abd")).Compare(StringView(TEXT("abc")), StringSearchCase::CaseSensitive) > 0);

            // Equal lengths, difference in the middle
            CHECK(StringView(TEXT("abcx")).Compare(StringView(TEXT("abdx")), StringSearchCase::CaseSensitive) < 0);
            CHECK(StringView(TEXT("abdx")).Compare(StringView(TEXT("abcx")), StringSearchCase::CaseSensitive) > 0);

            // Different lengths, same prefix
            CHECK(StringView(TEXT("abcxx")).Compare(StringView(TEXT("abc")), StringSearchCase::CaseSensitive) > 0);
            CHECK(StringView(TEXT("abc")).Compare(StringView(TEXT("abcxx")), StringSearchCase::CaseSensitive) < 0);

            // Different lengths, different prefix
            CHECK(StringView(TEXT("abcx")).Compare(StringView(TEXT("abd")), StringSearchCase::CaseSensitive) < 0);
            CHECK(StringView(TEXT("abd")).Compare(StringView(TEXT("abcx")), StringSearchCase::CaseSensitive) > 0);
            CHECK(StringView(TEXT("abc")).Compare(StringView(TEXT("abdx")), StringSearchCase::CaseSensitive) < 0);
            CHECK(StringView(TEXT("abdx")).Compare(StringView(TEXT("abc")), StringSearchCase::CaseSensitive) > 0);

            // Case differences
            CHECK(StringView(TEXT("a")).Compare(StringView(TEXT("A")), StringSearchCase::CaseSensitive) > 0);
            CHECK(StringView(TEXT("A")).Compare(StringView(TEXT("a")), StringSearchCase::CaseSensitive) < 0);

            // Operators
            CHECK(StringView(TEXT("")) == StringView(TEXT("")));
            CHECK(StringView(TEXT("xx")) != StringView(TEXT("")));
            CHECK(!(StringView(TEXT("abcx")) == StringView(TEXT("xxx"))));
            CHECK(StringView(TEXT("abcx")) != StringView(TEXT("xxx")));
            CHECK(StringView(TEXT("xxx")) == StringView(TEXT("xxx")));
            CHECK(!(StringView(TEXT("xxx")) != StringView(TEXT("xxx"))));
        }

        SECTION("ignore case")
        {
            //Null string views
            CHECK(StringView().Compare(StringView(), StringSearchCase::IgnoreCase) == 0);
            CHECK(StringView().Compare(StringView(TEXT("xxx")), StringSearchCase::IgnoreCase) < 0);
            CHECK(StringView(TEXT("xxx")).Compare(StringView(), StringSearchCase::IgnoreCase) > 0);

            // Empty strings
            CHECK(StringView(TEXT("")).Compare(StringView(TEXT("")), StringSearchCase::IgnoreCase) == 0);
            CHECK(StringView(TEXT("")).Compare(StringView(TEXT("xxx")), StringSearchCase::IgnoreCase) < 0);
            CHECK(StringView(TEXT("xxx")).Compare(StringView(TEXT("")), StringSearchCase::IgnoreCase) > 0);

            // Equal lengths, difference at end
            CHECK(StringView(TEXT("xxx")).Compare(StringView(TEXT("xxx")), StringSearchCase::IgnoreCase) == 0);
            CHECK(StringView(TEXT("abc")).Compare(StringView(TEXT("abd")), StringSearchCase::IgnoreCase) < 0);
            CHECK(StringView(TEXT("abd")).Compare(StringView(TEXT("abc")), StringSearchCase::IgnoreCase) > 0);

            // Equal lengths, difference in the middle
            CHECK(StringView(TEXT("abcx")).Compare(StringView(TEXT("abdx")), StringSearchCase::IgnoreCase) < 0);
            CHECK(StringView(TEXT("abdx")).Compare(StringView(TEXT("abcx")), StringSearchCase::IgnoreCase) > 0);

            // Different lengths, same prefix
            CHECK(StringView(TEXT("abcxx")).Compare(StringView(TEXT("abc")), StringSearchCase::IgnoreCase) > 0);
            CHECK(StringView(TEXT("abc")).Compare(StringView(TEXT("abcxx")), StringSearchCase::IgnoreCase) < 0);

            // Different lengths, different prefix
            CHECK(StringView(TEXT("abcx")).Compare(StringView(TEXT("abd")), StringSearchCase::IgnoreCase) < 0);
            CHECK(StringView(TEXT("abd")).Compare(StringView(TEXT("abcx")), StringSearchCase::IgnoreCase) > 0);
            CHECK(StringView(TEXT("abc")).Compare(StringView(TEXT("abdx")), StringSearchCase::IgnoreCase) < 0);
            CHECK(StringView(TEXT("abdx")).Compare(StringView(TEXT("abc")), StringSearchCase::IgnoreCase) > 0);

            // Case differences
            CHECK(StringView(TEXT("a")).Compare(StringView(TEXT("A")), StringSearchCase::IgnoreCase) == 0);
            CHECK(StringView(TEXT("A")).Compare(StringView(TEXT("a")), StringSearchCase::IgnoreCase) == 0);
        }
    }
}
