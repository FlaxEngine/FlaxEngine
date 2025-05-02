// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Core/Types/Nullable.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("Nullable")
{
    SECTION("Trivial Type")
    {
        Nullable<int> a;

        REQUIRE(a.HasValue() == false);
        REQUIRE(a.GetValueOr(2) == 2);

        a = 1;

        REQUIRE(a.HasValue() == true);
        REQUIRE(a.GetValue() == 1);
        REQUIRE(a.GetValueOr(2) == 1);

        a.Reset();

        REQUIRE(a.HasValue() == false);
    }

    SECTION("Move-Only Type")
    {
        struct MoveOnly
        {
            MoveOnly()  = default;
            ~MoveOnly() = default;

            MoveOnly(const MoveOnly&) = delete;
            MoveOnly(MoveOnly&&)      = default;

            MoveOnly& operator=(const MoveOnly&) = delete;
            MoveOnly& operator=(MoveOnly&&)      = default;
        };

        Nullable<MoveOnly> a;

        REQUIRE(a.HasValue() == false);

        a = MoveOnly();

        REQUIRE(a.HasValue() == true);
    }

    SECTION("Bool Type")
    {
        Nullable<bool> a;

        REQUIRE(a.HasValue() == false);
        REQUIRE(a.GetValueOr(true) == true);
        REQUIRE(a.IsTrue() == false);
        REQUIRE(a.IsFalse() == false);

        a = false;

        REQUIRE(a.HasValue() == true);
        REQUIRE(a.GetValue() == false);
        REQUIRE(a.GetValueOr(true) == false);

        REQUIRE(a.IsTrue() == false);
        REQUIRE(a.IsFalse() == true);

        a = true;

        REQUIRE(a.IsTrue() == true);
        REQUIRE(a.IsFalse() == false);

        a.Reset();

        REQUIRE(a.HasValue() == false);
    }

    SECTION("Lifetime (No Construction)")
    {
        struct DoNotConstruct
        {
            DoNotConstruct() { FAIL("DoNotConstruct must not be constructed."); }
        };

        Nullable<DoNotConstruct> a;
        a.Reset();
    }

    SECTION("Lifetime")
    {
        struct Lifetime
        {
            int* _constructed;
            int* _destructed;

            Lifetime(int* constructed, int* destructed)
                : _constructed(constructed)
                , _destructed(destructed)
            {
                ++(*_constructed);
            }

            Lifetime(Lifetime&& other) noexcept
                : _constructed(other._constructed)
                , _destructed(other._destructed)
            {
                ++(*_constructed);
            }

            Lifetime() = delete;
            Lifetime& operator=(const Lifetime&) = delete;
            Lifetime& operator=(Lifetime&&) = delete;

            ~Lifetime()
            {
                ++(*_destructed);
            }
        };

        int constructed = 0, destructed = 0;
        REQUIRE(constructed == destructed);

        {

            Nullable<Lifetime> a = Lifetime(&constructed, &destructed);
            REQUIRE(a.HasValue());
            REQUIRE(constructed == destructed + 1);

            a.Reset();
            REQUIRE(!a.HasValue());
            REQUIRE(constructed == destructed);
        }
        REQUIRE(constructed == destructed);

        {
            Nullable<Lifetime> b = Lifetime(&constructed, &destructed);
            REQUIRE(constructed == destructed + 1);
        }
        REQUIRE(constructed == destructed);

        {
            Nullable<Lifetime> c = Lifetime(&constructed, &destructed);
            Nullable<Lifetime> d = MoveTemp(c);
            REQUIRE(constructed == destructed + 1);
        }
        REQUIRE(constructed == destructed);
    }

    SECTION("Matching")
    {
        Nullable<int> a;
        Nullable<int> b = 2;

        a.Match(
            [](int) { FAIL("Null nullable must not match value handler."); },
            []() {}
        );

        b.Match(
            [](int) {},
            []() { FAIL("Nullable with valid value must not match null handler."); }
        );
    }
};
