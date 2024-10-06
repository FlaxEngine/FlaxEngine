// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
			MoveOnly() = default;
            ~MoveOnly() = default;

			MoveOnly(const MoveOnly&) = delete;
			MoveOnly(MoveOnly&&) = default;

			// MoveOnly& operator=(const MoveOnly&) = delete;
			// MoveOnly& operator=(MoveOnly&&) = default;
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
	    int constructed = 0, destructed = 0;

        struct Lifetime
		{
			int& constructed;
			int& destructed;

			Lifetime(int& constructed, int& destructed)
				: constructed(constructed)
				, destructed(destructed)
			{
				constructed++;
			}

			Lifetime(const Lifetime& other)
				: constructed(other.constructed)
				, destructed(other.destructed)
			{
				constructed++;
			}

			~Lifetime()
			{
				destructed++;
			}
		};

		{
			Nullable<Lifetime> a = Lifetime(constructed, destructed);

			REQUIRE(constructed == 1);
			REQUIRE(destructed == 0);

			a.Reset();

			REQUIRE(constructed == 1);
			REQUIRE(destructed == 1);
		}

	    {
		    Nullable<Lifetime> a = Lifetime(constructed, destructed);
	    }

		REQUIRE(constructed == 2);
		REQUIRE(destructed == 2);
	}

    SECTION("Matching")
    {
	    Nullable<int> a;
		Nullable<int> b = 2;

        a.Match(
            [](int value) { FAIL("Null nullable must not match value handler."); },
			[]() {}
        );

        b.Match(
			[](int value) {},
            []() { FAIL("Nullable with valid value must not match null handler."); }
		);
    }
};
