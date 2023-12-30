// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Base class for VisibleIf attributes
    /// </summary>
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public abstract class VisibleIfBaseAttribute : Attribute
    {
        /// <summary>
        /// The member name.
        /// </summary>
        public string MemberName;

        /// <summary>
        /// True if invert member value when computing the visibility value.
        /// </summary>
        public bool Invert;

        /// <summary>
        /// The type this VisibleIf attribute checks for.
        /// </summary>
        public abstract Type MemberType { get; }

        /// <summary>
        /// Initializes a new instance of a VisibleIf attribute class.
        /// </summary>
        /// <param name="memberName">The name of the field or property of the object. Must be a type matching <see cref="MemberType"/>.</param>
        /// <param name="invert">True if invert member value when computing the visibility value.</param>
        protected VisibleIfBaseAttribute(string memberName, bool invert) {
            MemberName = memberName;
            Invert = invert;
        }
    }

    /// <summary>
    /// Shows property/field in the editor only if the specified member has a given value. Can be used to hide properties based on other properties (also private properties). The given member has to be bool type.
    /// </summary>
    /// <seealso cref="System.Attribute" />
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class VisibleIfAttribute : VisibleIfBaseAttribute
    {
        /// <inheritdoc />
        public override Type MemberType => typeof(bool);

        private VisibleIfAttribute() : base(null, false)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="VisibleIfAttribute"/> class.
        /// </summary>
        /// <param name="memberName">The name of the field or property of the object. Must be a bool type.</param>
        /// <param name="invert">True if invert member value when computing the visibility value.</param>
        public VisibleIfAttribute(string memberName, bool invert = false) : base(memberName, invert)
        {
        }
    }

    /// <summary>
    /// Shows property/field in the editor only if the specified member has a given value. Can be used to hide properties based on other properties (also private properties).
    /// </summary>
    /// <seealso cref="System.Attribute" />
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class VisibleIfValueAttribute : VisibleIfBaseAttribute
    {
        /// <summary>
        /// Value to compare when computing the visibility value.
        /// </summary>
        public object Value;

        /// <inheritdoc />
        public override Type MemberType => Value?.GetType();

        private VisibleIfValueAttribute() : base(null, false)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="VisibleIfValueAttribute"/> class.
        /// </summary>
        /// <param name="memberName">The name of the field or property of the object.</param>
        /// <param name="value">Value to compare when computing the visibility value.</param>
        /// <param name="invert">True if invert member value when computing the visibility value.</param>
        public VisibleIfValueAttribute(string memberName, object value, bool invert = false) : base(memberName, invert)
        {
            Value = value;
            Invert = invert;
        }
    }
}
