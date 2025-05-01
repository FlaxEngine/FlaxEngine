// Copyright (c) Wojciech Figat. All rights reserved.

using System.Linq;
using System.Reflection;
using FlaxEditor.GUI.Input;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// The slider element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    public class SliderElement : LayoutElement, IFloatValueEditor, IIntegerValueEditor
    {
        /// <summary>
        /// The slider control.
        /// </summary>
        public readonly SliderControl Slider;

        /// <summary>
        /// Initializes a new instance of the <see cref="SliderElement"/> class.
        /// </summary>
        public SliderElement()
        {
            Slider = new SliderControl(0);
        }

        /// <summary>
        /// Sets the editor limits from member <see cref="RangeAttribute"/>.
        /// </summary>
        /// <param name="member">The member.</param>
        public void SetLimits(MemberInfo member)
        {
            // Try get limit attribute for value min/max range setting and slider speed
            if (member != null)
            {
                var attributes = member.GetCustomAttributes(true);
                var limit = attributes.FirstOrDefault(x => x is RangeAttribute);
                if (limit != null)
                {
                    Slider.SetLimits((RangeAttribute)limit);
                }
            }
        }

        /// <summary>
        /// Sets the editor limits from member <see cref="RangeAttribute"/>.
        /// </summary>
        /// <param name="limit">The limit.</param>
        public void SetLimits(RangeAttribute limit)
        {
            if (limit != null)
            {
                Slider.SetLimits(limit);
            }
        }

        /// <inheritdoc />
        public override Control Control => Slider;

        /// <inheritdoc />
        public float Value
        {
            get => Slider.Value;
            set => Slider.Value = value;
        }

        /// <inheritdoc />
        int IIntegerValueEditor.Value
        {
            get => (int)Slider.Value;
            set => Slider.Value = value;
        }

        /// <inheritdoc cref="IFloatValueEditor.IsSliding" />
        public bool IsSliding => Slider.IsSliding;

        /// <inheritdoc />
        public void SetLimits(LimitAttribute limit)
        {
            if (limit != null)
            {
                Slider.SetLimits(limit);
            }
        }
    }
}
