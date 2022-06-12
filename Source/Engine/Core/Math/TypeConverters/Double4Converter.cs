// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;
using System.ComponentModel;
using System.Globalization;

namespace FlaxEngine.TypeConverters
{
    internal class Double4Converter : TypeConverter
    {
        /// <inheritdoc />
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
        {
            if (sourceType == typeof(string))
            {
                return true;
            }
            return base.CanConvertFrom(context, sourceType);
        }

        /// <inheritdoc />
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
        {
            if (value is string str)
            {
                string[] v = str.Split(',');
                return new Double4(double.Parse(v[0]), double.Parse(v[1]), double.Parse(v[2]), double.Parse(v[3]));
            }
            return base.ConvertFrom(context, culture, value);
        }

        /// <inheritdoc />
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
        {
            if (destinationType == typeof(string))
            {
                var v = (Double4)value;
                return v.X + "," + v.Y + "," + v.Z + "," + v.W;
            }
            return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}
