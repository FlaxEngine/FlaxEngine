// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// The enum editor element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    public class EnumElement : LayoutElement
    {
        /// <summary>
        /// The combo box used to show enum values.
        /// </summary>
        public EnumComboBox ComboBox;

        /// <summary>
        /// Initializes a new instance of the <see cref="EnumElement"/> class.
        /// </summary>
        /// <param name="type">The enum type.</param>
        /// <param name="customBuildEntriesDelegate">The custom entries layout builder. Allows to hide existing or add different enum values to editor.</param>
        /// <param name="formatMode">The formatting mode.</param>
        public EnumElement(Type type, EnumComboBox.BuildEntriesDelegate customBuildEntriesDelegate = null, EnumDisplayAttribute.FormatMode formatMode = EnumDisplayAttribute.FormatMode.Default)
        {
            ComboBox = new EnumComboBox(type, customBuildEntriesDelegate, formatMode);
        }

        /// <inheritdoc />
        public override Control Control => ComboBox;
    }
}
