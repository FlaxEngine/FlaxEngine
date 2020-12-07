// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System.Linq;
using System.Reflection;
using FlaxEditor.GUI.Input;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// The double precision floating point value element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    public class DoubleValueElement : LayoutElement
    {
        /// <summary>
        /// The double value box.
        /// </summary>
        public readonly DoubleValueBox DoubleValue;

        /// <summary>
        /// Initializes a new instance of the <see cref="FloatValueElement"/> class.
        /// </summary>
        public DoubleValueElement()
        {
            DoubleValue = new DoubleValueBox(0);
        }

        /// <summary>
        /// Sets the editor limits from member <see cref="LimitAttribute"/>.
        /// </summary>
        /// <param name="member">The member.</param>
        public void SetLimits(MemberInfo member)
        {
            // Try get limit attribute for value min/max range setting and slider speed
            if (member != null)
            {
                var attributes = member.GetCustomAttributes(true);
                var limit = attributes.FirstOrDefault(x => x is LimitAttribute);
                if (limit != null)
                {
                    DoubleValue.SetLimits((LimitAttribute)limit);
                }
            }
        }

        /// <summary>
        /// Sets the editor limits from member <see cref="LimitAttribute"/>.
        /// </summary>
        /// <param name="limit">The limit.</param>
        public void SetLimits(LimitAttribute limit)
        {
            if (limit != null)
            {
                DoubleValue.SetLimits(limit);
            }
        }

        /// <summary>
        /// Sets the editor limits from the other <see cref="DoubleValueElement"/>.
        /// </summary>
        /// <param name="other">The other.</param>
        public void SetLimits(DoubleValueElement other)
        {
            if (other != null)
            {
                DoubleValue.SetLimits(other.DoubleValue);
            }
        }

        /// <inheritdoc />
        public override Control Control => DoubleValue;

        /// <summary>
        /// Gets or sets the value.
        /// </summary>
        public double Value
        {
            get => DoubleValue.Value;
            set => DoubleValue.Value = value;
        }

        /// <summary>
        /// Gets a value indicating whether user is using a slider.
        /// </summary>
        public bool IsSliding => DoubleValue.IsSliding;
    }
}
