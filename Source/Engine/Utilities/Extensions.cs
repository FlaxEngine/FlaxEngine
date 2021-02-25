// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace FlaxEngine.Utilities
{
    /// <summary>
    /// Collection of various extension methods.
    /// </summary>
    public static partial class Extensions
    {
        /// <summary>
        /// Creates deep clone for a class if all members of this class are marked as serializable (uses Json serialization).
        /// </summary>
        /// <param name="instance">The input instance of an object.</param>
        /// <typeparam name="T">The instance type of an object.</typeparam>
        /// <returns>Returns new object of provided class.</returns>
        public static T DeepClone<T>(this T instance)
        where T : new()
        {
            var json = Json.JsonSerializer.Serialize(instance);
            return Json.JsonSerializer.Deserialize<T>(json);
        }

        /// <summary>
        /// Creates raw clone for a structure using memory copy. Valid only for value types.
        /// </summary>
        /// <param name="instance">The input instance of an object.</param>
        /// <typeparam name="T">The instance type of an object.</typeparam>
        /// <returns>Returns new object of provided structure.</returns>
        public static T RawClone<T>(this T instance)
        {
            IntPtr ptr = Marshal.AllocHGlobal(Marshal.SizeOf(instance));
            try
            {
                Marshal.StructureToPtr(instance, ptr, false);
                return (T)Marshal.PtrToStructure(ptr, instance.GetType());
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }
        }

        /// <summary>
        /// Splits string into lines
        /// </summary>
        /// <param name="str">Text to split</param>
        /// <param name="removeEmptyLines">True if remove empty lines, otherwise keep them</param>
        /// <returns>Array with all lines</returns>
        public static string[] GetLines(this string str, bool removeEmptyLines = false)
        {
            return str.Split(new[]
            {
                "\r\n",
                "\r",
                "\n"
            }, removeEmptyLines ? StringSplitOptions.RemoveEmptyEntries : StringSplitOptions.None);
        }

        /// <summary>
        /// Gets a random double.
        /// </summary>
        /// <param name="random">The random.</param>
        /// <param name="maxValue">The maximum value</param>
        /// <returns>A random double</returns>
        public static double NextDouble(this Random random, double maxValue)
        {
            return random.NextDouble() * maxValue;
        }

        /// <summary>
        /// Gets a random double.
        /// </summary>
        /// <param name="random">The random.</param>
        /// <param name="minValue">The minimum value</param>
        /// <param name="maxValue">The maximum value</param>
        /// <returns>A random double</returns>
        public static double NextDouble(this Random random, double minValue, double maxValue)
        {
            return random.NextDouble() * (maxValue - minValue) + minValue;
        }

        /// <summary>
        /// Gets a random float.
        /// </summary>
        /// <param name="random">The random.</param>
        /// <returns>A random float</returns>
        public static float NextFloat(this Random random)
        {
            return (float)random.NextDouble();
        }

        /// <summary>
        /// Gets a random float.
        /// </summary>
        /// <param name="random">The random.</param>
        /// <param name="maxValue">The maximum value</param>
        /// <returns>A random float</returns>
        public static float NextFloat(this Random random, float maxValue)
        {
            return (float)random.NextDouble() * maxValue;
        }

        /// <summary>
        /// Gets a random float.
        /// </summary>
        /// <param name="random">The random.</param>
        /// <param name="minValue">The minimum value</param>
        /// <param name="maxValue">The maximum value</param>
        /// <returns></returns>
        public static float NextFloat(this Random random, float minValue, float maxValue)
        {
            return (float)random.NextDouble() * (maxValue - minValue) + minValue;
        }

        /// <summary>
        /// Gets a random Color.
        /// </summary>
        /// <param name="random">The random.</param>
        /// <returns></returns>
        public static Color NextColor(this Random random)
        {
            return new Color(NextFloat(random, 1.0f),
                             NextFloat(random, 1.0f),
                             NextFloat(random, 1.0f),
                             NextFloat(random, 1.0f));
        }

        /// <summary>
        /// Gets a random Vector2 with components in range [0;1].
        /// </summary>
        /// <param name="random">The random.</param>
        /// <returns></returns>
        public static Vector2 NextVector2(this Random random)
        {
            return new Vector2(NextFloat(random, 1.0f),
                               NextFloat(random, 1.0f));
        }

        /// <summary>
        /// Gets a random Vector3 with components in range [0;1].
        /// </summary>
        /// <param name="random">The random.</param>
        /// <returns></returns>
        public static Vector3 NextVector3(this Random random)
        {
            return new Vector3(NextFloat(random, 1.0f),
                               NextFloat(random, 1.0f),
                               NextFloat(random, 1.0f));
        }

        /// <summary>
        /// Gets a random Vector4 with components in range [0;1].
        /// </summary>
        /// <param name="random">The random.</param>
        /// <returns></returns>
        public static Vector4 NextVector4(this Random random)
        {
            return new Vector4(NextFloat(random, 1.0f),
                               NextFloat(random, 1.0f),
                               NextFloat(random, 1.0f),
                               NextFloat(random, 1.0f));
        }

        /// <summary>
        /// Gets a random Quaternion.
        /// </summary>
        /// <param name="random">The random.</param>
        /// <returns></returns>
        public static Quaternion NextQuaternion(this Random random)
        {
            return Quaternion.Euler(NextFloat(random, -180, 180),
                                    NextFloat(random, -180, 180),
                                    NextFloat(random, -180, 180));
        }

        /// <summary>
        /// Gets a random 64-bit signed integer value.
        /// </summary>
        /// <param name="random">The random.</param>
        /// <returns></returns>
        internal static long NextLong(this Random random)
        {
            var numArray = new byte[8];
            random.NextBytes(numArray);
            return (long)(BitConverter.ToUInt64(numArray, 0) & 9223372036854775807L);
        }

        /// <summary>
        /// Generates a random normalized 2D direction vector.
        /// </summary>
        /// <param name="random">An instance of <see cref="Random"/>.</param>
        /// <returns>A random normalized 2D direction vector.</returns>
        public static Vector2 NextDirection2D(this Random random)
        {
            return Vector2.Normalize(new Vector2((float)random.NextDouble(), (float)random.NextDouble()));
        }

        /// <summary>
        /// Generates a random normalized 3D direction vector.
        /// </summary>
        /// <param name="random">An instance of <see cref="Random"/>.</param>
        /// <returns>A random normalized 3D direction vector.</returns>
        public static Vector3 NextDirection3D(this Random random)
        {
            return Vector3.Normalize(new Vector3((float)random.NextDouble(), (float)random.NextDouble(), (float)random.NextDouble()));
        }

        /// <summary>
        /// Generates a random point in a circle of a given radius.
        /// </summary>
        /// <param name="random">An instance of <see cref="Random"/>.</param>
        /// <param name="radius">Radius of circle. Default 1.0f.</param>
        /// <returns>A random point in a circle of a given radius.</returns>
        public static Vector2 PointInACircle(this Random random, float radius = 1.0f)
        {
            var randomRadius = (float)random.NextDouble() * radius;

            return new Vector2
            {
                X = (float)Math.Cos(random.NextDouble()) * randomRadius,
                Y = (float)Math.Sin(random.NextDouble()) * randomRadius,
            };
        }

        /// <summary>
        /// Adds the elements of the specified collection to the end of the <see cref="ICollection{T}"/>.
        /// </summary>
        /// <typeparam name="T">The type of elements in the collection.</typeparam>
        /// <param name="destination">The <see cref="ICollection{T}"/> to add items to.</param>
        /// <param name="collection">The collection whose elements should be added to the end of the <paramref name="destination"/>. It can contain elements that are <see langword="null"/>, if type <typeparamref name="T"/> is a reference type.</param>
        /// <exception cref="ArgumentNullException">If <paramref name="destination"/> or <paramref name="collection"/> are <see langword="null"/>.</exception>
        public static void AddRange<T>(this ICollection<T> destination, IEnumerable<T> collection)
        {
            if (destination == null)
            {
                throw new ArgumentNullException(nameof(destination));
            }

            if (collection == null)
            {
                throw new ArgumentNullException(nameof(collection));
            }

            foreach (var item in collection)
            {
                destination.Add(item);
            }
        }

        /// <summary>
        /// Enqueues the elements of the specified collection to the <see cref="Queue{T}"/>.
        /// </summary>
        /// <typeparam name="T">The type of elements in the collection.</typeparam>
        /// <param name="queue">The <see cref="Queue{T}"/> to add items to.</param>
        /// <param name="collection">The collection whose elements should be added to the <paramref name="queue"/>. It can contain elements that are <see langword="null"/>, if type <typeparamref name="T"/> is a reference type.</param>
        /// <exception cref="ArgumentNullException">If <paramref name="queue"/> or <paramref name="collection"/> are <see langword="null"/>.</exception>
        public static void EnqueueRange<T>(this Queue<T> queue, IEnumerable<T> collection)
        {
            if (queue == null)
            {
                throw new ArgumentNullException(nameof(queue));
            }

            if (collection == null)
            {
                throw new ArgumentNullException(nameof(collection));
            }

            foreach (var item in collection)
            {
                queue.Enqueue(item);
            }
        }

        /// <summary>
        /// Pushes the elements of the specified collection to the <see cref="Stack{T}"/>.
        /// </summary>
        /// <typeparam name="T">The type of elements in the collection.</typeparam>
        /// <param name="stack">The <see cref="Stack{T}"/> to add items to.</param>
        /// <param name="collection">The collection whose elements should be pushed on to the <paramref name="stack"/>. It can contain elements that are <see langword="null"/>, if type <typeparamref name="T"/> is a reference type.</param>
        /// <exception cref="ArgumentNullException">If <paramref name="stack"/> or <paramref name="collection"/> are <see langword="null"/>.</exception>
        public static void PushRange<T>(this Stack<T> stack, IEnumerable<T> collection)
        {
            if (stack == null)
            {
                throw new ArgumentNullException(nameof(stack));
            }

            if (collection == null)
            {
                throw new ArgumentNullException(nameof(collection));
            }

            foreach (var item in collection)
            {
                stack.Push(item);
            }
        }

        /// <summary>
        /// Performs the specified action on each element of the <see cref="IEnumerable{T}"/>.
        /// </summary>
        /// <typeparam name="T">The type of the elements of the input sequence.</typeparam>
        /// <param name="source">The sequence of elements to execute the <see cref="IEnumerable{T}"/>.</param>
        /// <param name="action">The <see cref="Action{T}"/> delegate to perform on each element of the <see cref="IEnumerable{T}"/>1.</param>
        /// <exception cref="ArgumentException"><paramref name="source"/> or <paramref name="action"/> is <see langword="null"/>.</exception>
        public static void ForEach<T>(this IEnumerable<T> source, Action<T> action)
        {
            if (source == null)
            {
                throw new ArgumentNullException(nameof(source));
            }

            if (action == null)
            {
                throw new ArgumentNullException(nameof(action));
            }

            foreach (var item in source)
            {
                action(item);
            }
        }

        /// <summary>
        /// Chooses a random item from the collection.
        /// </summary>
        /// <typeparam name="T">The type of the elements of the input sequence.</typeparam>
        /// <param name="random">An instance of <see cref="Random"/>.</param>
        /// <param name="collection">Collection to choose item from.</param>
        /// <returns>A random item from collection</returns>
        /// <exception cref="ArgumentNullException">If the random argument is null.</exception>
        /// <exception cref="ArgumentNullException">If the collection is null.</exception>
        public static T Choose<T>(this Random random, IList<T> collection)
        {
            if (random == null)
            {
                throw new ArgumentNullException(nameof(random));
            }

            if (collection == null)
            {
                throw new ArgumentNullException(nameof(collection));
            }

            return collection[random.Next(collection.Count)];
        }

        /// <summary>
        /// Chooses a random item.
        /// </summary>
        /// <typeparam name="T">The type of the elements of the input sequence.</typeparam>
        /// <param name="random">An instance of <see cref="Random"/>.</param>
        /// <param name="collection">Collection to choose item from.</param>
        /// <returns>A random item from collection</returns>
        /// <exception cref="ArgumentNullException">If the random  is null.</exception>
        /// <exception cref="ArgumentNullException">If the collection is null.</exception>
        public static T Choose<T>(this Random random, params T[] collection)
        {
            if (random == null)
            {
                throw new ArgumentNullException(nameof(random));
            }

            if (collection == null)
            {
                throw new ArgumentNullException(nameof(collection));
            }

            return collection[random.Next(collection.Length)];
        }

        /// <summary>
        /// Shuffles the collection in place.
        /// </summary>
        /// <typeparam name="T">The type of the elements of the input sequence.</typeparam>
        /// <param name="random">An instance of <see cref="Random"/>.</param>
        /// <param name="collection">Collection to shuffle.</param>
        /// <exception cref="ArgumentNullException">If the random argument is null.</exception>
        /// <exception cref="ArgumentNullException">If the random collection is null.</exception>
        public static void Shuffle<T>(this Random random, IList<T> collection)
        {
            if (random == null)
            {
                throw new ArgumentNullException(nameof(random));
            }

            if (collection == null)
            {
                throw new ArgumentNullException(nameof(collection));
            }

            int n = collection.Count;
            while (n > 1)
            {
                n--;
                int k = random.Next(n + 1);
                T value = collection[k];
                collection[k] = collection[n];
                collection[n] = value;
            }
        }
    }
}
