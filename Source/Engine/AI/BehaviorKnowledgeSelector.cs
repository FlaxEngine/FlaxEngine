// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Customizes editor of <see cref="BehaviorKnowledgeSelector{T}"/> or <see cref="BehaviorKnowledgeSelectorAny"/>.
    /// </summary>
    /// <seealso cref="System.Attribute" />
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class BehaviorKnowledgeSelectorAttribute : Attribute
    {
        /// <summary>
        /// Changes selector editor to allow to pick only whole goals.
        /// </summary>
        public bool IsGoalSelector;

        /// <summary>
        /// Initializes a new instance of the <see cref="BehaviorKnowledgeSelectorAttribute"/> structure.
        /// </summary>
        /// <param name="isGoalSelector">Changes selector editor to allow to pick only whole goals.</param>
        public BehaviorKnowledgeSelectorAttribute(bool isGoalSelector = false)
        {
            IsGoalSelector = isGoalSelector;
        }
    }

#if FLAX_EDITOR
    [CustomEditor(typeof(FlaxEditor.CustomEditors.Editors.BehaviorKnowledgeSelectorEditor))]
#endif
    partial struct BehaviorKnowledgeSelectorAny : IComparable, IComparable<BehaviorKnowledgeSelectorAny>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="BehaviorKnowledgeSelectorAny"/> structure.
        /// </summary>
        /// <param name="path">The selector path.</param>
        public BehaviorKnowledgeSelectorAny(string path)
        {
            Path = path;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="BehaviorKnowledgeSelectorAny"/> structure.
        /// </summary>
        /// <param name="other">The other selector.</param>
        public BehaviorKnowledgeSelectorAny(BehaviorKnowledgeSelectorAny other)
        {
            Path = other.Path;
        }

        /// <summary>
        /// Implicit cast operator from selector to string.
        /// </summary>
        /// <param name="value">Selector</param>
        /// <returns>Path</returns>
        public static implicit operator string(BehaviorKnowledgeSelectorAny value)
        {
            return value.Path;
        }

        /// <summary>
        /// Implicit cast operator from string to selector.
        /// </summary>
        /// <param name="value">Path</param>
        /// <returns>Selector</returns>
        public static implicit operator BehaviorKnowledgeSelectorAny(string value)
        {
            return new BehaviorKnowledgeSelectorAny(value);
        }

        /// <summary>
        /// Sets the selected knowledge value.
        /// </summary>
        /// <param name="knowledge">The knowledge container to access.</param>
        /// <param name="value">The value to set.</param>
        /// <returns>True if set value, otherwise false.</returns>
        public bool Set(BehaviorKnowledge knowledge, object value)
        {
            return knowledge != null && knowledge.Set(Path, value);
        }

        /// <summary>
        /// Gets the selected knowledge value.
        /// </summary>
        /// <param name="knowledge">The knowledge container to access.</param>
        /// <returns>The output value or null (if cannot read it - eg. missing goal or no blackboard entry of that name).</returns>
        public object Get(BehaviorKnowledge knowledge)
        {
            object value = null;
            if (knowledge != null)
                knowledge.Get(Path, out value);
            return value;
        }

        /// <summary>
        /// Tries to get the selected knowledge value. Returns true if got value, otherwise false.
        /// </summary>
        /// <param name="knowledge">The knowledge container to access.</param>
        /// <param name="value">The output value.</param>
        /// <returns>True if got value, otherwise false.</returns>
        public bool TryGet(BehaviorKnowledge knowledge, out object value)
        {
            value = null;
            return knowledge != null && knowledge.Get(Path, out value);
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return Path;
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return Path?.GetHashCode() ?? 0;
        }

        /// <inheritdoc />
        public int CompareTo(object obj)
        {
            if (obj is BehaviorKnowledgeSelectorAny other)
                return CompareTo(other);
            return 0;
        }

        /// <inheritdoc />
        public int CompareTo(BehaviorKnowledgeSelectorAny other)
        {
            return string.Compare(Path, other.Path, StringComparison.Ordinal);
        }
    }

    /// <summary>
    /// Behavior knowledge value selector that can reference blackboard item, behavior goal or sensor values.
    /// </summary>
#if FLAX_EDITOR
    [CustomEditor(typeof(FlaxEditor.CustomEditors.Editors.BehaviorKnowledgeSelectorEditor))]
#endif
    public struct BehaviorKnowledgeSelector<T> : IComparable, IComparable<BehaviorKnowledgeSelectorAny>, IComparable<BehaviorKnowledgeSelector<T>>
    {
        /// <summary>
        /// Selector path that redirects to the specific knowledge value.
        /// </summary>
        public string Path;

        /// <summary>
        /// Initializes a new instance of the <see cref="BehaviorKnowledgeSelector{T}"/> structure.
        /// </summary>
        /// <param name="path">The selector path.</param>
        public BehaviorKnowledgeSelector(string path)
        {
            Path = path;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="BehaviorKnowledgeSelector{T}"/> structure.
        /// </summary>
        /// <param name="other">The other selector.</param>
        public BehaviorKnowledgeSelector(BehaviorKnowledgeSelectorAny other)
        {
            Path = other.Path;
        }

        /// <summary>
        /// Implicit cast operator from selector to string.
        /// </summary>
        /// <param name="value">Selector</param>
        /// <returns>Path</returns>
        public static implicit operator string(BehaviorKnowledgeSelector<T> value)
        {
            return value.Path;
        }

        /// <summary>
        /// Implicit cast operator from string to selector.
        /// </summary>
        /// <param name="value">Path</param>
        /// <returns>Selector</returns>
        public static implicit operator BehaviorKnowledgeSelector<T>(string value)
        {
            return new BehaviorKnowledgeSelector<T>(value);
        }

        /// <summary>
        /// Sets the selected knowledge value.
        /// </summary>
        /// <param name="knowledge">The knowledge container to access.</param>
        /// <param name="value">The value to set.</param>
        /// <returns>True if set value value, otherwise false.</returns>
        public bool Set(BehaviorKnowledge knowledge, T value)
        {
            return knowledge != null && knowledge.Set(Path, value);
        }

        /// <summary>
        /// Gets the selected knowledge value.
        /// </summary>
        /// <param name="knowledge">The knowledge container to access.</param>
        /// <returns>The output value or null (if cannot read it - eg. missing goal or no blackboard entry of that name).</returns>
        public T Get(BehaviorKnowledge knowledge)
        {
            if (knowledge != null && knowledge.Get(Path, out var value))
                return Utilities.VariantUtils.Cast<T>(value);
            return default;
        }

        /// <summary>
        /// Tries to get the selected knowledge value. Returns true if got value, otherwise false.
        /// </summary>
        /// <param name="knowledge">The knowledge container to access.</param>
        /// <param name="value">The output value.</param>
        /// <returns>True if got value, otherwise false.</returns>
        public bool TryGet(BehaviorKnowledge knowledge, out T value)
        {
            value = default;
            object tmp = null;
            bool result = knowledge != null && knowledge.Get(Path, out tmp);
            if (result)
                value = Utilities.VariantUtils.Cast<T>(tmp);
            return result;
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return Path;
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return Path?.GetHashCode() ?? 0;
        }

        /// <inheritdoc />
        public int CompareTo(object obj)
        {
            if (obj is BehaviorKnowledgeSelectorAny otherAny)
                return CompareTo(otherAny);
            if (obj is BehaviorKnowledgeSelector<T> other)
                return CompareTo(other);
            return 0;
        }

        /// <inheritdoc />
        public int CompareTo(BehaviorKnowledgeSelectorAny other)
        {
            return string.Compare(Path, other.Path, StringComparison.Ordinal);
        }

        /// <inheritdoc />
        public int CompareTo(BehaviorKnowledgeSelector<T> other)
        {
            return string.Compare(Path, other.Path, StringComparison.Ordinal);
        }
    }
}
