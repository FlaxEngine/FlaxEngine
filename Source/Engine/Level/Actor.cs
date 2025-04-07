// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    partial class Actor
    {
        /// <summary>
        /// Returns true if object is fully static on the scene, otherwise false.
        /// </summary>
        [Unmanaged]
        [Tooltip("Returns true if object is fully static on the scene, otherwise false.")]
        public bool IsStatic => StaticFlags == StaticFlags.FullyStatic;

        /// <summary>
        /// Returns true if object has static transform.
        /// </summary>
        [Unmanaged]
        [Tooltip("Returns true if object has static transform.")]
        public bool IsTransformStatic => (StaticFlags & StaticFlags.Transform) == StaticFlags.Transform;

        /// <summary>
        /// Adds the actor static flags.
        /// </summary>
        /// <param name="flags">The flags to add.</param>
        [Unmanaged]
        [Tooltip("Adds the actor static flags.")]
        public void AddStaticFlags(StaticFlags flags)
        {
            StaticFlags |= flags;
        }

        /// <summary>
        /// Removes the actor static flags.
        /// </summary>
        /// <param name="flags">The flags to remove.</param>
        [Unmanaged]
        [Tooltip("Removes the actor static flags.")]
        public void RemoveStaticFlags(StaticFlags flags)
        {
            StaticFlags &= ~flags;
        }

        /// <summary>
        /// Sets a single static flag to the desire value.
        /// </summary>
        /// <param name="flag">The flag to change.</param>
        /// <param name="value">The target value of the flag.</param>
        [Unmanaged]
        [Tooltip("Sets a single static flag to the desire value.")]
        public void SetStaticFlag(StaticFlags flag, bool value)
        {
            StaticFlags = StaticFlags & ~flag | (value ? flag : StaticFlags.None);
        }

        /// <summary>
        /// Returns true if object has given flag(s) set.
        /// </summary>
        /// <param name="flag">The flag(s) to check.</param>
        /// <returns>True if has flag(s) set, otherwise false.</returns>
        [Unmanaged]
        [Tooltip("Returns true if object has given flag(s) set.")]
        public bool HasStaticFlag(StaticFlags flag)
        {
            return (StaticFlags & flag) == flag;
        }

        /// <summary>
        /// The rotation as Euler angles in degrees.
        /// </summary>
        /// <remarks>
        /// The x, y, and z angles represent a rotation z degrees around the z axis, x degrees around the x axis, and y degrees around the y axis (in that order).
        /// Angles order (xyz): pitch, yaw and roll.
        /// </remarks>
        [HideInEditor, NoSerialize, NoAnimate]
        public Float3 EulerAngles
        {
            get => Orientation.EulerAngles;
            set
            {
                Quaternion.Euler(ref value, out var orientation);
                Internal_SetOrientation(__unmanagedPtr, ref orientation);
            }
        }

        /// <summary>
        /// The local rotation as Euler angles in degrees.
        /// </summary>
        /// <remarks>
        /// The x, y, and z angles represent a rotation z degrees around the z axis, x degrees around the x axis, and y degrees around the y axis (in that order).
        /// Angles order (xyz): pitch, yaw and roll.
        /// </remarks>
        [HideInEditor, NoSerialize, NoAnimate]
        public Float3 LocalEulerAngles
        {
            get => LocalOrientation.EulerAngles;
            set
            {
                Quaternion.Euler(ref value, out var orientation);
                Internal_SetLocalOrientation(__unmanagedPtr, ref orientation);
            }
        }

        /// <summary>
        /// Returns true if actor has any children
        /// </summary>
        [HideInEditor, NoSerialize]
        public bool HasChildren => Internal_GetChildrenCount(__unmanagedPtr) != 0;

        /// <summary>
        /// Resets actor local transform.
        /// </summary>
        public void ResetLocalTransform()
        {
            LocalTransform = Transform.Identity;
        }

        /// <summary>
        /// Creates a new child actor of the given type.
        /// </summary>
        /// <param name="type">Type of the actor.</param>
        /// <returns>The child actor.</returns>
        public Actor AddChild(Type type)
        {
            if (type.IsAbstract)
                return null;

            var result = (Actor)New(type);
            result.SetParent(this, false, false);
            return result;
        }

        /// <summary>
        /// Creates a new child actor of the given type.
        /// </summary>
        /// <typeparam name="T">Type of the actor to search for. Includes any actors derived from the type.</typeparam>
        /// <returns>The child actor.</returns>
        public T AddChild<T>() where T : Actor
        {
            if (typeof(T).IsAbstract)
                return null;

            var result = New<T>();
            result.SetParent(this, false, false);
            return result;
        }

        /// <summary>
        /// Finds the child actor of the given type.
        /// </summary>
        /// <typeparam name="T">Type of the actor to search for. Includes any actors derived from the type.</typeparam>
        /// <returns>The child actor or null if failed to find.</returns>
        public T GetChild<T>() where T : Actor
        {
            return GetChild(typeof(T)) as T;
        }

        /// <summary>
        /// Tries to the child actor of the given type.
        /// </summary>
        /// <typeparam name="T">Type of the actor to search for. Includes any actors derived from the type.</typeparam>
        /// <param name="actor">The returned actor, valid only if method returns true.</param>
        /// <returns>True if found an child actor of that type or false if failed to find.</returns>
        public bool TryGetChild<T>(out T actor) where T : Actor
        {
            actor = GetChild(typeof(T)) as T;
            return (object)actor != null;
        }

        /// <summary>
        /// Finds the child actor of the given type or creates a new one.
        /// </summary>
        /// <typeparam name="T">Type of the actor to search for. Includes any actors derived from the type.</typeparam>
        /// <returns>The child actor.</returns>
        public T GetOrAddChild<T>() where T : Actor
        {
            var result = GetChild<T>();
            if (result == null)
            {
                if (typeof(T).IsAbstract)
                    return null;

                result = New<T>();
                result.SetParent(this, false, false);
            }
            return result;
        }

        /// <summary>
        /// Creates a new script of a specific type and adds it to the actor.
        /// </summary>
        /// <param name="type">Type of the script to create.</param>
        /// <returns>The created script instance, null otherwise.</returns>
        public Script AddScript(Type type)
        {
            if (type.IsAbstract)
                return null;

            var script = (Script)New(type);
            script.Parent = this;
            return script;
        }

        /// <summary>
        /// Creates a new script of a specific type and adds it to the actor.
        /// </summary>
        /// <typeparam name="T">Type of the script to search for. Includes any scripts derived from the type.</typeparam>
        /// <returns>The created script instance, null otherwise.</returns>
        public T AddScript<T>() where T : Script
        {
            if (typeof(T).IsAbstract)
                return null;

            var script = New<T>();
            script.Parent = this;
            return script;
        }

        /// <summary>
        /// Finds the script of the given type from this actor.
        /// </summary>
        /// <typeparam name="T">Type of the script to search for. Includes any scripts derived from the type.</typeparam>
        /// <returns>The script or null if failed to find.</returns>
        public T GetScript<T>() where T : class
        {
            return GetScript(typeof(T)) as T;
        }

        /// <summary>
        /// Tries to find the script of the given type from this actor.
        /// </summary>
        /// <typeparam name="T">Type of the script to search for. Includes any scripts derived from the type.</typeparam>
        /// <param name="script">The returned script, valid only if method returns true.</param>
        /// <returns>True if found a script of that type or false if failed to find.</returns>
        public bool TryGetScript<T>(out T script) where T : class
        {
            script = GetScript(typeof(T)) as T;
            return (object)script != null;
        }

        /// <summary>
        /// Tries to find the script of the given type in this actor hierarchy (checks this actor and all children hierarchy).
        /// </summary>
        /// <typeparam name="T">Type of the object.</typeparam>
        /// <returns>Script instance if found, null otherwise.</returns>
        public T FindScript<T>() where T : class
        {
            return FindScript(typeof(T)) as T;
        }

        /// <summary>
        /// Tries to find the actor of the given type in this actor hierarchy (checks this actor and all children hierarchy).
        /// </summary>
        /// <typeparam name="T">Type of the object.</typeparam>
        /// <param name="activeOnly">Finds only a active actor.</param>
        /// <returns>Actor instance if found, null otherwise.</returns>
        public T FindActor<T>(bool activeOnly = false) where T : Actor
        {
            return FindActor(typeof(T), activeOnly) as T;
        }

        /// <summary>
        /// Tries to find the actor of the given type and name in this actor hierarchy (checks this actor and all children hierarchy).
        /// </summary>
        /// <param name="name">Name of the object.</param>
        /// <typeparam name="T">Type of the object.</typeparam>
        /// <returns>Actor instance if found, null otherwise.</returns>
        public T FindActor<T>(string name) where T : Actor
        {
            return FindActor(typeof(T), name) as T;
        }

        /// <summary>
        /// Tries to find actor of the given type and tag in this actor hierarchy (checks this actor and all children hierarchy).
        /// </summary>
        /// <param name="tag">A tag on the object.</param>
        /// <typeparam name="T">Type of the object.</typeparam>
        /// <param name="activeOnly">Finds only an active actor.</param>
        /// <returns>Actor instance if found, null otherwise.</returns>
        public T FindActor<T>(Tag tag, bool activeOnly = false) where T : Actor
        {
            return FindActor(typeof(T), tag, activeOnly) as T;
        }

        /// <summary>
        /// Searches for all actors of a specific type in this actor children list.
        /// </summary>
        /// <typeparam name="T">Type of the actor to search for. Includes any actors derived from the type.</typeparam>
        /// <returns>All actors matching the specified type</returns>
        public T[] GetChildren<T>() where T : Actor
        {
            var count = ChildrenCount;
            var length = 0;
            for (int i = 0; i < count; i++)
            {
                if (GetChild(i) is T)
                    length++;
            }
            if (length == 0)
                return Utils.GetEmptyArray<T>();
            var output = new T[length];
            length = 0;
            for (int i = 0; i < count; i++)
            {
                if (GetChild(i) is T obj)
                    output[length++] = obj;
            }
            return output;
        }

        /// <summary>
        /// Searches for all scripts of a specific type from this actor.
        /// </summary>
        /// <typeparam name="T">Type of the scripts to search for. Includes any scripts derived from the type.</typeparam>
        /// <returns>All scripts matching the specified type.</returns>
        public T[] GetScripts<T>() where T : class
        {
            var count = ScriptsCount;
            var length = 0;
            for (int i = 0; i < count; i++)
            {
                if (GetScript(i) is T)
                    length++;
            }
            if (length == 0)
                return Utils.GetEmptyArray<T>();
            var output = new T[length];
            length = 0;
            for (int i = 0; i < count; i++)
            {
                if (GetScript(i) is T obj)
                    output[length++] = obj;
            }
            return output;
        }

        /// <summary>
        /// Gets the matrix that transforms a point from the world space to local space of the actor.
        /// </summary>
        public Matrix WorldToLocalMatrix
        {
            get
            {
                GetWorldToLocalMatrix(out var worldToLocal);
                return worldToLocal;
            }
        }

        /// <summary>
        /// Gets the matrix that transforms a point from the local space of the actor to world space.
        /// </summary>
        public Matrix LocalToWorldMatrix
        {
            get
            {
                GetLocalToWorldMatrix(out var localToWorld);
                return localToWorld;
            }
        }

        /// <summary>
        /// Rotates the actor around axis passing through point in world-space by angle (in degrees).
        /// </summary>
        /// <remarks>Modifies both the position and the rotation of the actor (scale remains the same).</remarks>
        /// <param name="point">The point (world-space).</param>
        /// <param name="axis">The axis (normalized).</param>
        /// <param name="angle">The angle (in degrees).</param>
        /// /// <param name="orientActor">Whether to orient the actor the same amount as rotation.</param>
        public void RotateAround(Vector3 point, Vector3 axis, float angle, bool orientActor = true)
        {
            var transform = Transform;
            var q = Quaternion.RotationAxis(axis, angle * Mathf.DegreesToRadians);
            if (Vector3.NearEqual(point, transform.Translation) && orientActor)
                transform.Orientation *= q;
            else
            {
                var dif = (transform.Translation - point) * q;
                transform.Translation = point + dif;
                if (orientActor)
                    transform.Orientation *= q;
            }
            Transform = transform;
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return $"{Name} ({GetType().Name})";
        }

#if FLAX_EDITOR
        private bool ShowTransform => !(this is UIControl);
#endif
    }
}
