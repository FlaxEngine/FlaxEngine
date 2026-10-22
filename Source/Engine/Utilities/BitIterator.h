// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

// Small utility to iterate over all set bits from a given mask.
struct BitIterator
{
    // Iterates over the mask of bits.
    struct Iterator
    {
        Iterator(uint32 flags)
            : _remaining(flags)
            , _current(flags ? (flags & (~flags + 1u)) : 0u)
        {
        }

        uint32 operator*() const
        {
            return _current;
        }

        Iterator& operator++()
        {
            // Clear lowest set bit and move to the next one
            _remaining &= _remaining - 1;
            _current = _remaining ? (_remaining & (~_remaining + 1u)) : 0u;
            return *this;
        }

        friend bool operator!=(const Iterator& a, const Iterator& b)
        {
            return a._current != b._current;
        }
    private:
        uint32 _remaining, _current;
    };

    uint32 Flags;

    BitIterator(uint32 flags)
        : Flags(flags)
    {
    }

    template<typename T>
    BitIterator(T flags)
        : Flags((uint32)flags)
    {
        static_assert(__is_enum(T), "Only enum or uint32 types are supported.");
    }

    Iterator begin() const { return Iterator(Flags); }
    Iterator end() const { return Iterator(0u); }
};
