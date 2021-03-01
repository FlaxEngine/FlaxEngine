// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections;
using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit arrays.
    /// </summary>
    [CustomEditor(typeof(Array)), DefaultEditor]
    public class ArrayEditor : CollectionEditor
    {
        /// <inheritdoc />
        public override int Count => (Values[0] as Array)?.Length ?? 0;

        /// <inheritdoc />
        protected override IList Allocate(int size)
        {
            var arrayType = Values.Type;
            var elementType = arrayType.GetElementType();
            return Array.CreateInstance(elementType, size);
        }

        /// <inheritdoc />
        protected override void Resize(int newSize)
        {
            var array = Values[0] as Array;
            var oldSize = array?.Length ?? 0;

            if (oldSize != newSize)
            {
                // Allocate new array
                var arrayType = Values.Type;
                var elementType = arrayType.GetElementType();
                var newValues = Array.CreateInstance(elementType, newSize);

                var sharedCount = Mathf.Min(oldSize, newSize);
                if (array != null && sharedCount > 0)
                {
                    // Copy old values
                    Array.Copy(array, 0, newValues, 0, sharedCount);

                    if (elementType.IsValueType)
                    {
                        // Fill new entries with the last value
                        for (int i = oldSize; i < newSize; i++)
                            Array.Copy(array, oldSize - 1, newValues, i, 1);
                    }
                    else
                    {
                        // Initialize new entries with default values
                        var defaultValue = TypeUtils.GetDefaultValue(new ScriptType(elementType));
                        for (int i = oldSize; i < newSize; i++)
                            newValues.SetValue(defaultValue, i);
                    }
                }
                else if (newSize > 0)
                {
                    // Initialize new entries with default values
                    var defaultValue = TypeUtils.GetDefaultValue(new ScriptType(elementType));
                    for (int i = 0; i < newSize; i++)
                        newValues.SetValue(defaultValue, i);
                }

                SetValue(newValues);
            }
        }

        /// <inheritdoc />
        protected override IList CloneValues()
        {
            var array = Values[0] as Array;
            if (array == null)
                return null;

            var size = array.Length;
            var arrayType = Values.Type;
            var elementType = arrayType.GetElementType();
            var cloned = Array.CreateInstance(elementType, size);

            Array.Copy(array, 0, cloned, 0, size);

            return cloned;
        }
    }
}
