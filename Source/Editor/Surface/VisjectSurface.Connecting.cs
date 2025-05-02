// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// The Visject Surface connection creation handler object.
    /// </summary>
    [HideInEditor]
    public interface IConnectionInstigator
    {
        /// <summary>
        /// Gets the connection origin point (in surface node space).
        /// </summary>
        Float2 ConnectionOrigin { get; }

        /// <summary>
        /// Determines whether this surface object is connected with the specified other object.
        /// </summary>
        /// <param name="other">The other object to check.</param>
        /// <returns><c>true</c> if connection between given two objects exists; otherwise, <c>false</c>.</returns>
        bool AreConnected(IConnectionInstigator other);

        /// <summary>
        /// Determines whether this surface object can be connected with the specified other object.
        /// </summary>
        /// <param name="other">The other object to check.</param>
        /// <returns><c>true</c> if connection can be created; otherwise, <c>false</c>.</returns>
        bool CanConnectWith(IConnectionInstigator other);

        /// <summary>
        /// Draws the connecting line.
        /// </summary>
        /// <param name="startPos">The start position.</param>
        /// <param name="endPos">The end position.</param>
        /// <param name="color">The color.</param>
        void DrawConnectingLine(ref Float2 startPos, ref Float2 endPos, ref Color color);

        /// <summary>
        /// Created the new connection with the specified other object.
        /// </summary>
        /// <param name="other">The other.</param>
        void Connect(IConnectionInstigator other);
    }

    public partial class VisjectSurface
    {
        internal static bool CanUseDirectCastStatic(ScriptType from, ScriptType to, bool supportsImplicitCastFromObjectToBoolean = true)
        {
            if (from == ScriptType.Null || to == ScriptType.Null)
                return false;
            bool result = from == to || to.IsAssignableFrom(from);
            if (!result)
            {
                // Implicit casting is supported for reference types
                var toIsReference = to.IsReference;
                var fromIsReference = from.IsReference;
                if (toIsReference && !fromIsReference)
                {
                    var toTypeName = to.TypeName;
                    return !string.IsNullOrEmpty(toTypeName) && from.TypeName == toTypeName.Substring(0, toTypeName.Length - 1);
                }
                if (!toIsReference && fromIsReference)
                {
                    var fromTypeName = from.TypeName;
                    return !string.IsNullOrEmpty(fromTypeName) && to.TypeName == fromTypeName.Substring(0, fromTypeName.Length - 1);
                }

                // Implicit casting is supported for object reference to test whenever it is valid
                var toType = to.Type;
                if (supportsImplicitCastFromObjectToBoolean && toType == typeof(bool) && ScriptType.FlaxObject.IsAssignableFrom(from))
                {
                    return true;
                }

                // Implicit casting is supported for primitive types
                var fromType = from.Type;
                if (fromType == typeof(bool) ||
                    fromType == typeof(byte) ||
                    fromType == typeof(char) ||
                    fromType == typeof(short) ||
                    fromType == typeof(ushort) ||
                    fromType == typeof(int) ||
                    fromType == typeof(uint) ||
                    fromType == typeof(long) ||
                    fromType == typeof(ulong) ||
                    fromType == typeof(float) ||
                    fromType == typeof(double) ||
                    fromType == typeof(Vector2) ||
                    fromType == typeof(Vector3) ||
                    fromType == typeof(Vector4) ||
                    fromType == typeof(Float2) ||
                    fromType == typeof(Float3) ||
                    fromType == typeof(Float4) ||
                    fromType == typeof(Double2) ||
                    fromType == typeof(Double3) ||
                    fromType == typeof(Double4) ||
                    fromType == typeof(Int2) ||
                    fromType == typeof(Int3) ||
                    fromType == typeof(Int4) ||
                    fromType == typeof(Color) ||
                    fromType == typeof(Quaternion))
                {
                    if (toType == typeof(bool) ||
                        toType == typeof(byte) ||
                        toType == typeof(char) ||
                        toType == typeof(short) ||
                        toType == typeof(ushort) ||
                        toType == typeof(int) ||
                        toType == typeof(uint) ||
                        toType == typeof(long) ||
                        toType == typeof(ulong) ||
                        toType == typeof(float) ||
                        toType == typeof(double) ||
                        toType == typeof(Vector2) ||
                        toType == typeof(Vector3) ||
                        toType == typeof(Vector4) ||
                        toType == typeof(Float2) ||
                        toType == typeof(Float3) ||
                        toType == typeof(Float4) ||
                        toType == typeof(Double2) ||
                        toType == typeof(Double3) ||
                        toType == typeof(Double4) ||
                        toType == typeof(Int2) ||
                        toType == typeof(Int3) ||
                        toType == typeof(Int4) ||
                        toType == typeof(Color) ||
                        toType == typeof(Quaternion))
                    {
                        result = true;
                    }
                }
            }
            return result;
        }

        /// <summary>
        /// Checks if a type is compatible with another type and can be casted by using a connection hint
        /// </summary>
        /// <param name="from">Source type</param>
        /// <param name="to">Type to check compatibility with</param>
        /// <param name="hint">Hint to check if casting is possible</param>
        /// <returns>True if the source type is compatible with the target type</returns>
        public static bool IsTypeCompatible(ScriptType from, ScriptType to, ConnectionsHint hint)
        {
            if (from == ScriptType.Null && hint != ConnectionsHint.None)
            {
                if ((hint & ConnectionsHint.Anything) == ConnectionsHint.Anything)
                    return true;
                if ((hint & ConnectionsHint.Value) == ConnectionsHint.Value && to.Type != typeof(void))
                    return true;
                if ((hint & ConnectionsHint.Enum) == ConnectionsHint.Enum && to.IsEnum)
                    return true;
                if ((hint & ConnectionsHint.Array) == ConnectionsHint.Array && to.IsArray)
                    return true;
                if ((hint & ConnectionsHint.Dictionary) == ConnectionsHint.Dictionary && to.IsDictionary)
                    return true;
                if ((hint & ConnectionsHint.Vector) == ConnectionsHint.Vector)
                {
                    var t = to.Type;
                    if (t == typeof(Vector2) ||
                        t == typeof(Vector3) ||
                        t == typeof(Vector4) ||
                        t == typeof(Float2) ||
                        t == typeof(Float3) ||
                        t == typeof(Float4) ||
                        t == typeof(Double2) ||
                        t == typeof(Double3) ||
                        t == typeof(Double4) ||
                        t == typeof(Int2) ||
                        t == typeof(Int3) ||
                        t == typeof(Int4) ||
                        t == typeof(Color))
                    {
                        return true;
                    }
                }
                if ((hint & ConnectionsHint.Scalar) == ConnectionsHint.Scalar)
                {
                    var t = to.Type;
                    if (t == typeof(bool) ||
                        t == typeof(char) ||
                        t == typeof(byte) ||
                        t == typeof(short) ||
                        t == typeof(ushort) ||
                        t == typeof(int) ||
                        t == typeof(uint) ||
                        t == typeof(long) ||
                        t == typeof(ulong) ||
                        t == typeof(float) ||
                        t == typeof(double))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        /// <summary>
        /// Checks if a type is compatible with another type and can be casted by using a connection hint
        /// </summary>
        /// <param name="from">Source type</param>
        /// <param name="to">Target type</param>
        /// <param name="hint">Connection hint</param>
        /// <returns>True if any method of casting or compatibility check succeeds</returns>
        public static bool FullCastCheck(ScriptType from, ScriptType to, ConnectionsHint hint)
        {
            // Yes, from and to are switched on purpose
            if (CanUseDirectCastStatic(to, from, false))
                return true;
            if (IsTypeCompatible(from, to, hint))
                return true;
            // Same here
            return to.CanCastTo(from);
        }

        /// <summary>
        /// Checks if can use direct conversion from one type to another.
        /// </summary>
        /// <param name="from">Source type.</param>
        /// <param name="to">Target type.</param>
        /// <returns>True if can use direct conversion, otherwise false.</returns>
        public bool CanUseDirectCast(ScriptType from, ScriptType to)
        {
            return CanUseDirectCastStatic(from, to, _supportsImplicitCastFromObjectToBoolean);
        }

        /// <summary>
        /// Begins connecting surface objects action.
        /// </summary>
        /// <param name="instigator">The connection instigator (eg. start box).</param>
        public void ConnectingStart(IConnectionInstigator instigator)
        {
            if (instigator != null && instigator != _connectionInstigator)
            {
                _connectionInstigator = instigator;
                StartMouseCapture();
            }
        }

        /// <summary>
        /// Callback for surface objects connections instigators to indicate mouse over control event (used to draw preview connections).
        /// </summary>
        /// <param name="instigator">The instigator.</param>
        public void ConnectingOver(IConnectionInstigator instigator)
        {
            _lastInstigatorUnderMouse = instigator;
        }

        /// <summary>
        /// Ends connecting surface objects action.
        /// </summary>
        /// <param name="end">The end object (eg. end box).</param>
        public void ConnectingEnd(IConnectionInstigator end)
        {
            // Ensure that there was a proper start box
            if (_connectionInstigator == null)
                return;

            var start = _connectionInstigator;
            _connectionInstigator = null;

            // Check if boxes are different and end box is specified
            if (start == end || end == null)
                return;

            // Connect them
            if (start.CanConnectWith(end))
            {
                start.Connect(end);
            }
        }
    }
}
