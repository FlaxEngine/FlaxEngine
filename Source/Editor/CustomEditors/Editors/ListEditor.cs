// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections;
using System.Collections.Generic;
using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit lists.
    /// </summary>
    [CustomEditor(typeof(List<>)), DefaultEditor]
    public class ListEditor : CollectionEditor
    {
        /// <inheritdoc />
        public override int Count => (Values[0] as IList)?.Count ?? 0;

        /// <inheritdoc />
        protected override IList Allocate(int size)
        {
            var listType = Values.Type;
            var list = (IList)listType.CreateInstance();
            var defaultValue = TypeUtils.GetDefaultValue(ElementType);
            for (int i = 0; i < size; i++)
                list.Add(defaultValue);
            return list;
        }

        /// <inheritdoc />
        protected override void Resize(int newSize)
        {
            var list = Values[0] as IList;
            var oldSize = list?.Count ?? 0;

            if (oldSize != newSize)
            {
                // Allocate new list
                var listType = Values.Type;
                var newValues = (IList)listType.CreateInstance();
                var elementType = ElementType;

                var sharedCount = Mathf.Min(oldSize, newSize);
                if (list != null && sharedCount > 0)
                {
                    // Copy old values
                    for (int i = 0; i < sharedCount; i++)
                        newValues.Add(list[i]);

                    if (elementType.IsValueType || NotNullItems)
                    {
                        // Fill new entries with the last value
                        for (int i = oldSize; i < newSize; i++)
                            newValues.Add(list[oldSize - 1]);
                    }
                    else
                    {
                        // Initialize new entries with default values
                        var defaultValue = TypeUtils.GetDefaultValue(elementType);
                        for (int i = oldSize; i < newSize; i++)
                            newValues.Add(defaultValue);
                    }
                }
                else if (newSize > 0)
                {
                    // Fill new entries with default value
                    var defaultValue = TypeUtils.GetDefaultValue(elementType);
                    for (int i = oldSize; i < newSize; i++)
                        newValues.Add(defaultValue);
                }

                SetValue(newValues);
            }
        }

        /// <inheritdoc />
        protected override IList CloneValues()
        {
            var list = Values[0] as IList;
            if (list == null)
                return null;

            var size = list.Count;
            var listType = Values.Type;
            var cloned = (IList)listType.CreateInstance();

            for (int i = 0; i < size; i++)
                cloned.Add(list[i]);

            return cloned;
        }
    }
}
