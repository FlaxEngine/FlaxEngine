// Copyright (c) Wojciech Figat. All rights reserved.

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
                return i;
        }
        return INVALID_INDEX;
    }

    /// <summary>
    /// Searches for the specified object using a custom query and returns it or default value.
    /// </summary>
    /// <param name="obj">The target collection.</param>
    /// <param name="predicate">The prediction function. Should return true for the target element to find.</param>
    /// <returns>The first found item or default value if nothing found.</returns>
    template<typename T, typename AllocationType>
    static T First(const Array<T, AllocationType>& obj, const Function<bool(const T&)>& predicate)
    {
        for (int32 i = 0; i < obj.Count(); i++)
        {
            if (predicate(obj[i]))
                return obj[i];
        }
        return T();
    }

    /// <summary>
    /// Searches for the specified object using a custom query and returns it or default value.
    /// </summary>
    /// <param name="obj">The target collection.</param>
    /// <param name="predicate">The prediction function. Should return true for the target element to find.</param>
    /// <returns>The first found item or default value if nothing found.</returns>
    template<typename T, typename AllocationType>
    static T* First(const Array<T*, AllocationType>& obj, const Function<bool(const T*)>& predicate)
    {
        for (int32 i = 0; i < obj.Count(); i++)
        {
            if (predicate(obj[i]))
                return obj[i];
        }
        return nullptr;
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
                return true;
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
                return false;
        }
        return true;
    }

    /// <summary>
    /// All Any operator returns true if all elements match the predicate. It does not select the element, but returns true if all elements are matching.
    /// </summary>
    /// <param name="obj">The target collection.</param>
    /// <param name="predicate">The prediction function.</param>
    /// <returns>True if all elements in the collection matches the prediction, otherwise false.</returns>
    template<typename T, typename AllocationType>
    static int32 All(const Array<T*, AllocationType>& obj, const Function<bool(const T*)>& predicate)
    {
        for (int32 i = 0; i < obj.Count(); i++)
        {
            if (!predicate(obj[i]))
                return false;
        }
        return true;
    }

    /// <summary>
    /// Filters a sequence of values based on a predicate.
    /// </summary>
    /// <param name="obj">The target collection.</param>
    /// <param name="predicate">The prediction function. Return true for elements that should be included in result list.</param>
    /// <param name="result">The result list with items that passed the predicate.</param>
    template<typename T, typename AllocationType>
    static void Where(const Array<T, AllocationType>& obj, const Function<bool(const T&)>& predicate, Array<T, AllocationType>& result)
    {
        for (const T& i : obj)
        {
            if (predicate(i))
                result.Add(i);
        }
    }

    /// <summary>
    /// Filters a sequence of values based on a predicate.
    /// </summary>
    /// <param name="obj">The target collection.</param>
    /// <param name="predicate">The prediction function. Return true for elements that should be included in result list.</param>
    /// <returns>The result list with items that passed the predicate.</returns>
    template<typename T, typename AllocationType>
    static Array<T, AllocationType> Where(const Array<T, AllocationType>& obj, const Function<bool(const T&)>& predicate)
    {
        Array<T, AllocationType> result;
        Where(obj, predicate, result);
        return result;
    }

    /// <summary>
    /// Projects each element of a sequence into a new form.
    /// </summary>
    /// <param name="obj">The target collection.</param>
    /// <param name="selector">A transform function to apply to each source element; the second parameter of the function represents the index of the source element.</param>
    /// <param name="result">The result list whose elements are the result of invoking the transform function on each element of source.</param>
    template<typename TResult, typename TSource, typename AllocationType>
    static void Select(const Array<TSource, AllocationType>& obj, const Function<TResult(const TSource&)>& selector, Array<TResult, AllocationType>& result)
    {
        for (const TSource& i : obj)
            result.Add(MoveTemp(selector(i)));
    }

    /// <summary>
    /// Projects each element of a sequence into a new form.
    /// </summary>
    /// <param name="obj">The target collection.</param>
    /// <param name="selector">A transform function to apply to each source element; the second parameter of the function represents the index of the source element.</param>
    /// <returns>The result list whose elements are the result of invoking the transform function on each element of source.</returns>
    template<typename TResult, typename TSource, typename AllocationType>
    static Array<TResult, AllocationType> Select(const Array<TSource, AllocationType>& obj, const Function<TResult(const TSource&)>& selector)
    {
        Array<TResult, AllocationType> result;
        Select(obj, selector, result);
        return result;
    }

    /// <summary>
    /// Removes all the elements that match the conditions defined by the specified predicate.
    /// </summary>
    /// <param name="obj">The target collection to modify.</param>
    /// <param name="predicate">A transform function that defines the conditions of the elements to remove.</param>
    template<typename T, typename AllocationType>
    static void RemoveAll(Array<T, AllocationType>& obj, const Function<bool(const T&)>& predicate)
    {
        for (int32 i = obj.Count() - 1; i >= 0; i--)
        {
            if (predicate(obj[i]))
                obj.RemoveAtKeepOrder(i);
        }
    }

    /// <summary>
    /// Removes all the elements that match the conditions defined by the specified predicate.
    /// </summary>
    /// <param name="obj">The target collection to process.</param>
    /// <param name="predicate">A transform function that defines the conditions of the elements to remove.</param>
    /// <returns>The result list whose elements are the result of invoking the transform function on each element of source.</returns>
    template<typename T, typename AllocationType>
    static Array<T, AllocationType> RemoveAll(const Array<T, AllocationType>& obj, const Function<bool(const T&)>& predicate)
    {
        Array<T, AllocationType> result;
        for (const T& i : obj)
        {
            if (!predicate(i))
                result.Ass(i);
        }
        return result;
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
        Dictionary<TKey, IGrouping<TKey, TSource>> data;
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
