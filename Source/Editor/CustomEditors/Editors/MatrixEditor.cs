// Copyright (c) Wojciech Figat. All rights reserved.

using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit Matrix value type properties.
    /// </summary>
    [CustomEditor(typeof(Matrix)), DefaultEditor]
    public class MatrixEditor : CustomEditor
    {
        /// <summary>
        /// The 16 components editors.
        /// </summary>
        protected readonly FloatValueElement[] Elements = new FloatValueElement[16];

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var grid = layout.CustomContainer<UniformGridPanel>();
            var gridControl = grid.CustomControl;
            gridControl.ClipChildren = false;
            gridControl.Height = TextBox.DefaultHeight * 4;
            gridControl.SlotsHorizontally = 4;
            gridControl.SlotsVertically = 4;

            LimitAttribute limit = null;
            var attributes = Values.GetAttributes();
            if (attributes != null)
            {
                limit = (LimitAttribute)attributes.FirstOrDefault(x => x is LimitAttribute);
            }

            for (int i = 0; i < 16; i++)
            {
                var elemnt = grid.FloatValue();
                elemnt.SetLimits(limit);
                elemnt.ValueBox.ValueChanged += OnValueChanged;
                elemnt.ValueBox.SlidingEnd += ClearToken;
                Elements[i] = elemnt;
            }
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked)
                return;

            var isSliding = false;
            for (int i = 0; i < 16; i++)
            {
                isSliding = Elements[i].IsSliding;
            }
            var token = isSliding ? this : null;
            var value = new Matrix();
            for (int i = 0; i < 16; i++)
            {
                value[i] = Elements[i].ValueBox.Value;
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
            else
            {
                var value = (Matrix)Values[0];
                for (int i = 0; i < 16; i++)
                {
                    Elements[i].ValueBox.Value = value[i];
                }
            }
        }
    }
}
