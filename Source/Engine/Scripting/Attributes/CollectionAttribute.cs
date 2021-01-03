// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// This attributes provides additional information on a member collection.
    /// </summary>
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property | AttributeTargets.Class)]
    public sealed class CollectionAttribute : Attribute
    {
        /// <summary>
        /// Gets or sets whether this collection is read-only. If <c>true</c>, applications using this collection should not allow to add or remove items.
        /// </summary>
        public bool ReadOnly;

        /// <summary>
        /// Gets or sets whether the items of this collection can be reordered. If <c>true</c>, applications using this collection should provide users a way to reorder items.
        /// </summary>
        public bool CanReorderItems = true;

        /// <summary>
        /// Gets or sets whether the items of this collection can be null. If <c>true</c>, applications using this collection should prevent user to add null items to the collection.
        /// </summary>
        public bool NotNullItems;

        /// <summary>
        /// Custom editor class typename for collection values editing.
        /// </summary>
        public string OverrideEditorTypeName;

        /// <summary>
        /// The spacing amount between collection items in the UI.
        /// </summary>
        public float Spacing;
    }
}
