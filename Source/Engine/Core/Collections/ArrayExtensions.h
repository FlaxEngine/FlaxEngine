// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "../Collections/Array.h"
#include "../Collections/Dictionary.h"
#include "../Delegate.h"

class ArrayExtensions;

/// <summary>
/// Represents a collection of objects that have a common key.
/// </summary>
template<typename TKey, typename TSource>
class IGrouping : public Array<TSource>
{
    friend ArrayExtensions;

protected:
    TKey _key;

public:
    /// <summary>
    /// Gets the common key.
    /// </summary>
    FORCE_INLINE const TKey& GetKey() const
    {
        return _key;
    }

    /// <summary>
    /// Gets the common key.
    /// </summary>
    FORCE_INLINE TKey GetKey()
    {
        return _key;
    }
};

/// <summary>
/// Array collection extension methods and helpers.
/// </summary>
class ArrayExtensions
{
public:
    /// <summary>
    /// Searches for the specified object using a custom query and returns the zero-based index of the first occurrence within the entire collection.
    /// </summary>
    /// <param name="obj">The target collection.</param>
    /// <param name="predicate">The prediction function. Should return true for the target element to find.</param>
    /// <returns>The index of the element or -1 if nothing found.</returns>
    template<typename T, typename AllocationType>
    static int32 IndexOf(const Array<T, AllocationType>& obj, const Function<bool(const T&)>& predicate)
    {
        for (int32 i = 0; i < obj.Count(); i++)
        {
            if (predicate(obj[i]))
            {
                return i;
            }
        }
        return INVALID_INDEX;
    }

    /// <summary>
    /// The Any operator checks, if there are any elements in the collection matching the predicate. It does not select the element, but returns true if at least one element is matched.
    /// </summary>
    /// <param name="obj">The target collection.</param>
    /// <param name="predicate">The prediction function.</param>
    /// <returns>True if any element in the collection matches the prediction, otherwise false.</returns>
    template<typename T, typename AllocationType>
    static bool Any(const Array<T, AllocationType>& obj, const Function<bool(const T&)>& predicate)
    {
        for (int32 i = 0; i < obj.Count(); i++)
        {
            if (predicate(obj[i]))
            {
                return true;
            }
        }
        return false;
    }

    /// <summary>
    /// All Any operator returns true if all elements match the predicate. It does not select the element, but returns true if all elements are matching.
    /// </summary>
    /// <param name="obj">The target collection.</param>
    /// <param name="predicate">The prediction function.</param>
    /// <returns>True if all elements in the collection matches the prediction, otherwise false.</returns>
    template<typename T, typename AllocationType>
    static int32 All(const Array<T, AllocationType>& obj, const Function<bool(const T&)>& predicate)
    {
        for (int32 i = 0; i < obj.Count(); i++)
        {
            if (!predicate(obj[i]))
            {
                return false;
            }
        }
        return true;
    }

    /// <summary>
    /// Groups the elements of a sequence according to a specified key selector function.
    /// </summary>
    /// <param name="obj">The collection whose elements to group.</param>
    /// <param name="keySelector">A function to extract the key for each element.</param>
    /// <param name="result">The result collection with groups.</param>
    template<typename TSource, typename TKey, typename AllocationType>
    static void GroupBy(const Array<TSource, AllocationType>& obj, const Function<TKey(TSource const&)>& keySelector, Array<IGrouping<TKey, TSource>, AllocationType>& result)
    {
        Dictionary<TKey, IGrouping<TKey, TSource>> data(static_cast<int32>(obj.Count() * 3.0f));
        for (int32 i = 0; i < obj.Count(); i++)
        {
            const TKey key = keySelector(obj[i]);
            auto& group = data[key];
            group._key = key;
            group.Add(obj[i]);
        }
        result.Clear();
        data.GetValues(result);
    }
};
