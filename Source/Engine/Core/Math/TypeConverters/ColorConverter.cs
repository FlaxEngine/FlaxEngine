// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_EDITOR
using System;
using System.ComponentModel;
using System.Globalization;

namespace FlaxEngine.TypeConverters
{
    internal class ColorConverter : TypeConverter
    {
        /// <inheritdoc />
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
        {
            if (sourceType == typeof(string))
                return true;
            return base.CanConvertFrom(context, sourceType);
        }

        /// <inheritdoc />
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
        {
            if (destinationType == typeof(string))
                return false;
            return base.CanConvertTo(context, destinationType);
        }

        /// <inheritdoc />
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
        {
            if (value is string str)
            {
                string[] v = str.Split(',');
                if (v.Length == 4)
                    return new Color(float.Parse(v[0], culture), float.Parse(v[1], culture), float.Parse(v[2], culture), float.Parse(v[3], culture));
                if (v.Length == 3)
                    return new Color(float.Parse(v[0], culture), float.Parse(v[1], culture), float.Parse(v[2], culture), 1.0f);
                throw new FormatException("Invalid Color value format.");
            }
            return base.ConvertFrom(context, culture, value);
        }

        /// <inheritdoc />
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
        {
            if (destinationType == typeof(string))
            {
                var v = (Color)value;
                return v.R.ToString(culture) + "," + v.G.ToString(culture) + "," + v.B.ToString(culture) + "," + v.A.ToString(culture);
            }
            return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}
#endif
