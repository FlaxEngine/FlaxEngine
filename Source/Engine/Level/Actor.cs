// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
        public bool IsStatic => StaticFlags == FlaxEngine.StaticFlags.FullyStatic;

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
        [Tooltip("Returns true if object has given flag(s) set..")]
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
        public Vector3 EulerAngles
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
        public Vector3 LocalEulerAngles
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
            var result = (Actor)New(type);
            result.SetParent(this, false);
            return result;
        }

        /// <summary>
        /// Creates a new child actor of the given type.
        /// </summary>
        /// <typeparam name="T">Type of the actor to search for. Includes any actors derived from the type.</typeparam>
        /// <returns>The child actor.</returns>
        public T AddChild<T>() where T : Actor
        {
            var result = New<T>();
            result.SetParent(this, false);
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
            return actor != null;
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
                result = New<T>();
                result.SetParent(this, false);
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
            var script = New<T>();
            script.Parent = this;
            return script;
        }

        /// <summary>
        /// Finds the script of the given type from this actor.
        /// </summary>
        /// <typeparam name="T">Type of the script to search for. Includes any scripts derived from the type.</typeparam>
        /// <returns>The script or null if failed to find.</returns>
        public T GetScript<T>() where T : Script
        {
            return GetScript(typeof(T)) as T;
        }

        /// <summary>
        /// Tries to find the script of the given type from this actor.
        /// </summary>
        /// <typeparam name="T">Type of the script to search for. Includes any scripts derived from the type.</typeparam>
        /// <param name="script">The returned script, valid only if method returns true.</param>
        /// <returns>True if found a script of that type or false if failed to find.</returns>
        public bool TryGetScript<T>(out T script) where T : Script
        {
            script = GetScript(typeof(T)) as T;
            return script != null;
        }

        /// <summary>
        /// Searches for all actors of a specific type in this actor children list.
        /// </summary>
        /// <typeparam name="T">Type of the actor to search for. Includes any actors derived from the type.</typeparam>
        /// <returns>All actors matching the specified type</returns>
        public T[] GetChildren<T>() where T : Actor
        {
            // TODO: use a proper array allocation and converting on backend to reduce memory allocations
            var children = GetChildren(typeof(T));
            var output = new T[children.Length];
            for (int i = 0; i < children.Length; i++)
                output[i] = (T)children[i];
            return output;
        }

        /// <summary>
        /// Searches for all scripts of a specific type from this actor.
        /// </summary>
        /// <typeparam name="T">Type of the scripts to search for. Includes any scripts derived from the type.</typeparam>
        /// <returns>All scripts matching the specified type.</returns>
        public T[] GetScripts<T>() where T : Script
        {
            // TODO: use a proper array allocation and converting on backend to reduce memory allocations
            var scripts = GetScripts(typeof(T));
            var output = new T[scripts.Length];
            for (int i = 0; i < scripts.Length; i++)
                output[i] = (T)scripts[i];
            return output;
        }

        /// <summary>
        /// Destroys the children. Calls Object.Destroy on every child actor and unlink them for the parent.
        /// </summary>
        /// <param name="timeLeft">The time left to destroy object (in seconds).</param>
        [NoAnimate]
        public void DestroyChildren(float timeLeft = 0.0f)
        {
            Actor[] children = Children;
            for (var i = 0; i < children.Length; i++)
            {
                children[i].Parent = null;
                Destroy(children[i], timeLeft);
            }
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

        /// <inheritdoc />
        public override string ToString()
        {
            return $"{Name} ({GetType().Name})";
        }
    }
}
