// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Surface node element types
    /// </summary>
    [HideInEditor]
    public enum NodeElementType
    {
        /// <summary>
        /// The invalid element.
        /// </summary>
        Invalid = 0,

        /// <summary>
        /// The input box.
        /// </summary>
        Input = 1,

        /// <summary>
        /// The output box.
        /// </summary>
        Output = 2,

        /// <summary>
        /// The text.
        /// </summary>
        Text = 3,

        /// <summary>
        /// The bool value.
        /// </summary>
        BoolValue = 4,

        /// <summary>
        /// The integer value.
        /// </summary>
        IntegerValue = 5,

        /// <summary>
        /// The float value.
        /// </summary>
        FloatValue = 6,

        /// <summary>
        /// The rotation value.
        /// </summary>
        RotationValue = 10,

        /// <summary>
        /// The color picker.
        /// </summary>
        ColorValue = 11,

        /// <summary>
        /// The combo box.
        /// </summary>
        ComboBox = 12,

        /// <summary>
        /// The asset picker.
        /// </summary>
        Asset = 13,

        /// <summary>
        /// The text box.
        /// </summary>
        TextBox = 14,

        /// <summary>
        /// The skeleton bone index selection.
        /// </summary>
        SkeletonBoneIndexSelect = 15,

        /// <summary>
        /// The bounding box value.
        /// </summary>
        BoxValue = 16,

        /// <summary>
        /// The enum value (as int).
        /// </summary>
        EnumValue = 17,

        /// <summary>
        /// The skeleton node name selection.
        /// </summary>
        SkeletonNodeNameSelect = 18,

        /// <summary>
        /// The actor picker.
        /// </summary>
        Actor = 19,

        /// <summary>
        /// The unsigned integer value.
        /// </summary>
        UnsignedIntegerValue = 20,
    }
}
