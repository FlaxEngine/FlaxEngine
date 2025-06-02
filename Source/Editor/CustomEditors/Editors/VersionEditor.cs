// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit Version value type properties.
    /// </summary>
    [CustomEditor(typeof(Version)), DefaultEditor]
    public class VersionEditor : CustomEditor
    {
        /// <summary>
        /// The Version.Major component editor.
        /// </summary>
        protected IntegerValueElement Major;

        /// <summary>
        /// The Version.Minor component editor.
        /// </summary>
        protected IntegerValueElement Minor;

        /// <summary>
        /// The Version.Build component editor.
        /// </summary>
        protected IntegerValueElement Build;

        /// <summary>
        /// The Version.Revision component editor.
        /// </summary>
        protected IntegerValueElement Revision;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var grid = layout.UniformGrid();
            var gridControl = grid.CustomControl;
            gridControl.ClipChildren = false;
            gridControl.Height = TextBox.DefaultHeight;
            gridControl.SlotsHorizontally = 4;
            gridControl.SlotsVertically = 1;

            Major = grid.IntegerValue();
            Major.IntValue.SetLimits(0, 100000000);
            Major.IntValue.ValueChanged += OnValueChanged;
            Major.IntValue.SlidingEnd += ClearToken;

            Minor = grid.IntegerValue();
            Minor.IntValue.SetLimits(0, 100000000);
            Minor.IntValue.ValueChanged += OnValueChanged;
            Minor.IntValue.SlidingEnd += ClearToken;

            Build = grid.IntegerValue();
            Build.IntValue.SetLimits(-1, 100000000);
            Build.IntValue.ValueChanged += OnValueChanged;
            Build.IntValue.SlidingEnd += ClearToken;

            Revision = grid.IntegerValue();
            Revision.IntValue.SetLimits(-1, 100000000);
            Revision.IntValue.ValueChanged += OnValueChanged;
            Revision.IntValue.SlidingEnd += ClearToken;
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked)
                return;

            var isSliding = Major.IsSliding || Minor.IsSliding || Build.IsSliding || Revision.IsSliding;
            var token = isSliding ? this : null;
            Version value;
            var major = Major.IntValue.Value;
            var minor = Minor.IntValue.Value;
            var build = Build.IntValue.Value;
            var revision = Revision.IntValue.Value;
            if (build > -1)
            {
                if (revision > 0)
                    value = new Version(major, minor, build, revision);
                else
                    value = new Version(major, minor, build);
            }
            else
            {
                value = new Version(major, minor);
            }
            SetValue(value, token);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
                // TODO: support different values for ValueBox<T>
            }
            else if (Values[0] is Version value)
            {
                Major.IntValue.Value = value.Major;
                Minor.IntValue.Value = value.Minor;
                Build.IntValue.Value = value.Build;
                Revision.IntValue.Value = value.Revision;
            }
        }
    }
}
