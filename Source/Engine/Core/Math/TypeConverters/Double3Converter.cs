// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if FLAX_EDITOR
using System;
using System.ComponentModel;
using System.Globalization;

namespace FlaxEngine.TypeConverters
{
    internal class Double3Converter : VectorConverter
    {
        /// <inheritdoc />
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
        {
            if (value is string str)
            {
                string[] v = GetParts(str);
                return new Double3(double.Parse(v[0], culture), double.Parse(v[1], culture), double.Parse(v[2], culture));
            }
            return base.ConvertFrom(context, culture, value);
        }

        /// <inheritdoc />
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
        {
            if (destinationType == typeof(string))
            {
                var v = (Double3)value;
                return v.X.ToString(culture) + "," + v.Y.ToString(culture) + "," + v.Z.ToString(culture);
            }
            return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}
#endif
