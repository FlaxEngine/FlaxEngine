// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    internal class PostProcessSettingAttribute : Attribute
    {
        public int Bit;

        public PostProcessSettingAttribute(int bit)
        {
            Bit = bit;
        }
    }
}
