// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Specifies a options for an asset reference picker in the editor. Allows to customize view or provide custom value assign policy.
    /// </summary>
    /// <seealso cref="System.Attribute" />
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public class AssetReferenceAttribute : Attribute
    {
        /// <summary>
        /// The full name of the asset type to link. Use null or empty to skip it. Can be used as file extension filter if starts with a dot and used over string property.
        /// </summary>
        public string TypeName;

        /// <summary>
        /// True if use asset picker with a smaller height (single line), otherwise will use with full icon.
        /// </summary>
        public bool UseSmallPicker;

        /// <summary>
        /// Initializes a new instance of the <see cref="AssetReferenceAttribute"/> class.
        /// </summary>
        /// <param name="useSmallPicker">True if use asset picker with a smaller height (single line), otherwise will use with full icon.</param>
        public AssetReferenceAttribute(bool useSmallPicker)
        {
            TypeName = null;
            UseSmallPicker = useSmallPicker;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AssetReferenceAttribute"/> class.
        /// </summary>
        /// <param name="typeName">The full name of the asset type to link. Use null or empty to skip it.</param>
        /// <param name="useSmallPicker">True if use asset picker with a smaller height (single line), otherwise will use with full icon.</param>
        public AssetReferenceAttribute(Type typeName = null, bool useSmallPicker = false)
        {
            TypeName = typeName?.FullName;
            UseSmallPicker = useSmallPicker;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AssetReferenceAttribute"/> class.
        /// </summary>
        /// <param name="typeName">The full name of the asset type to link. Use null or empty to skip it. Can be used as file extension filter if starts with a dot and used over string property.</param>
        /// <param name="useSmallPicker">True if use asset picker with a smaller height (single line), otherwise will use with full icon.</param>
        public AssetReferenceAttribute(string typeName = null, bool useSmallPicker = false)
        {
            TypeName = typeName;
            UseSmallPicker = useSmallPicker;
        }
    }

#if USE_NETCORE
    /// <summary>
    /// Specifies a options for an asset reference picker in the editor. Allows to customize view or provide custom value assign policy.
    /// </summary>
    /// <seealso cref="System.Attribute" />
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public class AssetReferenceAttribute<T> : AssetReferenceAttribute
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AssetReferenceAttribute"/> class for generic type T.
        /// </summary>
        /// <param name="useSmallPicker">True if use asset picker with a smaller height (single line), otherwise will use with full icon.</param>
        public AssetReferenceAttribute(bool useSmallPicker = false)
            : base(typeof(T), useSmallPicker)
        {
        }
    }
#endif
}
